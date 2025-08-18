
//
// Created by adesola on test creation.
//

#include "common.h"
#include "epoch_metadata/metadata_options.h"
#include <catch2/catch_all.hpp>
#include <glaze/glaze.hpp>
#include <yaml-cpp/yaml.h>

using namespace epoch_metadata;

TEST_CASE("MetaDataOptionDefinition - JSON read/write with glaze",
          "[MetaDataOptionDefinition]") {
  SECTION("Serialize and deserialize double value") {
    MetaDataOptionDefinition original(42.5);
    std::string json = glz::write_json(original).value_or("");
    REQUIRE_FALSE(json.empty());

    MetaDataOptionDefinition deserialized;
    auto result = glz::read_json(deserialized, json);
    REQUIRE_FALSE(result);
    REQUIRE(deserialized.GetDecimal() == 42.5);
    REQUIRE(deserialized == original);
  }

  SECTION("Serialize and deserialize bool value") {
    MetaDataOptionDefinition original(true);
    std::string json = glz::write_json(original).value_or("");
    REQUIRE_FALSE(json.empty());

    MetaDataOptionDefinition deserialized;
    auto result = glz::read_json(deserialized, json);
    REQUIRE_FALSE(result);
    REQUIRE(deserialized.GetBoolean() == true);
    REQUIRE(deserialized == original);
  }

  SECTION("Serialize and deserialize string value") {
    MetaDataOptionDefinition original(std::string("test_string"));
    std::string json = glz::write_json(original).value_or("");
    REQUIRE_FALSE(json.empty());

    MetaDataOptionDefinition deserialized;
    auto result = glz::read_json(deserialized, json);
    REQUIRE_FALSE(result);
    REQUIRE(deserialized.GetSelectOption() == "test_string");
    REQUIRE(deserialized == original);
  }

  SECTION("Serialize and deserialize MetaDataArgRef") {
    MetaDataArgRef ref{"test_ref"};
    MetaDataOptionDefinition original(ref);
    std::string json = glz::write_json(original).value_or("");
    REQUIRE_FALSE(json.empty());

    MetaDataOptionDefinition deserialized;
    auto result = glz::read_json(deserialized, json);
    REQUIRE_FALSE(result);
    REQUIRE(deserialized.GetRef() == "test_ref");
    REQUIRE(deserialized == original);
  }
}

TEST_CASE("MetaDataOptionDefinition - GetValueByType",
          "[MetaDataOptionDefinition]") {
  SECTION("Get double value correctly") {
    MetaDataOptionDefinition def(123.45);
    REQUIRE(def.GetDecimal() == 123.45);
    REQUIRE(def.GetInteger() == 123);
    REQUIRE(def.GetNumericValue() == 123.45);
  }

  SECTION("Get bool value correctly") {
    MetaDataOptionDefinition def(true);
    REQUIRE(def.GetBoolean() == true);
    REQUIRE(def.GetNumericValue() == 1.0);

    MetaDataOptionDefinition def_false(false);
    REQUIRE(def_false.GetBoolean() == false);
    REQUIRE(def_false.GetNumericValue() == 0.0);
  }

  SECTION("Get string value correctly") {
    MetaDataOptionDefinition def(std::string("select_value"));
    REQUIRE(def.GetSelectOption() == "select_value");
  }

  SECTION("Get MetaDataArgRef value correctly") {
    MetaDataArgRef ref{"reference_name"};
    MetaDataOptionDefinition def(ref);
    REQUIRE(def.GetRef() == "reference_name");
  }

  SECTION("GetValueByType throws on wrong type access") {
    MetaDataOptionDefinition def(42.0);

    // Should throw when trying to get bool from double
    REQUIRE_THROWS_AS(def.GetBoolean(), std::runtime_error);

    // Should throw when trying to get string from double
    REQUIRE_THROWS_AS(def.GetSelectOption(), std::runtime_error);

    // Should throw when trying to get ref from double
    REQUIRE_THROWS_AS(def.GetRef(), std::runtime_error);
  }
}

