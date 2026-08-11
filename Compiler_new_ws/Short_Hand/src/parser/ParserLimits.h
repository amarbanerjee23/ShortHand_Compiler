#ifndef SHORTHAND_PARSER_LIMITS_H
#define SHORTHAND_PARSER_LIMITS_H

#include "../visitors/DiagnosticCodes.h"

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>

namespace shorthand {
namespace parser {

struct ParserLimits {
    std::size_t max_source_bytes;
    std::size_t max_scanner_matches;
    std::size_t max_token_bytes;
    std::size_t max_nesting_depth;
};

namespace detail {

inline constexpr std::size_t MaxSourceBytes = 4U * 1024U * 1024U;
inline constexpr std::size_t MaxScannerMatches = 250000U;
inline constexpr std::size_t MaxTokenBytes = 16U * 1024U;
inline constexpr std::size_t MaxNestingDepth = 256U;

struct ParserGuardState {
    ParserLimits limits{MaxSourceBytes,
                        MaxScannerMatches,
                        MaxTokenBytes,
                        MaxNestingDepth};
    std::size_t scanner_matches = 0;
    std::size_t nesting_depth = 0;
    std::string failure_code;
    std::string failure_message;
    bool emitted = false;
};

inline ParserGuardState guard_state;

inline std::size_t lowerOnlyEnvironmentLimit(const char *name,
                                             std::size_t production_ceiling) {
    const char *raw = std::getenv(name);
    if (raw == nullptr || *raw == '\0') return production_ceiling;

    errno = 0;
    char *end = nullptr;
    const unsigned long long parsed = std::strtoull(raw, &end, 10);
    if (errno != 0 || end == raw || *end != '\0' || parsed == 0) {
        return production_ceiling;
    }

    return static_cast<std::size_t>(std::min(
        parsed, static_cast<unsigned long long>(production_ceiling)));
}

inline std::string limitMessage(const char *subject,
                                std::uint64_t observed,
                                std::size_t maximum) {
    std::ostringstream out;
    out << subject << " " << observed << " exceeds maximum " << maximum;
    return out.str();
}

}  // namespace detail

inline void resetParserGuard() {
    detail::guard_state = detail::ParserGuardState{};
    detail::guard_state.limits.max_source_bytes =
        detail::lowerOnlyEnvironmentLimit("SHORTHAND_MAX_SOURCE_BYTES",
                                          detail::MaxSourceBytes);
    detail::guard_state.limits.max_scanner_matches =
        detail::lowerOnlyEnvironmentLimit("SHORTHAND_MAX_SCANNER_MATCHES",
                                          detail::MaxScannerMatches);
    detail::guard_state.limits.max_token_bytes =
        detail::lowerOnlyEnvironmentLimit("SHORTHAND_MAX_TOKEN_BYTES",
                                          detail::MaxTokenBytes);
    detail::guard_state.limits.max_nesting_depth =
        detail::lowerOnlyEnvironmentLimit("SHORTHAND_MAX_NESTING_DEPTH",
                                          detail::MaxNestingDepth);
}

inline ParserLimits currentParserLimits() {
    return detail::guard_state.limits;
}

inline void recordParserGuardFailure(const char *code,
                                     const std::string &message) {
    if (!detail::guard_state.failure_code.empty()) return;
    detail::guard_state.failure_code =
        code == nullptr ? diagnostics::ParserSyntaxError : code;
    detail::guard_state.failure_message = message;
}

inline bool hasParserGuardFailure() {
    return !detail::guard_state.failure_code.empty();
}

inline bool validateSourceBytes(std::uint64_t source_bytes) {
    // Source-size validation is a pre-scanner check. Re-read the lower-only
    // environment ceiling here so a caller cannot accidentally perform the
    // file-size check before resetParserGuard() has initialized this process's
    // configured limit. The value can never exceed the production ceiling.
    detail::guard_state.limits.max_source_bytes =
        detail::lowerOnlyEnvironmentLimit("SHORTHAND_MAX_SOURCE_BYTES",
                                          detail::MaxSourceBytes);
    if (source_bytes <= detail::guard_state.limits.max_source_bytes) return true;
    recordParserGuardFailure(
        diagnostics::ParserSourceLimitExceeded,
        detail::limitMessage("source bytes",
                             source_bytes,
                             detail::guard_state.limits.max_source_bytes));
    return false;
}

inline bool noteScannerMatch(std::size_t) {
    if (hasParserGuardFailure()) return false;
    ++detail::guard_state.scanner_matches;
    if (detail::guard_state.scanner_matches <=
        detail::guard_state.limits.max_scanner_matches) {
        return true;
    }
    recordParserGuardFailure(
        diagnostics::ParserScannerBudgetExceeded,
        detail::limitMessage("scanner matches",
                             detail::guard_state.scanner_matches,
                             detail::guard_state.limits.max_scanner_matches));
    return false;
}

inline bool noteTokenBytes(std::size_t token_bytes) {
    if (hasParserGuardFailure()) return false;
    if (token_bytes <= detail::guard_state.limits.max_token_bytes) return true;
    recordParserGuardFailure(
        diagnostics::ParserTokenLimitExceeded,
        detail::limitMessage("token bytes",
                             token_bytes,
                             detail::guard_state.limits.max_token_bytes));
    return false;
}

inline bool enterDelimiter() {
    if (hasParserGuardFailure()) return false;
    if (detail::guard_state.nesting_depth >=
        detail::guard_state.limits.max_nesting_depth) {
        recordParserGuardFailure(
            diagnostics::ParserNestingLimitExceeded,
            detail::limitMessage("delimiter nesting depth",
                                 detail::guard_state.nesting_depth + 1U,
                                 detail::guard_state.limits.max_nesting_depth));
        return false;
    }
    ++detail::guard_state.nesting_depth;
    return true;
}

inline void leaveDelimiter() {
    if (detail::guard_state.nesting_depth > 0) {
        --detail::guard_state.nesting_depth;
    }
}

inline const char *parserGuardFailureCode() {
    return detail::guard_state.failure_code.empty()
               ? diagnostics::ParserSyntaxError
               : detail::guard_state.failure_code.c_str();
}

inline std::string parserGuardFailureMessage() {
    return detail::guard_state.failure_message.empty()
               ? "parser guard failure"
               : detail::guard_state.failure_message;
}

inline void emitParserGuardFailure(const char *source_path,
                                   int first_line,
                                   int first_column,
                                   int last_line,
                                   int last_column) {
    if (detail::guard_state.emitted || !hasParserGuardFailure()) return;
    detail::guard_state.emitted = true;
    const char *path = source_path == nullptr ? "<input>" : source_path;
    std::fprintf(stderr, "----------------ERROR----------------\n");
    std::fprintf(stderr,
                 "%s:%d:%d: error: [%s] %s [range %d:%d-%d:%d]\n",
                 path,
                 first_line,
                 first_column,
                 parserGuardFailureCode(),
                 parserGuardFailureMessage().c_str(),
                 first_line,
                 first_column,
                 last_line,
                 last_column);
}

}  // namespace parser
}  // namespace shorthand

#endif
