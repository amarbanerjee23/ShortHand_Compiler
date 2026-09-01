#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <random>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace fs = std::filesystem;

namespace shorthand::lsp {

constexpr std::size_t kMaxMessageBytes = 1024 * 1024;
constexpr int kRequestCancelled = -32800;

class Json {
public:
    using Object = std::map<std::string, Json>;
    using Array = std::vector<Json>;
    using Storage = std::variant<std::nullptr_t, bool, double, std::string, Object, Array>;

    Json() : value_(nullptr) {}
    Json(std::nullptr_t) : value_(nullptr) {}
    Json(bool value) : value_(value) {}
    Json(int value) : value_(static_cast<double>(value)) {}
    Json(long long value) : value_(static_cast<double>(value)) {}
    Json(double value) : value_(value) {}
    Json(const char *value) : value_(std::string(value)) {}
    Json(std::string value) : value_(std::move(value)) {}
    Json(Object value) : value_(std::move(value)) {}
    Json(Array value) : value_(std::move(value)) {}

    bool isNull() const { return std::holds_alternative<std::nullptr_t>(value_); }
    const std::string *asString() const { return std::get_if<std::string>(&value_); }
    const Object *asObject() const { return std::get_if<Object>(&value_); }
    Object *asObject() { return std::get_if<Object>(&value_); }
    const Array *asArray() const { return std::get_if<Array>(&value_); }
    const bool *asBool() const { return std::get_if<bool>(&value_); }
    const double *asNumber() const { return std::get_if<double>(&value_); }

    const Json *get(const std::string &key) const {
        const Object *object = asObject();
        if (!object) return nullptr;
        auto it = object->find(key);
        return it == object->end() ? nullptr : &it->second;
    }

    std::optional<std::string> getString(const std::string &key) const {
        const Json *item = get(key);
        if (!item || !item->asString()) return std::nullopt;
        return *item->asString();
    }

    std::optional<long long> getInteger(const std::string &key) const {
        const Json *item = get(key);
        if (!item || !item->asNumber()) return std::nullopt;
        const double value = *item->asNumber();
        if (!std::isfinite(value) || std::floor(value) != value ||
            value < static_cast<double>(std::numeric_limits<long long>::min()) ||
            value > static_cast<double>(std::numeric_limits<long long>::max())) return std::nullopt;
        return static_cast<long long>(value);
    }

    std::string dump() const {
        std::ostringstream out;
        dumpInto(out);
        return out.str();
    }

private:
    Storage value_;

    static void dumpString(std::ostream &out, const std::string &value) {
        out << '"';
        for (unsigned char ch : value) {
            switch (ch) {
                case '"': out << "\\\""; break;
                case '\\': out << "\\\\"; break;
                case '\b': out << "\\b"; break;
                case '\f': out << "\\f"; break;
                case '\n': out << "\\n"; break;
                case '\r': out << "\\r"; break;
                case '\t': out << "\\t"; break;
                default:
                    if (ch < 0x20) {
                        out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                            << static_cast<int>(ch) << std::dec << std::setw(0);
                    } else {
                        out << static_cast<char>(ch);
                    }
            }
        }
        out << '"';
    }

    void dumpInto(std::ostream &out) const {
        if (isNull()) { out << "null"; return; }
        if (const bool *value = asBool()) { out << (*value ? "true" : "false"); return; }
        if (const double *value = asNumber()) {
            if (std::floor(*value) == *value &&
                *value >= static_cast<double>(std::numeric_limits<long long>::min()) &&
                *value <= static_cast<double>(std::numeric_limits<long long>::max())) {
                out << static_cast<long long>(*value);
            } else {
                out << std::setprecision(17) << *value;
            }
            return;
        }
        if (const std::string *value = asString()) { dumpString(out, *value); return; }
        if (const Object *object = asObject()) {
            out << '{';
            bool first = true;
            for (const auto &entry : *object) {
                if (!first) out << ',';
                first = false;
                dumpString(out, entry.first);
                out << ':';
                entry.second.dumpInto(out);
            }
            out << '}';
            return;
        }
        const Array &array = *asArray();
        out << '[';
        for (std::size_t i = 0; i < array.size(); ++i) {
            if (i) out << ',';
            array[i].dumpInto(out);
        }
        out << ']';
    }
};

class JsonParser {
public:
    explicit JsonParser(const std::string &text) : text_(text) {}

    std::optional<Json> parse(std::string &error) {
        try {
            skipWhitespace();
            Json value = parseValue(0);
            skipWhitespace();
            if (position_ != text_.size()) fail("trailing JSON content");
            return value;
        } catch (const std::runtime_error &ex) {
            error = ex.what();
            return std::nullopt;
        }
    }

private:
    const std::string &text_;
    std::size_t position_ = 0;