TEST_CASE("MetaDataOptionDefinition - AssertType",
          "[MetaDataOptionDefinition]") {
  SECTION("AssertType succeeds for correct Integer/Decimal types") {
    MetaDataOptionDefinition def(42.5);
    REQUIRE_NOTHROW(def.AssertType(epoch_core::MetaDataOptionType::Integer));
    REQUIRE_NOTHROW(def.AssertType(epoch_core::MetaDataOptionType::Decimal));
  }

  SECTION("AssertType succeeds for correct Boolean type") {
    MetaDataOptionDefinition def(true);
    REQUIRE_NOTHROW(def.AssertType(epoch_core::MetaDataOptionType::Boolean));
  }

  SECTION("AssertType succeeds for correct Select type with valid selection") {
    MetaDataOptionDefinition def(std::string("option1"));
    std::unordered_set<std::string> validSelections{"option1", "option2",
                                                    "option3"};
    REQUIRE_NOTHROW(def.AssertType(epoch_core::MetaDataOptionType::Select,
                                   validSelections));
  }

  SECTION("AssertType throws for incorrect types") {
    MetaDataOptionDefinition def(42.0);

    // Should throw when asserting bool type on double value
    REQUIRE_THROWS_AS(def.AssertType(epoch_core::MetaDataOptionType::Boolean),
                      std::runtime_error);

    // Should throw when asserting select type on double value
    std::unordered_set<std::string> selections{"option1"};
    REQUIRE_THROWS_AS(
        def.AssertType(epoch_core::MetaDataOptionType::Select, selections),
        std::runtime_error);
  }

  SECTION("AssertType throws for invalid select option") {
    MetaDataOptionDefinition def(std::string("invalid_option"));
    std::unordered_set<std::string> validSelections{"option1", "option2"};
    REQUIRE_THROWS_AS(
        def.AssertType(epoch_core::MetaDataOptionType::Select, validSelections),
        std::runtime_error);
  }

  SECTION("AssertType throws for Null type") {
    MetaDataOptionDefinition def(42.0);
    REQUIRE_THROWS_AS(def.AssertType(epoch_core::MetaDataOptionType::Null),
                      std::runtime_error);
  }
}

TEST_CASE("MetaDataOptionDefinition - GetNumericValue",
          "[MetaDataOptionDefinition]") {
  SECTION("GetNumericValue returns double for double type") {
    MetaDataOptionDefinition def(123.45);
    REQUIRE(def.GetNumericValue() == 123.45);
  }

  SECTION("GetNumericValue returns 1.0 for true bool") {
    MetaDataOptionDefinition def(true);
    REQUIRE(def.GetNumericValue() == 1.0);
  }

  SECTION("GetNumericValue returns 0.0 for false bool") {
    MetaDataOptionDefinition def(false);
    REQUIRE(def.GetNumericValue() == 0.0);
  }

  SECTION("GetNumericValue throws for string type") {
    MetaDataOptionDefinition def(std::string("not_numeric"));
    REQUIRE_THROWS_AS(def.GetNumericValue(), std::runtime_error);
  }

  SECTION("GetNumericValue throws for MetaDataArgRef type") {
    MetaDataArgRef ref{"reference"};
    MetaDataOptionDefinition def(ref);
    REQUIRE_THROWS_AS(def.GetNumericValue(), std::runtime_error);
  }
}

TEST_CASE("MetaDataOptionDefinition - GetHash", "[MetaDataOptionDefinition]") {
  SECTION("GetHash returns consistent values for same content") {
    MetaDataOptionDefinition def1(42.5);
    MetaDataOptionDefinition def2(42.5);
    REQUIRE(def1.GetHash() == def2.GetHash());

    MetaDataOptionDefinition def3(true);
    MetaDataOptionDefinition def4(true);
    REQUIRE(def3.GetHash() == def4.GetHash());

    MetaDataOptionDefinition def5(std::string("test"));
    MetaDataOptionDefinition def6(std::string("test"));
    REQUIRE(def5.GetHash() == def6.GetHash());
  }

  SECTION("GetHash returns different values for different content") {
    MetaDataOptionDefinition def1(42.5);
    MetaDataOptionDefinition def2(43.5);
    REQUIRE(def1.GetHash() != def2.GetHash());

    MetaDataOptionDefinition def3(true);
    MetaDataOptionDefinition def4(false);
    REQUIRE(def3.GetHash() != def4.GetHash());

    MetaDataOptionDefinition def5(std::string("test1"));
    MetaDataOptionDefinition def6(std::string("test2"));
    REQUIRE(def5.GetHash() != def6.GetHash());
  }

  SECTION("GetHash works with MetaDataArgRef") {
    MetaDataArgRef ref1{"ref1"};
    MetaDataArgRef ref2{"ref2"};
    MetaDataOptionDefinition def1(ref1);
    MetaDataOptionDefinition def2(ref2);
    MetaDataOptionDefinition def3(ref1);

    REQUIRE(def1.GetHash() != def2.GetHash());
    REQUIRE(def1.GetHash() == def3.GetHash());
  }
}

