#include "error_formatter.h"
#include <sstream>

namespace epoch_script {
namespace error_formatting {

std::string ErrorFormatter::AddLocationInfo(const std::string& msg, int line, int col) {
    std::string result = msg;
    // Tree-sitter provides 1-based line numbers (line 1 is the first line)
    // Only add location info if line > 0
    if (line > 0) {
        result += " (line " + std::to_string(line) + ", col " + std::to_string(col) + ")";
    }
    return result;
}

std::string ErrorFormatter::FormatList(const std::vector<std::string>& items,
                                       const std::string& separator) {
    std::ostringstream oss;
    for (size_t i = 0; i < items.size(); ++i) {
        if (i > 0) oss << separator;
        oss << items[i];
    }
    return oss.str();
}

std::string ErrorFormatter::FormatListInBrackets(const std::vector<std::string>& items) {
    return "[" + FormatList(items) + "]";
}

std::string ErrorFormatter::Indent(const std::string& text, int spaces) {
    return std::string(spaces, ' ') + text;
}

} // namespace error_formatting
} // namespace epoch_script