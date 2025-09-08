#pragma once
//
// Created by dewe on 7/5/23.
//
#include "mutex"
#include "spdlog/spdlog.h"
#include "unordered_map"
#include <glaze/glaze.hpp>
#include <set>
#include <unordered_set>

namespace epoch_metadata {

class ID {
public:
  ID(uint64_t value) : value(value) {}

  uint64_t Get() const noexcept { return value; }

  auto operator<=>(ID const &) const = default;

  friend std::ostream &operator<<(std::ostream &streamer, ID const &id) {
    streamer << (id.value == static_cast<uint64_t>(-1)
                     ? "INVALID_ID"
                     : std::to_string(id.value));
    return streamer;
  }

  std::string ToString() const noexcept { return std::to_string(value); }

  struct hash {
    inline size_t operator()(ID const &id) const { return id.value; }
  };

private:
  friend class OrderIDSequence;

  uint64_t value{};
};

const ID INVALID_ID{static_cast<uint64_t>(-1)};

struct IDSequence {
  inline ID GetNextID() {
    return {static_cast<uint64_t>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count())};
  }

  static IDSequence &GetInstance() {
    static IDSequence idSequence;
    return idSequence;
  }

  IDSequence() = default;
};

using IDHashSet = std::unordered_set<ID, ID::hash>;

template <class T> using IDHashMap = std::unordered_map<ID, T, ID::hash>;

inline ID RequestNewID() noexcept {
  return IDSequence::GetInstance().GetNextID();
}

template <class T> IDHashMap<T> MakeIDMap(std::vector<T> const &items) {
  IDHashMap<T> result;
  for (auto const &item : items) {
    result.emplace(item.GetID(), item);
  }
  return result;
}

} // namespace epoch_metadata

namespace glz {
template <> struct meta<epoch_metadata::ID> {
  static constexpr auto read = [](epoch_metadata::ID &x,
                                  const std::string &input) {
    x = std::stol(input);
  };

  static constexpr auto write = [](auto &x) -> auto { return x.Get(); };

  static constexpr auto value = glz::custom<read, write>;
};
} // namespace glz