TEST_CASE("MetaDataOptionDefinition - String override parsing",
          "[MetaDataOptionDefinition]") {
  SECTION("Parses boolean strings case-insensitively") {
    MetaDataOptionDefinition d1(std::string("true"));
    REQUIRE(d1.IsType<bool>());
    REQUIRE(d1.GetBoolean() == true);

    MetaDataOptionDefinition d2(std::string("FALSE"));
    REQUIRE(d2.IsType<bool>());
    REQUIRE(d2.GetBoolean() == false);

    MetaDataOptionDefinition d3(std::string("TrUe"));
    REQUIRE(d3.IsType<bool>());
    REQUIRE(d3.GetBoolean() == true);
  }

  SECTION("Trims whitespace and parses booleans") {
    MetaDataOptionDefinition d1(std::string("  false   "));
    REQUIRE(d1.IsType<bool>());
    REQUIRE(d1.GetBoolean() == false);

    MetaDataOptionDefinition d2(std::string("  true\t"));
    REQUIRE(d2.IsType<bool>());
    REQUIRE(d2.GetBoolean() == true);
  }

  SECTION("Parses numeric strings to double") {
    MetaDataOptionDefinition i1(std::string("42"));
    REQUIRE(i1.IsType<double>());
    REQUIRE(i1.GetDecimal() == Catch::Approx(42.0));

    MetaDataOptionDefinition n1(std::string("-3.5"));
    REQUIRE(n1.IsType<double>());
    REQUIRE(n1.GetDecimal() == Catch::Approx(-3.5));

    MetaDataOptionDefinition e1(std::string("1e3"));
    REQUIRE(e1.IsType<double>());
    REQUIRE(e1.GetDecimal() == Catch::Approx(1000.0));

    MetaDataOptionDefinition p1(std::string("+7.25"));
    REQUIRE(p1.IsType<double>());
    REQUIRE(p1.GetDecimal() == Catch::Approx(7.25));
  }

  SECTION("Trims whitespace and parses numerics") {
    MetaDataOptionDefinition w1(std::string("   10  "));
    REQUIRE(w1.IsType<double>());
    REQUIRE(w1.GetDecimal() == Catch::Approx(10.0));
  }

  SECTION("Parses string-like inputs (const char* and string_view)") {
    // const char*
    MetaDataOptionDefinition c1("false");
    REQUIRE(c1.IsType<bool>());
    REQUIRE(c1.GetBoolean() == false);

    // string_view
    std::string_view sv{"1e2"};
    MetaDataOptionDefinition c2(sv);
    REQUIRE(c2.IsType<double>());
    REQUIRE(c2.GetDecimal() == Catch::Approx(100.0));

    // mixed: leading/trailing whitespace via string_view
    std::string_view svw{"  +3.25  "};
    MetaDataOptionDefinition c3(svw);
    REQUIRE(c3.IsType<double>());
    REQUIRE(c3.GetDecimal() == Catch::Approx(3.25));
  }

  SECTION("Leaves non-parsable strings as string") {
    MetaDataOptionDefinition s1(std::string("abc"));
    REQUIRE(s1.IsType<std::string>());
    REQUIRE(s1.GetSelectOption() == "abc");

    MetaDataOptionDefinition s2(std::string("trueish"));
    REQUIRE(s2.IsType<std::string>());
    REQUIRE(s2.GetSelectOption() == "trueish");

    MetaDataOptionDefinition s3(std::string("10.5.3"));
    REQUIRE(s3.IsType<std::string>());
    REQUIRE(s3.GetSelectOption() == "10.5.3");

    MetaDataOptionDefinition s4(std::string("1e2x"));
    REQUIRE(s4.IsType<std::string>());
    REQUIRE(s4.GetSelectOption() == "1e2x");
  }
}

