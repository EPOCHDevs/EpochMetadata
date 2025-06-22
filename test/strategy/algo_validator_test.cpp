#include "epoch_metadata/strategy/algorithm_validator.h"
#include "epoch_metadata/strategy/ui_data.h"
#include "epoch_metadata/strategy/validation.h"
#include "epoch_metadata/strategy/validation_error.h"
#include "epoch_metadata/transforms/registry.h"
#include <catch.hpp>
#include <catch2/catch_message.hpp>
#include <glaze/glaze.hpp>
#include <catch2/generators/catch_generators.hpp>

using namespace epoch_metadata::strategy;

namespace {

UIData ParseUIData(const std::string &json) {
  UIData data;
  if (auto result = glz::read_json(data, json); result) {
    INFO("JSON parsing failed: " << glz::format_error(result, json));
    FAIL();
  }
  return data;
}

void ExpectValidationError(const ValidationResult &result,
                           ValidationCode expectedCode,
                           const std::string &expectedMessagePart = "") {
  INFO("Expecting validation error with code: "
       << ValidationCodeWrapper::ToString(expectedCode));
  REQUIRE_FALSE(result.has_value());

  const auto &issues = result.error();
  bool found = false;
  for (const auto &issue : issues) {
    if (issue.code == expectedCode) {
      found = true;
      if (!expectedMessagePart.empty()) {
        INFO("Found error message: " << issue.message);
        REQUIRE_THAT(issue.message,
                     Catch::Matchers::ContainsSubstring(expectedMessagePart));
      }
      REQUIRE(issue.suggestion.has_value());
      REQUIRE_FALSE(issue.suggestion.value().empty());
      break;
    }
  }
  if (!found) {
    INFO("Available error codes in result:");
    for (const auto &issue : issues) {
      INFO("  - Code: " << ValidationCodeWrapper::ToString(issue.code)
                        << ", Message: " << issue.message);
    }
  }
  REQUIRE(found);
}

} // namespace

TEST_CASE("AlgorithmValidator: Empty Graph", "[AlgorithmValidator]") {
  // Simplified JSON without meta-aata field
  const std::string json = R"({
        "nodes": [],
        "edges": [],
        "groups": [],
        "annotations": []
    })";

  // Try manual parsing first
  UIData data;

  if (const auto error = glz::read_json(data, json)) {
    INFO(glz::format_error(error));
  } else {
    const auto validationResult = ValidateUIData(data);
    ExpectValidationError(validationResult, ValidationCode::EmptyGraph,
                          "Graph contains no nodes");
  }
}

TEST_CASE("AlgorithmValidator: Missing Executor", "[AlgorithmValidator]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "node1",
                "type": "sma",
                "options": [{"id": "period", "value": 20}],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);
  const auto result = ValidateUIData(data);
  ExpectValidationError(result, ValidationCode::MissingExecutor,
                        "No TradeSignalExecutor");
}

TEST_CASE("AlgorithmValidator: Multiple Executors", "[AlgorithmValidator]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "executor1",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "executor2",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);
  auto result = ValidateUIData(data);

  ExpectValidationError(result, ValidationCode::MultipleExecutors,
                        "Found 2 TradeSignalExecutors");
}

TEST_CASE("AlgorithmValidator: Unknown Node Type", "[AlgorithmValidator]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "unknown_node",
                "type": "unknown_transform_type",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);
  auto result = ValidateUIData(data);

  ExpectValidationError(result, ValidationCode::UnknownNodeType,
                        "Unknown node type");
}

TEST_CASE("AlgorithmValidator: Empty Node Id", "[AlgorithmValidator]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "",
                "type": "sma",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);
  auto result = ValidateUIData(data);

  ExpectValidationError(result, ValidationCode::InvalidNodeId,
                        "Node has empty id");
}

TEST_CASE("AlgorithmValidator: Duplicate Node Id", "[AlgorithmValidator]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "duplicate_id",
                "type": "sma",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "duplicate_id",
                "type": "rsi",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);
  auto result = ValidateUIData(data);

  ExpectValidationError(result, ValidationCode::InvalidNodeId,
                        "Duplicate node id");
}

TEST_CASE("AlgorithmValidator: Orphaned Node with no connections",
          "[AlgorithmValidator]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "orphan",
                "type": "sma",
                "options": [{"id": "period", "value": 20}],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);
  auto result = ValidateUIData(data);

  ExpectValidationError(result, ValidationCode::OrphanedNode,
                        "has no connections");
}

TEST_CASE("AlgorithmValidator: Orphaned Node with no output connections",
          "[AlgorithmValidator]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "mds",
                "type": "market_data_source",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "orphan",
                "type": "sma",
                "options": [{"id": "period", "value": 20}],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [
            {
                "source": {"id": "mds", "handle": "c"},
                "target": {"id": "orphan", "handle": "*"}
            }
        ],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);
  auto result = ValidateUIData(data);

  ExpectValidationError(result, ValidationCode::OrphanedNode,
                        "has no output connections");
}

TEST_CASE("AlgorithmValidator: Invalid Edge - Unknown Node",
          "[AlgorithmValidator]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "node1",
                "type": "sma",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [
            {
                "source": {"id": "unknown_node", "handle": "output"},
                "target": {"id": "node1", "handle": "input"}
            }
        ],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);
  auto result = ValidateUIData(data);

  ExpectValidationError(result, ValidationCode::InvalidEdge,
                        "references unknown source node");
}

TEST_CASE("AlgorithmValidator: Invalid Edge - Unknown Handle",
          "[AlgorithmValidator]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "mds",
                "type": "market_data_source",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "sma",
                "type": "sma",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [
            {
                "source": {"id": "mds", "handle": "unknown_handle"},
                "target": {"id": "sma", "handle": "*"}
            }
        ],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);
  auto result = ValidateUIData(data);

  ExpectValidationError(result, ValidationCode::InvalidEdge,
                        "references unknown source handle");
}

