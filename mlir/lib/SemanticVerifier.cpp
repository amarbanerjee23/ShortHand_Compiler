#include "ShortHand/Conversion/Lowering.h"
#include <cmath>
#include <limits>
#include <map>
#include <set>

namespace shorthand::lowering {
namespace {
namespace sir = semantic_ir;
using T = sir::ScalarType;
using E = sir::Expression;
using S = sir::Statement;
struct Verifier {
    const sir::ProgramIR &p;
    std::string &error;
    std::map<std::string,sir::Variable> globals;
    std::map<std::string,const sir::Function *> functions;
    std::vector<std::map<std::string,sir::Variable>> scopes;
    std::vector<std::set<std::string>> labels;
    std::map<std::string,const sir::Composite *> composites;
    T result=T::Int32;
    std::string resultComposite;
    unsigned loops=0, depth=0;
    std::size_t nodes=0;
    Verifier(const sir::ProgramIR &program,std::string &diagnostic):p(program),error(diagnostic) {}
    bool fail(const std::string &text) { if(error.empty()) error=text; return false; }
    bool identifier(const std::string &s) {
        if(s.empty() || s.rfind("__sh_",0)==0 || s.rfind("short_",0)==0) return false;
        for(std::size_t i=0;i<s.size();++i) {
            const unsigned char c=s[i];
            if(!(c=='_' || (c>='a'&&c<='z') || (c>='A'&&c<='Z') || (i>0&&c>='0'&&c<='9'))) return false;
        }
        return true;
    }
    bool scalar(T t) { return t==T::Int32||t==T::Bool||t==T::Float64||t==T::String; }
    bool numeric(T t) { return t==T::Int32||t==T::Float64; }
    bool condition(T t) { return t==T::Bool||t==T::Int32; }
    bool valueType(T t,const std::string &id) { return t==T::Composite ? composites.count(id)!=0 : scalar(t)&&id.empty(); }
    bool same(const E &a,const E &b) { return a.type==b.type&&a.composite==b.composite; }
    std::vector<T> fields(const sir::Composite &c) {
        auto result=c.fields;
        if(c.kind==sir::Composite::Kind::Enum||c.kind==sir::Composite::Kind::Option||c.kind==sir::Composite::Kind::Result) result.insert(result.begin(),T::Int32);
        return result;
    }
    bool variable(const sir::Variable &v) {
        return (identifier(v.name)&&valueType(v.type,v.composite)&&v.extent>=0&&v.extent<=65536&&
            !(v.extent>0&&(v.type==T::String||v.type==T::Composite))) || fail("invalid variable type, name or extent: "+v.name);
    }
    const sir::Variable *lookup(const std::string &name) {
        for(auto it=scopes.rbegin();it!=scopes.rend();++it) { auto f=it->find(name); if(f!=it->end()) return &f->second; }
        auto f=globals.find(name); return f==globals.end()?nullptr:&f->second;
    }
    bool expr(const E &e, bool lvalue=false) {
        if(++nodes>100000 || depth++>=256) { --depth; return fail("SemanticIR work/depth limit exceeded"); }
        bool ok=exprImpl(e,lvalue); --depth; return ok;
    }
    bool exprImpl(const E &e,bool lvalue) {
        if(e.type!=T::Void&&!valueType(e.type,e.composite)) return fail("invalid expression type identity");
        if(lvalue && e.kind!=E::Kind::Variable && e.kind!=E::Kind::Index) return fail("assignment/input requires a variable");
        for(const auto &arg:e.operands) if(!expr(arg)) return false;
        switch(e.kind) {
        case E::Kind::Integer: return (e.type==T::Int32&&e.operands.empty())||fail("invalid integer expression");
        case E::Kind::Boolean: return (e.type==T::Bool&&(e.integer==0||e.integer==1)&&e.operands.empty())||fail("invalid bool expression");
        case E::Kind::Float: return (e.type==T::Float64&&std::isfinite(e.decimal)&&e.operands.empty())||fail("invalid float expression");
        case E::Kind::String: return (e.type==T::String&&e.operands.empty()&&e.name.find('\0')==std::string::npos)||fail("invalid string expression");
        case E::Kind::Variable: case E::Kind::Index: {
            auto *v=lookup(e.name); bool index=e.kind==E::Kind::Index;
            return (v&&v->type==e.type&&v->composite==e.composite&&(v->extent>0)==index&&e.operands.size()==(index?1U:0U)&&
                    (!index||e.operands[0].type==T::Int32))||fail("unresolved or mistyped variable: "+e.name);
        }
        case E::Kind::Unary:
            return (e.operands.size()==1&&e.opcode==1&&e.type==e.operands[0].type&&numeric(e.type))||fail("invalid unary expression");
        case E::Kind::Binary: {
            if(e.operands.size()!=2) return fail("binary expression requires two operands");
            T a=e.operands[0].type,b=e.operands[1].type;
            if(a!=b) return fail("binary operands require exact types");
            bool ok=false;
            if(e.opcode>=1&&e.opcode<=4) ok=numeric(a)&&e.type==a;
            else if(e.opcode==5) ok=a==T::Int32&&e.type==a;
            else if(e.opcode>=6&&e.opcode<=9) ok=numeric(a)&&e.type==T::Bool;
            else if(e.opcode==10||e.opcode==11) ok=scalar(a)&&e.type==T::Bool;
            else if(e.opcode==12||e.opcode==13) ok=condition(a)&&e.type==T::Bool;
            return ok||fail("unsupported binary operation/type");
        }
        case E::Kind::Call: {
            auto f=functions.find(e.name);
            if(f==functions.end()||f->second->result!=e.type||f->second->result_composite!=e.composite||f->second->parameters.size()!=e.operands.size()) return fail("unresolved/mistyped call: "+e.name);
            for(std::size_t i=0;i<e.operands.size();++i) if(e.operands[i].type!=f->second->parameters[i].type||e.operands[i].composite!=f->second->parameters[i].composite) return fail("call argument type mismatch");
            return true;
        }
        case E::Kind::Construct: {
            if(e.type!=T::Composite) return fail("constructor requires composite type");
            const auto &c=*composites.at(e.composite);
            if(c.kind==sir::Composite::Kind::Slice) return fail("slice requires an array owner");
            auto expected=fields(c);
            if(expected.size()!=e.operands.size()) return fail("constructor arity mismatch");
            for(std::size_t i=0;i<expected.size();++i) if(expected[i]!=e.operands[i].type) return fail("constructor field type mismatch");
            return true;
        }
        case E::Kind::Project: {
            if(e.operands.size()!=1||e.operands[0].type!=T::Composite) return fail("projection requires a composite value");
            const auto &c=*composites.at(e.operands[0].composite); auto expected=fields(c);
            return (c.kind!=sir::Composite::Kind::Slice&&e.integer>=0&&static_cast<std::size_t>(e.integer)<expected.size()&&e.type==expected[e.integer])||fail("projection index/type mismatch");
        }
        case E::Kind::Slice: {
            auto v=globals.find(e.name);
            if(e.type!=T::Composite||composites.at(e.composite)->kind!=sir::Composite::Kind::Slice||v==globals.end()||v->second.extent<=0||lookup(e.name)!=&v->second||e.operands.size()!=2) return fail("slice requires an unshadowed global array with program lifetime");
            return (v->second.type==composites.at(e.composite)->fields[0]&&e.operands[0].type==T::Int32&&e.operands[1].type==T::Int32)||fail("slice element/range type mismatch");
        }
        case E::Kind::SliceIndex: {
            auto *v=lookup(e.name); if(!v||v->type!=T::Composite) return fail("slice indexing requires a slice variable");
            const auto &c=*composites.at(v->composite);
            return (c.kind==sir::Composite::Kind::Slice&&e.type==c.fields[0]&&e.operands.size()==1&&e.operands[0].type==T::Int32)||fail("slice index type mismatch");
        }
        case E::Kind::TensorIndex:
            for(const auto &t:p.tensors) if(t.name==e.name) return (t.element_type==sir::ElementType::Float32&&e.type==T::Float64&&e.operands.size()==1&&e.operands[0].type==T::Int32)||fail("tensor access requires f32 tensor, int32 index and float64 result");
            return fail("unknown tensor");
        }
        return fail("unknown expression kind");
    }
    bool stmt(const S &s) {
        if(++nodes>100000 || depth++>=256) { --depth; return fail("SemanticIR work/depth limit exceeded"); }
        bool ok=stmtImpl(s); --depth; return ok;
    }
    bool stmtImpl(const S &s) {
        for(const auto &e:s.expressions) if(!expr(e)) return false;
        auto count=[&](std::size_t n) { return s.expressions.size()==n||fail("statement operand count mismatch"); };
        switch(s.kind) {
        case S::Kind::Block: {
            if(!count(0)) return false;
            if(s.lexical_scope) scopes.push_back({});
            labels.push_back({});
            for(const auto &child:s.body) if(child.kind==S::Kind::Label && (!identifier(child.name)||!labels.back().insert(child.name).second)) return fail("invalid or duplicate label");
            for(const auto &child:s.body) if(!stmt(child)) return false;
            labels.pop_back(); if(s.lexical_scope) scopes.pop_back(); return true;
        }
        case S::Kind::Declare:
            if(!count(0)) return false;
            for(const auto &v:s.variables) if(!variable(v)||!scopes.back().emplace(v.name,v).second) return fail("duplicate/invalid local variable");
            return true;
        case S::Kind::Assign:
            return (count(2)&&expr(s.expressions[0],true)&&same(s.expressions[0],s.expressions[1]))||fail("assignment type mismatch");
        case S::Kind::Update: {
            if(!count(2)||s.expressions[0].type!=T::Composite||!expr(s.expressions[0],true)) return fail("update requires a record variable");
            const auto &c=*composites.at(s.expressions[0].composite);
            return (c.kind==sir::Composite::Kind::Record&&s.declaration<c.fields.size()&&c.fields[s.declaration]==s.expressions[1].type)||fail("record update type mismatch");
        }
        case S::Kind::Expression: return count(1);
        case S::Kind::Read:
            for(const auto &e:s.expressions) if(!expr(e,true)||!numeric(e.type)) return fail("unsupported read target");
            return !s.expressions.empty()||fail("empty read");
        case S::Kind::Print:
            for(const auto &e:s.expressions) if(!scalar(e.type)) return fail("cannot print void");
            return !s.expressions.empty()||fail("empty print");
        case S::Kind::If: case S::Kind::While:
            if(!count(1)||!condition(s.expressions[0].type)) return fail("invalid control condition");
            break;
        case S::Kind::For:
            if(!count(4)||!expr(s.expressions[0],true)) return false;
            for(const auto &e:s.expressions) if(e.type!=T::Int32) return fail("for operands require int32");
            break;
        case S::Kind::Break: case S::Kind::Continue: return (count(0)&&loops>0)||fail("loop control outside loop");
        case S::Kind::Return: return (count(result==T::Void?0:1)&&(result==T::Void||(s.expressions[0].type==result&&s.expressions[0].composite==resultComposite)))||fail("return type mismatch");
        case S::Kind::Goto:
            return (s.expressions.size()<=1&&(!s.expressions.size()||condition(s.expressions[0].type))&&!labels.empty()&&labels.back().count(s.name))||fail("unresolved goto");
        case S::Kind::Label: return (count(0)&&!labels.empty()&&labels.back().count(s.name))||fail("unresolved label");
        case S::Kind::Model: return (count(0)&&s.declaration<p.models.size())||fail("invalid model declaration index");
        case S::Kind::Tensor: return (count(0)&&s.declaration<p.tensors.size())||fail("invalid tensor declaration index");
        case S::Kind::Infer: return (count(0)&&s.declaration<p.inferences.size())||fail("invalid inference index");
        case S::Kind::Contract: return (count(0)&&s.declaration<p.contracts.size())||fail("invalid contract index");
        case S::Kind::Measurement: return (count(0)&&s.declaration<p.measurements.size())||fail("invalid measurement index");
        case S::Kind::LegacyInfer: return (count(0)&&s.strings.size()==3)||fail("invalid legacy inference");
        case S::Kind::GreenReport:
            if(!count(3)) return false;
            for(const auto &e:s.expressions) if(e.type!=T::Int32) return fail("legacy GreenAI quantities require int32");
            return true;
        case S::Kind::Evidence: return count(0);
        default: return fail("unknown statement kind");
        }
        bool loop=s.kind==S::Kind::For||s.kind==S::Kind::While;
        if(loop) ++loops;
        for(const auto &child:s.body) if(!stmt(child)) return false;
        if(loop) --loops;
        for(const auto &child:s.alternative) if(!stmt(child)) return false;
        return true;
    }
    bool run() {
        for(const auto &c:p.composites) {
            if(!identifier(c.name)||!composites.emplace(c.name,&c).second) return fail("invalid/duplicate composite identity");
            using K=sir::Composite::Kind;
            if(c.fields.size()>256||c.names.size()>256) return fail("composite size limit exceeded");
            if(c.kind==K::Record) { if(c.fields.empty()) return fail("empty record"); }
            else if(c.kind==K::Enum) { if(!c.fields.empty()||c.names.empty()) return fail("invalid enum"); }
            else if(c.kind==K::Option||c.kind==K::Slice) { if(c.fields.size()!=1) return fail("invalid payload arity"); }
            else if(c.kind==K::Result) { if(c.fields.size()!=2) return fail("invalid result arity"); }
            else return fail("unknown composite kind");
            if(c.kind!=K::Enum&&c.fields.size()!=c.names.size()) return fail("field names/types differ");
            for(T field:c.fields) if(!scalar(field)||(c.kind==K::Slice&&!numeric(field))) return fail("unsupported composite payload type");
            std::set<std::string> names;
            for(const auto &name:c.names) if(!identifier(name)||!names.insert(name).second) return fail("invalid/duplicate composite field or variant");
        }
        const std::set<std::string> reserved={"main","printf","scanf","strcmp","write","exit","malloc","free"};
        for(const auto &v:p.globals) if(!variable(v)||reserved.count(v.name)||!globals.emplace(v.name,v).second) return fail("invalid/duplicate/reserved global");
        for(const auto &f:p.functions) {
            if(!identifier(f.name)||reserved.count(f.name)||globals.count(f.name)||!functions.emplace(f.name,&f).second||(!valueType(f.result,f.result_composite)&&!(f.result==T::Void&&f.result_composite.empty()))) return fail("invalid/duplicate/reserved function");
        }
        std::set<std::string> modelNames,tensorNames,contractNames;
        auto tensorShape=[&](const sir::TensorShape &s) { return s.isStatic()&&s.elementCount()>0&&s.elementCount()<=65536; };
        for(const auto &m:p.models) {
            if(!identifier(m.name)||!modelNames.insert(m.name).second||!tensorShape(m.input_shape)||!tensorShape(m.output_shape)||
               m.precision==sir::ElementType::Unknown||m.format==sir::ModelFormat::Unknown||m.backend_preference.empty()) return fail("invalid model signature or duplicate model (maximum 65536 tensor elements)");
        }
        for(const auto &t:p.tensors) {
            if(!identifier(t.name)||!tensorNames.insert(t.name).second||!tensorShape(t.shape)||t.element_type==sir::ElementType::Unknown||
               (!t.values.empty()&&static_cast<int64_t>(t.values.size())!=t.shape.elementCount())) return fail("invalid tensor shape/initializer or duplicate tensor (maximum 65536 elements)");
            for(double v:t.values) {
                if(!std::isfinite(v)) return fail("non-finite tensor initializer");
                double limit=0;
                switch(t.element_type) {
                case sir::ElementType::Float32: limit=std::numeric_limits<float>::max(); break;
                case sir::ElementType::Float16: limit=65504; break;
                case sir::ElementType::BFloat16: limit=3.3895313892515355e38; break;
                case sir::ElementType::Int4: limit=8; break;
                case sir::ElementType::Int8: limit=128; break;
                case sir::ElementType::Int32: limit=2147483648.0; break;
                default: return fail("unknown tensor element type");
                }
                bool integer=t.element_type==sir::ElementType::Int4||t.element_type==sir::ElementType::Int8||t.element_type==sir::ElementType::Int32;
                if(v < -limit || v > limit || (integer&&(v>=limit||std::trunc(v)!=v))) return fail("tensor initializer is outside its exact element domain");
            }
        }
        for(const auto &c:p.contracts) if(!identifier(c.name)||!contractNames.insert(c.name).second) return fail("duplicate/invalid contract");
        for(const auto &i:p.inferences) if(!modelNames.count(i.model_name)||!tensorNames.count(i.input_tensor_name)||!tensorNames.count(i.output_tensor_name)) return fail("unresolved inference reference");
        for(const auto &m:p.measurements) if(!contractNames.count(m.workload)) return fail("unresolved measurement contract");
        for(const auto &f:p.functions) {
            scopes.push_back({}); result=f.result; resultComposite=f.result_composite;
            for(const auto &v:f.parameters) if(!variable(v)||v.extent!=0||!scopes.back().emplace(v.name,v).second) return fail("invalid parameter");
            if(!stmt(f.body)) return false;
            scopes.pop_back();
        }
        scopes.push_back({}); result=T::Int32; resultComposite.clear();
        return stmt(p.body);
    }
};
}
bool verifySemanticIR(const semantic_ir::ProgramIR &program,std::string &error) {
    error.clear(); Verifier verifier{program,error}; return verifier.run();
}
}
