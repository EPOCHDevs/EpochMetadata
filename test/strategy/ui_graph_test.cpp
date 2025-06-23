//
// Created by dewe on 2/23/25.
//
#include "epoch_metadata/strategy/ui_graph.h"
#include "epoch_metadata/strategy/validation.h"
#include <catch.hpp>
#include <glaze/glaze.hpp>

namespace {
constexpr auto MARKET_DATA_SOURCE = "market_data_source";
constexpr auto TRADE_SIGNAL_EXECUTOR = "trade_signal_executor";

// Helper function to convert ValidationIssues to string for testing
std::string ValidationIssuesToString(
    const epoch_metadata::strategy::ValidationIssues &issues) {
  return epoch_metadata::strategy::FormatValidationIssues(issues);
}

// Helper function to parse UIData from JSON string
epoch_metadata::strategy::UIData ParseUIData(const std::string &json) {
  epoch_metadata::strategy::UIData data;
  if (auto result = glz::read_json(data, json); result) {
    INFO("JSON parsing failed: " << glz::format_error(result, json));
    FAIL();
  }
  return data;
}
} // namespace

// Test 1: Basic Executor and Single Algorithm Node
TEST_CASE("CreateAlgorithmMetaData: Basic Executor and Single Algorithm Node",
          "[CreateAlgorithmMetaData]") {
  const std::string json = R"({
    "nodes": [
      {
        "id": "exec1",
        "type": "trade_signal_executor",
        "options": [
          {
            "id": "closeIfIndecisive",
            "value": false,
            "isExposed": false
          }
        ],
        "metadata": {},
        "timeframe": null
      },
      {
        "id": "algo1",
        "type": "previous_gt",
        "options": [
          {
            "id": "periods",
            "value": 1,
            "isExposed": false
          }
        ],
        "metadata": {},
        "timeframe": null
      },
      {
        "id": "data1",
        "type": "market_data_source",
        "options": [],
        "metadata": {},
        "timeframe": null
      }
    ],
    "edges": [
      {
        "source": {"id": "data1", "handle": "c"},
        "target": {"id": "algo1", "handle": "*"}
      },
      {
        "source": {"id": "algo1", "handle": "result"},
        "target": {"id": "exec1", "handle": "long"}
      }
    ],
    "groups": [],
    "annotations": []
  })";

  auto data = ParseUIData(json);

  // Call CreateAlgorithmMetaData.
  auto result = epoch_metadata::strategy::CreateAlgorithmMetaData(data);
  {
    INFO((result.has_value() ? std::string{}
                             : epoch_metadata::strategy::FormatValidationIssues(
                                   result.error())));
    REQUIRE(result.has_value());
  }
  epoch_metadata::strategy::PartialTradeSignalMetaData meta = result.value();

  // Verify executor.
  REQUIRE(meta.executor.id == "exec1");
  REQUIRE(meta.executor.type == TRADE_SIGNAL_EXECUTOR);
  REQUIRE(meta.executor.options.contains("closeIfIndecisive"));
  REQUIRE(meta.executor.options["closeIfIndecisive"].GetBoolean() == false);
  REQUIRE(meta.executor.inputs.contains("long"));
  // Executor input should reference algorithm node output.
  REQUIRE(meta.executor.inputs["long"].front() == "algo1#result");

  // Verify algorithm nodes (now includes DataSource)
  REQUIRE(meta.algorithm.size() == 2); // 1 DataSource + 1 Algorithm

  // Verify DataSource node is included
  auto dataSourceNode = std::find_if(
      meta.algorithm.begin(), meta.algorithm.end(),
      [](const auto &node) { return node.type == MARKET_DATA_SOURCE; });
  REQUIRE(dataSourceNode != meta.algorithm.end());
  REQUIRE(dataSourceNode->id == "data1");
  // DataSource nodes should have no inputs but should be included in algorithm
  // list
  REQUIRE(dataSourceNode->inputs.empty());

  // Find the actual algorithm node (skip DataSource)
  auto algoNode =
      std::find_if(meta.algorithm.begin(), meta.algorithm.end(),
                   [](const auto &node) { return node.type == "previous_gt"; });
  REQUIRE(algoNode != meta.algorithm.end());
  REQUIRE(algoNode->id == "algo1");
  REQUIRE(algoNode->type == "previous_gt");
  // DataSource input mapping: now retains full id#handle format
  REQUIRE(algoNode->inputs.contains("*"));
  REQUIRE(algoNode->inputs["*"].front() == "data1#c");
  // Non-exposed option is expected to be copied.
  REQUIRE(algoNode->options.contains("periods"));
  REQUIRE(algoNode->options["periods"].GetInteger() == 1);
}

