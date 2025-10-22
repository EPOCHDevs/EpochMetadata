#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <epoch_frame/frame_or_series.h>
#include <epoch_metadata/transforms/itransform.h>
#include <epoch_metadata/transforms/registry.h>
#include <epoch_metadata/transforms/transform_configuration.h>
#include <epoch_metadata/transforms/transform_registry.h>
#include <glaze/glaze.hpp>
#include <yaml-cpp/yaml.h>

namespace epoch_metadata::selectors {

// ISelector extends ITransform to add interactive selector widget capability
// Returns empty DataFrame to prevent merging into the main computation graph
class ISelector : public epoch_metadata::transform::ITransform {
public:
  explicit ISelector(epoch_metadata::transform::TransformConfiguration config, bool skipRename = false)
      : ITransform(std::move(config)) {
    if (!skipRename) {
      // Build column mapping from inputs
      BuildColumnMappings();
    }
  }

  // TransformData normalizes column names and generates selector output
  // Returns empty DataFrame to prevent merging into computation graph
  epoch_frame::DataFrame TransformData(const epoch_frame::DataFrame &df) const override {
    // 1. Get expected columns from configuration inputs
    std::vector<std::string> inputColumns;
    for (const auto& [inputId, columns] : m_config.GetInputs()) {
      inputColumns.insert(inputColumns.end(), columns.begin(), columns.end());
    }
    for (const auto& column : m_config.GetTransformDefinition().GetMetadata().requiredDataSources) {
      inputColumns.emplace_back(column);
    }

    if (inputColumns.empty()) {
      // No inputs configured, return empty DataFrame
      return epoch_frame::DataFrame(
          df.index(),
          arrow::Table::MakeEmpty(arrow::schema(arrow::FieldVector{}))
              .MoveValueUnsafe());
    }

    // 2. Rename columns to canonical names
    auto normalizedDf = m_columnMappings.empty() ? df[inputColumns] : df[inputColumns].rename(m_columnMappings);

    // 3. Child classes implement generateSelector() to fill m_selectorData
    generateSelector(normalizedDf);

    // 4. Return empty DataFrame to prevent merging into main computation table
    return epoch_frame::DataFrame(
        df.index(),
        arrow::Table::MakeEmpty(arrow::schema(arrow::FieldVector{}))
            .MoveValueUnsafe());
  }

  // Public getter for the generated selector data (JSON string)
  std::string GetSelectorData() const {
    return m_selectorData;
  }

  virtual ~ISelector() = default;

protected:
  // Child classes only need to implement this to fill m_selectorData
  virtual void generateSelector(const epoch_frame::DataFrame &normalizedDf) const = 0;

  void BuildColumnMappings() {
    // Map input columns like "gap_classifier#result" to expected names like "gap"
    const auto inputs = m_config.GetInputs();
    for (const auto& [inputId, inputColumns] : inputs) {
      for (auto const& column: inputColumns) {
        m_columnMappings.emplace(column, inputId);
      }
    }

    for (const auto& column : m_config.GetTransformDefinition().GetMetadata().requiredDataSources) {
      m_columnMappings.emplace(column, column);
    }
  }

  mutable std::string m_selectorData;  // JSON output for selector widget
  std::unordered_map<std::string, std::string> m_columnMappings;
};

// Template specialization pattern for selector metadata
// Each selector implementation should specialize this
template<typename SelectorClass>
struct SelectorMetadata {
  static epoch_metadata::transforms::TransformsMetaData Get() {
    // Child classes specialize this to provide their metadata
    static_assert(sizeof(SelectorClass) == 0,
                  "Selector class must specialize SelectorMetadata<T>::Get()");
    return {};
  }
};

// Unified registration function that handles both metadata and transform
template<typename SelectorClass>
void RegisterSelector() {
  // 1. Get and register the metadata
  auto metadata = SelectorMetadata<SelectorClass>::Get();
  epoch_metadata::transforms::ITransformRegistry::GetInstance().Register(metadata);

  // 2. Register the transform factory using metadata.id
  epoch_metadata::transform::Register<SelectorClass>(metadata.id);
}

} // namespace epoch_metadata::selectors
