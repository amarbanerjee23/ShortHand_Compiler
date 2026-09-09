#pragma once
#include <map>
#include <set>
#include <string>
#include <vector>

namespace shorthand::c3eco {
class Json {
  public:
    enum class Kind { Null, Boolean, Number, String, Array, Object };

    Kind kind = Kind::Null;
    bool boolean = false;
    double number = 0.0;
    std::string string;
    std::vector<Json> array;
    std::map<std::string, Json> object;
};

// Shared bounded parser and the unchanged PR90 assessment producer.
void require(bool condition, const std::string &message);
std::string readFile(const std::string &path);
std::string jsonEscape(const std::string &value);
Json parseJson(std::string source);
const Json &jsonMember(const Json &object, const std::string &key);
const Json &jsonMember(const Json &object, const char *key);
std::string jsonString(const Json &object, const std::string &key);
bool jsonBoolean(const Json &object, const std::string &key);
double jsonNumber(const Json &object, const std::string &key);
void requireExactKeys(const Json &object, const std::set<std::string> &keys,
                      const std::string &description);
bool validIsoDate(const std::string &value);
int daysInMonth(int year, int month);
std::vector<std::string> splitTsv(const std::string &line);
void assessDirectory(const std::string &directory, const std::string &output,
                     const std::string &markdownOutput = "");
} // namespace shorthand::c3eco
