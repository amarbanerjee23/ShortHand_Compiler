#ifndef SHORTHAND_AST_SOURCE_RANGE_H
#define SHORTHAND_AST_SOURCE_RANGE_H

#include <cstddef>
#include <string>

struct SourcePosition {
    int line = 0;
    int column = 0;
    std::size_t offset = 0;

    bool valid() const { return line > 0 && column > 0; }
};

struct SourceRange {
    SourcePosition begin;
    SourcePosition end;

    bool valid() const {
        return begin.valid() && end.valid() &&
               (end.line > begin.line ||
                (end.line == begin.line && end.column >= begin.column));
    }

    std::string toString() const;
};

void shorthand_set_ast_source_range(const void *node, const SourceRange &range);
SourceRange shorthand_get_ast_source_range(const void *node);
bool shorthand_has_ast_source_range(const void *node);
void shorthand_clear_ast_source_ranges();

#endif
