//
// Created by Rhythm on 12/8/2024.
//

#include "../metadata.h"
#include <catch.hpp>


//TEST_CASE("IOMetaData::TOJSON Generates correct JSON", "[IOMetaData]")
//{
//    using namespace stratifyx::metadata::transforms;
//    rapidjson::Document document;
//    auto& allocator = document.GetAllocator();
//
//    IOMetaData testMetaData{IODataType::Decimal, "test_name"};
//    rapidjson::Value jsonValue = testMetaData.ToJson(allocator);
//
//    REQUIRE(jsonValue.IsObject());
//    REQUIRE(jsonValue.HasMember("type"));
//    REQUIRE(jsonValue["type"].IsString());
//    REQUIRE(std::string(jsonValue["type"].GetString()) == "Decimal");
//
//    REQUIRE(jsonValue.HasMember("name"));
//    REQUIRE(jsonValue["name"].IsString());
//    REQUIRE(std::string(jsonValue["name"].GetString()) == "test_name");
//}

TEST_CASE("TransformsMetaData::ToJson generates correct JSON", "[TransformsMetaData]") {
    using namespace stratifyx::metadata;

    rapidjson::Document document;
    auto &allocator = document.GetAllocator();

    MetaDataArg sampleArg{
            "arg_id",               // id
            "arg_name",             // name
            "10.5",        // defaultValue
            MetaDataArgType::Decimal, // type
            "Argument description", // desc
            {"value1", "value2"},   // values
            true,                   // isRequired
            {"label1", "label2"}    // labels
    };

    transforms::TransformsMetaData testTransformsMetaData{
            .id="test_id",                              // id
            .name="test_name",                            // name
            .options{sampleArg},                            // args
            .type=transforms::TIType::Overlay,                        // type
            .isCrossSectional=true,                                   // isCrossSectional
            .desc="Test description",                     // desc
            .inputs={
                    transforms::IOMetaData{.type=transforms::IODataType::Decimal, .id="input_id", .name="input"}},  // inputs
            .outputs={
                    transforms::IOMetaData{.type=transforms::IODataType::String, .id="output_id", .name="output"}}  // outputs
    };
    REQUIRE(testTransformsMetaData.ToJson() ==
            R"({"id":"","name":"","options":[],"type":0,"isCrossSectional":false,"desc":"","inputs":[],"outputs":[]})");
}