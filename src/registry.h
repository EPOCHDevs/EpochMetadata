//
// Created by dewe on 9/11/24.
//
#pragma once

#include <fmt/format.h>
#include <functional>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace epoch_metadata {
template <class MetaDataT, size_t variant = 0> class IMetaDataRegistry {
public:
  static IMetaDataRegistry &GetInstance() {
    static IMetaDataRegistry instance;
    return instance;
  }

  void Register(MetaDataT metaData) {
    auto name = metaData.id;
    m_registry[name] = std::move(metaData);
  }

  void Register(std::vector<MetaDataT> const &metaDataList) {
    for (auto const &metaData : metaDataList) {
      m_registry[metaData.id] = metaData;
    }
  }

  const MetaDataT *GetMetaData(const std::string &name) const {
    try {
      return &m_registry.at(name);
    } catch (std::out_of_range &e) {
      const auto error = fmt::format("MetaData not found: {}", name);
      throw std::runtime_error(error);
    }
  }

  const std::unordered_map<std::string, MetaDataT> &GetMetaData() const {
    return m_registry;
  }

  bool IsValid(const std::string &name) const {
    return m_registry.contains(name);
  }

private:
  std::unordered_map<std::string, MetaDataT> m_registry;
};
} // namespace epoch_metadata