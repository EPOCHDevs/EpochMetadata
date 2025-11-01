//
// Created by dewe on 9/10/24.
//

#pragma once
#include "epoch_script/core/metadata_options.h"
#include "epoch_script/core/time_frame.h"
#include "enums.h"
#include "session_variant.h"
#include <glaze/json/generic.hpp>
#include <variant>
#include <optional>

// including here ensure all transforms have been serialized
namespace epoch_script::strategy
{
  // Forward declaration
  struct AlgorithmNode;

  // PythonSource - encapsulates EpochScript source code with pre-compiled metadata
  class PythonSource
  {
  private:
    std::string source_;
    std::vector<AlgorithmNode> compilationResult_;
    bool isIntraday_{false};
    std::optional<epoch_core::BaseDataTimeFrame> baseTimeframe_;
    size_t m_executor_count{};

  public:
    // Default constructor
    PythonSource() = default;

    // Constructor that compiles source and extracts metadata
    explicit PythonSource(std::string src);

    // Const getters
    const std::string &GetSource() const { return source_; }
    const std::vector<AlgorithmNode> &GetCompilationResult() const { return compilationResult_; }
    bool IsIntraday() const { return isIntraday_; }
    const std::optional<epoch_core::BaseDataTimeFrame> &GetBaseTimeframe() const { return baseTimeframe_; }

    // Equality operator for comparison
    bool operator==(const PythonSource &other) const
    {
      return source_ == other.source_;
    }

    size_t getExecutorCount() const { return m_executor_count; }

    // Glaze serialization support - friend for custom serialization
    friend struct glz::meta<PythonSource>;
  };

  struct AlgorithmBaseMetaData
  {
    std::string id;
    std::string name;
    MetaDataOptionList options{};
    std::string desc{};
    std::vector<std::string> tags{};
  };

  struct AlgorithmMetaData
  {
    std::string id;
    std::string name;
    MetaDataOptionList options{};
    std::string desc{};
    bool requiresTimeframe{true};
    std::vector<std::string> tags{};
  };

  using InputMapping = std::unordered_map<std::string, std::vector<std::string>>;
  struct AlgorithmNode
  {
    std::string type;
    std::string id{};
    epoch_script::MetaDataArgDefinitionMapping options{};
    InputMapping inputs{};
    std::optional<TimeFrame> timeframe{};
    std::optional<SessionVariant> session{};
    bool operator==(const AlgorithmNode &other) const
    {
      return type == other.type && id == other.id && options == other.options &&
             inputs == other.inputs && timeframe == other.timeframe &&
             (session == other.session);
    }
  };

  struct TradeSignalMetaData
  {
    std::string id;
    std::string name;
    MetaDataOptionList options{};
    std::string desc{};
    bool requiresTimeframe{true};
    PythonSource source;
    std::vector<std::string> tags{};
  };

  struct PartialTradeSignalMetaData
  {
    MetaDataOptionList options;
    std::vector<AlgorithmNode> algorithm;
    AlgorithmNode executor;
  };

  // Copy member variables to support glaze serialization form decomposition
} // namespace epoch_script::strategy

namespace YAML
{
  template <>
  struct convert<epoch_script::strategy::SessionVariant>
  {
    static bool decode(const Node &node,
                       epoch_script::strategy::SessionVariant &out);
  };

  template <>
  struct convert<epoch_script::strategy::AlgorithmNode>
  {
    static bool decode(YAML::Node const &,
                       epoch_script::strategy::AlgorithmNode &);
  };

  template <>
  struct convert<epoch_script::strategy::AlgorithmBaseMetaData>
  {
    static bool decode(YAML::Node const &,
                       epoch_script::strategy::AlgorithmBaseMetaData &);
  };

  template <>
  struct convert<epoch_script::strategy::AlgorithmMetaData>
  {
    static bool decode(YAML::Node const &,
                       epoch_script::strategy::AlgorithmMetaData &);
  };

  epoch_script::strategy::TradeSignalMetaData decode(glz::generic const &);

  glz::generic encode(epoch_script::strategy::TradeSignalMetaData const &);

} // namespace YAML

// Glaze serialization for PythonSource
namespace glz
{
  // Custom serialization for PythonSource - serialize/deserialize as string
  template <>
  struct to<JSON, epoch_script::strategy::PythonSource>
  {
    template <auto Opts>
    static void op(const epoch_script::strategy::PythonSource &x, auto &&...args) noexcept
    {
      serialize<JSON>::op<Opts>(x.GetSource(), args...);
    }
  };

  template <>
  struct from<JSON, epoch_script::strategy::PythonSource>
  {
    template <auto Opts>
    static void op(epoch_script::strategy::PythonSource &value, auto &&...args)
    {
      std::string source_code;
      parse<JSON>::op<Opts>(source_code, args...);
      value = epoch_script::strategy::PythonSource(source_code);
    }
  };
} // namespace glz