#include "component_error.h"
#include <sstream>

namespace epoch_script {
namespace error_formatting {

std::string UnknownComponentError::Format(int line, int col) const {
    std::ostringstream oss;
    oss << "Unknown component '" << component_name_ << "()'\n";
    oss << Indent("This component is not registered or does not exist.") << "\n";
    oss << Indent("Check the component name for typos or verify it's included in the system.");
    return AddLocationInfo(oss.str(), line, col);
}

std::string TupleUnpackError::Format(int line, int col) const {
    std::ostringstream oss;

    oss << "Tuple unpacking error for component '" << component_name_ << "()'\n";

    // Output information
    oss << Indent("Component returns: " + std::to_string(output_count_) +
                  " output" + (output_count_ == 1 ? "" : "s"));
    if (!output_names_.empty()) {
        oss << " " << FormatListInBrackets(output_names_);
    }
    oss << "\n";

    // Variable count
    oss << Indent("Trying to unpack into: " + std::to_string(variable_count_) +
                  " variable" + (variable_count_ == 1 ? "" : "s"));

    return AddLocationInfo(oss.str(), line, col);
}

std::string UnknownHandleError::Format(int line, int col) const {
    std::ostringstream oss;

    oss << "Unknown handle '" << handle_name_ << "' on node '" << node_name_ << "'\n";
    oss << Indent("Component: " + component_name_ + "()") << "\n";

    if (!valid_handles_.empty()) {
        oss << Indent("Valid handles: " + FormatList(valid_handles_));
    } else {
        oss << Indent("This component has no accessible handles");
    }

    return AddLocationInfo(oss.str(), line, col);
}

} // namespace error_formatting
} // namespace epoch_script