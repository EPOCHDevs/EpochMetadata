#pragma once
//
// Created by dewe on 1/8/23.
//
#include "epoch_metadata/transforms/transform_configuration.h"
#include "memory"
#include <epoch_frame/dataframe.h>
#include <epoch_frame/series.h>
#include "epoch_protos/tearsheet.pb.h"

namespace epoch_metadata::transform {

  // Forward declaration for SelectorData
  struct SelectorData {
    std::string title;
    std::vector<epoch_metadata::CardColumnSchema> schemas;
    epoch_frame::DataFrame data;

    SelectorData() = default;
    SelectorData(std::string title_,
                 std::vector<epoch_metadata::CardColumnSchema> schemas_,
                 epoch_frame::DataFrame data_)
        : title(std::move(title_)),
          schemas(std::move(schemas_)),
          data(std::move(data_)) {}
  };

struct ITransformBase {

  virtual std::string GetId() const = 0;

  virtual std::string GetName() const = 0;

  virtual epoch_metadata::MetaDataOptionDefinition
  GetOption(std::string const &param) const = 0;

  virtual epoch_metadata::MetaDataOptionList GetOptionsMetaData() const = 0;

  virtual std::string GetOutputId(const std::string &output) const = 0;

  virtual std::string GetOutputId() const = 0;

  virtual std::string GetInputId(const std::string &inputId) const = 0;

  virtual std::string GetInputId() const = 0;

  virtual std::vector<std::string> GetInputIds() const = 0;

  virtual std::vector<epoch_metadata::transforms::IOMetaData>
  GetOutputMetaData() const = 0;

  virtual epoch_metadata::TimeFrame GetTimeframe() const = 0;

  virtual TransformConfiguration GetConfiguration() const = 0;

  virtual epoch_frame::DataFrame
  TransformData(const epoch_frame::DataFrame &) const = 0;

  virtual  epoch_proto::TearSheet GetTearSheet() const = 0;
  virtual  SelectorData GetSelectorData() const = 0;

  virtual ~ITransformBase() = default;
};

class ITransform : public ITransformBase {

public:
  explicit ITransform(TransformConfiguration config)
      : m_config(std::move(config)) {}

  std::string GetId() const final { return m_config.GetId(); }

  inline std::string GetName() const final {
    return m_config.GetTransformName();
  }

  epoch_metadata::MetaDataOptionDefinition
  GetOption(std::string const &param) const final {
    return m_config.GetOptionValue(param);
  }

  epoch_metadata::MetaDataOptionList GetOptionsMetaData() const final {
    return m_config.GetTransformDefinition().GetMetadata().options;
  }

  std::string GetOutputId(const std::string &output) const override {
    return m_config.GetOutputId(output);
  }

  std::string GetOutputId() const final { return m_config.GetOutputId(); }

  std::string GetInputId(const std::string &inputId) const override {
    return m_config.GetInput(inputId);
  }

  std::string GetInputId() const override { return m_config.GetInput(); }

  std::vector<std::string> GetInputIds() const override {
    std::vector<std::string> result;

    const auto inputs = m_config.GetTransformDefinition().GetMetadata().inputs;
    std::ranges::for_each(
        inputs, [&](epoch_metadata::transforms::IOMetaData const &io) {
          auto out = m_config.GetInputs(io.id);
          if (out.empty()) {
            AssertFromStream(
                m_config.GetTransformName() ==
                    epoch_metadata::transforms::TRADE_SIGNAL_EXECUTOR_ID,
                "Only trade signal executor can have unconnected inputs.");
            return;
          }
          result.insert(result.end(), out.begin(), out.end());
        });
    return result;
  }

  std::vector<epoch_metadata::transforms::IOMetaData>
  GetOutputMetaData() const override {
    return m_config.GetOutputs();
  }

  epoch_metadata::TimeFrame GetTimeframe() const final {
    return m_config.GetTimeframe();
  }

  TransformConfiguration GetConfiguration() const final { return m_config; }

  friend std::ostream &operator<<(std::ostream &os, ITransform const &model) {
    os << model.m_config.ToString();
    return os;
  }

  epoch_proto::TearSheet GetTearSheet() const override {
    return epoch_proto::TearSheet::default_instance();
  }

  SelectorData GetSelectorData() const override {
    return {};
  }

  ~ITransform() override = default;
  using Ptr = std::shared_ptr<ITransform>;

protected:
  TransformConfiguration m_config;

  static auto GetValidSeries(epoch_frame::Series const &input) {
    const auto output = input.loc(input.is_valid());
    return std::pair{output.contiguous_array(), output};
  }

  epoch_frame::DataFrame MakeResult(epoch_frame::Series const &series) const {
    return series.to_frame(GetOutputId());
  }

  // Build column rename mapping for input-based SQL queries
  // Maps input column names to SLOT0, SLOT1, SLOT2, etc. based on order
  std::unordered_map<std::string, std::string> BuildVARGInputRenameMapping() const {
    std::unordered_map<std::string, std::string> renameMap;
    auto slot = m_config.GetInputs();
    AssertFromStream(slot.size() == 1, "Expected a VARG");
    for (auto const &[i, column] : std::views::enumerate(slot.begin()->second)) {
      renameMap[column] = "SLOT" + std::to_string(i);
    }

    return renameMap;
  }
};

using ITransformBasePtr = std::unique_ptr<ITransformBase>;
} // namespace epoch_metadata::transform