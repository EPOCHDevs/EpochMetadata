//
// Created by adesola on 1/27/25.
//

#pragma once
#include <epoch_metadata/metadata_options.h>
#include <epoch_metadata/strategy/metadata.h>
#include <glaze/glaze.hpp>

CREATE_ENUM(GenericFunctionAuthor, User, Epoch);

namespace epoch_metadata::strategy {
struct GenericFunction {
  std::string type;
  epoch_metadata::MetaDataArgDefinitionMapping args{};
  std::optional<epoch_metadata::TimeFrame> timeframe{};
  std::optional<UIData> data{};

  bool operator==(const GenericFunction &other) const {
    return (type == other.type) && (args == other.args) &&
           (timeframe == other.timeframe) &&
           ((data && other.data) && (*data == *other.data));
  }
};

template <typename T> struct TemplatedGenericFunction {
  T type;
  MetaDataArgDefinitionMapping args;
};
bool EqualsOptionalGenericFunction(std::optional<GenericFunction> const &lhs,
                                   std::optional<GenericFunction> const &rhs);
} // namespace epoch_metadata::strategy