#include "ParserLimits.h"

#include "../visitors/DiagnosticCodes.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <sstream>

namespace shorthand {
namespace parser {
namespace {

constexpr std::size_t kMaxSourceBytes = 4U * 1024U * 1024U;
constexpr std::size_t kMaxScannerMatches = 250000U;
constexpr std::size_t kMaxTokenBytes = 16U * 1024U;
constexpr std::size_t kMaxNestingDepth = 256U;

struct ParserGuardState {
    ParserLimits limits{kMaxSourceBytes,
                        kMaxScannerMatches,
                        kMaxTokenBytes,
                        kMaxNestingDepth};
    std::size_t scanner_matches = 0;
    std::size_t nesting_depth = 0;
    std::string failure_code;
    std::string failure_message;
    bool emitted = false;
};

ParserGuardState state;

std::size_t lowerOnlyEnvironmentLimit(const char *name,
                                      std::size_t production_ceiling) {
    const char *raw = std::getenv(name);
    if (raw == nullptr || *raw == '\0') return production_ceiling;

    errno = 0;
    char *end = nullptr;
    const unsigned long long parsed = std::strtoull(raw, &end, 10);
    if (errno != 0 || end == raw || *end != '\0' || parsed == 0) {
        return production_ceiling;
    }

    const unsigned long long ceiling =
        static_cast<unsigned long long>(production_ceiling);
    return static_cast<std::size_t>(std::min(parsed, ceiling));
}

std::string limitMessage(const char *subject,
                         std::uint64_t observed,
                         std::size_t maximum) {
    std::ostringstream out;
    out << subject << " " << observed << " exceeds maximum " << maximum;
    return out.str();
}

}  // namespace

void resetParserGuard() {
    state = ParserGuardState{};
    state.limits.max_source_bytes =
        lowerOnlyEnvironmentLimit("SHORTHAND_MAX_SOURCE_BYTES", kMaxSourceBytes);
    state.limits.max_scanner_matches = lowerOnlyEnvironmentLimit(
        "SHORTHAND_MAX_SCANNER_MATCHES", kMaxScannerMatches);
    state.limits.max_token_bytes =
        lowerOnlyEnvironmentLimit("SHORTHAND_MAX_TOKEN_BYTES", kMaxTokenBytes);
    state.limits.max_nesting_depth = lowerOnlyEnvironmentLimit(
        "SHORTHAND_MAX_NESTING_DEPTH", kMaxNestingDepth);
}

ParserLimits currentParserLimits() { return state.limits; }

void recordParserGuardFailure(const char *code, const std::string &message) {
    if (!state.failure_code.empty()) return;
    state.failure_code = code == nullptr ? diagnostics::ParserSyntaxError : code;
    state.failure_message = message;
}

bool validateSourceBytes(std::uint64_t source_bytes) {
    if (source_bytes <= state.limits.max_source_bytes) return true;
    recordParserGuardFailure(
        diagnostics::ParserSourceLimitExceeded,
        limitMessage("source bytes", source_bytes, state.limits.max_source_bytes));
    return false;
}

bool noteScannerMatch(std::size_t) {
    if (hasParserGuardFailure()) return false;
    ++state.scanner_matches;
    if (state.scanner_matches <= state.limits.max_scanner_matches) return true;
    recordParserGuardFailure(
        diagnostics::ParserScannerBudgetExceeded,
        limitMessage("scanner matches",
                     state.scanner_matches,
                     state.limits.max_scanner_matches));
    return false;
}

bool noteTokenBytes(std::size_t token_bytes) {
    if (hasParserGuardFailure()) return false;
    if (token_bytes <= state.limits.max_token_bytes) return true;
    recordParserGuardFailure(
        diagnostics::ParserTokenLimitExceeded,
        limitMessage("token bytes", token_bytes, state.limits.max_token_bytes));
    return false;
}

bool enterDelimiter() {
    if (hasParserGuardFailure()) return false;
    if (state.nesting_depth >= state.limits.max_nesting_depth) {
        recordParserGuardFailure(
            diagnostics::ParserNestingLimitExceeded,
            limitMessage("delimiter nesting depth",
                         state.nesting_depth + 1U,
                         state.limits.max_nesting_depth));
        return false;
    }
    ++state.nesting_depth;
    return true;
}

void leaveDelimiter() {
    if (state.nesting_depth > 0) --state.nesting_depth;
}

bool hasParserGuardFailure() { return !state.failure_code.empty(); }

const char *parserGuardFailureCode() {
    return state.failure_code.empty() ? diagnostics::ParserSyntaxError
                                      : state.failure_code.c_str();
}

std::string parserGuardFailureMessage() {
    return state.failure_message.empty() ? "parser guard failure"
                                         : state.failure_message;
}

void emitParserGuardFailure(const char *source_path,
                            int first_line,
                            int first_column,
                            int last_line,
                            int last_column) {
    if (state.emitted || !hasParserGuardFailure()) return;
    state.emitted = true;
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
