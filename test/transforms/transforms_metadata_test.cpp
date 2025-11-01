//
// Created by Rhythm on 12/8/2024.
//

#include "epochflow/transforms/metadata.h"
#include <catch.hpp>

TEST_CASE("TransformsMetaData::ToJson generates correct JSON",
          "[TransformsMetaData]") {
  using namespace epochflow;

  MetaDataOption sampleArg{
      .id = "arg_id",
      .name = "arg_name",
      .type = epoch_core::MetaDataOptionType::Decimal,
      .defaultValue = epochflow::MetaDataOptionDefinition{10.5},
      .isRequired = true,
      .selectOption = {{"label1", "value1"}, {"label2", "value2"}}};

  auto sampleArgStr = glz::write_json(sampleArg).value();

  transforms::TransformsMetaData testTransformsMetaData{
      .id = "test_id",
      .category = epoch_core::TransformCategory::Trend,
      .plotKind = epoch_core::TransformPlotKind::h_line,
      .name = "test_name",
      .options{sampleArg},
      .isCrossSectional = true,
      .desc = "Test description",
      .inputs = {transforms::IOMetaData{.type = epoch_core::IODataType::Decimal,
                                        .id = "input_id",
                                        .name = "input"}},
      .outputs = {transforms::IOMetaData{.type = epoch_core::IODataType::String,
                                         .id = "output_id",
                                         .name = "output"}},
      .atLeastOneInputRequired = true,
      .tags = {},
      .requiresTimeFrame = false,
      .requiredDataSources = {"c"}};
  auto inputsStr = glz::write_json(testTransformsMetaData.inputs).value();
  auto outputsStr = glz::write_json(testTransformsMetaData.outputs).value();

  REQUIRE(
      glz::write_json(testTransformsMetaData).value() ==
      std::format(
          R"({{"id":"test_id","category":"Trend","plotKind":"h_line","name":"test_name","options":[{}],"isCrossSectional":true,"desc":"Test description","inputs":{},"outputs":{},"atLeastOneInputRequired":{},"tags":[],"requiresTimeFrame":false,"requiredDataSources":["c"],"intradayOnly":false,"allowNullInputs":false,"strategyTypes":[],"relatedTransforms":[],"assetRequirements":[],"usageContext":"","limitations":""}})",
          sampleArgStr, inputsStr, outputsStr,
          testTransformsMetaData.atLeastOneInputRequired));
}

TEST_CASE("IOMetaDataConstants: List and Struct types are available",
          "[IOMetaDataConstants]") {
  using namespace epochflow::transforms;

  SECTION("List metadata constants exist") {
    REQUIRE(IOMetaDataConstants::LIST_INPUT_METADATA.type ==
            epoch_core::IODataType::List);
    REQUIRE(IOMetaDataConstants::LIST_OUTPUT_METADATA.type ==
            epoch_core::IODataType::List);
    REQUIRE(IOMetaDataConstants::LIST_INPUT_METADATA.id == "SLOT");
    REQUIRE(IOMetaDataConstants::LIST_OUTPUT_METADATA.id == "result");
  }

  SECTION("Struct metadata constants exist") {
    REQUIRE(IOMetaDataConstants::STRUCT_INPUT_METADATA.type ==
            epoch_core::IODataType::Struct);
    REQUIRE(IOMetaDataConstants::STRUCT_OUTPUT_METADATA.type ==
            epoch_core::IODataType::Struct);
    REQUIRE(IOMetaDataConstants::STRUCT_INPUT_METADATA.id == "SLOT");
    REQUIRE(IOMetaDataConstants::STRUCT_OUTPUT_METADATA.id == "result");
  }

  SECTION("List and Struct are in the MAP") {
    REQUIRE(IOMetaDataConstants::MAP.contains("LIST"));
    REQUIRE(IOMetaDataConstants::MAP.contains("LIST_RESULT"));
    REQUIRE(IOMetaDataConstants::MAP.contains("STRUCT"));
    REQUIRE(IOMetaDataConstants::MAP.contains("STRUCT_RESULT"));

    REQUIRE(IOMetaDataConstants::MAP.at("LIST").type ==
            epoch_core::IODataType::List);
    REQUIRE(IOMetaDataConstants::MAP.at("LIST_RESULT").type ==
            epoch_core::IODataType::List);
    REQUIRE(IOMetaDataConstants::MAP.at("STRUCT").type ==
            epoch_core::IODataType::Struct);
    REQUIRE(IOMetaDataConstants::MAP.at("STRUCT_RESULT").type ==
            epoch_core::IODataType::Struct);
  }
}