    [[noreturn]] void fail(const std::string &message) const {
        throw std::runtime_error(message + " at byte " + std::to_string(position_));
    }

    void skipWhitespace() {
        while (position_ < text_.size() &&
               (text_[position_] == ' ' || text_[position_] == '\n' ||
                text_[position_] == '\r' || text_[position_] == '\t')) ++position_;
    }

    bool consume(char expected) {
        if (position_ < text_.size() && text_[position_] == expected) {
            ++position_;
            return true;
        }
        return false;
    }

    static void appendUtf8(std::string &out, unsigned codepoint) {
        if (codepoint <= 0x7F) out.push_back(static_cast<char>(codepoint));
        else if (codepoint <= 0x7FF) {
            out.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        } else if (codepoint <= 0xFFFF) {
            out.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
            out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
            out.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
    }

    unsigned parseHex4() {
        if (position_ + 4 > text_.size()) fail("truncated unicode escape");
        unsigned value = 0;
        for (int i = 0; i < 4; ++i) {
            char ch = text_[position_++];
            value <<= 4;
            if (ch >= '0' && ch <= '9') value |= static_cast<unsigned>(ch - '0');
            else if (ch >= 'a' && ch <= 'f') value |= static_cast<unsigned>(ch - 'a' + 10);
            else if (ch >= 'A' && ch <= 'F') value |= static_cast<unsigned>(ch - 'A' + 10);
            else fail("invalid unicode escape");
        }
        return value;
    }

    std::string parseString() {
        if (!consume('"')) fail("expected string");
        std::string out;
        while (position_ < text_.size()) {
            unsigned char ch = static_cast<unsigned char>(text_[position_++]);
            if (ch == '"') return out;
            if (ch < 0x20) fail("unescaped control character");
            if (ch != '\\') { out.push_back(static_cast<char>(ch)); continue; }
            if (position_ >= text_.size()) fail("truncated escape");
            char escaped = text_[position_++];
            switch (escaped) {
                case '"': out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                case '/': out.push_back('/'); break;
                case 'b': out.push_back('\b'); break;
                case 'f': out.push_back('\f'); break;
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                case 'u': {
                    unsigned codepoint = parseHex4();
                    if (codepoint >= 0xD800 && codepoint <= 0xDBFF) {
                        if (position_ + 2 > text_.size() || text_[position_] != '\\' || text_[position_ + 1] != 'u')
                            fail("missing low surrogate");
                        position_ += 2;
                        unsigned low = parseHex4();
                        if (low < 0xDC00 || low > 0xDFFF) fail("invalid low surrogate");
                        codepoint = 0x10000 + ((codepoint - 0xD800) << 10) + (low - 0xDC00);
                    } else if (codepoint >= 0xDC00 && codepoint <= 0xDFFF) {
                        fail("unexpected low surrogate");
                    }
                    appendUtf8(out, codepoint);
                    break;
                }
                default: fail("invalid escape");
            }
        }
        fail("unterminated string");
    }

    Json parseNumber() {
        std::size_t start = position_;
        if (consume('-')) {}
        if (consume('0')) {
            if (position_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[position_])))
                fail("leading zero in number");
        } else {
            if (position_ >= text_.size() || !std::isdigit(static_cast<unsigned char>(text_[position_])))
                fail("invalid number");
            while (position_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[position_]))) ++position_;
        }
        if (consume('.')) {
            if (position_ >= text_.size() || !std::isdigit(static_cast<unsigned char>(text_[position_]))) fail("invalid fraction");
            while (position_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[position_]))) ++position_;
        }
        if (position_ < text_.size() && (text_[position_] == 'e' || text_[position_] == 'E')) {
            ++position_;
            if (position_ < text_.size() && (text_[position_] == '+' || text_[position_] == '-')) ++position_;
            if (position_ >= text_.size() || !std::isdigit(static_cast<unsigned char>(text_[position_]))) fail("invalid exponent");
            while (position_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[position_]))) ++position_;
        }
        const std::string token = text_.substr(start, position_ - start);
        char *end = nullptr;
        errno = 0;
        double value = std::strtod(token.c_str(), &end);
        if (errno == ERANGE || !end || *end != '\0' || !std::isfinite(value)) fail("number out of range");
        return Json(value);
    }

    Json parseArray(std::size_t depth) {
        consume('[');
        Json::Array values;
        skipWhitespace();
        if (consume(']')) return Json(std::move(values));
        while (true) {
            skipWhitespace();
            values.push_back(parseValue(depth + 1));
            skipWhitespace();
            if (consume(']')) break;
            if (!consume(',')) fail("expected comma in array");
        }
        return Json(std::move(values));
    }

    Json parseObject(std::size_t depth) {
        consume('{');
        Json::Object values;
        skipWhitespace();
        if (consume('}')) return Json(std::move(values));
        while (true) {
            skipWhitespace();
            if (position_ >= text_.size() || text_[position_] != '"') fail("expected object key");
            std::string key = parseString();
            skipWhitespace();
            if (!consume(':')) fail("expected colon after object key");
            skipWhitespace();
            if (values.find(key) != values.end()) fail("duplicate object key");
            values.emplace(std::move(key), parseValue(depth + 1));
            skipWhitespace();
            if (consume('}')) break;
            if (!consume(',')) fail("expected comma in object");
        }
        return Json(std::move(values));
    }

    Json parseValue(std::size_t depth) {
        if (depth > 64) fail("JSON nesting limit exceeded");
        skipWhitespace();
        if (position_ >= text_.size()) fail("unexpected end of JSON");
        char ch = text_[position_];
        if (ch == '"') return Json(parseString());
        if (ch == '{') return parseObject(depth);
        if (ch == '[') return parseArray(depth);
        if (ch == '-' || std::isdigit(static_cast<unsigned char>(ch))) return parseNumber();
        if (text_.compare(position_, 4, "true") == 0) { position_ += 4; return Json(true); }
        if (text_.compare(position_, 5, "false") == 0) { position_ += 5; return Json(false); }
        if (text_.compare(position_, 4, "null") == 0) { position_ += 4; return Json(nullptr); }
        fail("invalid JSON value");
    }
};

