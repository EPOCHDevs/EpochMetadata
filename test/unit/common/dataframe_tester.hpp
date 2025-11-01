#ifndef DATAFRAME_TESTER_HPP
#define DATAFRAME_TESTER_HPP

#include "common/transform_tester_base.hpp"

namespace epoch {
namespace test {

// DataFrame output implementation for this repository
class DataFrameOutput : public IOutputType {
public:
    Table data;

    DataFrameOutput() = default;
    explicit DataFrameOutput(const Table& table) : data(table) {}

    std::string getType() const override { return "dataframe"; }
    bool equals(const IOutputType& other) const override;
    std::string toString() const override;

    // Factory method for creating from YAML
    static std::unique_ptr<IOutputType> fromYAML(const YAML::Node& node);
};

// Convenience typedef for DataFrame testing
using DataFrameTransformTester = TransformTesterBase<Table>;

// Helper function to register DataFrame type
inline void registerDataFrameType() {
    OutputTypeRegistry::instance().registerType("dataframe",
        [](const YAML::Node& node) { return DataFrameOutput::fromYAML(node); });
}

} // namespace test
} // namespace epoch

#endif // DATAFRAME_TESTER_HPP