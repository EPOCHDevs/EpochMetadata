//
// Created by Rhythm on 12/8/2024.
//

#include "../metadata.h"
#include <catch.hpp>


TEST_CASE("TransformsMetaData::ToJson generates correct JSON", "[TransformsMetaData]") {
    using namespace metadata;

    MetaDataOption sampleArg{
            .id="arg_id",
            .name="arg_name",
            .type=MetaDataOptionType::Decimal,
            .defaultValue=10.5,
            .isRequired=true,
            .selectOption={{"label1", "value1"}, {"label2", "value2"}}
    };

    auto sampleArgStr = glz::write_json(sampleArg).value();

    transforms::TransformsMetaData testTransformsMetaData{
            .id="test_id",
            .name="test_name",
            .options{sampleArg},
            .type=TransformType::Overlay,
            .isCrossSectional=true,
            .desc="Test description",
            .inputs={
                    transforms::IOMetaData{.type=IODataType::Decimal, .id="input_id", .name="input"}},
            .outputs={
                    transforms::IOMetaData{.type=IODataType::String, .id="output_id", .name="output"}}
    };
    auto inputsStr = glz::write_json(testTransformsMetaData.inputs).value();
    auto outputsStr = glz::write_json(testTransformsMetaData.outputs).value();

    REQUIRE(glz::write_json(testTransformsMetaData).value() ==
            fmt::format(
                    R"({{"id":"test_id","name":"test_name","options":[{}],"type":"Overlay","isCrossSectional":true,"desc":"Test description","inputs":{},"outputs":{},"atLeastOneInputRequired":{}}})",
                    sampleArgStr, inputsStr, outputsStr, testTransformsMetaData.atLeastOneInputRequired));
}