TEST_CASE("AlgorithmValidator: Invalid Edge - Empty Handle",
          "[AlgorithmValidator]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "mds",
                "type": "market_data_source",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [
            {
                "source": {"id": "mds", "handle": ""},
                "target": {"id": "executor", "handle": "long"}
            }
        ],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);
  auto result = ValidateUIData(data);

  ExpectValidationError(result, ValidationCode::InvalidEdge,
                        "references unknown source handle");
}

TEST_CASE("AlgorithmValidator: Self Loop Edge", "[AlgorithmValidator]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "node1",
                "type": "sma",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [
            {
                "source": {"id": "node1", "handle": "result"},
                "target": {"id": "node1", "handle": "*"}
            }
        ],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);
  auto result = ValidateUIData(data);

  ExpectValidationError(result, ValidationCode::InvalidEdge,
                        "same source and target");
}

TEST_CASE("AlgorithmValidator: Cycle Detection", "[AlgorithmValidator]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "node1",
                "type": "sma",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "node2",
                "type": "sma",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "node3",
                "type": "sma",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [
            {
                "source": {"id": "node1", "handle": "output"},
                "target": {"id": "node2", "handle": "input"}
            },
            {
                "source": {"id": "node2", "handle": "output"},
                "target": {"id": "node3", "handle": "input"}
            },
            {
                "source": {"id": "node3", "handle": "output"},
                "target": {"id": "node1", "handle": "input"}
            }
        ],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);
  auto result = ValidateUIData(data);

  ExpectValidationError(result, ValidationCode::CycleDetected,
                        "Cycle detected");
}

TEST_CASE("AlgorithmValidator: Timeframe Mismatch", "[AlgorithmValidator]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "mds",
                "type": "market_data_source",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "sma1",
                "type": "sma",
                "options": [],
                "metadata": {},
                "timeframe": {"type": "hour", "interval": 1}
            },
            {
                "id": "sma2",
                "type": "sma",
                "options": [],
                "metadata": {},
                "timeframe": {"type": "day", "interval": 1}
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [
            {
                "source": {"id": "mds", "handle": "c"},
                "target": {"id": "sma1", "handle": "*"}
            },
            {
                "source": {"id": "sma1", "handle": "result"},
                "target": {"id": "sma2", "handle": "*"}
            },
            {
                "source": {"id": "sma2", "handle": "result"},
                "target": {"id": "executor", "handle": "long"}
            }
        ],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);
  auto result = ValidateUIData(data);

  ExpectValidationError(result, ValidationCode::TimeframeMismatch,
                        "has timeframe set but requiresTimeFrame is false");
}

TEST_CASE("AlgorithmValidator: Multiple Input Timeframes",
          "[AlgorithmValidator]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "mds1",
                "type": "market_data_source",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "mds2",
                "type": "market_data_source",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "atr1",
                "type": "atr",
                "options": [{"id": "period", "value": 14}],
                "metadata": {},
                "timeframe": {"type": "hour", "interval": 1}
            },
            {
                "id": "atr2",
                "type": "atr",
                "options": [{"id": "period", "value": 21}],
                "metadata": {},
                "timeframe": {"type": "day", "interval": 1}
            },
            {
                "id": "add",
                "type": "add",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [
            {
                "source": {"id": "mds1", "handle": "c"},
                "target": {"id": "atr1", "handle": "*"}
            },
            {
                "source": {"id": "mds2", "handle": "c"},
                "target": {"id": "atr2", "handle": "*"}
            },
            {
                "source": {"id": "atr1", "handle": "result"},
                "target": {"id": "add", "handle": "*0"}
            },
            {
                "source": {"id": "atr2", "handle": "result"},
                "target": {"id": "add", "handle": "*1"}
            },
            {
                "source": {"id": "add", "handle": "result"},
                "target": {"id": "executor", "handle": "long"}
            }
        ],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);
  auto result = ValidateUIData(data);

  ExpectValidationError(result, ValidationCode::TimeframeMismatch,
                        "Mixed timeframes detected");
}

TEST_CASE("AlgorithmValidator: Input Timeframe does not match target timeframe",
          "[AlgorithmValidator]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "mds",
                "type": "market_data_source",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "atr1",
                "type": "atr",
                "options": [{"id": "period", "value": 14}],
                "metadata": {},
                "timeframe": {"type": "hour", "interval": 1}
            },
            {
                "id": "atr2",
                "type": "atr",
                "options": [{"id": "period", "value": 21}],
                "metadata": {},
                "timeframe": {"type": "day", "interval": 1}
            },
            {
                "id": "add",
                "type": "add",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [
            {
                "source": {"id": "mds", "handle": "c"},
                "target": {"id": "atr1", "handle": "*"}
            },
            {
                "source": {"id": "mds", "handle": "c"},
                "target": {"id": "atr2", "handle": "*"}
            },
            {
                "source": {"id": "atr1", "handle": "result"},
                "target": {"id": "add", "handle": "*0"}
            },
            {
                "source": {"id": "atr2", "handle": "result"},
                "target": {"id": "add", "handle": "*1"}
            },
            {
                "source": {"id": "add", "handle": "result"},
                "target": {"id": "executor", "handle": "long"}
            }
        ],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);
  auto result = ValidateUIData(data);

  ExpectValidationError(result, ValidationCode::TimeframeMismatch,
                        "Mixed timeframes detected");
}

TEST_CASE("AlgorithmValidator: Missing Required Option",
          "[AlgorithmValidator]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "mds",
                "type": "market_data_source",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "sma",
                "type": "sma",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [
            {
                "source": {"id": "mds", "handle": "c"},
                "target": {"id": "sma", "handle": "*"}
            },
            {
                "source": {"id": "sma", "handle": "result"},
                "target": {"id": "executor", "handle": "long"}
            }
        ],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);
  auto result = ValidateUIData(data);

  // SMA requires period option
  ExpectValidationError(result, ValidationCode::MissingRequiredOption,
                        "missing required option");
}

