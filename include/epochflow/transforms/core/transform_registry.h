#pragma once
//
// Created by dewe on 1/10/23.
//
#include "any"
#include "itransform.h"
#include "functional"
#include "memory"
#include "string"
#include "vector"
#include <iostream>

namespace epochflow::transform {

using FunctionInterface =
    std::function<ITransformBasePtr(const TransformConfiguration &)>;

struct TransformRegistry {
  void Register(std::string const &id, FunctionInterface func) {
    m_registry[id] = std::move(func);
  }

  FunctionInterface Get(std::string const &name) const {
    if (m_registry.contains(name)) {
      return m_registry.at(name);
    }
    throw std::runtime_error(name + " not in TI Registry");
  }

  std::unique_ptr<ITransformBase>
  Get(TransformConfiguration const &config) const {
    std::string transformName = config.GetTransformName();
    if (m_registry.contains(transformName)) {
      return m_registry.at(transformName)(config);
    }
    throw std::runtime_error(transformName + " not in TI Registry");
  }

  const auto &GetAll() const { return m_registry; }

  static TransformRegistry &GetInstance() {
    static TransformRegistry instance;
    return instance;
  }

private:
  std::unordered_map<std::string, FunctionInterface> m_registry;

  TransformRegistry() = default;
};

template <class T> void Register(std::string const &id) {
  TransformRegistry::GetInstance().Register(
      id, [](TransformConfiguration const &config) {
        return std::make_unique<T>(config);
      });
}
} // namespace epochflow::transform

#define REGISTER_TRANSFORM(id, T) Register<T>(#id)
#define MAKE_TRANSFORM(config)                                                 \
  epochflow::transform::TransformRegistry::GetInstance().Get(config)
