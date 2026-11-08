#include "StaticAffineModel.h"
#include "../module/Sha256.h"
#include "../runtime/ImmutableOnnxEnvelope.h"
#include <cmath>
#include <cstring>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string_view>

namespace shorthand::aot {
namespace {
constexpr std::size_t maxBytes=16*1024*1024,maxElements=65536,maxWeights=1024*1024;
void require(bool value,const char *reason) { if(!value) throw std::runtime_error(reason); }
struct Budget { std::size_t fields=0,weights=0; };
struct Field { unsigned number=0,wire=0; std::uint64_t integer=0; std::string_view bytes; };

// Length-delimited views always refer to the one owned immutable snapshot.
class Message {
    std::string_view data_;
    std::size_t pos_=0;
    Budget &budget_;
    std::set<unsigned> singular_;
public:
    Message(std::string_view data,Budget &budget):data_(data),budget_(budget) {}
    bool more() const { return pos_<data_.size(); }
    std::uint64_t integer() {
        std::uint64_t value=0;
        for(unsigned shift=0;shift<70;shift+=7) {
            require(more(),"truncated protobuf integer");
            const auto c=static_cast<unsigned char>(data_[pos_++]);
            require(shift!=63 || c<=1,"overflowing protobuf integer");
            value|=std::uint64_t(c&127)<<shift;
            if(!(c&128)) return value;
        }
        throw std::runtime_error("overflowing protobuf integer");
    }
    Field next() {
        require(++budget_.fields<=1000000,"protobuf field limit exceeded");
        const auto key=integer();
        require(key>>3 && (key>>3)<=536870911,"invalid protobuf field number");
        Field f; f.number=static_cast<unsigned>(key>>3); f.wire=key&7;
        if(f.wire==0) f.integer=integer();
        else {
            require(f.wire==1 || f.wire==2 || f.wire==5,"invalid protobuf wire type");
            const auto size=f.wire==2?integer():std::uint64_t(f.wire==1?8:4);
            require(size<=data_.size()-pos_,"truncated protobuf field");
            f.bytes=data_.substr(pos_,static_cast<std::size_t>(size)); pos_+=size;
        }
        return f;
    }
    void one(const Field &f) { require(singular_.insert(f.number).second,"duplicate singular protobuf field"); }
};
void wire(const Field &f,unsigned expected) { require(f.wire==expected,"wrong protobuf wire type"); }
void utf8(std::string_view s) {
    for(std::size_t i=0;i<s.size();) {
        const auto c=static_cast<unsigned char>(s[i++]);
        if(c<128) continue;
        unsigned count=0; std::uint32_t value=0,minimum=0;
        if(c>=0xc2 && c<=0xdf) { count=1; value=c&31; minimum=0x80; }
        else if(c>=0xe0 && c<=0xef) { count=2; value=c&15; minimum=0x800; }
        else if(c>=0xf0 && c<=0xf4) { count=3; value=c&7; minimum=0x10000; }
        else require(false,"invalid protobuf UTF-8");
        require(count<=s.size()-i,"truncated protobuf UTF-8");
        while(count--) { const auto next=static_cast<unsigned char>(s[i++]); require((next&0xc0)==0x80,"invalid protobuf UTF-8"); value=(value<<6)|(next&63); }
        require(value>=minimum && value<=0x10ffff && !(value>=0xd800 && value<=0xdfff),"invalid protobuf UTF-8");
    }
}
std::string text(const Field &f) { wire(f,2); utf8(f.bytes); return std::string(f.bytes); }
void name(const std::string &s) { require(!s.empty() && s.size()<=4096 && s.find('\0')==std::string::npos,"empty or oversized graph name"); }
void metadata(const Field &field,Budget &budget,std::set<std::string> &keys) {
    wire(field,2); Message m(field.bytes,budget); std::string key; bool hasKey=false,hasValue=false;
    while(m.more()) {
        auto f=m.next(); m.one(f);
        if(f.number==1) { key=text(f); hasKey=true; }
        else if(f.number==2) { (void)text(f); hasValue=true; }
        else require(false,"unknown metadata field");
    }
    require(hasKey && hasValue && !key.empty() && keys.insert(key).second,"invalid or duplicate metadata key");
}
std::size_t product(const std::vector<std::size_t> &dims,std::size_t limit) {
    require(!dims.empty(),"empty tensor shape"); std::size_t count=1;
    for(auto dim:dims) { require(dim>0 && dim<=1024 && count<=limit/dim,"tensor shape limit exceeded"); count*=dim; }
    return count;
}
std::vector<std::size_t> shape(std::string_view data,Budget &budget) {
    Message m(data,budget); std::vector<std::size_t> result;
    while(m.more()) {
        auto f=m.next(); require(f.number==1,"unknown shape field"); wire(f,2);
        Message dim(f.bytes,budget); std::uint64_t value=0; bool seen=false;
        while(dim.more()) {
            auto d=dim.next(); dim.one(d);
            if(d.number==1) { wire(d,0); value=d.integer; seen=true; }
            else if(d.number==3) (void)text(d);
            else require(false,"only static dimension values are supported");
        }
        require(seen && value>0 && value<=1024 && result.size()<2,"static rank-2 dimension bound exceeded");
        result.push_back(static_cast<std::size_t>(value));
    }
    require(result.size()==2,"rank-2 tensor type required"); (void)product(result,maxElements); return result;
}
std::vector<std::size_t> type(std::string_view data,Budget &budget) {
    Message m(data,budget); std::vector<std::size_t> dims; bool tensor=false;
    while(m.more()) {
        auto f=m.next(); m.one(f);
        if(f.number==6) { (void)text(f); continue; }
        require(f.number==1,"only FP32 tensor types are supported"); wire(f,2); tensor=true;
        Message t(f.bytes,budget); std::uint64_t element=0; bool shaped=false;
        while(t.more()) {
            auto e=t.next(); t.one(e);
            if(e.number==1) { wire(e,0); element=e.integer; }
            else if(e.number==2) { wire(e,2); dims=shape(e.bytes,budget); shaped=true; }
            else require(false,"unknown tensor type field");
        }
        require(element==1 && shaped,"static FP32 tensor type required");
    }
    require(tensor,"missing tensor type"); return dims;
}
struct Value { std::string name; std::vector<std::size_t> dims; };
Value value(std::string_view data,Budget &budget) {
    Message m(data,budget); Value result; std::set<std::string> keys;
    while(m.more()) {
        auto f=m.next(); if(f.number!=4) m.one(f);
        if(f.number==1) result.name=text(f);
        else if(f.number==2) { wire(f,2); result.dims=type(f.bytes,budget); }
        else if(f.number==3) (void)text(f);
        else if(f.number==4) metadata(f,budget,keys);
        else require(false,"unknown value-info field");
    }
    name(result.name); require(result.dims.size()==2,"missing tensor shape"); return result;
}
float fp32(std::string_view bytes) {
    static_assert(sizeof(float)==4 && std::numeric_limits<float>::is_iec559,"IEEE FP32 required");
    require(bytes.size()==4,"invalid FP32 encoding"); std::uint32_t bits=0;
    for(unsigned i=0;i<4;++i) bits|=std::uint32_t(static_cast<unsigned char>(bytes[i]))<<(i*8);
    float result; std::memcpy(&result,&bits,4); require(std::isfinite(result),"nonfinite model constant"); return result;
}
struct Constant { std::string name; std::vector<std::size_t> dims; std::vector<float> values; };
Constant constant(std::string_view data,Budget &budget) {
    Message m(data,budget); Constant c; std::uint64_t element=0; std::string_view raw; std::set<std::string> keys;
    auto dimension=[&](std::uint64_t d) { require(d>0 && d<=1024 && c.dims.size()<2,"constant shape bound exceeded"); c.dims.push_back(static_cast<std::size_t>(d)); };
    auto floats=[&](std::string_view bytes) {
        require(bytes.size()%4==0 && bytes.size()/4<=maxWeights-c.values.size(),"constant value bound exceeded");
        for(std::size_t i=0;i<bytes.size();i+=4) c.values.push_back(fp32(bytes.substr(i,4)));
    };
    while(m.more()) {
        auto f=m.next(); if(f.number!=1 && f.number!=4 && f.number!=16) m.one(f);
        switch(f.number) {
        case 1:
            if(f.wire==0) dimension(f.integer);
            else { wire(f,2); Message packed(f.bytes,budget); while(packed.more()) dimension(packed.integer()); }
            break;
        case 2: wire(f,0); element=f.integer; break;
        case 4: require(f.wire==2 || f.wire==5,"invalid float_data wire type"); floats(f.bytes); break;
        case 8: c.name=text(f); break;
        case 9: wire(f,2); raw=f.bytes; break;
        case 12: (void)text(f); break;
        case 14: wire(f,0); require(f.integer==0,"external constants are forbidden"); break;
        case 16: metadata(f,budget,keys); break;
        default: require(false,"unknown, external, segmented or mixed constant encoding");
        }
    }
    name(c.name); require(element==1,"FP32 constant required"); const auto count=product(c.dims,maxWeights);
    require(count<=maxWeights+1024-budget.weights,"aggregate constant element limit exceeded"); budget.weights+=count;
    require(raw.empty()!=c.values.empty(),"exactly one FP32 constant encoding required");
    if(!raw.empty()) { require(raw.size()==count*4,"raw constant length mismatch"); floats(raw); }
    require(c.values.size()==count,"constant element count mismatch"); return c;
}
struct Node { std::string op; std::vector<std::string> input,output; bool attributes=false; };
Node node(std::string_view data,Budget &budget) {
    Message m(data,budget); Node result; std::set<std::string> keys;
    while(m.more()) {
        auto f=m.next(); if(f.number!=1 && f.number!=2 && f.number!=5 && f.number!=9) m.one(f);
        switch(f.number) {
        case 1: require(result.input.size()<64,"node input bound exceeded"); result.input.push_back(text(f)); break;
        case 2: require(result.output.size()<16,"node output bound exceeded"); result.output.push_back(text(f)); break;
        case 3: case 6: (void)text(f); break;
        case 4: result.op=text(f); break;
        case 5: wire(f,2); result.attributes=true; break; // Envelope checks nested dependencies; a backend must validate semantics.
        case 7: require(text(f).empty(),"custom operator domains are forbidden"); break;
        case 9: metadata(f,budget,keys); break;
        default: require(false,"unknown node field or device configuration");
        }
    }
    name(result.op); require(!result.input.empty() && !result.output.empty(),"node must have inputs and outputs");
    for(const auto &s:result.input) name(s);
    for(const auto &s:result.output) name(s);
    return result;
}
struct Graph { std::vector<Node> nodes; std::vector<Constant> constants; std::vector<Value> input,output,intermediate; };
Graph graph(std::string_view data,Budget &budget) {
    Message m(data,budget); Graph result; std::set<std::string> keys;
    while(m.more()) {
        auto f=m.next();
        switch(f.number) {
        case 1: wire(f,2); require(result.nodes.size()<1024,"node limit exceeded"); result.nodes.push_back(node(f.bytes,budget)); break;
        case 2: case 10: m.one(f); (void)text(f); break;
        case 5: wire(f,2); require(result.constants.size()<1024,"initializer limit exceeded"); result.constants.push_back(constant(f.bytes,budget)); break;
        case 11: case 12: case 13: {
            wire(f,2); auto &list=f.number==11?result.input:f.number==12?result.output:result.intermediate;
            require(list.size()<(f.number==13?1024U:1U),"graph I/O or metadata limit exceeded"); list.push_back(value(f.bytes,budget)); break;
        }
        case 16: metadata(f,budget,keys); break;
        default: require(false,"unknown, sparse or quantized graph representation");
        }
    }
    require(result.input.size()==1 && result.output.size()==1 && !result.nodes.empty(),"one input/output and a nonempty graph required");
    std::set<std::string> available{result.input[0].name},metadataNames;
    for(const auto &c:result.constants) require(available.insert(c.name).second,"duplicate initializer or input name");
    for(const auto &n:result.nodes) {
        for(const auto &s:n.input) require(available.count(s),"unresolved or cyclic node input");
        for(const auto &s:n.output) require(available.insert(s).second,"duplicate graph value producer");
    }
    require(available.count(result.output[0].name),"unresolved graph output");
    for(const auto &v:result.intermediate) require(available.count(v.name) && metadataNames.insert(v.name).second,"unresolved or duplicate value metadata");
    return result;
}
Graph model(std::string_view bytes,Budget &budget) {
    Message m(bytes,budget); Graph result; bool foundGraph=false,foundOpset=false,foundIR=false; std::set<std::string> keys;
    while(m.more()) {
        auto f=m.next(); if(f.number!=14) m.one(f);
        switch(f.number) {
        case 1: wire(f,0); require(f.integer>=8 && f.integer<=10,"supported ONNX IR versions are 8 through 10"); foundIR=true; break;
        case 2: case 3: case 4: case 6: (void)text(f); break;
        case 5: wire(f,0); break;
        case 7: wire(f,2); result=graph(f.bytes,budget); foundGraph=true; break;
        case 8: {
            wire(f,2); Message opset(f.bytes,budget); bool version=false;
            while(opset.more()) {
                auto o=opset.next(); opset.one(o);
                if(o.number==1) require(text(o).empty(),"standard ONNX domain required");
                else if(o.number==2) { wire(o,0); require(o.integer==13,"opset 13 required"); version=true; }
                else require(false,"unknown opset field");
            }
            require(version,"missing opset version"); foundOpset=true; break;
        }
        case 14: metadata(f,budget,keys); break;
        default: require(false,"unknown model field or unsupported dependency container");
        }
    }
    require(foundIR && foundGraph && foundOpset,"missing model envelope field"); return result;
}
}

ImportResult inspectStaticAffine(std::vector<unsigned char> snapshot) {
    ImportResult result;
    try {
        require(!snapshot.empty() && snapshot.size()<=maxBytes,"model byte limit exceeded");
        // Reuse the runtime's closed dependency-envelope check before importing.
        runtime::immutable_detail::Envelope(snapshot).validate();
        Budget budget; const std::string_view bytes(reinterpret_cast<const char *>(snapshot.data()),snapshot.size());
        auto g=model(bytes,budget); const auto &input=g.input[0],&output=g.output[0];
        result.model.batch_=input.dims[0]; result.model.inputs_=input.dims[1]; result.model.outputs_=output.dims[1];
        result.model.sha256_=crypto::sha256(std::string(bytes));
        result.model.snapshot_=std::move(snapshot);
        if(g.nodes.size()!=2 || g.constants.size()!=2 || g.nodes[0].op!="MatMul" || g.nodes[1].op!="Add" ||
           g.nodes[0].attributes || g.nodes[1].attributes || g.nodes[0].input.size()!=2 || g.nodes[1].input.size()!=2 ||
           g.nodes[0].output.size()!=1 || g.nodes[1].output.size()!=1) {
            result.disposition=ImportDisposition::RequiresBackendValidation;
            result.reason="outside bounded MatMul+Add subset; real backend validation required"; return result;
        }
        const auto &mm=g.nodes[0],&add=g.nodes[1];
        require(input.dims[0]==output.dims[0],"affine batch shape mismatch");
        std::map<std::string,const Constant *> constants;
        for(const auto &c:g.constants) constants.emplace(c.name,&c);
        require(mm.input[0]==input.name && constants.count(mm.input[1]),"unsupported MatMul wiring");
        const auto *weights=constants.at(mm.input[1]);
        const auto *bias=g.constants[0].name==weights->name?&g.constants[1]:&g.constants[0];
        require(((add.input[0]==mm.output[0] && add.input[1]==bias->name) ||
                 (add.input[1]==mm.output[0] && add.input[0]==bias->name)) && add.output[0]==output.name,"unsupported Add wiring");
        require(weights->dims==std::vector<std::size_t>{input.dims[1],output.dims[1]} &&
                bias->dims==std::vector<std::size_t>{output.dims[1]},"affine constant shape mismatch");
        for(const auto &v:g.intermediate) require(v.name==mm.output[0] && v.dims==output.dims,"affine intermediate signature mismatch");
        result.model.weights_=weights->values; result.model.bias_=bias->values;
        result.disposition=ImportDisposition::Supported; result.reason="bounded static FP32 MatMul+Add";
    } catch(const std::runtime_error &error) {
        result=ImportResult{}; result.reason=error.what();
    }
    return result;
}
}
