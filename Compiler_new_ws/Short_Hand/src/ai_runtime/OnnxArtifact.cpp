#include "OnnxArtifact.h"
#include <fstream>
#include <limits>
#include <map>
#include <stdexcept>
namespace shorthand::ai {
namespace {
enum class Message { Model,Graph,Node,Attribute,Tensor,Sparse,Training,Function };
class Reader {
public:
    explicit Reader(const std::string &path):file(path,std::ios::binary) {
        if (!file) throw std::runtime_error("onnx_artifact_open_failed");
    }
    std::set<std::string> external;
    void message(Message type,std::uint64_t end,unsigned depth) {
        if (depth>64) fail();
        std::map<std::string,std::string> entries; bool external_location=false;
        while (position<end) {
            if (++fields>1000000) fail();
            const auto key=varint(end); const auto field=key>>3, wire=key&7;
            if (!field || field>536870911) fail();
            if (wire==0) {
                auto value=varint(end);
                if (type==Message::Tensor && field==14) { if (value>1) fail(); external_location=value==1; }
                if (type==Message::Tensor && field==2 && value!=1 && value!=6 && value!=7 && value!=9)
                    throw std::runtime_error("qualification_model_contains_non_fp32_tensor_precision");
                continue;
            }
            if (wire==1 || wire==5) { skip(wire==1?8:4,end); continue; }
            if (wire!=2) fail();
            const auto size=varint(end); if (size>end-position) fail(); const auto limit=position+size;
            if (type==Message::Tensor && field==13) {
                std::string name,value; bool has_name=false,has_value=false;
                while (position<limit) {
                    auto k=varint(limit); if (k!=10 && k!=18) fail();
                    auto n=varint(limit); auto s=text(n,limit);
                    if (k==10) { if (has_name) fail(); name=s; has_name=true; }
                    else { if (has_value) fail(); value=s; has_value=true; }
                }
                if (!has_name || !has_value || !entries.emplace(name,value).second) fail();
            } else if (type==Message::Node && field==4) {
                const auto op=text(size,limit);
                if (op=="QuantizeLinear" || op=="DequantizeLinear" || op.rfind("QLinear",0)==0 || op=="MatMulInteger" || op=="ConvInteger")
                    throw std::runtime_error("qualification_model_contains_quantization");
            } else {
                Message nested; bool visit=true;
                switch (type) {
                case Message::Model: if (field==7) nested=Message::Graph; else if (field==20) nested=Message::Training; else if (field==25) nested=Message::Function; else visit=false; break;
                case Message::Graph: if (field==1) nested=Message::Node; else if (field==5) nested=Message::Tensor; else if (field==15) nested=Message::Sparse; else visit=false; break;
                case Message::Node: if (field==5) nested=Message::Attribute; else visit=false; break;
                case Message::Attribute: if (field==5 || field==10) nested=Message::Tensor; else if (field==6 || field==11) nested=Message::Graph; else if (field==22 || field==23) nested=Message::Sparse; else visit=false; break;
                case Message::Sparse: if (field==1 || field==2) nested=Message::Tensor; else visit=false; break;
                case Message::Training: if (field==1 || field==2) nested=Message::Graph; else visit=false; break;
                case Message::Function: if (field==7) nested=Message::Node; else if (field==11) nested=Message::Attribute; else visit=false; break;
                default: visit=false; break;
                }
                if (visit) message(nested,limit,depth+1); else skip(size,limit);
            }
            if (position!=limit) fail();
        }
        if (type==Message::Tensor && (external_location || !entries.empty())) {
            if (!external_location || !entries.count("location")) fail();
            const auto &name=entries.at("location");
            // Single filenames only: no traversal, drive paths, separators or controls.
            if (name.empty() || name=="." || name==".." || name.size()>255 || name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-")!=std::string::npos)
                throw std::runtime_error("unsafe_onnx_external_location");
            for (const auto &e:entries) {
                if (e.first!="location" && e.first!="offset" && e.first!="length" && e.first!="checksum") fail();
                if (e.first=="offset" || e.first=="length") {
                    if (e.second.empty() || e.second.size()>19 || e.second.find_first_not_of("0123456789")!=std::string::npos) fail();
                }
            }
            external.insert(name);
            if (external.size()>32) fail();
        }
    }
private:
    std::ifstream file; std::uint64_t position=0,fields=0;
    [[noreturn]] static void fail() { throw std::runtime_error("invalid_or_oversize_onnx_envelope"); }
    std::uint64_t varint(std::uint64_t end) {
        std::uint64_t value=0;
        for (unsigned shift=0;shift<70;shift+=7) {
            if (position>=end) fail();
            int next=file.get(); if (next<0) fail(); ++position;
            auto byte=static_cast<unsigned>(next); if (shift==63 && byte>1) fail();
            value|=std::uint64_t(byte&127)<<shift; if (!(byte&128)) return value;
        }
        fail();
    }
    void skip(std::uint64_t size,std::uint64_t end) {
        if (size>end-position || size>std::uint64_t(std::numeric_limits<std::streamoff>::max())) fail();
        file.seekg(static_cast<std::streamoff>(size),std::ios::cur); if (!file) fail(); position+=size;
    }
    std::string text(std::uint64_t size,std::uint64_t end) {
        if (size>4096 || size>end-position) fail();
        std::string s(static_cast<std::size_t>(size),'\0');
        file.read(s.data(),static_cast<std::streamsize>(size)); if (!file) fail(); position+=size; return s;
    }
};
}
std::set<std::string> inspectOnnxExternalFiles(const std::string &path,std::uint64_t maximum_bytes) {
    std::ifstream size(path,std::ios::binary|std::ios::ate); auto n=size.tellg();
    if (n<=0 || static_cast<std::uint64_t>(n)>maximum_bytes) throw std::runtime_error("onnx_model_size_limit");
    Reader reader(path); reader.message(Message::Model,static_cast<std::uint64_t>(n),0); return reader.external;
}
}