TEST_CASE("AlgorithmValidator: Invalid Option Type", "[AlgorithmValidator]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "mds",
                "type": "market_data_source",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "sma",
                "type": "sma",
                "options": [
                    {"id": "period", "value": "not_a_number", "isExposed": false}
                ],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [
            {
                "source": {"id": "mds", "handle": "c"},
                "target": {"id": "sma", "handle": "*"}
            },
            {
                "source": {"id": "sma", "handle": "result"},
                "target": {"id": "executor", "handle": "long"}
            }
        ],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);
  auto result = ValidateUIData(data);

  ExpectValidationError(result, ValidationCode::InvalidOptionReference,
                        "invalid type");
}

TEST_CASE("AlgorithmValidator: Unknown Option", "[AlgorithmValidator]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "mds",
                "type": "market_data_source",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "sma",
                "type": "sma",
                "options": [
                    {"id": "period", "value": 20, "isExposed": false},
                    {"id": "unknown_option", "value": 42, "isExposed": false}
                ],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [
            {
                "source": {"id": "mds", "handle": "c"},
                "target": {"id": "sma", "handle": "*"}
            },
            {
                "source": {"id": "sma", "handle": "result"},
                "target": {"id": "executor", "handle": "long"}
            }
        ],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);
  auto result = ValidateUIData(data);

  ExpectValidationError(result, ValidationCode::InvalidOptionReference,
                        "Unknown option");
}

TEST_CASE("AlgorithmValidator: Exposed Option Without Name",
          "[AlgorithmValidator]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "mds",
                "type": "market_data_source",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "sma",
                "type": "sma",
                "options": [
                    {"id": "period", "value": 20, "name": "", "isExposed": true}
                ],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [
            {
                "source": {"id": "mds", "handle": "c"},
                "target": {"id": "sma", "handle": "*"}
            },
            {
                "source": {"id": "sma", "handle": "result"},
                "target": {"id": "executor", "handle": "long"}
            }
        ],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);
  auto result = ValidateUIData(data);

  ExpectValidationError(result, ValidationCode::InvalidOptionReference,
                        "empty display name");
}

TEST_CASE("AlgorithmValidator: Valid Complex Graph", "[AlgorithmValidator]") {
  // Using the provided example JSON
  const std::string json = R"({
      "nodes": [
        {
          "id": "mds",
          "type": "market_data_source",
          "options": [],
          "metadata": {
            "parentId": null
          },
          "timeframe": null
        },
        {
          "id": "min50",
          "type": "min",
          "options": [
            {
              "id": "period",
              "value": 50,
              "name": "Period",
              "isExposed": false
            }
          ],
          "metadata": {
            "parentId": null
          },
          "timeframe": null
        },
        {
          "id": "gt1",
          "type": "gt",
          "options": [],
          "metadata": {
            "parentId": null
          },
          "timeframe": null
        },
        {
          "id": "executor",
          "type": "trade_signal_executor",
          "options": [
            {
              "id": "closeIfIndecisive",
              "value": false,
              "name": "Exit If Indecisive",
              "isExposed": false
            }
          ],
          "metadata": {
            "parentId": null
          },
          "timeframe": null
        }
      ],
      "edges": [

        {
          "source": {
            "id": "mds",
            "handle": "l"
          },
          "target": {
            "id": "min50",
            "handle": "*"
          }
        },
        {
          "source": {
            "id": "min50",
            "handle": "result"
          },
          "target": {
            "id": "gt1",
            "handle": "*0"
          }
        },
        {
          "source": {
            "id": "mds",
            "handle": "l"
          },
          "target": {
            "id": "gt1",
            "handle": "*1"
          }
        },
        {
          "source": {
            "id": "gt1",
            "handle": "result"
          },
          "target": {
            "id": "executor",
            "handle": "long"
          }
        },
        {
          "source": {
            "id": "gt1",
            "handle": "result"
          },
          "target": {
            "id": "executor",
            "handle": "short"
          }
        }
      ],
      "groups": [],
      "annotations": []
    })";

  auto data = ParseUIData(json);
  auto result = ValidateUIData(data);

  auto error = result.has_value() ? std::string{} : FormatValidationIssues(result.error());
  INFO(error);

  // This should be valid
  REQUIRE(result.has_value());

  const auto &sortedNodes = result.value();
  REQUIRE(sortedNodes.size() == 4);

  // Check topological order
  std::unordered_map<std::string, size_t> nodeOrder;
  for (size_t i = 0; i < sortedNodes.size(); ++i) {
    nodeOrder[sortedNodes[i].id] = i;
  }

  // mds should come before min50 and gt1
  REQUIRE(nodeOrder["mds"] < nodeOrder["min50"]);
  REQUIRE(nodeOrder["mds"] < nodeOrder["gt1"]);

  // min50 and gt1 should come before executor
  REQUIRE(nodeOrder["min50"] < nodeOrder["gt1"]);
  REQUIRE(nodeOrder["gt1"] < nodeOrder["executor"]);
}