struct Position { int line = 0; int character = 0; };
struct SourceLocation { std::string uri; int line = 0; int start = 0; int end = 0; };
struct Document { std::string text; long long version = 0; };

static std::vector<std::string> splitLines(const std::string &text) {
    std::vector<std::string> lines;
    std::string current;
    for (char ch : text) {
        if (ch == '\n') { lines.push_back(current); current.clear(); }
        else if (ch != '\r') current.push_back(ch);
    }
    lines.push_back(current);
    return lines;
}

static bool identifierChar(unsigned char ch) {
    return std::isalnum(ch) || ch == '_';
}

static std::size_t utf16ColumnToByte(const std::string &line, int utf16Column) {
    if (utf16Column <= 0) return 0;
    std::size_t byte = 0;
    int units = 0;
    while (byte < line.size() && units < utf16Column) {
        unsigned char lead = static_cast<unsigned char>(line[byte]);
        std::size_t width = 1;
        unsigned codepoint = lead;
        if ((lead & 0xE0) == 0xC0 && byte + 1 < line.size()) { width = 2; codepoint = lead & 0x1F; }
        else if ((lead & 0xF0) == 0xE0 && byte + 2 < line.size()) { width = 3; codepoint = lead & 0x0F; }
        else if ((lead & 0xF8) == 0xF0 && byte + 3 < line.size()) { width = 4; codepoint = lead & 0x07; }
        for (std::size_t i = 1; i < width; ++i) {
            unsigned char continuation = static_cast<unsigned char>(line[byte + i]);
            if ((continuation & 0xC0) != 0x80) { width = 1; codepoint = lead; break; }
            codepoint = (codepoint << 6) | (continuation & 0x3F);
        }
        int needed = codepoint > 0xFFFF ? 2 : 1;
        if (units + needed > utf16Column) break;
        units += needed;
        byte += width;
    }
    return byte;
}

static int byteColumnToUtf16(const std::string &line, std::size_t byteColumn) {
    byteColumn = std::min(byteColumn, line.size());
    std::size_t byte = 0;
    int units = 0;
    while (byte < byteColumn) {
        unsigned char lead = static_cast<unsigned char>(line[byte]);
        std::size_t width = 1;
        unsigned codepoint = lead;
        if ((lead & 0xE0) == 0xC0 && byte + 1 < line.size()) { width = 2; codepoint = lead & 0x1F; }
        else if ((lead & 0xF0) == 0xE0 && byte + 2 < line.size()) { width = 3; codepoint = lead & 0x0F; }
        else if ((lead & 0xF8) == 0xF0 && byte + 3 < line.size()) { width = 4; codepoint = lead & 0x07; }
        for (std::size_t i = 1; i < width; ++i) {
            unsigned char continuation = static_cast<unsigned char>(line[byte + i]);
            if ((continuation & 0xC0) != 0x80) { width = 1; codepoint = lead; break; }
            codepoint = (codepoint << 6) | (continuation & 0x3F);
        }
        if (byte + width > byteColumn) break;
        units += codepoint > 0xFFFF ? 2 : 1;
        byte += width;
    }
    return units;
}

