#include "EnterpriseValues.h"
#include "../parser/ParserLimits.h"
#include "../type_system/ProductionTypeSystem.h"
#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
namespace shorthand::enterprise {
namespace {
namespace sir=semantic_ir;
using T=sir::ScalarType; using E=sir::Expression; using S=sir::Statement; using C=sir::Composite; using K=types::TypeKind;
struct Token { std::string text; unsigned line; bool quoted=false; };
bool letter(char c) { return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||c=='_'; }
bool digit(char c) { return c>='0'&&c<='9'; }
bool identifier(const std::string &s) { if(s.empty()||!letter(s[0])) return false; for(char c:s) if(!letter(c)&&!digit(c)) return false; return true; }
T scalar(const std::string &s) { if(s=="int32") return T::Int32; if(s=="bool") return T::Bool; if(s=="float64") return T::Float64; if(s=="string") return T::String; return T::Void; }
K kind(T t) { switch(t) { case T::Int32:return K::Int32; case T::Bool:return K::Bool; case T::Float64:return K::Float64; case T::String:return K::String; default:return K::Void; } }
struct Parser {
    const std::string &path; sir::ProgramIR &p; std::string &error; unsigned &failureLine;
    std::vector<Token> tokens; std::size_t at=0; unsigned depth=0;
    std::map<std::string,C> composites; std::map<std::string,sir::Variable> values;
    struct Borrow { std::string owner; bool mut=false,view=false; };
    std::map<std::string,Borrow> borrows; types::OwnershipTracker ownership;
    Parser(const std::string &file,sir::ProgramIR &program,std::string &diagnostic,unsigned &line):path(file),p(program),error(diagnostic),failureLine(line) {}
    const Token &peek() const { return tokens[at]; }
    bool fail(const std::string &message) { if(error.empty()) { error=message; failureLine=peek().line; } return false; }
    bool take(const std::string &s) { if(peek().text!=s||peek().quoted) return false; ++at; return true; }
    bool expect(const std::string &s) { return take(s)||fail("expected `"+s+"`"); }
    std::string name() { if(!identifier(peek().text)||peek().quoted) { fail("expected an identifier"); return {}; } return tokens[at++].text; }
    sir::SourceRange range() const { return {{path,static_cast<int>(peek().line),1},{path,static_cast<int>(peek().line),1}}; }
    bool tokenize() {
        std::ifstream input(path,std::ios::binary); std::ostringstream stream; stream<<input.rdbuf(); auto source=stream.str();
        if(!input||source.size()>parser::currentParserLimits().max_source_bytes) { error="unreadable or oversized source"; return false; }
        unsigned line=1;
        for(std::size_t i=0;i<source.size();) {
            char c=source[i]; if(c=='\n') { ++line; ++i; continue; } if(c==' '||c=='\t'||c=='\r') { ++i; continue; }
            if(c=='#'||(c=='/'&&i+1<source.size()&&source[i+1]=='/')) { while(i<source.size()&&source[i]!='\n') ++i; continue; }
            Token token{"",line,false};
            if(c=='"') {
                token.quoted=true; ++i;
                while(i<source.size()&&source[i]!='"') {
                    char ch=source[i++];
                    if(ch=='\\'&&i<source.size()) { ch=source[i++]; if(ch=='n') ch='\n'; else if(ch=='t') ch='\t'; else if(ch!='"'&&ch!='\\') { error="unsupported string escape"; failureLine=line; return false; } }
                    else if(ch=='\n'||ch=='\r') { error="unterminated string"; failureLine=line; return false; }
                    if(ch=='\0') { error="embedded NUL"; failureLine=line; return false; }
                    token.text+=ch;
                }
                if(i==source.size()) { error="unterminated string"; failureLine=line; return false; } ++i;
            } else if(letter(c)) { while(i<source.size()&&(letter(source[i])||digit(source[i]))) token.text+=source[i++]; }
            else if(digit(c)||c=='-') { token.text+=source[i++]; while(i<source.size()&&(digit(source[i])||source[i]=='.'||source[i]=='e'||source[i]=='E'||source[i]=='+'||source[i]=='-')) token.text+=source[i++]; }
            else { token.text+=c; ++i; }
            if(token.text.size()>parser::currentParserLimits().max_token_bytes||tokens.size()>=100000) { error="token/work limit exceeded"; failureLine=line; return false; }
            tokens.push_back(std::move(token));
        }
        tokens.push_back({"",line,false}); return true;
    }
    E integer(int n) { E e; e.integer=n; return e; }
    E zero(T t) { E e; e.type=t; e.kind=t==T::Float64?E::Kind::Float:t==T::String?E::Kind::String:t==T::Bool?E::Kind::Boolean:E::Kind::Integer; return e; }
    void append(S s) { p.body.body.push_back(std::move(s)); }
    bool reference(const std::string &n,E &e,bool write=false) {
        std::string owner=n; auto borrow=borrows.find(n);
        if(borrow!=borrows.end()&&!borrow->second.view) { owner=borrow->second.owner; if(write&&!borrow->second.mut) return fail("shared borrow is immutable"); }
        else { std::string why; if(!(write?ownership.assign(owner,why):ownership.read(owner,why))) return fail(why); }
        auto v=values.find(owner); if(v==values.end()) return fail("unknown value: "+owner);
        e.kind=E::Kind::Variable; e.name=owner; e.type=v->second.type; e.composite=v->second.composite; return true;
    }
    bool expression(E &e) {
        if(++depth>128) { --depth; return fail("expression depth limit exceeded"); }
        auto source=range(); bool ok=expressionImpl(e); if(!e.source_range.isKnown()) e.source_range=source; --depth; return ok;
    }
    bool expressionImpl(E &e) {
        if(peek().quoted) { e=zero(T::String); e.name=tokens[at++].text; return true; }
        if(take("true")) { e=zero(T::Bool); e.integer=1; return true; } if(take("false")) { e=zero(T::Bool); return true; }
        if(!peek().text.empty()&&(digit(peek().text[0])||peek().text[0]=='-')) {
            std::string literal=tokens[at++].text;
            if(literal.find_first_of(".eE")!=std::string::npos) { e=zero(T::Float64); char *end=nullptr; errno=0; e.decimal=std::strtod(literal.c_str(),&end); return (errno!=ERANGE&&end==literal.c_str()+literal.size()&&std::isfinite(e.decimal))||fail("invalid float64 literal"); }
            auto result=std::from_chars(literal.data(),literal.data()+literal.size(),e.integer); return (result.ec==std::errc{}&&result.ptr==literal.data()+literal.size())||fail("invalid int32 literal");
        }
        std::string n=name(); if(n.empty()) return false;
        auto ct=composites.find(n);
        if(ct!=composites.end()) {
            const C &c=ct->second; e.kind=E::Kind::Construct; e.type=T::Composite; e.composite=n;
            if(c.kind==C::Kind::Slice) return fail("slices require view and an array owner");
            if(c.kind==C::Kind::Enum) { if(!expect(".")) return false; auto variant=name(); for(std::size_t i=0;i<c.names.size();++i) if(c.names[i]==variant) { e.operands={integer(static_cast<int>(i))}; return true; } return fail("unknown enum variant"); }
            if(c.kind==C::Kind::Option||c.kind==C::Kind::Result) {
                if(!expect(".")) return false; auto variant=name();
                int tag=c.kind==C::Kind::Option?(variant=="some"?1:variant=="none"?0:-1):(variant=="ok"?0:variant=="error"?1:-1);
                if(tag<0) return fail("unknown option/result variant");
                e.operands.push_back(integer(tag)); for(T t:c.fields) e.operands.push_back(zero(t));
                if(c.kind==C::Kind::Option&&tag==0) return true;
                E payload; if(!expect("(")||!expression(payload)||!expect(")")) return false; e.operands[c.kind==C::Kind::Option?1:tag+1]=std::move(payload); return true;
            }
            if(!expect("(")) return false;
            for(std::size_t i=0;i<c.fields.size();++i) { E field; if((i&&!expect(","))||!expression(field)) return false; e.operands.push_back(std::move(field)); } return expect(")");
        }
        if(!reference(n,e)) return false;
        if(take(".")) {
            auto field=name(); if(e.type!=T::Composite) return fail("projection requires composite"); const C &c=composites.at(e.composite); int index=-1; T t=T::Void;
            if(c.kind!=C::Kind::Record&&c.kind!=C::Kind::Slice&&field=="tag") { index=0; t=T::Int32; }
            else if(c.kind!=C::Kind::Enum&&c.kind!=C::Kind::Slice) for(std::size_t i=0;i<c.names.size();++i) if(c.names[i]==field) { index=static_cast<int>(i)+(c.kind==C::Kind::Record?0:1); t=c.fields[i]; }
            if(index<0) return fail("unknown composite field"); E source=std::move(e); e=E{}; e.kind=E::Kind::Project; e.type=t; e.integer=index; e.operands.push_back(std::move(source));
        } else if(take("[")) {
            E index; if(!expression(index)||!expect("]")) return false;
            if(e.type==T::Composite) { const C &c=composites.at(e.composite); if(c.kind!=C::Kind::Slice) return fail("indexing requires array/slice"); e.type=c.fields[0]; e.composite.clear(); e.kind=E::Kind::SliceIndex; }
            else e.kind=E::Kind::Index; e.operands.push_back(std::move(index));
        } else if(values.at(e.name).extent>0) return fail("array requires an index");
        return true;
    }
    bool declaration(C c) {
        c.name=name(); if(c.name.empty()||composites.count(c.name)) return fail("duplicate/invalid type"); types::TypeDescriptor descriptor;
        if(c.kind==C::Kind::Record||c.kind==C::Kind::Enum) {
            if(!expect("{")) return false; std::vector<types::Field> fields;
            while(!take("}")) {
                if(c.names.size()>=256) return fail("field limit exceeded"); T t=T::Void;
                if(c.kind==C::Kind::Record) { t=scalar(name()); if(t==T::Void) return fail("record requires scalar fields"); }
                auto field=name(); if(field.empty()||!expect(";")) return false; c.names.push_back(field);
                if(c.kind==C::Kind::Record) { c.fields.push_back(t); fields.push_back({field,kind(t)}); }
            }
            descriptor=c.kind==C::Kind::Record?types::TypeDescriptor::record(c.name,fields):types::TypeDescriptor::enumeration(c.name,c.names);
        } else {
            if(!expect("<")) return false; T first=scalar(name()); if(first==T::Void) return fail("payload requires scalar"); c.fields.push_back(first);
            if(c.kind==C::Kind::Result) { if(!expect(",")) return false; T second=scalar(name()); if(second==T::Void) return fail("error requires scalar"); c.fields.push_back(second); c.names={"ok","error"}; descriptor=types::TypeDescriptor::result(kind(first),kind(second)); }
            else if(c.kind==C::Kind::Option) { c.names={"value"}; descriptor=types::TypeDescriptor::option(kind(first)); }
            else { c.names={"element"}; descriptor=types::TypeDescriptor::slice(kind(first)); }
            if(!expect(">")) return false;
        }
        std::string why; if(!descriptor.validate(why)) return fail(why); if(!expect(";")) return false; composites.emplace(c.name,c); p.composites.push_back(std::move(c)); return true;
    }
    bool addValue(sir::Variable v,const E &init) {
        if(values.count(v.name)||borrows.count(v.name)) return fail("duplicate value/borrow"); std::string why;
        if(!ownership.declareValue(v.name,why)||!ownership.initialize(v.name,why)) return fail(why); values[v.name]=v; p.globals.push_back(v);
        E target; target.kind=E::Kind::Variable; target.name=v.name; target.type=v.type; target.composite=v.composite; S s; s.kind=S::Kind::Assign; s.expressions={target,init}; append(s); return true;
    }
    bool statement() {
        auto loc=range();
        for(auto entry:{std::pair<const char *,C::Kind>{"record",C::Kind::Record},{"enum",C::Kind::Enum},{"option",C::Kind::Option},{"result",C::Kind::Result},{"slice",C::Kind::Slice}}) if(take(entry.first)) { C c; c.kind=entry.second; return declaration(c); }
        if(take("owned")) {
            sir::Variable v; auto tn=name(); v.type=scalar(tn);
            if(v.type==T::Void) { if(!composites.count(tn)) return fail("unknown type"); v.type=T::Composite; v.composite=tn; if(composites.at(tn).kind==C::Kind::Slice) return fail("slices require view"); }
            v.name=name(); v.source_range=loc; E init;
            if(v.name.empty()||!expect("=")||!expression(init)||!expect(";")) return false;
            if(init.type==T::Composite&&init.kind!=E::Kind::Construct) return fail("owned composite requires constructor or move"); return addValue(v,init);
        }
        if(take("array")) {
            sir::Variable v; v.type=scalar(name()); v.name=name(); v.source_range=loc; E extent;
            if(!expect("[")||!expression(extent)||!expect("]")||extent.kind!=E::Kind::Integer||extent.integer<=0||extent.integer>65536) return fail("array requires positive extent up to 65536"); v.extent=extent.integer;
            if(v.type!=T::Int32&&v.type!=T::Float64&&v.type!=T::Bool) return fail("array requires numeric/bool elements");
            if(!expect("=")||!expect("[")||values.count(v.name)||borrows.count(v.name)) return fail("invalid array declaration"); std::string why;
            if(!ownership.declareValue(v.name,why)||!ownership.initialize(v.name,why)) return fail(why); values[v.name]=v; p.globals.push_back(v);
            for(int i=0;i<v.extent;++i) { E init; if((i&&!expect(","))||!expression(init)) return false; E target; target.kind=E::Kind::Index; target.type=v.type; target.name=v.name; target.operands={integer(i)}; S s; s.kind=S::Kind::Assign; s.expressions={target,init}; append(s); }
            return expect("]")&&expect(";");
        }
        if(take("view")) {
            auto tn=name(); auto ct=composites.find(tn); if(ct==composites.end()||ct->second.kind!=C::Kind::Slice) return fail("view requires slice type");
            sir::Variable v; v.name=name(); v.type=T::Composite; v.composite=tn; v.source_range=loc; if(!expect("=")) return false;
            E init; init.kind=E::Kind::Slice; init.type=T::Composite; init.composite=tn; init.name=name(); init.source_range=loc;
            E offset,length; if(!expect("[")||!expression(offset)||!expect(",")||!expression(length)||!expect("]")||!expect(";")) return false; std::string why;
            if(!ownership.borrowShared(init.name,why)) return fail(why); init.operands={offset,length}; if(!addValue(v,init)) return false; borrows[v.name]={init.name,false,true}; return true;
        }
        if(take("print")) {
            S s; s.kind=S::Kind::Print; s.source_range=loc; do { E e; if(!expression(e)) return false; s.expressions.push_back(std::move(e)); } while(take(","));
            if(!expect(";")) return false; append(s); return true;
        }
        if(take("set")) {
            E target; target.source_range=loc; if(!reference(name(),target,true)) return false; S s; s.kind=S::Kind::Assign; s.source_range=loc;
            if(take(".")) { auto field=name(); if(target.type!=T::Composite) return fail("field update requires record"); const C &c=composites.at(target.composite); bool found=false; for(std::size_t i=0;i<c.names.size();++i) if(c.names[i]==field) { s.declaration=i; found=true; } if(!found||c.kind!=C::Kind::Record) return fail("unknown record field"); s.kind=S::Kind::Update; }
            else if(take("[")) { E index; if(!expression(index)||!expect("]")) return false; target.kind=E::Kind::Index; target.operands={index}; }
            else if(target.type==T::Composite) return fail("use move or field update");
            E value; if(!expect("=")||!expression(value)||!expect(";")) return false; s.expressions={target,value}; append(s); return true;
        }
        if(take("move")) {
            auto owner=name(); E init; if(!reference(owner,init)||!expect("to")) return false; if(borrows.count(owner)) return fail("release view before moving");
            auto v=values.at(owner); v.name=name(); v.source_range=loc; if(!expect(";")) return false; std::string why; return ownership.moveValue(owner,why)?addValue(v,init):fail(why);
        }
        if(take("borrow")) {
            bool mut=take("mutable"); if(!mut&&!expect("shared")) return false; auto owner=name(); if(!expect("as")) return false; auto handle=name(); if(!expect(";")) return false;
            if(values.count(handle)||borrows.count(handle)) return fail("duplicate borrow"); std::string why; if(!(mut?ownership.borrowMutable(owner,why):ownership.borrowShared(owner,why))) return fail(why); borrows[handle]={owner,mut,false}; return true;
        }
        if(take("release")) {
            auto handle=name(); if(!expect(";")) return false; auto found=borrows.find(handle); if(found==borrows.end()) return fail("unknown/released borrow");
            auto borrow=found->second; std::string why; if(!(borrow.mut?ownership.releaseMutable(borrow.owner,why):ownership.releaseShared(borrow.owner,why))) return fail(why);
            if(borrow.view&&!ownership.moveValue(handle,why)) return fail(why); borrows.erase(found); return true;
        }
        return fail("unknown enterprise value statement");
    }
    bool run() {
        if(!tokenize()) return false;
        if(!expect("language")||!expect("shorthand")||!expect(".")||!expect("enterprise_language")||!expect(".")||!expect("v2")||!expect(";")) return false;
        if(!expect("namespace")||name().empty()) return false; while(take(".")) if(name().empty()) return false; if(!expect(";")) return false;
        while(!peek().text.empty()) if(!statement()) return false;
        return borrows.empty()?error.empty():fail("all borrows/views must be released");
    }
};
}
bool isValueSource(const std::string &path) {
    std::ifstream input(path); std::string line;
    while(std::getline(input,line)) { auto first=line.find_first_not_of(" \t\r"); if(first==std::string::npos||line[first]=='#'||line.compare(first,2,"//")==0) continue; return line.find("shorthand.enterprise_language.v2")!=std::string::npos; } return false;
}
bool lowerValueSource(const std::string &path,sir::ProgramIR &program,std::string &error,unsigned &line) {
    program=sir::ProgramIR{}; error.clear(); line=1; Parser parser{path,program,error,line}; if(!parser.run()) { program=sir::ProgramIR{}; return false; } return true;
}
}