TEST_CASE("AlgorithmValidator: Mixed Timeframes Not Allowed",
          "[AlgorithmValidator]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "mds",
                "type": "market_data_source",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "atr1",
                "type": "atr",
                "options": [{"id": "period", "value": 14}],
                "metadata": {},
                "timeframe": {"type": "hour", "interval": 1}
            },
            {
                "id": "atr2",
                "type": "atr",
                "options": [{"id": "period", "value": 21}],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "gt",
                "type": "gt",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [
            {
                "source": {"id": "mds", "handle": "c"},
                "target": {"id": "atr1", "handle": "*"}
            },
            {
                "source": {"id": "mds", "handle": "c"},
                "target": {"id": "atr2", "handle": "*"}
            },
            {
                "source": {"id": "atr1", "handle": "result"},
                "target": {"id": "gt", "handle": "*0"}
            },
            {
                "source": {"id": "atr2", "handle": "result"},
                "target": {"id": "gt", "handle": "*1"}
            },
            {
                "source": {"id": "gt", "handle": "result"},
                "target": {"id": "executor", "handle": "long"}
            }
        ],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);
  auto result = ValidateUIData(data);

  // Mixed timeframes are no longer allowed in the simplified model
  ExpectValidationError(result, ValidationCode::TimeframeMismatch,
                        "Mixed timeframes detected");
}

TEST_CASE("AlgorithmValidator: Missing Required Input",
          "[AlgorithmValidator]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "add",
                "type": "add",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);
  auto result = ValidateUIData(data);

  // Add node requires at least one input
  ExpectValidationError(result, ValidationCode::MissingRequiredInput,
                        "no input connections");
}

TEST_CASE("AlgorithmValidator: Invalid Connection Count",
          "[AlgorithmValidator]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "mds",
                "type": "market_data_source",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "sub",
                "type": "sub",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [
            {
                "source": {"id": "mds", "handle": "c"},
                "target": {"id": "sub", "handle": "*0"}
            }
        ],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);
  auto result = ValidateUIData(data);

  // Sub requires exactly 2 inputs
  ExpectValidationError(result, ValidationCode::InvalidNodeConnection,
                        "input connections, expected");
}

TEST_CASE("AlgorithmValidator: Edge Data Type Mismatch",
          "[AlgorithmValidator]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "bool_node",
                "type": "gt",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "number_node",
                "type": "add",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [
            {
                "source": {"id": "bool_node", "handle": "result"},
                "target": {"id": "number_node", "handle": "*0"}
            }
        ],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);
  auto result = ValidateUIData(data);

  // Boolean output cannot connect to number input
  ExpectValidationError(result, ValidationCode::InvalidEdge, "type");
}

TEST_CASE("AlgorithmValidator: All Validation Phases Coverage",
          "[AlgorithmValidator]") {
  // This test ensures all validation phases are executed
  const std::string json = R"({
        "nodes": [
            {
                "id": "",
                "type": "unknown_type",
                "options": [
                    {"id": "unknown_opt", "value": 42},
                    {"id": "exposed_opt", "value": 10, "name": "", "isExposed": true}
                ],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "node1",
                "type": "sma",
                "options": [],
                "metadata": {},
                "timeframe": {"type": "hour", "interval": 1}
            },
            {
                "id": "node1",
                "type": "rsi",
                "options": [],
                "metadata": {},
                "timeframe": {"type": "day", "interval": 1}
            }
        ],
        "edges": [
            {
                "source": {"id": "unknown", "handle": ""},
                "target": {"id": "node1", "handle": ""}
            },
            {
                "source": {"id": "node1", "handle": "output"},
                "target": {"id": "node1", "handle": "input"}
            }
        ],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);
  auto result = ValidateUIData(data);

  REQUIRE_FALSE(result.has_value());

  const auto &issues = result.error();

  // Check we have multiple types of errors
  std::unordered_set<ValidationCode> foundCodes;
  for (const auto &issue : issues) {
    foundCodes.insert(issue.code);
  }

  // We should have caught multiple types of errors
  REQUIRE(foundCodes.size() > 3);
}

// Note: ValidationCache is an internal implementation detail and not exposed in
// the public API

TEST_CASE("AlgorithmValidator: Multiple Connections to Same Handle",
          "[AlgorithmValidator]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "mds1",
                "type": "market_data_source",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "mds2",
                "type": "market_data_source",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "sma",
                "type": "sma",
                "options": [{"id": "period", "value": 20}],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [
            {
                "source": {"id": "mds1", "handle": "c"},
                "target": {"id": "sma", "handle": "*"}
            },
            {
                "source": {"id": "mds2", "handle": "c"},
                "target": {"id": "sma", "handle": "*"}
            },
            {
                "source": {"id": "sma", "handle": "result"},
                "target": {"id": "executor", "handle": "long"}
            }
        ],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);
  auto result = ValidateUIData(data);

  // SMA doesn't allow multiple connections to same input
  ExpectValidationError(result, ValidationCode::InvalidNodeConnection,
                        "multiple");
}

TEST_CASE("AlgorithmValidator: Numeric Type Compatibility - Source Not Numeric",
          "[AlgorithmValidator]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "mds",
                "type": "market_data_source",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "gt_node",
                "type": "gt",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "add_node",
                "type": "add",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [
            {
                "source": {"id": "mds", "handle": "c"},
                "target": {"id": "gt_node", "handle": "*0"}
            },
            {
                "source": {"id": "mds", "handle": "c"},
                "target": {"id": "gt_node", "handle": "*1"}
            },
            {
                "source": {"id": "gt_node", "handle": "result"},
                "target": {"id": "add_node", "handle": "*0"}
            },
            {
                "source": {"id": "mds", "handle": "c"},
                "target": {"id": "add_node", "handle": "*1"}
            },
            {
                "source": {"id": "add_node", "handle": "result"},
                "target": {"id": "executor", "handle": "long"}
            }
        ],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);
  auto result = ValidateUIData(data);

  // GT node outputs Boolean, but ADD node expects numeric input
  ExpectValidationError(result, ValidationCode::InvalidEdge,
                        "with type 'Boolean' but expected type");
}