static std::string wordAt(const std::string &text, Position position) {
    const auto lines = splitLines(text);
    if (position.line < 0 || static_cast<std::size_t>(position.line) >= lines.size()) return {};
    const std::string &line = lines[static_cast<std::size_t>(position.line)];
    if (position.character < 0) return {};
    std::size_t index = utf16ColumnToByte(line, position.character);
    if (index > line.size()) index = line.size();
    if (index == line.size() && index > 0) --index;
    if (line.empty() || !identifierChar(static_cast<unsigned char>(line[index]))) {
        if (index > 0 && identifierChar(static_cast<unsigned char>(line[index - 1]))) --index;
        else return {};
    }
    std::size_t begin = index;
    std::size_t end = index + 1;
    while (begin > 0 && identifierChar(static_cast<unsigned char>(line[begin - 1]))) --begin;
    while (end < line.size() && identifierChar(static_cast<unsigned char>(line[end]))) ++end;
    return line.substr(begin, end - begin);
}

static std::string percentDecode(const std::string &value) {
    std::string out;
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '%' && i + 2 < value.size()) {
            auto hex = [](char ch) -> int {
                if (ch >= '0' && ch <= '9') return ch - '0';
                if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
                if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
                return -1;
            };
            int high = hex(value[i + 1]);
            int low = hex(value[i + 2]);
            if (high >= 0 && low >= 0) {
                out.push_back(static_cast<char>((high << 4) | low));
                i += 2;
                continue;
            }
        }
        out.push_back(value[i]);
    }
    return out;
}

static fs::path uriToPath(const std::string &uri) {
    const std::string prefix = "file://";
    if (uri.rfind(prefix, 0) != 0) return {};
    std::string decoded = percentDecode(uri.substr(prefix.size()));
#ifdef _WIN32
    if (decoded.size() >= 3 && decoded[0] == '/' && std::isalpha(static_cast<unsigned char>(decoded[1])) && decoded[2] == ':')
        decoded.erase(decoded.begin());
#endif
    return fs::path(decoded);
}

static std::string pathToUri(const fs::path &path) {
    std::string raw = fs::absolute(path).generic_string();
    std::ostringstream out;
#ifdef _WIN32
    out << "file:///";
#else
    out << "file://";
#endif
    for (unsigned char ch : raw) {
        if (std::isalnum(ch) || ch == '/' || ch == ':' || ch == '-' || ch == '_' || ch == '.' || ch == '~') out << static_cast<char>(ch);
        else out << '%' << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(ch)
                 << std::nouppercase << std::dec;
    }
    return out.str();
}

static Json makePosition(int line, int character) {
    return Json::Object{{"character", character}, {"line", line}};
}

static Json makeRange(int line, int start, int end) {
    return Json::Object{{"end", makePosition(line, end)}, {"start", makePosition(line, start)}};
}

static Json makeLocation(const SourceLocation &location) {
    return Json::Object{{"range", makeRange(location.line, location.start, location.end)}, {"uri", location.uri}};
}

static std::optional<Position> requestPosition(const Json &request) {
    const Json *params = request.get("params");
    const Json *position = params ? params->get("position") : nullptr;
    if (!position) return std::nullopt;
    auto line = position->getInteger("line");
    auto character = position->getInteger("character");
    if (!line || !character || *line < 0 || *character < 0 || *line > INT32_MAX || *character > INT32_MAX)
        return std::nullopt;
    return Position{static_cast<int>(*line), static_cast<int>(*character)};
}

static std::optional<std::string> requestUri(const Json &request) {
    const Json *params = request.get("params");
    const Json *document = params ? params->get("textDocument") : nullptr;
    if (!document) return std::nullopt;
    return document->getString("uri");
}

static std::string idKey(const Json *id) {
    if (!id || id->isNull()) return {};
    if (const std::string *value = id->asString()) return "s:" + *value;
    if (const double *value = id->asNumber()) {
        std::ostringstream out;
        out << "n:" << std::setprecision(17) << *value;
        return out.str();
    }
    return {};
}

class TempDirectory {
public:
    TempDirectory() {
        std::random_device rd;
        for (int attempt = 0; attempt < 32; ++attempt) {
            std::ostringstream name;
            name << "shorthand-lsp-" << std::hex << rd() << '-' << rd();
            path_ = fs::temp_directory_path() / name.str();
            std::error_code ec;
            if (fs::create_directory(path_, ec)) return;
        }
        throw std::runtime_error("unable to create temporary analysis directory");
    }
    ~TempDirectory() { std::error_code ec; fs::remove_all(path_, ec); }
    const fs::path &path() const { return path_; }
private:
    fs::path path_;
};

static std::string doubleQuoted(const std::string &value) {
    std::string out = "\"";
    for (char ch : value) {
        if (ch == '\\' || ch == '"' || ch == '$' || ch == '`') out.push_back('\\');
        out.push_back(ch);
    }
    out.push_back('"');
    return out;
}

static std::string readFile(const fs::path &path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream data;
    data << in.rdbuf();
    return data.str();
}

