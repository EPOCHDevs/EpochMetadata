//
// Created by adesola on 12/16/24.
//
#include "../common.h"
#include "epoch_metadata/transforms/registration.h"
#include "epoch_metadata/transforms/registry.h"
#include <catch.hpp>
#include <fstream>
#include <glaze/glaze.hpp>

#ifdef GLZ_ENABLE_JSON_PRETTY_PRINT
constexpr bool kWriteToCsv = true;
#else
constexpr bool kWriteToCsv = false;
#endif

TEST_CASE("Transform MetaData Total Count is Correct") {
  using namespace epoch_metadata::transforms;
  RegisterTransformMetadata(epoch_metadata::DEFAULT_YAML_LOADER);

  auto metadata = ITransformRegistry::GetInstance().GetMetaData();
  REQUIRE(metadata.size() == 231);

  if constexpr (kWriteToCsv) {

    std::ofstream csvStream("transforms.csv");
    csvStream
        << "ID;Category;RenderKind;PlotKind;Name;Description;Options;Inputs;"
           "Outputs;Tags;AtLeastOneInputRequired;RequiresTimeFrame;"
           "IsCrossSectional\n";

    auto pretty_tags = [](auto const &obj) {
      std::stringstream ss;
      for (auto const &tag : obj) {
        ss << tag << ", ";
      }
      return ss.str();
    };

    auto pretty_ids = [](auto const &objs) {
      std::stringstream ss;
      for (auto const &obj : objs) {
        ss << obj.id << ", ";
      }
      return ss.str();
    };

    for (auto const &transform : metadata | std::views::values) {
      csvStream << transform.id << ";" << transform.category << ";"
                << transform.renderKind << ";" << transform.plotKind << ";"
                << transform.name << ";" << transform.desc << ";"
                << pretty_ids(transform.options) << ";"
                << pretty_ids(transform.inputs) << ";"
                << pretty_ids(transform.outputs) << ";"
                << pretty_tags(transform.tags) << ";" << std::boolalpha
                << transform.atLeastOneInputRequired << ";"
                << transform.requiresTimeFrame << ";"
                << transform.isCrossSectional << std::noboolalpha << "\n";
    }
    csvStream.close();
  }
}