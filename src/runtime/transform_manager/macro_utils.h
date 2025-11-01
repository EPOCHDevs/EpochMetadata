#pragma once
//
// Created by dewe on 9/19/23.
//
#include <epoch_metadata/transforms/transform_configuration.h>

using epoch_metadata::transform::TransformConfiguration;

#define MAKE_GETTER_IMPL(Name, fieldName, Type)                                \
  inline Type Get##Name(const strategy::Context &ctx,                          \
                        const epoch_stratifyx::asset::Asset &asset) const {    \
    return ctx.GetValue<Type>(this->GetTimeframe(), asset,                     \
                              this->GetOutputId(#fieldName));                  \
  }                                                                            \
  inline bool Has##Name(const strategy::Context &ctx,                          \
                        const epoch_stratifyx::asset::Asset &asset) const {    \
    return ctx.Contains(this->GetTimeframe(), asset,                           \
                        this->GetOutputId(#fieldName));                        \
  }

#define MAKE_GETTER(Name, fieldName) MAKE_GETTER_IMPL(Name, fieldName, double)
#define MAKE_STRING_GETTER(Name, fieldName)                                    \
  MAKE_GETTER_IMPL(Name, fieldName, std::string)

#define GET_CURRENT_DECIMAL(cfg) ctx.GetOutputValue(*cfg, asset)
#define GET_CURRENT_BOOLEAN(cfg) ctx.GetOutputValue<bool>(*cfg, asset)
#define GET_CURRENT_DECIMAL_WITH_KEY(cfg, key)                                 \
  ctx.GetOutputValue<double>(*cfg, asset, key)
#define GET_CURRENT_SCALAR_WITH_KEY(cfg, key)                                  \
  ctx.GetScalar(cfg.GetTimeframe(), asset, key)

#define VALIDATE_HANDLE(handle, side)                                          \
  AssertFromStream(handle, side << " handle must be specified");               \
  AssertFromStream(handle->GetOutputs().front().type == IODataType::Boolean,   \
                   side << " handle type must be boolean -> got " +            \
                               glz::prettify(handle->GetOutputs().front()))