static Json::Array compilerDiagnostics(const std::string &text) {
    Json::Array diagnostics;
    try {
        TempDirectory temp;
        fs::path source = temp.path() / "document.short";
        fs::path stdoutPath = temp.path() / "compiler.out";
        fs::path stderrPath = temp.path() / "compiler.err";
        { std::ofstream out(source, std::ios::binary); out << text; }

        const char *configured = std::getenv("SHORTHAND_COMPILER");
        const std::string compiler = configured && *configured ? configured : "short_hand";
        std::string command = doubleQuoted(compiler) + " " + doubleQuoted(source.string()) + " parse >" +
                              doubleQuoted(stdoutPath.string()) + " 2>" + doubleQuoted(stderrPath.string());
        const int status = std::system(command.c_str());
        const std::string errors = readFile(stderrPath);
        if (status == 0) return diagnostics;

        const auto sourceLines = splitLines(text);
        const std::regex structured(R"((.+?):(\d+):(\d+):\s+(error|warning):\s+\[([^\]]+)\]\s+(.+?)\s+\[range\s+(\d+):(\d+)-(\d+):(\d+)\])");
        auto begin = std::sregex_iterator(errors.begin(), errors.end(), structured);
        auto end = std::sregex_iterator();
        for (auto it = begin; it != end; ++it) {
            const std::smatch &match = *it;
            int beginLine = std::max(1, std::stoi(match[7].str())) - 1;
            int beginByteColumn = std::max(1, std::stoi(match[8].str())) - 1;
            int endLine = std::max(1, std::stoi(match[9].str())) - 1;
            int endByteColumn = std::max(1, std::stoi(match[10].str()));
            int beginColumn = beginByteColumn;
            int endColumn = endByteColumn;
            if (beginLine >= 0 && static_cast<std::size_t>(beginLine) < sourceLines.size())
                beginColumn = byteColumnToUtf16(sourceLines[static_cast<std::size_t>(beginLine)],
                                                static_cast<std::size_t>(beginByteColumn));
            if (endLine >= 0 && static_cast<std::size_t>(endLine) < sourceLines.size())
                endColumn = byteColumnToUtf16(sourceLines[static_cast<std::size_t>(endLine)],
                                              static_cast<std::size_t>(endByteColumn));
            Json range = Json::Object{
                {"end", makePosition(endLine, endColumn)},
                {"start", makePosition(beginLine, beginColumn)}};
            diagnostics.emplace_back(Json::Object{
                {"code", match[5].str()},
                {"message", match[6].str()},
                {"range", std::move(range)},
                {"severity", match[4].str() == "warning" ? 2 : 1},
                {"source", "shorthand-compiler"}});
        }
        if (!diagnostics.empty()) return diagnostics;

        std::string message = "compiler parser rejected document";
        if (errors.find("not found") != std::string::npos || status == 32512 || status == 127) {
            message = "ShortHand compiler oracle unavailable; set SHORTHAND_COMPILER to the short_hand executable";
        }
        diagnostics.emplace_back(Json::Object{
            {"code", "SHLSP900"},
            {"message", message},
            {"range", makeRange(0, 0, 1)},
            {"severity", 1},
            {"source", "shorthand-lsp"}});
        return diagnostics;
    } catch (const std::exception &ex) {
        diagnostics.emplace_back(Json::Object{
            {"code", "SHLSP901"},
            {"message", std::string("compiler analysis failed: ") + ex.what()},
            {"range", makeRange(0, 0, 1)},
            {"severity", 1},
            {"source", "shorthand-lsp"}});
        return diagnostics;
    }
}

static std::optional<fs::path> findManifest(fs::path sourcePath) {
    std::error_code ec;
    fs::path current = fs::absolute(sourcePath, ec).parent_path();
    if (ec) return std::nullopt;
    for (int depth = 0; depth < 16 && !current.empty(); ++depth) {
        fs::path candidate = current / "shorthand.package";
        if (fs::is_regular_file(candidate, ec) && !ec) return candidate;
        fs::path parent = current.parent_path();
        if (parent == current) break;
        current = parent;
    }
    return std::nullopt;
}

static std::map<std::string, fs::path> loadModuleMap(const fs::path &manifest) {
    std::map<std::string, fs::path> modules;
    std::ifstream in(manifest);
    std::string line;
    const std::regex moduleLine(R"(^\s*module\s+([A-Za-z_][A-Za-z0-9_.]*)\s+([^\s#]+)\s*$)");
    while (std::getline(in, line)) {
        std::smatch match;
        if (std::regex_match(line, match, moduleLine))
            modules[match[1].str()] = manifest.parent_path() / match[2].str();
    }
    return modules;
}

