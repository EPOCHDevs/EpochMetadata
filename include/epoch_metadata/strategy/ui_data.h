#pragma once
#include "epoch_metadata/time_frame.h"
#include <epoch_metadata/metadata_options.h>
#include <glaze/glaze.hpp>

namespace epoch_metadata::strategy {
// ------------------------------------------------------------------
// 1) UI Data Structures
// ------------------------------------------------------------------
struct UIOption {
  std::string id;
  std::optional<MetaDataOptionDefinition::T> value;
  std::optional<std::string> name;
  bool isExposed = false;

  bool operator==(const UIOption &other) const = default;
};

struct UINodePosition {
  double x{0}, y{0};
  bool operator==(const UINodePosition &other) const = default;
};

struct UINodeMetadata {
  UINodePosition position{};
  std::optional<std::string> parentId{std::nullopt};
  double height{};
  double width{};

  bool operator==(const UINodeMetadata &other) const = default;
};

struct UINode {
  std::string id;
  std::string type;

  std::vector<UIOption> options{};
  UINodeMetadata metadata;
  std::optional<TimeFrame> timeframe{std::nullopt};

  bool operator==(const UINode &other) const = default;
};

struct UIVertex {
  std::string id;
  std::string handle;

  friend std::ostream &operator<<(std::ostream &os, const UIVertex &handle) {
    return os << "UIVertex(id=" << handle.id << ", handle=" << handle.handle
              << ")";
  }

  bool operator==(const UIVertex &other) const = default;
};

struct UIEdge {
  UIVertex source;
  UIVertex target;

  bool operator==(const UIEdge &other) const = default;
};

struct UIGroupNode {
  std::string id;
  std::string label;
  UINodePosition position{};
  double height{};
  double width{};

  bool operator==(const UIGroupNode &other) const = default;
};

struct UIAnnotationNode {
  std::string id;
  std::string content;
  UINodePosition position{};
  double height{};
  double width{};
  std::optional<std::string> parentId;

  bool operator==(const UIAnnotationNode &other) const = default;
};

// The full UI data
struct UIData {
  std::vector<UINode> nodes;
  std::vector<UIEdge> edges;
  std::vector<UIGroupNode> groups;
  std::vector<UIAnnotationNode> annotations;

  bool operator==(const UIData &other) const = default;
};
} // namespace epoch_metadata::strategy
