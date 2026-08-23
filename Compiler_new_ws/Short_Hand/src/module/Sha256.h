#ifndef SHORTHAND_SHA256_H
#define SHORTHAND_SHA256_H

#include <string>

namespace shorthand::crypto {

std::string sha256(const std::string &bytes);
bool sha256File(const std::string &path, std::string &digest);

}  // namespace shorthand::crypto

#endif
