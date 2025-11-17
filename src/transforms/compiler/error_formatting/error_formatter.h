#pragma once

#include <string>
#include <vector>
#include <sstream>

namespace epoch_script {
namespace error_formatting {

/**
 * Base class for formatting compiler error messages.
 * Provides consistent, structured error output with context.
 *
 * Error formatters can be streamed directly to std::ostream:
 *   ErrorFormatter error(...);
 *   std::cout << error;  // or ThrowError(error);
 */
class ErrorFormatter {
public:
    virtual ~ErrorFormatter() = default;

    /**
     * Format an error message with optional line/column information.
     * @param line Line number (0 or negative means no line info)
     * @param col Column number
     * @return Formatted error message
     */
    virtual std::string Format(int line = -1, int col = -1) const = 0;

    /**
     * Stream operator for easy integration with std::ostream
     */
    friend std::ostream& operator<<(std::ostream& os, const ErrorFormatter& error) {
        os << error.Format();
        return os;
    }

    /**
     * Implicit conversion to string for easy integration with ThrowError
     */
    operator std::string() const {
        return Format();
    }

protected:
    /**
     * Add location information to error message if available.
     */
    static std::string AddLocationInfo(const std::string& msg, int line, int col);

    /**
     * Format a list of items as a comma-separated string.
     * Example: ["a", "b", "c"] -> "a, b, c"
     */
    static std::string FormatList(const std::vector<std::string>& items,
                                   const std::string& separator = ", ");

    /**
     * Format a list in brackets.
     * Example: ["a", "b", "c"] -> "[a, b, c]"
     */
    static std::string FormatListInBrackets(const std::vector<std::string>& items);

    /**
     * Create an indented line.
     */
    static std::string Indent(const std::string& text, int spaces = 2);
};

} // namespace error_formatting
} // namespace epoch_script