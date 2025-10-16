//
// Created by adesola on 1/27/25.
//

#pragma once
#include <epoch_metadata/metadata_options.h>
#include <epoch_metadata/strategy/metadata.h>
#include <glaze/glaze.hpp>

CREATE_ENUM(GenericFunctionAuthor, User, Epoch);

namespace epoch_metadata::strategy
{
  struct GenericFunction
  {
    std::optional<std::string> type{};
    std::optional<epoch_metadata::MetaDataArgDefinitionMapping> args{};
    std::optional<epoch_metadata::TimeFrame> timeframe{};
    std::optional<UIData> data{};
    glz::generic kwarg{};

    bool operator==(const GenericFunction &other) const
    {
      return (type == other.type) && (args == other.args) &&
             (timeframe == other.timeframe) &&
             ((data && other.data) && (*data == *other.data)) &&
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
} // namespace epoch_metadata::strategy