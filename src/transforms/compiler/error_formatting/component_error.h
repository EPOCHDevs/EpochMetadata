#pragma once

#include "error_formatter.h"
#include <vector>

namespace epoch_script {
namespace error_formatting {

/**
 * Formats errors for unknown components.
 */
class UnknownComponentError : public ErrorFormatter {
public:
    explicit UnknownComponentError(const std::string& component_name)
        : component_name_(component_name) {}

    std::string Format(int line = -1, int col = -1) const override;

private:
    std::string component_name_;
};

/**
 * Formats errors for tuple unpacking mismatches.
 */
class TupleUnpackError : public ErrorFormatter {
public:
    TupleUnpackError(
        const std::string& component_name,
        size_t output_count,
        size_t variable_count,
        const std::vector<std::string>& output_names
    ) : component_name_(component_name),
        output_count_(output_count),
        variable_count_(variable_count),
        output_names_(output_names) {}

    std::string Format(int line = -1, int col = -1) const override;

private:
    std::string component_name_;
    size_t output_count_;
    size_t variable_count_;
    std::vector<std::string> output_names_;
};

/**
 * Formats errors for unknown output handles.
 */
class UnknownHandleError : public ErrorFormatter {
public:
    UnknownHandleError(
        const std::string& handle_name,
        const std::string& node_name,
        const std::string& component_name,
        const std::vector<std::string>& valid_handles
    ) : handle_name_(handle_name),
        node_name_(node_name),
        component_name_(component_name),
        valid_handles_(valid_handles) {}

    std::string Format(int line = -1, int col = -1) const override;

private:
    std::string handle_name_;
    std::string node_name_;
    std::string component_name_;
    std::vector<std::string> valid_handles_;
};

} // namespace error_formatting
} // namespace epoch_script