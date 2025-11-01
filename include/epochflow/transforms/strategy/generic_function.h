//
// Created by adesola on 1/27/25.
//

#pragma once
#include <epochflow/core/metadata_options.h>
#include <epochflow/transforms/strategy/metadata.h>
#include <glaze/glaze.hpp>

CREATE_ENUM(GenericFunctionAuthor, User, Epoch);

namespace epochflow::strategy
{
  struct GenericFunction
  {
    std::optional<std::string> type{};
    std::optional<epochflow::MetaDataArgDefinitionMapping> args{};
    std::optional<epochflow::TimeFrame> timeframe{};
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
} // namespace epochflow::strategy