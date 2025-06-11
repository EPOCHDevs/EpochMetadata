#pragma once
#include <epoch_metadata/metadata_options.h>
#include "epoch_metadata/time_frame.h"

namespace epoch_metadata::strategy {
  // ------------------------------------------------------------------
  // 1) UI Data Structures
  // ------------------------------------------------------------------
  struct UIOption {
    std::string id;
    std::optional<MetaDataOptionDefinition::T> value;
    std::optional<std::string> name;
    bool isExposed = false;
  };

  struct UINodePosition {
    double x{0}, y{0};
  };

  struct UINodeMetadata {
    UINodePosition position{};
    std::optional<std::string> parentId{std::nullopt};
    double height{};
    double width{};
  };

  struct UINode {
    std::string id;
    std::string type;

    std::vector<UIOption> options{};
    UINodeMetadata metadata;
    std::optional<TimeFrame> timeframe{std::nullopt};
  };

  struct UIVertex {
    std::string id;
    std::string handle;

    friend std::ostream &operator<<(std::ostream &os, const UIVertex &handle) {
      return os << "UIVertex(id=" << handle.id << ", handle=" << handle.handle
                << ")";
    }
  };

  struct UIEdge {
    UIVertex source;
    UIVertex target;
  };

  struct UIGroupNode {
    std::string id;
    std::string label;
    UINodePosition position{};
    double height{};
    double width{};
  };

  struct UIAnnotationNode {
    std::string id;
    std::string content;
    UINodePosition position{};
    double height{};
    double width{};
    std::optional<std::string> parentId;
  };

  // The full UI data
  struct UIData {
    std::vector<UINode> nodes;
    std::vector<UIEdge> edges;
    std::vector<UIGroupNode> groups;
    std::vector<UIAnnotationNode> annotations;
  };
} // namespace epoch_stratifyx::server