TEST_CASE("AlgorithmValidator: Numeric Type Compatibility - Target Not Numeric",
          "[AlgorithmValidator]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "mds",
                "type": "market_data_source",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "sma_node",
                "type": "sma",
                "options": [{"id": "period", "value": 20}],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "and_node",
                "type": "logical_and",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [
            {
                "source": {"id": "mds", "handle": "c"},
                "target": {"id": "sma_node", "handle": "*"}
            },
            {
                "source": {"id": "sma_node", "handle": "result"},
                "target": {"id": "and_node", "handle": "*0"}
            },
            {
                "source": {"id": "and_node", "handle": "result"},
                "target": {"id": "executor", "handle": "long"}
            }
        ],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);
  auto result = ValidateUIData(data);

  // SMA outputs Number, but AND node expects Boolean input
  ExpectValidationError(result, ValidationCode::InvalidEdge,
                        "with type 'Decimal' but expected type 'Boolean'");
}

TEST_CASE("AlgorithmValidator: Option Value Out of Range - Too Low",
          "[AlgorithmValidator]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "mds",
                "type": "market_data_source",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "sma",
                "type": "sma",
                                 "options": [{"id": "period", "value": 0}],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [
            {
                "source": {"id": "mds", "handle": "c"},
                "target": {"id": "sma", "handle": "*"}
            },
            {
                "source": {"id": "sma", "handle": "result"},
                "target": {"id": "executor", "handle": "long"}
            }
        ],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);
  auto result = ValidateUIData(data);

  // SMA period should be >= 1 (now that we fixed the metadata)
  ExpectValidationError(result, ValidationCode::OptionValueOutOfRange,
                        "value 0 is out of range");
}

TEST_CASE("AlgorithmValidator: Option Value Out of Range - Too High",
          "[AlgorithmValidator]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "mds",
                "type": "market_data_source",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "sma",
                "type": "sma",
                                 "options": [{"id": "period", "value": 15000}],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [
            {
                "source": {"id": "mds", "handle": "c"},
                "target": {"id": "sma", "handle": "*"}
            },
            {
                "source": {"id": "sma", "handle": "result"},
                "target": {"id": "executor", "handle": "long"}
            }
        ],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);
  auto result = ValidateUIData(data);

  // SMA period should be <= 10000 (max=10000)
  ExpectValidationError(result, ValidationCode::OptionValueOutOfRange,
                        "value 15000 is out of range");
}

TEST_CASE("AlgorithmValidator: Option Value In Valid Range",
          "[AlgorithmValidator]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "mds",
                "type": "market_data_source",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "sma1",
                "type": "sma",
                "options": [{"id": "period", "value": 50}],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "sma2",
                "type": "sma",
                "options": [{"id": "period", "value": 20}],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "gt",
                "type": "gt",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [
            {
                "source": {"id": "mds", "handle": "c"},
                "target": {"id": "sma1", "handle": "*"}
            },
            {
                "source": {"id": "mds", "handle": "c"},
                "target": {"id": "sma2", "handle": "*"}
            },
            {
                "source": {"id": "sma1", "handle": "result"},
                "target": {"id": "gt", "handle": "*0"}
            },
            {
                "source": {"id": "sma2", "handle": "result"},
                "target": {"id": "gt", "handle": "*1"}
            },
            {
                "source": {"id": "gt", "handle": "result"},
                "target": {"id": "executor", "handle": "long"}
            }
        ],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);
  auto result = ValidateUIData(data);

  // This should be valid - both SMA periods (50, 20) are within valid range [1,
  // 10000] and the graph has proper type connections (Decimal -> Decimal ->
  // Boolean)
  REQUIRE(result.has_value());
}

TEST_CASE("AlgorithmValidator: Multiple Option Validation Issues",
          "[AlgorithmValidator]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "mds",
                "type": "market_data_source",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "sma",
                "type": "sma",
                "options": [
                    {"id": "period", "value": -5},
                    {"id": "unknown_option", "value": 42},
                    {"id": "exposed_option", "value": 10, "name": "", "isExposed": true}
                ],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [
            {
                "source": {"id": "mds", "handle": "c"},
                "target": {"id": "sma", "handle": "*"}
            },
            {
                "source": {"id": "sma", "handle": "result"},
                "target": {"id": "executor", "handle": "long"}
            }
        ],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);
  auto result = ValidateUIData(data);

  REQUIRE_FALSE(result.has_value());

  const auto &issues = result.error();

  // Should have multiple validation errors
  std::unordered_set<ValidationCode> foundCodes;
  for (const auto &issue : issues) {
    foundCodes.insert(issue.code);
  }

  // Should find out of range, unknown option, and exposed option without name
  REQUIRE(foundCodes.contains(ValidationCode::OptionValueOutOfRange));
  REQUIRE(foundCodes.contains(ValidationCode::InvalidOptionReference));
}

TEST_CASE("AlgorithmValidator: Boolean Option Validation",
          "[AlgorithmValidator]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [
                    {"id": "closeIfIndecisive", "value": "not_a_boolean"}
                ],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);
  auto result = ValidateUIData(data);

  // Boolean option with string value should fail type validation
  ExpectValidationError(result, ValidationCode::InvalidOptionReference,
                        "invalid type");
}

TEST_CASE("AlgorithmValidator: Decimal Option Range Validation",
          "[AlgorithmValidator]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "sma11",
                "type": "sma",
                "options": [
                    {"id": "period", "value": 150000000}
                ],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [
            {
                "source": {"id": "sma11", "handle": "result"},
                "target": {"id": "executor", "handle": "long"}
            }
        ],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);
  auto result = ValidateUIData(data);

  // SMA period value is out of range (max=10000)
  ExpectValidationError(result, ValidationCode::OptionValueOutOfRange,
                        "value 1.5e+08 is out of range");
}