// Test 2: Exposed Options Processing
TEST_CASE("CreateAlgorithmMetaData: Exposed Option Processing",
          "[CreateAlgorithmMetaData]") {
  const std::string json = R"({
    "nodes": [
      {
        "id": "exec2",
        "type": "trade_signal_executor",
        "options": [
          {
            "id": "closeIfIndecisive",
            "value": false,
            "isExposed": false
          }
        ],
        "metadata": {},
        "timeframe": null
      },
      {
        "id": "algo2",
        "type": "previous_gt",
        "options": [
          {
            "id": "periods",
            "value": 20,
            "name": "Periods for Previous GT",
            "isExposed": true
          }
        ],
        "metadata": {},
        "timeframe": null
      },
      {
        "id": "data2",
        "type": "market_data_source",
        "options": [],
        "metadata": {},
        "timeframe": null
      }
    ],
    "edges": [
      {
        "source": {"id": "data2", "handle": "c"},
        "target": {"id": "algo2", "handle": "*"}
      },
      {
        "source": {"id": "algo2", "handle": "result"},
        "target": {"id": "exec2", "handle": "long"}
      }
    ],
    "groups": [],
    "annotations": []
  })";

  auto data = ParseUIData(json);

  // Call CreateAlgorithmMetaData.
  auto result = epoch_metadata::strategy::CreateAlgorithmMetaData(data);
  {
    INFO((result.has_value() ? std::string{}
                             : epoch_metadata::strategy::FormatValidationIssues(
                                   result.error())));
    REQUIRE(result.has_value());
  }
  epoch_metadata::strategy::PartialTradeSignalMetaData meta = result.value();

  // Verify executor.
  REQUIRE(meta.executor.id == "exec2");
  REQUIRE(meta.executor.inputs.contains("long"));
  REQUIRE(meta.executor.inputs["long"].front() == "algo2#result");

  // Verify algorithm nodes (now includes DataSource)
  REQUIRE(meta.algorithm.size() == 2); // 1 DataSource + 1 Algorithm

  // Find the actual algorithm node (skip DataSource)
  auto algoNode =
      std::find_if(meta.algorithm.begin(), meta.algorithm.end(),
                   [](const auto &node) { return node.type == "previous_gt"; });
  REQUIRE(algoNode != meta.algorithm.end());
  REQUIRE(algoNode->id == "algo2");
  // Type is set from the target handle.
  REQUIRE(algoNode->type == "previous_gt");
  // DataSource input mapping: now retains full id#handle format
  REQUIRE(algoNode->inputs.contains("*"));
  REQUIRE(algoNode->inputs["*"].front() == "data2#c");

  // Exposed option should now be a reference (starting with a dot).
  REQUIRE(algoNode->options.find("periods") != algoNode->options.end());
  std::string optRef = algoNode->options["periods"].GetRef();
  REQUIRE(optRef == "algo2#periods");

  // Global metadata.options vector should include the transformed option.
  bool found = false;
  std::string expectedId = std::format("{}#{}", "algo2", "periods");
  for (const auto &mdOpt : meta.options) {
    if (mdOpt.id == expectedId) {
      found = true;
      // Use GetDouble() here since the exposed option value is stored as a
      // double.
      REQUIRE(mdOpt.defaultValue.value().GetInteger() == 20);
      REQUIRE(mdOpt.name == "Periods for Previous GT");
    }
  }
  REQUIRE(found);
}

