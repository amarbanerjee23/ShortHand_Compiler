#ifndef SHORTHAND_PARSER_PARSE_SESSION_H
#define SHORTHAND_PARSER_PARSE_SESSION_H

#include <cstddef>

class AST_PROGRAM;
class AST_MODULE_PREAMBLE;

namespace shorthand::parser {

// Own all parser allocations made in this lexical scope, including partial
// parses. Keep the session alive until every AST consumer has finished. One
// compiler invocation uses a single session for its complete module graph.
// Nested sessions may release temporary parses without invalidating an outer
// graph. The generated parser is non-reentrant; this is not a concurrency API.
class ParseSession final {
public:
    ParseSession();
    ~ParseSession();
    ParseSession(const ParseSession &) = delete;
    ParseSession &operator=(const ParseSession &) = delete;
    ParseSession(ParseSession &&) = delete;
    ParseSession &operator=(ParseSession &&) = delete;

private:
    std::size_t checkpoint_;
    std::size_t scanner_checkpoint_;
    AST_PROGRAM *previous_program_;
    AST_MODULE_PREAMBLE *previous_preamble_;
};

// Live objects, not vector capacity: supports bounded-lifetime diagnostics.
std::size_t liveParserAllocationCount();
std::size_t liveScannerStringCount();

} // namespace shorthand::parser

#endif
