#pragma once

#include <cstddef>

#include <tools/tree.hpp>

namespace frontend {

struct SourceLocation {
    unsigned line;
    unsigned column;
};

struct SourceMap {
    std::string_view source;
    std::vector<unsigned> line_starts;

    SourceMap(std::string_view src) : source(src) {
        line_starts.push_back(0);
        for (size_t i = 0; i < src.size(); ++i)
            if (src[i] == '\n') line_starts.push_back(i + 1);
    }

    SourceLocation locate(std::string_view s) const {
        const char *ptr = s.data();
        unsigned idx = ptr - source.data();
        auto it = std::upper_bound(line_starts.begin(), line_starts.end(), idx);
        unsigned line = it - line_starts.begin();
        unsigned col = idx - line_starts[line - 1] + 1;
        return {line, col};
    }
};

struct ErrorMessage {
    std::string msg;
    SourceLocation loc;
};

}; // namespace frontend