TEST_CASE("AlgorithmValidator: SCALAR Node Timeframe Exclusion",
          "[AlgorithmValidator]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "mds",
                "type": "market_data_source",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "sma",
                "type": "sma",
                "options": [{"id": "period", "value": 20}],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "bool_scalar",
                "type": "bool_true",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "gt_node",
                "type": "gt",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [
            {
                "source": {"id": "mds", "handle": "c"},
                "target": {"id": "sma", "handle": "*"}
            },
            {
                "source": {"id": "sma", "handle": "result"},
                "target": {"id": "gt_node", "handle": "*0"}
            },
            {
                "source": {"id": "mds", "handle": "c"},
                "target": {"id": "gt_node", "handle": "*1"}
            },
            {
                "source": {"id": "gt_node", "handle": "result"},
                "target": {"id": "executor", "handle": "long"}
            },
            {
                "source": {"id": "bool_scalar", "handle": "result"},
                "target": {"id": "executor", "handle": "short"}
            }
        ],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);
  auto result = ValidateUIData(data);

  // This should be valid - SCALAR nodes (like "number") should be excluded
  // from timeframe validation, so connecting a node with timeframe to a
  // SCALAR node with no timeframe should not cause timeframe mismatch errors
  REQUIRE(result.has_value());

  const auto &sortedNodes = result.value();
  REQUIRE(sortedNodes.size() == 5);

  // Verify topological order
  std::unordered_map<std::string, size_t> nodeOrder;
  for (size_t i = 0; i < sortedNodes.size(); ++i) {
    nodeOrder[sortedNodes[i].id] = i;
  }

  // Data source should come before SMA
  REQUIRE(nodeOrder["mds"] < nodeOrder["sma"]);
  // SMA should come before gt_node
  REQUIRE(nodeOrder["sma"] < nodeOrder["gt_node"]);
  // gt_node should come before executor
  REQUIRE(nodeOrder["gt_node"] < nodeOrder["executor"]);
  // bool_scalar should come before executor (it's a SCALAR node)
  REQUIRE(nodeOrder["bool_scalar"] < nodeOrder["executor"]);
}

TEST_CASE("AlgorithmValidator: All Nodes No Timeframe - Valid",
          "[AlgorithmValidator]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "mds",
                "type": "market_data_source",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "sma1",
                "type": "sma",
                "options": [{"id": "period", "value": 20}],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "sma2", 
                "type": "sma",
                "options": [{"id": "period", "value": 50}],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "gt",
                "type": "gt",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [
            {
                "source": {"id": "mds", "handle": "c"},
                "target": {"id": "sma1", "handle": "*"}
            },
            {
                "source": {"id": "mds", "handle": "c"},
                "target": {"id": "sma2", "handle": "*"}
            },
            {
                "source": {"id": "sma1", "handle": "result"},
                "target": {"id": "gt", "handle": "*0"}
            },
            {
                "source": {"id": "sma2", "handle": "result"},
                "target": {"id": "gt", "handle": "*1"}
            },
            {
                "source": {"id": "gt", "handle": "result"},
                "target": {"id": "executor", "handle": "long"}
            }
        ],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);
  auto result = ValidateUIData(data);

  // This should be valid - all nodes have no timeframes
  REQUIRE(result.has_value());

  const auto &sortedNodes = result.value();
  REQUIRE(sortedNodes.size() == 5);
}

TEST_CASE("AlgorithmValidator: Market Data Source With Timeframe Is Valid",
          "[AlgorithmValidator]") {
    const std::string json = R"({
        "nodes": [
            {
                "id": "mds",
                "type": "market_data_source",
                "options": [],
                "metadata": {},
                "timeframe": {"type": "hour", "interval": 1}
            },
            {
                "id": "sma",
                "type": "sma",
                "options": [{"id": "period", "value": 20}],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "gt",
                "type": "gt",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "zero",
                "type": "zero",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [
            {
                "source": {"id": "mds", "handle": "c"},
                "target": {"id": "sma", "handle": "*"}
            },
            {
                "source": {"id": "sma", "handle": "result"},
                "target": {"id": "gt", "handle": "*0"}
            },
            {
                "source": {"id": "zero", "handle": "result"},
                "target": {"id": "gt", "handle": "*1"}
            },
            {
                "source": {"id": "gt", "handle": "result"},
                "target": {"id": "executor", "handle": "long"}
            }
        ],
        "groups": [],
        "annotations": []
    })";

    auto data = ParseUIData(json);
    auto result = ValidateUIData(data);
    auto errorStr = result.has_value() ? "" : FormatValidationIssues(result.error());
    INFO(errorStr);

    // DataSource nodes can now have timeframes directly, so this should be valid
    REQUIRE(result.has_value());

    const auto &sortedNodes = result.value();
    REQUIRE(sortedNodes.size() == 5);

    auto node = sortedNodes[1]; // mds
    REQUIRE(node.timeframe.has_value());
    // Timeframe should be 1 hour
    REQUIRE(node.timeframe->ToString() == "1H");
}

TEST_CASE("AlgorithmValidator: Node With Timeframe But RequiresTimeFrame False",
          "[AlgorithmValidator]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "mds",
                "type": "market_data_source",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "sma",
                "type": "sma",
                "options": [{"id": "period", "value": 20}],
                "metadata": {},
                "timeframe": {"type": "hour", "interval": 1}
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [
            {
                "source": {"id": "mds", "handle": "c"},
                "target": {"id": "sma", "handle": "*"}
            },
            {
                "source": {"id": "sma", "handle": "result"},
                "target": {"id": "executor", "handle": "long"}
            }
        ],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);
  auto result = ValidateUIData(data);

  // Should fail because trade_signal_executor has requiresTimeFrame=false but
  // timeframe is set
  ExpectValidationError(result, ValidationCode::TimeframeMismatch,
                        "has timeframe set but requiresTimeFrame is false");
}

// ============================================================================
// OPTIMIZATION TESTS
// ============================================================================

