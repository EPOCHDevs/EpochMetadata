//
// Created by dewe on 9/11/24.
//
#pragma once

#include <fmt/format.h>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace epoch_metadata {
template <class MetaDataT, size_t variant = 0> class IMetaDataRegistry {
public:
  static IMetaDataRegistry &GetInstance() noexcept {
    static IMetaDataRegistry instance;
    return instance;
  }

  void Register(MetaDataT metaData) noexcept {
    auto name = metaData.id;
    m_registry[name] = std::move(metaData);
  }

  void Register(std::vector<MetaDataT> const &metaDataList) noexcept {
    for (auto const &metaData : metaDataList) {
      m_registry[metaData.id] = metaData;
    }
  }

  std::optional<std::reference_wrapper<const MetaDataT>>
  GetMetaData(const std::string &name) const noexcept {
    if (auto iter = m_registry.find(name); iter != m_registry.end()) {
      return iter->second;
    }
    return std::nullopt;
  }

  const std::unordered_map<std::string, MetaDataT> &
  GetMetaData() const noexcept {
    return m_registry;
  }

  bool IsValid(const std::string &name) const noexcept {
    return m_registry.contains(name);
  }

private:
  std::unordered_map<std::string, MetaDataT> m_registry;
};
} // namespace epoch_metadata