// Test 3: Multiple Inputs Aggregation
TEST_CASE("CreateAlgorithmMetaData: Multiple Inputs Aggregation",
          "[CreateAlgorithmMetaData]") {
  const std::string json = R"({
    "nodes": [
      {
        "id": "exec3",
        "type": "trade_signal_executor",
        "options": [
          {
            "id": "closeIfIndecisive",
            "value": false,
            "isExposed": false
          }
        ],
        "metadata": {},
        "timeframe": null
      },
      {
        "id": "algo3",
        "type": "previous_gt",
        "options": [
          {
            "id": "periods",
            "value": 10,
            "isExposed": false
          }
        ],
        "metadata": {},
        "timeframe": null
      },
      {
        "id": "data3",
        "type": "market_data_source",
        "options": [],
        "metadata": {},
        "timeframe": null
      }
    ],
    "edges": [
      {
        "source": {"id": "data3", "handle": "c"},
        "target": {"id": "algo3", "handle": "*"}
      },
      {
        "source": {"id": "algo3", "handle": "result"},
        "target": {"id": "exec3", "handle": "long"}
      }
    ],
    "groups": [],
    "annotations": []
  })";

  auto data = ParseUIData(json);

  // Call CreateAlgorithmMetaData.
  auto result = epoch_metadata::strategy::CreateAlgorithmMetaData(data);
  {
    INFO((result.has_value() ? std::string{}
                             : epoch_metadata::strategy::FormatValidationIssues(
                                   result.error())));
    REQUIRE(result.has_value());
  }
  epoch_metadata::strategy::PartialTradeSignalMetaData meta = result.value();

  // Verify executor.
  REQUIRE(meta.executor.id == "exec3");
  REQUIRE(meta.executor.inputs.find("long") != meta.executor.inputs.end());
  REQUIRE(meta.executor.inputs["long"].front() == "algo3#result");

  // Verify algorithm nodes (now includes DataSource)
  REQUIRE(meta.algorithm.size() == 2); // 1 DataSource + 1 Algorithm

  // Find the actual algorithm node (skip DataSource)
  auto algoNode =
      std::find_if(meta.algorithm.begin(), meta.algorithm.end(),
                   [](const auto &node) { return node.type == "previous_gt"; });
  REQUIRE(algoNode != meta.algorithm.end());
  REQUIRE(algoNode->id == "algo3");
  // Type set from first edge creating the node.
  REQUIRE(algoNode->type == "previous_gt");

  // DataSource input mapping: now retains full id#handle format
  REQUIRE(algoNode->inputs.find("*") != algoNode->inputs.end());
  REQUIRE(algoNode->inputs["*"].front() == "data3#c");
}