TEST_CASE("CreateMetaDataArgDefinition - Error paths",
          "[MetaDataOptionDefinition]") {
  SECTION("Throws on non-scalar YAML node") {
    YAML::Node nonScalarNode;
    nonScalarNode["key"] = "value"; // Make it a map, not scalar

    MetaDataOption option;
    option.id = "test_option";
    option.type = epoch_core::MetaDataOptionType::Integer;

    REQUIRE_THROWS_AS(CreateMetaDataArgDefinition(nonScalarNode, option),
                      std::runtime_error);
  }

  SECTION("Throws on invalid MetaDataOptionType") {
    YAML::Node scalarNode(42);

    MetaDataOption option;
    option.id = "test_option";
    option.type = epoch_core::MetaDataOptionType::Null;

    REQUIRE_THROWS_AS(CreateMetaDataArgDefinition(scalarNode, option),
                      std::runtime_error);
  }

  SECTION("Successfully creates Integer definition") {
    YAML::Node scalarNode(42);

    MetaDataOption option;
    option.id = "test_option";
    option.type = epoch_core::MetaDataOptionType::Integer;

    auto result = CreateMetaDataArgDefinition(scalarNode, option);
    REQUIRE(result.GetInteger() == 42);
    REQUIRE(result.IsType(epoch_core::MetaDataOptionType::Integer));
  }

  SECTION("Successfully creates Decimal definition") {
    YAML::Node scalarNode(42.5);

    MetaDataOption option;
    option.id = "test_option";
    option.type = epoch_core::MetaDataOptionType::Decimal;

    auto result = CreateMetaDataArgDefinition(scalarNode, option);
    REQUIRE(result.GetDecimal() == 42.5);
    REQUIRE(result.IsType(epoch_core::MetaDataOptionType::Decimal));
  }

  SECTION("Successfully creates Boolean definition") {
    YAML::Node scalarNode(true);

    MetaDataOption option;
    option.id = "test_option";
    option.type = epoch_core::MetaDataOptionType::Boolean;

    auto result = CreateMetaDataArgDefinition(scalarNode, option);
    REQUIRE(result.GetBoolean() == true);
    REQUIRE(result.IsType(epoch_core::MetaDataOptionType::Boolean));
  }

  SECTION("Successfully creates Select definition") {
    YAML::Node scalarNode("option1");

    MetaDataOption option;
    option.id = "test_option";
    option.type = epoch_core::MetaDataOptionType::Select;

    auto result = CreateMetaDataArgDefinition(scalarNode, option);
    REQUIRE(result.GetSelectOption() == "option1");
    REQUIRE(result.IsType(epoch_core::MetaDataOptionType::Select));
  }
}

TEST_CASE("MetaDataOptionDefinition - IsType method",
          "[MetaDataOptionDefinition]") {
  SECTION("IsType correctly identifies double types for Integer and Decimal") {
    MetaDataOptionDefinition def(42.5);
    REQUIRE(def.IsType(epoch_core::MetaDataOptionType::Integer));
    REQUIRE(def.IsType(epoch_core::MetaDataOptionType::Decimal));
    REQUIRE_FALSE(def.IsType(epoch_core::MetaDataOptionType::Boolean));
    REQUIRE_FALSE(def.IsType(epoch_core::MetaDataOptionType::Select));
    REQUIRE_FALSE(def.IsType(epoch_core::MetaDataOptionType::Null));
  }

  SECTION("IsType correctly identifies bool type") {
    MetaDataOptionDefinition def(true);
    REQUIRE(def.IsType(epoch_core::MetaDataOptionType::Boolean));
    REQUIRE_FALSE(def.IsType(epoch_core::MetaDataOptionType::Integer));
    REQUIRE_FALSE(def.IsType(epoch_core::MetaDataOptionType::Decimal));
    REQUIRE_FALSE(def.IsType(epoch_core::MetaDataOptionType::Select));
    REQUIRE_FALSE(def.IsType(epoch_core::MetaDataOptionType::Null));
  }

  SECTION("IsType correctly identifies string type for Select") {
    MetaDataOptionDefinition def(std::string("option"));
    REQUIRE(def.IsType(epoch_core::MetaDataOptionType::Select));
    REQUIRE_FALSE(def.IsType(epoch_core::MetaDataOptionType::Integer));
    REQUIRE_FALSE(def.IsType(epoch_core::MetaDataOptionType::Decimal));
    REQUIRE_FALSE(def.IsType(epoch_core::MetaDataOptionType::Boolean));
    REQUIRE_FALSE(def.IsType(epoch_core::MetaDataOptionType::Null));
  }

  SECTION("IsType returns false for Null type") {
    MetaDataOptionDefinition def(42.0);
    REQUIRE_FALSE(def.IsType(epoch_core::MetaDataOptionType::Null));
  }
}