TEST_CASE("AlgorithmOptimizer: Remove Orphan Nodes",
          "[AlgorithmOptimization]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "orphan1",
                "type": "sma",
                "options": [{"id": "period", "value": 20}],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "orphan2",
                "type": "rsi",
                "options": [{"id": "period", "value": 14}],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "mds",
                "type": "market_data_source",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "connected_sma",
                "type": "sma",
                "options": [{"id": "period", "value": 50}],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [
            {
                "source": {"id": "mds", "handle": "c"},
                "target": {"id": "connected_sma", "handle": "*"}
            },
            {
                "source": {"id": "connected_sma", "handle": "result"},
                "target": {"id": "executor", "handle": "long"}
            }
        ],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);

  // Original data should have 5 nodes
  REQUIRE(data.nodes.size() == 5);

  // Optimize to remove orphans
  auto optimized = OptimizeUIData(data);

  // Should only have 3 nodes left (mds, connected_sma, executor)
  REQUIRE(optimized.nodes.size() == 3);

  // Check that orphan nodes were removed
  for (const auto &node : optimized.nodes) {
    REQUIRE(node.id != "orphan1");
    REQUIRE(node.id != "orphan2");
  }

  // Check that connected nodes remain
  bool hasMds = false, hasConnectedSma = false, hasExecutor = false;
  for (const auto &node : optimized.nodes) {
    if (node.id == "mds")
      hasMds = true;
    if (node.id == "connected_sma")
      hasConnectedSma = true;
    if (node.id == "executor")
      hasExecutor = true;
  }
  REQUIRE(hasMds);
  REQUIRE(hasConnectedSma);
  REQUIRE(hasExecutor);
}

TEST_CASE("AlgorithmOptimizer: Remove Stuck Bool Nodes From Executor",
          "[AlgorithmOptimization]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "bool_true",
                "type": "bool_true",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "bool_false",
                "type": "bool_false",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "mds",
                "type": "market_data_source",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "gt",
                "type": "gt",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [
            {
                "source": {"id": "mds", "handle": "c"},
                "target": {"id": "gt", "handle": "*0"}
            },
            {
                "source": {"id": "mds", "handle": "h"},
                "target": {"id": "gt", "handle": "*1"}
            },
            {
                "source": {"id": "gt", "handle": "result"},
                "target": {"id": "executor", "handle": "long"}
            },
            {
                "source": {"id": "bool_true", "handle": "result"},
                "target": {"id": "executor", "handle": "allow"}
            },
            {
                "source": {"id": "bool_false", "handle": "result"},
                "target": {"id": "executor", "handle": "allow"}
            }
        ],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);

  // Original should have 2 edges to "allow" handle
  int allowConnections = 0;
  for (const auto &edge : data.edges) {
    if (edge.target.id == "executor" && edge.target.handle == "allow") {
      allowConnections++;
    }
  }
  REQUIRE(allowConnections == 2);

  // Optimize to remove stuck bool connections
  auto optimized = OptimizeUIData(data);

  // Should have no edges to "allow" handle from bool_true/bool_false
  int optimizedAllowConnections = 0;
  for (const auto &edge : optimized.edges) {
    if (edge.target.id == "executor" && edge.target.handle == "allow") {
      REQUIRE(edge.source.id != "bool_true");
      REQUIRE(edge.source.id != "bool_false");
      optimizedAllowConnections++;
    }
  }
  REQUIRE(optimizedAllowConnections == 0);
}

TEST_CASE("AlgorithmOptimizer: Preserve Valid Options",
          "[AlgorithmOptimization]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "mds",
                "type": "market_data_source",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "sma_with_period",
                "type": "sma",
                "options": [{"id": "period", "value": 20}],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [
            {
                "source": {"id": "mds", "handle": "c"},
                "target": {"id": "sma_with_period", "handle": "*"}
            },
            {
                "source": {"id": "sma_with_period", "handle": "result"},
                "target": {"id": "executor", "handle": "long"}
            }
        ],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);

  // SMA node should have period option initially
  auto smaNode =
      std::find_if(data.nodes.begin(), data.nodes.end(),
                   [](const UINode &n) { return n.id == "sma_with_period"; });
  REQUIRE(smaNode != data.nodes.end());
  REQUIRE(smaNode->options.size() == 1);

  // Optimize (should preserve valid options)
  auto optimized = OptimizeUIData(data);

  // SMA node should still have period option with same value
  auto optimizedSmaNode =
      std::find_if(optimized.nodes.begin(), optimized.nodes.end(),
                   [](const UINode &n) { return n.id == "sma_with_period"; });
  REQUIRE(optimizedSmaNode != optimized.nodes.end());

  bool foundPeriodOption = false;
  for (const auto &option : optimizedSmaNode->options) {
    if (option.id == "period") {
      foundPeriodOption = true;
      REQUIRE(option.value.has_value());
      REQUIRE(std::get<double>(option.value.value()) == 20.0);
      break;
    }
  }
  REQUIRE(foundPeriodOption);
}

