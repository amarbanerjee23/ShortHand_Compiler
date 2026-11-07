#include "aot/StaticAffineModel.h"
#include "module/Sha256.h"
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
std::string fingerprint(const std::vector<float> &values) {
    std::string bytes;
    for(float value:values) {
        std::uint32_t bits=0; std::memcpy(&bits,&value,4);
        for(unsigned i=0;i<4;++i) bytes.push_back(static_cast<char>((bits>>(8*i))&255));
    }
    return shorthand::crypto::sha256(bytes);
}
int main(int argc,char **argv) {
    if(argc!=2) return 2;
    std::ifstream stream(argv[1],std::ios::binary);
    if(!stream) return 2;
    std::vector<unsigned char> bytes(16*1024*1024+1);
    stream.read(reinterpret_cast<char *>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
    bytes.resize(static_cast<std::size_t>(stream.gcount()));
    const auto expected=shorthand::crypto::sha256(std::string(bytes.begin(),bytes.end()));
    const auto result=shorthand::aot::inspectStaticAffine(bytes);
    // Mutating the caller's original input cannot change the owned snapshot.
    for(auto &byte:bytes) byte^=255;
    const auto &m=result.model;
    if(result.disposition!=shorthand::aot::ImportDisposition::Rejected &&
       (m.sha256()!=expected || shorthand::crypto::sha256(std::string(m.snapshot().begin(),m.snapshot().end()))!=expected))
        throw std::runtime_error("snapshot ownership failure");
    std::cout<<"{\"disposition\":"<<static_cast<int>(result.disposition)<<",\"reason\":\""<<result.reason
             <<"\",\"sha256\":\""<<m.sha256()<<"\",\"shape\":["<<m.batch()<<','<<m.inputs()<<','<<m.outputs()
             <<"],\"owned_bytes\":"<<m.snapshot().size()<<",\"weights_sha256\":\""<<fingerprint(m.weights())
             <<"\",\"bias_sha256\":\""<<fingerprint(m.bias())<<"\"}\n";
}
