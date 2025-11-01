#ifndef SELECTOR_OUTPUT_HPP
#define SELECTOR_OUTPUT_HPP

#include "common/transform_tester_base.hpp"
#include <epochflow/transforms/core/itransform.h>
#include <sstream>
#include <iostream>
#include <unordered_set>

namespace epoch {
namespace test {

class SelectorOutput : public IOutputType {
public:
    // Store the actual selector data
    epochflow::transform::SelectorData selectorData;

    SelectorOutput() = default;

    std::string getType() const override { return "selector_data"; }

    bool equals(const IOutputType& other) const override {
        auto* otherSelector = dynamic_cast<const SelectorOutput*>(&other);
        if (!otherSelector) return false;

        return compareSelectorData(selectorData, otherSelector->selectorData);
    }

    std::string toString() const override {
        std::stringstream ss;
        ss << "Selector Output:\n";
        ss << "  Title: " << selectorData.title << "\n";
        ss << "  Icon: " << static_cast<int>(selectorData.icon) << "\n";
        ss << "  Schema Count: " << selectorData.schemas.size() << "\n";
        if (selectorData.pivot_index.has_value()) {
            ss << "  Pivot Index: " << selectorData.pivot_index.value() << "\n";
        } else {
            ss << "  Pivot Index: (none)\n";
        }
        ss << "  DataFrame rows: " << selectorData.data.num_rows() << "\n";
        ss << "  DataFrame cols: " << selectorData.data.num_cols() << "\n";
        return ss.str();
    }

private:
    bool compareSelectorData(const epochflow::transform::SelectorData& a,
                            const epochflow::transform::SelectorData& b) const {
        // Compare title
        if (a.title != b.title) {
            std::cerr << "DEBUG compareSelectorData: Title mismatch - a='" << a.title
                      << "', b='" << b.title << "'" << std::endl;
            return false;
        }

        // Compare icon
        if (a.icon != b.icon) {
            std::cerr << "DEBUG compareSelectorData: Icon mismatch - a=" << static_cast<int>(a.icon)
                      << ", b=" << static_cast<int>(b.icon) << std::endl;
            return false;
        }

        // Compare schema count
        if (a.schemas.size() != b.schemas.size()) {
            std::cerr << "DEBUG compareSelectorData: Schema count mismatch - a=" << a.schemas.size()
                      << ", b=" << b.schemas.size() << std::endl;
            return false;
        }

        // Compare pivot_index
        if (a.pivot_index != b.pivot_index) {
            std::cerr << "DEBUG compareSelectorData: Pivot index mismatch - a=";
            if (a.pivot_index.has_value()) std::cerr << a.pivot_index.value();
            else std::cerr << "(none)";
            std::cerr << ", b=";
            if (b.pivot_index.has_value()) std::cerr << b.pivot_index.value();
            else std::cerr << "(none)";
            std::cerr << std::endl;
            return false;
        }

        // Compare DataFrame if expected has data
        if (b.data.num_rows() > 0 || b.data.num_cols() > 0) {
            // Expected has DataFrame data - validate it
            if (a.data.num_rows() != b.data.num_rows()) {
                std::cerr << "DEBUG compareSelectorData: DataFrame row count mismatch - a=" << a.data.num_rows()
                          << ", b=" << b.data.num_rows() << std::endl;
                return false;
            }

            if (a.data.num_cols() != b.data.num_cols()) {
                std::cerr << "DEBUG compareSelectorData: DataFrame col count mismatch - a=" << a.data.num_cols()
                          << ", b=" << b.data.num_cols() << std::endl;
                return false;
            }

            // Compare column names (order-independent)
            auto a_cols = a.data.column_names();
            auto b_cols = b.data.column_names();

            // Convert to sets for order-independent comparison
            std::unordered_set<std::string> a_cols_set(a_cols.begin(), a_cols.end());
            std::unordered_set<std::string> b_cols_set(b_cols.begin(), b_cols.end());

            if (a_cols_set != b_cols_set) {
                std::cerr << "DEBUG compareSelectorData: DataFrame column names mismatch" << std::endl;
                std::cerr << "  Actual columns: ";
                for (const auto& col : a_cols) std::cerr << col << " ";
                std::cerr << std::endl << "  Expected columns: ";
                for (const auto& col : b_cols) std::cerr << col << " ";
                std::cerr << std::endl;
                return false;
            }

            // Compare DataFrame data using equals() method (compare data only, not index)
            // For selector data, the index may be different (actual has datetime index, expected has default uint64)
            // So we compare column data only
            for (const auto& colName : b_cols) {
                try {
                    auto a_col = a.data[colName];
                    auto b_col = b.data[colName];

                    if (!a_col.equals(b_col)) {
                        std::cerr << "DEBUG compareSelectorData: Column '" << colName << "' data does not match" << std::endl;
                        std::cerr << "Actual column:\n" << a_col << std::endl;
                        std::cerr << "Expected column:\n" << b_col << std::endl;
                        return false;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "DEBUG compareSelectorData: Error comparing column '" << colName << "': " << e.what() << std::endl;
                    return false;
                }
            }
        }

        return true;
    }
};

} // namespace test
} // namespace epoch

#endif // SELECTOR_OUTPUT_HPP
