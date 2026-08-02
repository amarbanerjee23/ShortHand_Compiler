#include "SourceRange.h"

#include <mutex>
#include <sstream>
#include <unordered_map>

namespace {
std::mutex range_mutex;
std::unordered_map<const void *, SourceRange> ranges;
}

std::string SourceRange::toString() const {
    if (!valid()) return "<unknown>";
    std::ostringstream out;
    out << begin.line << ':' << begin.column << '-' << end.line << ':' << end.column;
    return out.str();
}

void shorthand_set_ast_source_range(const void *node, const SourceRange &range) {
    if (node == nullptr || !range.valid()) return;
    std::lock_guard<std::mutex> lock(range_mutex);
    ranges[node] = range;
}

SourceRange shorthand_get_ast_source_range(const void *node) {
    if (node == nullptr) return {};
    std::lock_guard<std::mutex> lock(range_mutex);
    auto it = ranges.find(node);
    return it == ranges.end() ? SourceRange{} : it->second;
}

bool shorthand_has_ast_source_range(const void *node) {
    return shorthand_get_ast_source_range(node).valid();
}

void shorthand_clear_ast_source_ranges() {
    std::lock_guard<std::mutex> lock(range_mutex);
    ranges.clear();
}