// Test 4: Error Case – Exposed Option in Executor Node
TEST_CASE(
    "CreateAlgorithmMetaData: Exposed Option in Executor Node Triggers Error",
    "[CreateAlgorithmMetaData]") {
  epoch_metadata::strategy::UIData data;

  // Executor node with an exposed option (not allowed)
  epoch_metadata::strategy::UINode executor;
  executor.id = "exec4";
  executor.type = TRADE_SIGNAL_EXECUTOR;
  executor.options.push_back(epoch_metadata::strategy::UIOption{
      "closeIfIndecisive", false,
      std::make_optional(std::string("Should not be exposed")), true});
  data.nodes.push_back(executor);

  // Dummy algorithm node to complete graph.
  epoch_metadata::strategy::UINode algo;
  algo.id = "algo_dummy";
  algo.type = "previous_gt";
  algo.options.push_back(
      epoch_metadata::strategy::UIOption{"periods", 10.0, std::nullopt, false});
  data.nodes.push_back(algo);

  // PriceBar node for input
  epoch_metadata::strategy::UINode priceBar;
  priceBar.id = "data_dummy";
  priceBar.type = MARKET_DATA_SOURCE;
  data.nodes.push_back(priceBar);

  // Edge from PriceBar to algo
  epoch_metadata::strategy::UIVertex dsVertex{"data_dummy", "c"};
  epoch_metadata::strategy::UIVertex algoVertex{"algo_dummy", "*"};
  data.edges.push_back({dsVertex, algoVertex});

  // Edge from algo to executor.
  epoch_metadata::strategy::UIVertex algoOut{"algo_dummy", "result"};
  epoch_metadata::strategy::UIVertex execV{"exec4", "long"};
  data.edges.push_back({algoOut, execV});

  // Expect exception due to exposed option on executor.
  auto result = epoch_metadata::strategy::CreateAlgorithmMetaData(data);
  REQUIRE_FALSE(result.has_value());
  REQUIRE_THAT(epoch_metadata::strategy::FormatValidationIssues(result.error()),
               Catch::Matchers::ContainsSubstring(
                   "TradeSignalExecutor options cannot be exposed"));
}

// Test 5: Error Case – Missing Name for Exposed Option
TEST_CASE(
    "CreateAlgorithmMetaData: Missing Name for Exposed Option Triggers Error",
    "[CreateAlgorithmMetaData]") {
  epoch_metadata::strategy::UIData data;

  // Executor node.
  epoch_metadata::strategy::UINode executor;
  executor.id = "exec5";
  executor.type = TRADE_SIGNAL_EXECUTOR;
  executor.options.push_back(epoch_metadata::strategy::UIOption{
      "closeIfIndecisive", false, std::nullopt, false});
  data.nodes.push_back(executor);

  // Algorithm node with an exposed option missing a name.
  epoch_metadata::strategy::UINode algo;
  algo.id = "algo5";
  algo.type = "previous_gt";
  algo.options.push_back(
      epoch_metadata::strategy::UIOption{"periods", 30.0, std::nullopt, true});
  data.nodes.push_back(algo);

  // PriceBar node.
  epoch_metadata::strategy::UINode PriceBar;
  PriceBar.id = "data5";
  PriceBar.type = MARKET_DATA_SOURCE;
  data.nodes.push_back(PriceBar);

  // Edges:
  epoch_metadata::strategy::UIVertex dsV{"data5", "c"};
  epoch_metadata::strategy::UIVertex algoV{"algo5", "*"};
  data.edges.push_back({dsV, algoV});
  epoch_metadata::strategy::UIVertex algoOut{"algo5", "result"};
  epoch_metadata::strategy::UIVertex execV{"exec5", "long"};
  data.edges.push_back({algoOut, execV});

  // Expect exception due to missing name for exposed option.
  auto result = epoch_metadata::strategy::CreateAlgorithmMetaData(data);
  REQUIRE_FALSE(result.has_value());
  REQUIRE_THAT(epoch_metadata::strategy::FormatValidationIssues(result.error()),
               Catch::Matchers::ContainsSubstring("empty display name"));
}