static std::optional<SourceLocation> importDefinition(const std::string &uri,
                                                       const std::string &text,
                                                       const std::string &word) {
    fs::path sourcePath = uriToPath(uri);
    if (sourcePath.empty()) return std::nullopt;
    auto manifest = findManifest(sourcePath);
    if (!manifest) return std::nullopt;
    auto modules = loadModuleMap(*manifest);
    const std::regex importLine(R"(^\s*import\s+([A-Za-z_][A-Za-z0-9_.]*)(?:\s+as\s+([A-Za-z_][A-Za-z0-9_]*))?\s*;)");
    for (const auto &line : splitLines(text)) {
        std::smatch match;
        if (!std::regex_search(line, match, importLine)) continue;
        const std::string module = match[1].str();
        const std::string alias = match[2].matched ? match[2].str() : module.substr(module.find_last_of('.') + 1);
        if (word != alias && module.find(word) == std::string::npos) continue;
        auto found = modules.find(module);
        if (found != modules.end()) {
            const std::string targetText = readFile(found->second);
            const auto targetLines = splitLines(targetText);
            const std::string marker = "module " + module;
            for (std::size_t lineIndex = 0; lineIndex < targetLines.size(); ++lineIndex) {
                std::size_t markerColumn = targetLines[lineIndex].find(marker);
                if (markerColumn == std::string::npos) continue;
                std::size_t nameColumn = markerColumn + std::string("module ").size();
                return SourceLocation{pathToUri(found->second), static_cast<int>(lineIndex),
                                      byteColumnToUtf16(targetLines[lineIndex], nameColumn),
                                      byteColumnToUtf16(targetLines[lineIndex], nameColumn + module.size())};
            }
            return SourceLocation{pathToUri(found->second), 0, 0, 0};
        }
    }
    return std::nullopt;
}

static std::optional<SourceLocation> localDefinition(const std::string &uri,
                                                      const std::string &text,
                                                      const std::string &word) {
    if (word.empty()) return std::nullopt;
    const auto lines = splitLines(text);
    const std::regex declaration(R"((?:^|[^A-Za-z0-9_])(?:def\s+)?(?:int|float|double|string|bool|void|tensor|model)\s+([A-Za-z_][A-Za-z0-9_]*))");
    for (std::size_t i = 0; i < lines.size(); ++i) {
        auto begin = std::sregex_iterator(lines[i].begin(), lines[i].end(), declaration);
        auto end = std::sregex_iterator();
        for (auto it = begin; it != end; ++it) {
            const std::smatch &match = *it;
            if (match[1].str() != word) continue;
            std::size_t column = lines[i].find(word, static_cast<std::size_t>(match.position(0)));
            return SourceLocation{uri, static_cast<int>(i), byteColumnToUtf16(lines[i], column),
                                  byteColumnToUtf16(lines[i], column + word.size())};
        }
    }
    return std::nullopt;
}

static Json::Array documentSymbols(const std::string &text) {
    Json::Array symbols;
    const auto lines = splitLines(text);
    const std::regex functionDecl(R"(\bdef\s+(?:int|float|double|string|bool|void)\s+([A-Za-z_][A-Za-z0-9_]*))");
    const std::regex valueDecl(R"(\b(?:int|float|double|string|bool|void|tensor|model)\s+([A-Za-z_][A-Za-z0-9_]*))");
    const std::regex moduleDecl(R"(^\s*(?:package|module)\s+([A-Za-z_][A-Za-z0-9_.]*))");
    for (std::size_t i = 0; i < lines.size(); ++i) {
        std::smatch match;
        int kind = 13;
        std::string name;
        if (std::regex_search(lines[i], match, functionDecl)) { name = match[1].str(); kind = 12; }
        else if (std::regex_search(lines[i], match, moduleDecl)) { name = match[1].str(); kind = 2; }
        else if (std::regex_search(lines[i], match, valueDecl)) { name = match[1].str(); kind = 13; }
        else continue;
        std::size_t column = lines[i].find(name);
        Json range = makeRange(static_cast<int>(i), byteColumnToUtf16(lines[i], column),
                               byteColumnToUtf16(lines[i], column + name.size()));
        symbols.emplace_back(Json::Object{{"kind", kind}, {"name", name}, {"range", range}, {"selectionRange", range}});
    }
    return symbols;
}

static Json::Array completions(const std::string &text) {
    static const std::array<const char *, 35> keywords = {
        "package", "module", "import", "as", "def", "int", "float", "string", "bool",
        "tensor", "model", "infer", "print", "if", "else", "while", "return", "greenai_contract",
        "greenai_measure", "certification_profile", "certification", "functional_unit", "workload", "boundary",
        "measurement_plan", "ai_lifecycle", "rag_pipeline", "token_budget", "model_routing",
        "guardrails", "true", "false", "cpu", "gpu", "auto"};
    std::set<std::string> labels;
    for (const char *keyword : keywords) labels.insert(keyword);
    const std::regex identifier(R"([A-Za-z_][A-Za-z0-9_]*)");
    for (auto it = std::sregex_iterator(text.begin(), text.end(), identifier); it != std::sregex_iterator(); ++it)
        labels.insert(it->str());
    Json::Array items;
    for (const auto &label : labels) items.emplace_back(Json::Object{{"kind", 14}, {"label", label}});
    return items;
}