TEST_CASE("AlgorithmOptimizer: Clamp Option Values",
          "[AlgorithmOptimization]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "mds",
                "type": "market_data_source",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "sma_out_of_range",
                "type": "sma",
                "options": [{"id": "period", "value": -5}],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [
            {
                "source": {"id": "mds", "handle": "c"},
                "target": {"id": "sma_out_of_range", "handle": "*"}
            },
            {
                "source": {"id": "sma_out_of_range", "handle": "result"},
                "target": {"id": "executor", "handle": "long"}
            }
        ],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);

  // Original period should be -5 (out of range)
  auto smaNode =
      std::find_if(data.nodes.begin(), data.nodes.end(),
                   [](const UINode &n) { return n.id == "sma_out_of_range"; });
  REQUIRE(smaNode != data.nodes.end());

  auto periodOption =
      std::find_if(smaNode->options.begin(), smaNode->options.end(),
                   [](const UIOption &opt) { return opt.id == "period"; });
  REQUIRE(periodOption != smaNode->options.end());
  REQUIRE(periodOption->value.has_value());
  REQUIRE(std::get<double>(periodOption->value.value()) == -5.0);

  // Optimize to clamp values
  auto optimized = OptimizeUIData(data);

  // Period should be clamped to minimum value (1)
  auto optimizedSmaNode =
      std::find_if(optimized.nodes.begin(), optimized.nodes.end(),
                   [](const UINode &n) { return n.id == "sma_out_of_range"; });
  REQUIRE(optimizedSmaNode != optimized.nodes.end());

  auto optimizedPeriodOption = std::find_if(
      optimizedSmaNode->options.begin(), optimizedSmaNode->options.end(),
      [](const UIOption &opt) { return opt.id == "period"; });
  REQUIRE(optimizedPeriodOption != optimizedSmaNode->options.end());
  REQUIRE(optimizedPeriodOption->value.has_value());
  REQUIRE(std::get<double>(optimizedPeriodOption->value.value()) >=
          1.0); // Should be clamped to min
}

TEST_CASE("AlgorithmOptimizer: Remove Unnecessary Timeframes",
          "[AlgorithmOptimization]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "number_with_timeframe",
                "type": "number",
                "options": [{"id": "value", "value": 42.0}],
                "metadata": {},
                "timeframe": {"type": "hour", "interval": 1}
            },
            {
                "id": "mds",
                "type": "market_data_source",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [
            {
                "source": {"id": "number_with_timeframe", "handle": "result"},
                "target": {"id": "executor", "handle": "long"}
            }
        ],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);

  // Number node should have timeframe initially
  auto numberNode =
      std::find_if(data.nodes.begin(), data.nodes.end(), [](const UINode &n) {
        return n.id == "number_with_timeframe";
      });
  REQUIRE(numberNode != data.nodes.end());
  REQUIRE(numberNode->timeframe.has_value());

  // Optimize to remove unnecessary timeframes
  auto optimized = OptimizeUIData(data);

  // Number node should no longer have timeframe (scalar nodes don't require
  // timeframes)
  auto optimizedNumberNode = std::find_if(
      optimized.nodes.begin(), optimized.nodes.end(),
      [](const UINode &n) { return n.id == "number_with_timeframe"; });
  REQUIRE(optimizedNumberNode != optimized.nodes.end());
  REQUIRE_FALSE(optimizedNumberNode->timeframe.has_value());
}

TEST_CASE("AlgorithmOptimizer: Full Optimization Pipeline",
          "[AlgorithmOptimization]") {
  const std::string json = R"({
        "nodes": [
            {
                "id": "orphan",
                "type": "sma",
                "options": [{"id": "period", "value": 20}],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "bool_true",
                "type": "bool_true",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "mds",
                "type": "market_data_source",
                "options": [],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "sma_incomplete",
                "type": "sma",
                "options": [{"id": "period", "value": -10}],
                "metadata": {},
                "timeframe": null
            },
            {
                "id": "number_with_timeframe",
                "type": "number",
                "options": [{"id": "value", "value": 5.0}],
                "metadata": {},
                "timeframe": {"type": "hour", "interval": 1}
            },
            {
                "id": "executor",
                "type": "trade_signal_executor",
                "options": [],
                "metadata": {},
                "timeframe": null
            }
        ],
        "edges": [
            {
                "source": {"id": "mds", "handle": "c"},
                "target": {"id": "sma_incomplete", "handle": "*"}
            },
            {
                "source": {"id": "sma_incomplete", "handle": "result"},
                "target": {"id": "executor", "handle": "long"}
            },
            {
                "source": {"id": "number_with_timeframe", "handle": "result"},
                "target": {"id": "executor", "handle": "short"}
            },
            {
                "source": {"id": "bool_true", "handle": "result"},
                "target": {"id": "executor", "handle": "allow"}
            }
        ],
        "groups": [],
        "annotations": []
    })";

  auto data = ParseUIData(json);
  REQUIRE(data.nodes.size() == 6);
  REQUIRE(data.edges.size() == 4);

  // Optimize with full pipeline
  auto optimized = OptimizeUIData(data);

  // Should remove orphan node
  REQUIRE(optimized.nodes.size() == 5); // One less than original

  // Should not have orphan node
  auto orphanFound =
      std::find_if(optimized.nodes.begin(), optimized.nodes.end(),
                   [](const UINode &n) { return n.id == "orphan"; });
  REQUIRE(orphanFound == optimized.nodes.end());

  // Should remove bool_true connection to allow handle
  bool hasBoolTrueAllowConnection = false;
  for (const auto &edge : optimized.edges) {
    if (edge.source.id == "bool_true" && edge.target.id == "executor" &&
        edge.target.handle == "allow") {
      hasBoolTrueAllowConnection = true;
    }
  }
  REQUIRE_FALSE(hasBoolTrueAllowConnection);

  // Should clamp SMA period value
  auto smaNode =
      std::find_if(optimized.nodes.begin(), optimized.nodes.end(),
                   [](const UINode &n) { return n.id == "sma_incomplete"; });
  REQUIRE(smaNode != optimized.nodes.end());

  auto periodOption =
      std::find_if(smaNode->options.begin(), smaNode->options.end(),
                   [](const UIOption &opt) { return opt.id == "period"; });
  REQUIRE(periodOption != smaNode->options.end());
  REQUIRE(periodOption->value.has_value());
  REQUIRE(std::get<double>(periodOption->value.value()) >=
          1.0); // Should be clamped

  // Should remove timeframe from number node
  auto numberNode = std::find_if(
      optimized.nodes.begin(), optimized.nodes.end(),
      [](const UINode &n) { return n.id == "number_with_timeframe"; });
  REQUIRE(numberNode != optimized.nodes.end());
  REQUIRE_FALSE(numberNode->timeframe.has_value());
}
