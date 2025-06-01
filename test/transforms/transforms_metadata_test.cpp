//
// Created by Rhythm on 12/8/2024.
//

#include "epoch_metadata/transforms/metadata.h"
#include <catch.hpp>

TEST_CASE("TransformsMetaData::ToJson generates correct JSON",
          "[TransformsMetaData]") {
  using namespace epoch_metadata;

  MetaDataOption sampleArg{
      .id = "arg_id",
      .name = "arg_name",
      .type = epoch_core::MetaDataOptionType::Decimal,
      .defaultValue = 10.5,
      .isRequired = true,
      .selectOption = {{"label1", "value1"}, {"label2", "value2"}}};

  auto sampleArgStr = glz::write_json(sampleArg).value();

  transforms::TransformsMetaData testTransformsMetaData{
      .id = "test_id",
      .category = epoch_core::TransformCategory::Trend,
      .renderKind = epoch_core::TransformNodeRenderKind::Simple,
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
          R"({{"id":"test_id","category":"Trend","renderKind":"Simple","plotKind":"h_line","name":"test_name","options":[{}],"isCrossSectional":true,"desc":"Test description","inputs":{},"outputs":{},"atLeastOneInputRequired":{},"tags":[],"requiresTimeFrame":false,"requiredDataSources":["c"]}})",
          sampleArgStr, inputsStr, outputsStr,
          testTransformsMetaData.atLeastOneInputRequired));
}