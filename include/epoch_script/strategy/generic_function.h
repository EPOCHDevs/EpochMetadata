//
// Created by adesola on 1/27/25.
//

#pragma once
#include <epoch_script/core/metadata_options.h>
#include <epoch_script/strategy/metadata.h>
#include <glaze/glaze.hpp>

CREATE_ENUM(GenericFunctionAuthor, User, Epoch);

namespace epoch_script::strategy
{
  struct GenericFunction
  {
    std::optional<std::string> type{};
    std::optional<epoch_script::MetaDataArgDefinitionMapping> args{};
    std::optional<epoch_script::TimeFrame> timeframe{};
    std::optional<PythonSource> source{};
    glz::generic kwarg{};

    bool operator==(const GenericFunction &other) const
    {
      return (type == other.type) && (args == other.args) &&
             (timeframe == other.timeframe) &&
             (source == other.source) &&
             (glz::write_json(kwarg) == glz::write_json(other.kwarg));
    }
  };

  template <typename T>
  struct TemplatedGenericFunction
  {
    T type;
    MetaDataArgDefinitionMapping args;
  };
  bool EqualsOptionalGenericFunction(std::optional<GenericFunction> const &lhs,
                                     std::optional<GenericFunction> const &rhs);
} // namespace epoch_script::strategy