TEST_CASE("MetaDataOptionDefinition - ToString method",
          "[MetaDataOptionDefinition]") {
  SECTION("ToString works for double") {
    MetaDataOptionDefinition def(42.5);
    std::string result = def.ToString();
    REQUIRE(result == "42.500000");
  }

  SECTION("ToString works for bool") {
    MetaDataOptionDefinition def_true(true);
    MetaDataOptionDefinition def_false(false);
    REQUIRE(def_true.ToString() == "1");
    REQUIRE(def_false.ToString() == "0");
  }

  SECTION("ToString works for string") {
    MetaDataOptionDefinition def(std::string("test_string"));
    REQUIRE(def.ToString() == "test_string");
  }

  SECTION("ToString works for MetaDataArgRef") {
    MetaDataArgRef ref{"reference_name"};
    MetaDataOptionDefinition def(ref);
    REQUIRE(def.ToString() == "reference_name");
  }
}

TEST_CASE("MetaDataOptionDefinition - Template AssertType method",
          "[MetaDataOptionDefinition]") {
  SECTION("Template AssertType succeeds for correct types") {
    MetaDataOptionDefinition def_double(42.5);
    REQUIRE_NOTHROW(def_double.AssertType<double>());

    MetaDataOptionDefinition def_bool(true);
    REQUIRE_NOTHROW(def_bool.AssertType<bool>());

    MetaDataOptionDefinition def_string(std::string("test"));
    REQUIRE_NOTHROW(def_string.AssertType<std::string>());

    MetaDataArgRef ref{"test"};
    MetaDataOptionDefinition def_ref(ref);
    REQUIRE_NOTHROW(def_ref.AssertType<MetaDataArgRef>());
  }

  SECTION("Template AssertType throws for incorrect types") {
    MetaDataOptionDefinition def(42.5);

    REQUIRE_THROWS_AS(def.AssertType<bool>(), std::runtime_error);
    REQUIRE_THROWS_AS(def.AssertType<std::string>(), std::runtime_error);
    REQUIRE_THROWS_AS(def.AssertType<MetaDataArgRef>(), std::runtime_error);
  }
}

TEST_CASE("MetaDataOptionDefinition - Edge cases and comprehensive coverage",
          "[MetaDataOptionDefinition]") {
  SECTION("Default constructor creates valid object") {
    MetaDataOptionDefinition def;
    // Default should be double with value 0
    REQUIRE(def.IsType<double>());
    REQUIRE(def.GetDecimal() == 0.0);
  }

  SECTION("GetInteger handles negative values correctly") {
    MetaDataOptionDefinition def(-42.7);
    REQUIRE(def.GetInteger() == -42);
    REQUIRE(def.GetDecimal() == -42.7);
  }

  SECTION("Equality operator works correctly") {
    MetaDataOptionDefinition def1(42.5);
    MetaDataOptionDefinition def2(42.5);
    MetaDataOptionDefinition def3(43.5);

    REQUIRE(def1 == def2);
    REQUIRE_FALSE(def1 == def3);

    MetaDataArgRef ref1{"test"};
    MetaDataArgRef ref2{"test"};
    MetaDataArgRef ref3{"different"};

    MetaDataOptionDefinition def_ref1(ref1);
    MetaDataOptionDefinition def_ref2(ref2);
    MetaDataOptionDefinition def_ref3(ref3);

    REQUIRE(def_ref1 == def_ref2);
    REQUIRE_FALSE(def_ref1 == def_ref3);
  }

  SECTION("GetVariant returns correct variant") {
    MetaDataOptionDefinition def(42.5);
    auto variant = def.GetVariant();
    REQUIRE(std::holds_alternative<double>(variant));
    REQUIRE(std::get<double>(variant) == 42.5);
  }
}