// Test 6: Topological Sorting of Algorithm Nodes
TEST_CASE("CreateAlgorithmMetaData: Topological Sorting of Algorithm Nodes",
          "[CreateAlgorithmMetaData]") {
  epoch_metadata::strategy::UIData data;

  // Executor node.
  epoch_metadata::strategy::UINode executor;
  executor.id = "exec6";
  executor.type = TRADE_SIGNAL_EXECUTOR;
  executor.options.push_back(epoch_metadata::strategy::UIOption{
      "closeIfIndecisive", false, std::nullopt, false});
  data.nodes.push_back(executor);

  // Algorithm nodes with dependency: algo6 -> algo7.
  epoch_metadata::strategy::UINode algo6;
  algo6.id = "algo6";
  algo6.type = "sma";
  algo6.options.push_back(
      epoch_metadata::strategy::UIOption{"period", 10.0, std::nullopt, false});
  data.nodes.push_back(algo6);

  epoch_metadata::strategy::UINode algo7;
  algo7.id = "algo7";
  algo7.type = "previous_gt";
  algo7.options.push_back(
      epoch_metadata::strategy::UIOption{"periods", 15.0, std::nullopt, false});
  data.nodes.push_back(algo7);

  // Create a dependency edge: algo6 outputs to algo7.
  epoch_metadata::strategy::UIVertex out6{"algo6", "result"};
  epoch_metadata::strategy::UIVertex in7{"algo7", "*"};
  data.edges.push_back({out6, in7});
  // Also, add an edge from a PriceBar to algo6 using valid handle "c".
  epoch_metadata::strategy::UINode PriceBar;
  PriceBar.id = "data6";
  PriceBar.type = MARKET_DATA_SOURCE;
  data.nodes.push_back(PriceBar);
  epoch_metadata::strategy::UIVertex dsV{"data6", "c"};
  epoch_metadata::strategy::UIVertex in6{"algo6", "*"};
  data.edges.push_back({dsV, in6});

  // Edge from algo7 to executor.
  epoch_metadata::strategy::UIVertex out7{"algo7", "result"};
  epoch_metadata::strategy::UIVertex execV{"exec6", "long"};
  data.edges.push_back({out7, execV});

  // Call CreateAlgorithmMetaData.
  auto result = epoch_metadata::strategy::CreateAlgorithmMetaData(data);
  {
    INFO((result.has_value() ? std::string{}
                             : epoch_metadata::strategy::FormatValidationIssues(
                                   result.error())));
    REQUIRE(result.has_value());
  }
  epoch_metadata::strategy::PartialTradeSignalMetaData meta = result.value();

  // Verify topological order: algo6 must appear before algo7.
  REQUIRE(meta.algorithm.size() == 3); // 1 DataSource + 2 Algorithms

  REQUIRE(meta.algorithm[0].id == "data6");
  REQUIRE(meta.algorithm[1].id == "algo6");
  REQUIRE(meta.algorithm[2].id == "algo7");
}

// Test 7: Cyclic Dependency Graph Detection
TEST_CASE("CreateAlgorithmMetaData: Cyclic Dependency Detection",
          "[CreateAlgorithmMetaData]") {
  epoch_metadata::strategy::UIData data;

  // Executor node
  epoch_metadata::strategy::UINode executor;
  executor.id = "exec7";
  executor.type = TRADE_SIGNAL_EXECUTOR;
  executor.options.push_back(epoch_metadata::strategy::UIOption{
      "closeIfIndecisive", false, std::nullopt, false});
  data.nodes.push_back(executor);

  // Create two algorithm nodes to form a cycle: algo1 -> algo2 -> algo1
  epoch_metadata::strategy::UINode algo1;
  algo1.id = "algo1";
  algo1.type = "previous_gt";
  algo1.options.push_back(
      epoch_metadata::strategy::UIOption{"periods", 10.0, std::nullopt, false});
  data.nodes.push_back(algo1);

  epoch_metadata::strategy::UINode algo2;
  algo2.id = "algo2";
  algo2.type = "previous_gt";
  algo2.options.push_back(
      epoch_metadata::strategy::UIOption{"periods", 20.0, std::nullopt, false});
  data.nodes.push_back(algo2);

  // PriceBar node for initial input
  epoch_metadata::strategy::UINode priceBar;
  priceBar.id = "data7";
  priceBar.type = MARKET_DATA_SOURCE;
  data.nodes.push_back(priceBar);

  // Create edges to form a cycle
  // PriceBar -> algo1 (initial input)
  epoch_metadata::strategy::UIVertex dsVertex{"data7", "c"};
  epoch_metadata::strategy::UIVertex algo1Vertex{"algo1", "*"};
  data.edges.push_back({dsVertex, algo1Vertex});

  // algo1 -> algo2
  epoch_metadata::strategy::UIVertex out1{"algo1", "result"};
  epoch_metadata::strategy::UIVertex in2{"algo2", "*"};
  data.edges.push_back({out1, in2});

  // algo2 -> algo1 (completing the cycle)
  // This creates a cycle since algo1 already has input from PriceBar
  // but we're trying to add another input from algo2
  epoch_metadata::strategy::UIVertex out2{"algo2", "result"};
  epoch_metadata::strategy::UIVertex in1{"algo1", "*"};
  data.edges.push_back({out2, in1});

  // Also connect algo2 to executor so the graph is complete
  epoch_metadata::strategy::UIVertex out2ToExec{"algo2", "result"};
  epoch_metadata::strategy::UIVertex execVertex{"exec7", "long"};
  data.edges.push_back({out2ToExec, execVertex});

  // Call CreateAlgorithmMetaData - should fail due to the cycle
  auto result = epoch_metadata::strategy::CreateAlgorithmMetaData(data);

  // Verify that the function detected the cycle and returned an error
  REQUIRE_FALSE(result.has_value());
  REQUIRE_THAT(ValidationIssuesToString(result.error()),
               Catch::Matchers::ContainsSubstring("Cycle detected"));
}