class Server {
public:
    int run(std::istream &in, std::ostream &out, std::ostream &err) {
        out_ = &out;
        err_ = &err;
        while (!exitRequested_) {
            std::string body;
            ReadResult read = readMessage(in, body);
            if (read == ReadResult::End) break;
            if (read == ReadResult::Fatal) return 2;

            std::string parseError;
            JsonParser parser(body);
            auto parsed = parser.parse(parseError);
            if (!parsed || !parsed->asObject()) {
                sendError(nullptr, -32700, "Parse error");
                continue;
            }
            handle(*parsed);
        }
        return shutdownRequested_ ? 0 : 1;
    }

private:
    enum class ReadResult { Ok, End, Fatal };
    std::ostream *out_ = nullptr;
    std::ostream *err_ = nullptr;
    std::unordered_map<std::string, Document> documents_;
    std::set<std::string> cancelled_;
    bool initialized_ = false;
    bool shutdownRequested_ = false;
    bool exitRequested_ = false;

    ReadResult readMessage(std::istream &in, std::string &body) {
        std::string line;
        std::optional<std::size_t> contentLength;
        bool sawHeader = false;
        while (std::getline(in, line)) {
            sawHeader = true;
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty()) break;
            const std::string prefix = "Content-Length:";
            if (line.rfind(prefix, 0) == 0) {
                if (contentLength) { *err_ << "error: duplicate Content-Length header\n"; return ReadResult::Fatal; }
                std::string value = line.substr(prefix.size());
                value.erase(0, value.find_first_not_of(" \t"));
                if (value.empty() || value.find_first_not_of("0123456789") != std::string::npos) {
                    *err_ << "error: invalid Content-Length header\n"; return ReadResult::Fatal;
                }
                try {
                    unsigned long long parsed = std::stoull(value);
                    if (parsed == 0 || parsed > kMaxMessageBytes) {
                        *err_ << "error: Content-Length exceeds bounded LSP message size\n"; return ReadResult::Fatal;
                    }
                    contentLength = static_cast<std::size_t>(parsed);
                } catch (...) {
                    *err_ << "error: invalid Content-Length header\n"; return ReadResult::Fatal;
                }
            }
        }
        if (!sawHeader && in.eof()) return ReadResult::End;
        if (!contentLength) { *err_ << "error: missing Content-Length header\n"; return ReadResult::Fatal; }
        body.assign(*contentLength, '\0');
        in.read(body.data(), static_cast<std::streamsize>(*contentLength));
        if (static_cast<std::size_t>(in.gcount()) != *contentLength) {
            *err_ << "error: truncated LSP message body\n"; return ReadResult::Fatal;
        }
        return ReadResult::Ok;
    }

    void send(const Json &message) {
        const std::string body = message.dump();
        *out_ << "Content-Length: " << body.size() << "\r\n\r\n" << body;
        out_->flush();
    }

    void sendResult(const Json *id, Json result) {
        send(Json::Object{{"id", id ? *id : Json(nullptr)}, {"jsonrpc", "2.0"}, {"result", std::move(result)}});
    }

    void sendError(const Json *id, int code, const std::string &message) {
        send(Json::Object{{"error", Json::Object{{"code", code}, {"message", message}}},
                          {"id", id ? *id : Json(nullptr)}, {"jsonrpc", "2.0"}});
    }

    void publishDiagnostics(const std::string &uri, const Document &document) {
        send(Json::Object{{"jsonrpc", "2.0"}, {"method", "textDocument/publishDiagnostics"},
                          {"params", Json::Object{{"diagnostics", compilerDiagnostics(document.text)},
                                                   {"uri", uri}, {"version", document.version}}}});
    }

    void handle(const Json &request) {
        const Json *id = request.get("id");
        const auto method = request.getString("method");
        if (!method) { if (id) sendError(id, -32600, "Invalid Request"); return; }

        if (*method == "$/cancelRequest") {
            const Json *params = request.get("params");
            const Json *cancelId = params ? params->get("id") : nullptr;
            const std::string key = idKey(cancelId);
            if (!key.empty()) cancelled_.insert(key);
            return;
        }

        const std::string key = idKey(id);
        if (!key.empty() && cancelled_.erase(key) != 0) {
            sendError(id, kRequestCancelled, "Request cancelled");
            return;
        }

        if (*method == "initialize") {
            initialized_ = true;
            Json::Object capabilities{
                {"completionProvider", Json::Object{{"resolveProvider", false}, {"triggerCharacters", Json::Array{Json(".")}}}},
                {"definitionProvider", true},
                {"documentSymbolProvider", true},
                {"hoverProvider", true},
                {"positionEncoding", "utf-16"},
                {"textDocumentSync", 1}};
            sendResult(id, Json::Object{{"capabilities", std::move(capabilities)},
                                        {"serverInfo", Json::Object{{"name", "shorthand-lsp"}, {"version", "0.1.0"}}}});
            return;
        }
        if (*method == "initialized") return;
        if (*method == "shutdown") { shutdownRequested_ = true; sendResult(id, Json(nullptr)); return; }
        if (*method == "exit") { exitRequested_ = true; return; }
        if (!initialized_) { if (id) sendError(id, -32002, "Server not initialized"); return; }
        if (shutdownRequested_) { if (id) sendError(id, -32600, "Server is shutting down"); return; }

        if (*method == "textDocument/didOpen") {
            const Json *params = request.get("params");
            const Json *doc = params ? params->get("textDocument") : nullptr;
            auto uri = doc ? doc->getString("uri") : std::nullopt;
            auto text = doc ? doc->getString("text") : std::nullopt;
            auto version = doc ? doc->getInteger("version") : std::nullopt;
            if (!uri || !text) return;
            Document document{*text, version.value_or(0)};
            documents_[*uri] = document;
            publishDiagnostics(*uri, document);
            return;
        }
        if (*method == "textDocument/didChange") {
            auto uri = requestUri(request);
            const Json *params = request.get("params");
            const Json *changes = params ? params->get("contentChanges") : nullptr;
            if (!uri || !changes || !changes->asArray() || changes->asArray()->empty()) return;
            const Json &last = changes->asArray()->back();
            auto text = last.getString("text");
            if (!text) return;
            auto found = documents_.find(*uri);
            if (found == documents_.end()) return;
            found->second.text = *text;
            const Json *doc = params->get("textDocument");
            if (doc) found->second.version = doc->getInteger("version").value_or(found->second.version + 1);
            publishDiagnostics(*uri, found->second);
            return;
        }
        if (*method == "textDocument/didClose") {
            auto uri = requestUri(request);
            if (!uri) return;
            documents_.erase(*uri);
            send(Json::Object{{"jsonrpc", "2.0"}, {"method", "textDocument/publishDiagnostics"},
                              {"params", Json::Object{{"diagnostics", Json::Array{}}, {"uri", *uri}}}});
            return;
        }

        auto uri = requestUri(request);
        if (!uri) { if (id) sendError(id, -32602, "Missing textDocument URI"); return; }
        auto found = documents_.find(*uri);
        if (found == documents_.end()) { if (id) sendError(id, -32602, "Document is not open"); return; }

        if (*method == "textDocument/completion") {
            sendResult(id, completions(found->second.text));
            return;
        }
        if (*method == "textDocument/documentSymbol") {
            sendResult(id, documentSymbols(found->second.text));
            return;
        }
        if (*method == "textDocument/hover") {
            auto position = requestPosition(request);
            if (!position) { sendError(id, -32602, "Missing position"); return; }
            std::string word = wordAt(found->second.text, *position);
            if (word.empty()) { sendResult(id, Json(nullptr)); return; }
            static const std::set<std::string> keywords = {"package","module","import","as","def","int","float","string","bool","tensor","model","infer","print","if","else","while","return","greenai_contract","greenai_measure","certification_profile","certification","functional_unit","workload","boundary","measurement_plan","ai_lifecycle","rag_pipeline","token_budget","model_routing","guardrails"};
            std::string value = keywords.count(word) ? "`" + word + "` - ShortHand language keyword"
                                                     : "`" + word + "` - ShortHand source symbol";
            sendResult(id, Json::Object{{"contents", Json::Object{{"kind", "markdown"}, {"value", value}}}});
            return;
        }
        if (*method == "textDocument/definition") {
            auto position = requestPosition(request);
            if (!position) { sendError(id, -32602, "Missing position"); return; }
            std::string word = wordAt(found->second.text, *position);
            auto imported = importDefinition(*uri, found->second.text, word);
            if (imported) { sendResult(id, makeLocation(*imported)); return; }
            auto local = localDefinition(*uri, found->second.text, word);
            if (local) { sendResult(id, makeLocation(*local)); return; }
            sendResult(id, Json(nullptr));
            return;
        }

        if (id) sendError(id, -32601, "Method not found");
    }
};

} // namespace shorthand::lsp

int main() {
    shorthand::lsp::Server server;
    return server.run(std::cin, std::cout, std::cerr);
}
