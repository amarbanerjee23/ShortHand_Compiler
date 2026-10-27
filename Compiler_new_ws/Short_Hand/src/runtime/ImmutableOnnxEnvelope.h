#pragma once
// Private bounded protobuf walk over the already-owned snapshot. ORT validates
// graph/operator semantics after this dependency check. No file access here.
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace shorthand::runtime::immutable_detail {
class Envelope {
    enum class Kind { Model, Graph, Node, Attribute, Tensor, Sparse, Opset };
    const std::vector<unsigned char> &bytes;
    std::size_t pos=0, fields=0;
    [[noreturn]] static void fail() { throw std::runtime_error("unsupported_immutable_onnx_envelope"); }
    std::uint64_t integer(std::size_t end) {
        std::uint64_t value=0;
        for(unsigned shift=0;shift<70;shift+=7) {
            if(pos>=end) fail();
            const unsigned c=bytes[pos++];
            if(shift==63 && c>1) fail();
            value|=std::uint64_t(c&127)<<shift;
            if(!(c&128)) return value;
        }
        fail();
    }
    static bool listed(unsigned field,std::initializer_list<unsigned> values) {
        for(auto value:values) if(field==value) return true;
        return false;
    }
    void walk(Kind kind,std::size_t end,unsigned depth) {
        if(depth>64) fail();
        unsigned graphs=0, irs=0, versions=0, imports=0;
        while(pos<end) {
            if(++fields>1000000) fail();
            const auto key=integer(end);
            if((key>>3)==0 || (key>>3)>536870911) fail();
            const unsigned field=static_cast<unsigned>(key>>3), wire=key&7;
            // Closed set of dependency-bearing containers. New IR/device,
            // training/function, segmented or external representations reject.
            bool allowed=false;
            switch(kind) {
            case Kind::Model: allowed=listed(field,{1,2,3,4,5,6,7,8,14}); break;
            case Kind::Graph: allowed=listed(field,{1,2,5,10,11,12,13,15,16}); break;
            case Kind::Node: allowed=listed(field,{1,2,3,4,5,6,7,9}); break;
            case Kind::Attribute: allowed=listed(field,{1,2,3,4,5,6,7,8,9,10,11,13,14,15,20,22,23}); break;
            case Kind::Tensor: allowed=listed(field,{1,2,4,5,6,7,8,9,10,11,12,14,16}); break;
            case Kind::Sparse: allowed=listed(field,{1,2,3}); break;
            case Kind::Opset: allowed=listed(field,{1,2}); break;
            }
            if(!allowed) fail();
            // Reject a wrong-wire dependency field instead of treating it as
            // an unknown scalar and missing a nested tensor or graph.
            Kind nested=Kind::Model; bool visit=false;
            if(kind==Kind::Model && field==7) { nested=Kind::Graph; visit=true; ++graphs; }
            if(kind==Kind::Model && field==8) { nested=Kind::Opset; visit=true; ++imports; }
            if(kind==Kind::Graph && field==1) { nested=Kind::Node; visit=true; }
            if(kind==Kind::Graph && field==5) { nested=Kind::Tensor; visit=true; }
            if(kind==Kind::Graph && field==15) { nested=Kind::Sparse; visit=true; }
            if(kind==Kind::Node && field==5) { nested=Kind::Attribute; visit=true; }
            if(kind==Kind::Attribute && listed(field,{5,10})) { nested=Kind::Tensor; visit=true; }
            if(kind==Kind::Attribute && listed(field,{6,11})) { nested=Kind::Graph; visit=true; }
            if(kind==Kind::Attribute && listed(field,{22,23})) { nested=Kind::Sparse; visit=true; }
            if(kind==Kind::Sparse && listed(field,{1,2})) { nested=Kind::Tensor; visit=true; }
            const bool domain=(kind==Kind::Node && field==7) || (kind==Kind::Opset && field==1);
            const bool ir=kind==Kind::Model && field==1;
            const bool version=kind==Kind::Opset && field==2;
            const bool location=kind==Kind::Tensor && field==14;
            if((visit || domain) && wire!=2) fail();
            if((ir || version || location) && wire!=0) fail();
            if(wire==0) {
                const auto value=integer(end);
                if(ir && (++irs!=1 || value<8 || value>10)) fail();
                if(version && (++versions!=1 || value!=13)) fail();
                if(location && value!=0) fail();
            } else if(wire==1 || wire==5) {
                const auto size=wire==1?8U:4U;
                if(size>end-pos) fail();
                pos+=size;
            } else if(wire==2) {
                const auto size=integer(end);
                if(size>end-pos) fail();
                const auto limit=pos+static_cast<std::size_t>(size);
                if(domain && size!=0) fail(); // standard empty ONNX domain only
                if(visit) walk(nested,limit,depth+1);
                else pos=limit;
                if(pos!=limit) fail();
            } else fail();
        }
        if(kind==Kind::Model && (irs!=1 || graphs!=1 || imports!=1)) fail();
        if(kind==Kind::Opset && versions!=1) fail();
    }
public:
    explicit Envelope(const std::vector<unsigned char> &snapshot):bytes(snapshot) {}
    void validate() { walk(Kind::Model,bytes.size(),0); }
};
}