// Test 8: Unknown Node Type Detection
TEST_CASE("CreateAlgorithmMetaData: Unknown Node Type Detection",
          "[CreateAlgorithmMetaData]") {
  epoch_metadata::strategy::UIData data;

  // Executor node
  epoch_metadata::strategy::UINode executor;
  executor.id = "exec8";
  executor.type = TRADE_SIGNAL_EXECUTOR;
  executor.options.push_back(epoch_metadata::strategy::UIOption{
      "closeIfIndecisive", false, std::nullopt, false});
  data.nodes.push_back(executor);

  // Algorithm node with unknown type
  epoch_metadata::strategy::UINode algo;
  algo.id = "algo_unknown";
  algo.type =
      "non_existent_indicator_type"; // This type doesn't exist in the registry
  algo.options.push_back(
      epoch_metadata::strategy::UIOption{"periods", 10.0, std::nullopt, false});
  data.nodes.push_back(algo);

  // PriceBar node
  epoch_metadata::strategy::UINode priceBar;
  priceBar.id = "data8";
  priceBar.type = MARKET_DATA_SOURCE;
  data.nodes.push_back(priceBar);

  // Create edges
  // PriceBar -> algo
  epoch_metadata::strategy::UIVertex dsVertex{"data8", "c"};
  epoch_metadata::strategy::UIVertex algoVertex{"algo_unknown", "*"};
  data.edges.push_back({dsVertex, algoVertex});

  // algo -> executor
  epoch_metadata::strategy::UIVertex algoOut{"algo_unknown", "result"};
  epoch_metadata::strategy::UIVertex execVertex{"exec8", "long"};
  data.edges.push_back({algoOut, execVertex});

  // Call CreateAlgorithmMetaData - should fail due to unknown node type
  auto result = epoch_metadata::strategy::CreateAlgorithmMetaData(data);

  // Verify that the function detected the unknown type and returned an error
  REQUIRE_FALSE(result.has_value());
  REQUIRE_THAT(ValidationIssuesToString(result.error()),
               Catch::Matchers::ContainsSubstring("Unknown node type"));
}

