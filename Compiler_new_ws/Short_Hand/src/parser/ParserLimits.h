#ifndef SHORTHAND_PARSER_LIMITS_H
#define SHORTHAND_PARSER_LIMITS_H

#include <cstddef>
#include <cstdint>
#include <string>

namespace shorthand {
namespace parser {

struct ParserLimits {
    std::size_t max_source_bytes;
    std::size_t max_scanner_matches;
    std::size_t max_token_bytes;
    std::size_t max_nesting_depth;
};

void resetParserGuard();
ParserLimits currentParserLimits();

bool validateSourceBytes(std::uint64_t source_bytes);
bool noteScannerMatch(std::size_t lexeme_bytes);
bool noteTokenBytes(std::size_t token_bytes);
bool enterDelimiter();
void leaveDelimiter();

void recordParserGuardFailure(const char *code, const std::string &message);
bool hasParserGuardFailure();
const char *parserGuardFailureCode();
std::string parserGuardFailureMessage();

void emitParserGuardFailure(const char *source_path,
                            int first_line,
                            int first_column,
                            int last_line,
                            int last_column);

}  // namespace parser
}  // namespace shorthand

#endif
