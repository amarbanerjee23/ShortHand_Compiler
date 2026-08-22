#include "Sha256.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>

namespace shorthand::crypto {
namespace {

constexpr std::array<std::uint32_t, 64> kRoundConstants = {
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
    0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U, 0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
    0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU, 0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
    0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U, 0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
    0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U, 0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
    0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U, 0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
    0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
    0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U, 0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U
};

std::uint32_t rotateRight(std::uint32_t value, unsigned count) {
    return (value >> count) | (value << (32U - count));
}

class Sha256Context final {
public:
    bool update(const std::uint8_t *bytes, std::size_t length) {
        if (length > std::numeric_limits<std::uint64_t>::max() - total_bytes_) return false;
        total_bytes_ += static_cast<std::uint64_t>(length);
        while (length != 0U) {
            const std::size_t available = block_.size() - block_size_;
            const std::size_t count = length < available ? length : available;
            for (std::size_t i = 0; i < count; ++i) block_[block_size_ + i] = bytes[i];
            block_size_ += count;
            bytes += count;
            length -= count;
            if (block_size_ == block_.size()) {
                transform(block_.data());
                block_size_ = 0;
            }
        }
        return true;
    }

    bool finish(std::string &digest) {
        if (total_bytes_ > std::numeric_limits<std::uint64_t>::max() / 8U) return false;
        const std::uint64_t bit_count = total_bytes_ * 8U;
        block_[block_size_++] = 0x80U;
        if (block_size_ > 56U) {
            while (block_size_ < block_.size()) block_[block_size_++] = 0U;
            transform(block_.data());
            block_size_ = 0;
        }
        while (block_size_ < 56U) block_[block_size_++] = 0U;
        for (unsigned i = 0; i < 8U; ++i)
            block_[56U + i] = static_cast<std::uint8_t>((bit_count >> (56U - i * 8U)) & 0xffU);
        transform(block_.data());
        block_size_ = 0;

        std::ostringstream out;
        out << std::hex << std::setfill('0');
        for (std::uint32_t value : state_) out << std::setw(8) << value;
        digest = out.str();
        return true;
    }

private:
    void transform(const std::uint8_t *block) {
        std::array<std::uint32_t, 64> words{};
        for (std::size_t i = 0; i < 16U; ++i) {
            const std::size_t index = i * 4U;
            words[i] = (static_cast<std::uint32_t>(block[index]) << 24U) |
                       (static_cast<std::uint32_t>(block[index + 1U]) << 16U) |
                       (static_cast<std::uint32_t>(block[index + 2U]) << 8U) |
                       static_cast<std::uint32_t>(block[index + 3U]);
        }
        for (std::size_t i = 16U; i < 64U; ++i) {
            const std::uint32_t s0 = rotateRight(words[i - 15U], 7U) ^
                                     rotateRight(words[i - 15U], 18U) ^
                                     (words[i - 15U] >> 3U);
            const std::uint32_t s1 = rotateRight(words[i - 2U], 17U) ^
                                     rotateRight(words[i - 2U], 19U) ^
                                     (words[i - 2U] >> 10U);
            words[i] = words[i - 16U] + s0 + words[i - 7U] + s1;
        }

        std::uint32_t a = state_[0];
        std::uint32_t b = state_[1];
        std::uint32_t c = state_[2];
        std::uint32_t d = state_[3];
        std::uint32_t e = state_[4];
        std::uint32_t f = state_[5];
        std::uint32_t g = state_[6];
        std::uint32_t h = state_[7];
        for (std::size_t i = 0; i < 64U; ++i) {
            const std::uint32_t sum1 = rotateRight(e, 6U) ^ rotateRight(e, 11U) ^ rotateRight(e, 25U);
            const std::uint32_t choose = (e & f) ^ ((~e) & g);
            const std::uint32_t temp1 = h + sum1 + choose + kRoundConstants[i] + words[i];
            const std::uint32_t sum0 = rotateRight(a, 2U) ^ rotateRight(a, 13U) ^ rotateRight(a, 22U);
            const std::uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
            const std::uint32_t temp2 = sum0 + majority;
            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }
        state_[0] += a;
        state_[1] += b;
        state_[2] += c;
        state_[3] += d;
        state_[4] += e;
        state_[5] += f;
        state_[6] += g;
        state_[7] += h;
    }

    std::array<std::uint32_t, 8> state_ = {
        0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
        0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U
    };
    std::array<std::uint8_t, 64> block_{};
    std::size_t block_size_ = 0;
    std::uint64_t total_bytes_ = 0;
};

}  // namespace

std::string sha256(const std::string &bytes) {
    Sha256Context context;
    std::string digest;
    if (!context.update(reinterpret_cast<const std::uint8_t *>(bytes.data()), bytes.size()) ||
        !context.finish(digest)) return std::string();
    return digest;
}

bool sha256File(const std::string &path, std::string &digest) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    Sha256Context context;
    std::array<char, 8192> buffer{};
    while (in) {
        in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const std::streamsize count = in.gcount();
        if (count > 0 && !context.update(
                reinterpret_cast<const std::uint8_t *>(buffer.data()),
                static_cast<std::size_t>(count))) return false;
    }
    if (in.bad()) return false;
    return context.finish(digest);
}

}  // namespace shorthand::crypto