// Test 9: Invalid Edge Detection
TEST_CASE("CreateAlgorithmMetaData: Invalid Edge Detection",
          "[CreateAlgorithmMetaData]") {
  epoch_metadata::strategy::UIData data;

  // Executor node
  epoch_metadata::strategy::UINode executor;
  executor.id = "exec9";
  executor.type = TRADE_SIGNAL_EXECUTOR;
  executor.options.push_back(epoch_metadata::strategy::UIOption{
      "closeIfIndecisive", false, std::nullopt, false});
  data.nodes.push_back(executor);

  // Algorithm node
  epoch_metadata::strategy::UINode algo;
  algo.id = "algo9";
  algo.type = "previous_gt";
  algo.options.push_back(
      epoch_metadata::strategy::UIOption{"periods", 10.0, std::nullopt, false});
  data.nodes.push_back(algo);

  // Create an edge that references a non-existent source node
  epoch_metadata::strategy::UIVertex nonExistentSource{"non_existent_node",
                                                       "result"};
  epoch_metadata::strategy::UIVertex algoVertex{"algo9", "*"};
  data.edges.push_back({nonExistentSource, algoVertex});

  // Create a valid edge from algo to executor
  epoch_metadata::strategy::UIVertex algoOut{"algo9", "result"};
  epoch_metadata::strategy::UIVertex execVertex{"exec9", "long"};
  data.edges.push_back({algoOut, execVertex});

  // Call CreateAlgorithmMetaData - should fail due to the invalid edge
  auto result = epoch_metadata::strategy::CreateAlgorithmMetaData(data);

  // Verify that the function detected the invalid edge and returned an error
  REQUIRE_FALSE(result.has_value());
  REQUIRE_THAT(ValidationIssuesToString(result.error()),
               Catch::Matchers::ContainsSubstring("unknown source node"));
}

// Test 10: Multiple Executors Detection
TEST_CASE("CreateAlgorithmMetaData: Multiple Executors Detection",
          "[CreateAlgorithmMetaData]") {
  epoch_metadata::strategy::UIData data;

  // First executor node
  epoch_metadata::strategy::UINode executor1;
  executor1.id = "exec10_1";
  executor1.type = TRADE_SIGNAL_EXECUTOR;
  executor1.options.push_back(epoch_metadata::strategy::UIOption{
      "closeIfIndecisive", false, std::nullopt, false});
  data.nodes.push_back(executor1);

  // Second executor node
  epoch_metadata::strategy::UINode executor2;
  executor2.id = "exec10_2";
  executor2.type = TRADE_SIGNAL_EXECUTOR;
  executor2.options.push_back(epoch_metadata::strategy::UIOption{
      "closeIfIndecisive", true, std::nullopt, false});
  data.nodes.push_back(executor2);

  // Algorithm node
  epoch_metadata::strategy::UINode algo;
  algo.id = "algo10";
  algo.type = "previous_gt";
  algo.options.push_back(
      epoch_metadata::strategy::UIOption{"periods", 10.0, std::nullopt, false});
  data.nodes.push_back(algo);

  // PriceBar node
  epoch_metadata::strategy::UINode priceBar;
  priceBar.id = "data10";
  priceBar.type = MARKET_DATA_SOURCE;
  data.nodes.push_back(priceBar);

  // Create edges
  // PriceBar -> algo
  epoch_metadata::strategy::UIVertex dsVertex{"data10", "c"};
  epoch_metadata::strategy::UIVertex algoVertex{"algo10", "*"};
  data.edges.push_back({dsVertex, algoVertex});

  // algo -> executor1
  epoch_metadata::strategy::UIVertex algoOut1{"algo10", "result"};
  epoch_metadata::strategy::UIVertex exec1Vertex{"exec10_1", "long"};
  data.edges.push_back({algoOut1, exec1Vertex});

  // algo -> executor2
  epoch_metadata::strategy::UIVertex algoOut2{"algo10", "result"};
  epoch_metadata::strategy::UIVertex exec2Vertex{"exec10_2", "short"};
  data.edges.push_back({algoOut2, exec2Vertex});

  // Call CreateAlgorithmMetaData - should fail due to multiple executors
  auto result = epoch_metadata::strategy::CreateAlgorithmMetaData(data);

  // Verify that the function detected multiple executors and returned an error
  REQUIRE_FALSE(result.has_value());
  REQUIRE_THAT(
      ValidationIssuesToString(result.error()),
      Catch::Matchers::ContainsSubstring("Found 2 TradeSignalExecutors"));
}