#include "module/Sha256.h"

#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char **argv) {
    if (shorthand::crypto::sha256("") !=
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855") return 1;
    if (shorthand::crypto::sha256("abc") !=
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad") return 2;
    if (shorthand::crypto::sha256("The quick brown fox jumps over the lazy dog") !=
        "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592") return 3;
    const std::string million_a(1000000U, 'a');
    const std::string expected = "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0";
    if (shorthand::crypto::sha256(million_a) != expected) return 4;
    if (argc == 2) {
        {
            std::ofstream out(argv[1], std::ios::binary | std::ios::trunc);
            out.write(million_a.data(), static_cast<std::streamsize>(million_a.size()));
            if (!out.good()) return 5;
        }
        std::string file_digest;
        if (!shorthand::crypto::sha256File(argv[1], file_digest) || file_digest != expected) return 6;
    }
    std::cout << "PASS SHA-256 known-answer vectors\n";
    return 0;
}
