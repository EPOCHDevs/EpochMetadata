//
// Created by dewe on 9/12/24.
//

#include "metadata_options.h"
#include "doc_deserialization_helper.h"
#include <unordered_set>
#include <tl/to.hpp>


namespace metadata {
    void MetaDataOptionDefinition::AssertType(MetaDataOptionType const &argType,
                                              std::unordered_set<std::string> const &selections) const {
        switch (argType) {
            case MetaDataOptionType::Integer:
                AssertType<int64_t>();
                break;
            case MetaDataOptionType::Decimal:
                AssertType<double>();
                break;
            case MetaDataOptionType::Boolean:
                AssertType<bool>();
                break;
            case MetaDataOptionType::Select: {
                auto option = GetValueByType<std::string>();

                AssertWithTraceFromStream(selections.contains(option),
                                          "Invalid select member: " << option << ", Expected one of "
                                                                    << epoch::toString(tl::to<std::vector>(selections)));
                break;
            }
            case MetaDataOptionType::Null:
                throw std::runtime_error("Null value not allowed.");
        }
    }

    double MetaDataOptionDefinition::GetNumericValue() const {
        if (std::holds_alternative<double>(m_optionsVariant)) {
            return GetDecimal();
        }
        if (std::holds_alternative<int64_t>(m_optionsVariant)) {
            return static_cast<double>(GetInteger());
        }
        if (std::holds_alternative<bool>(m_optionsVariant)) {
            return static_cast<double>(GetBoolean());
        }

        throw std::runtime_error("Invalid Numeric MetaDataOptionType Type");
    }

    MetaDataOptionDefinition::T CreateMetaDataArgDefinition(YAML::Node const &node, MetaDataOption const &arg) {
        AssertWithTraceFromStream(node.IsScalar(),
                                  "invalid transform option type: " << node << ", expected a scalar for " << arg.id
                                                                    << ".");
        switch (arg.type) {
            case MetaDataOptionType::Integer: {
                return node.as<int64_t>();
            }
            case MetaDataOptionType::Decimal: {
                return node.as<double>();
            }
            case MetaDataOptionType::Boolean: {
                return node.as<bool>();
            }
            case MetaDataOptionType::Select: {
                return node.as<std::string>();
            }
            case MetaDataOptionType::Null:
                break;
        }
        throw std::runtime_error("Invalid MetaDataOptionType Type");
    }

    void MetaDataOption::decode(const YAML::Node &element) {
        static std::unordered_map<std::string, MetaDataOption> PLACEHOLDER_MAP{
                {"PERIOD", {.id="period", .name="Period", .type=MetaDataOptionType::Integer}}
        };

        if (element.IsScalar()) {
            *this = epoch::lookup(PLACEHOLDER_MAP, element.as<std::string>());
            return;
        }

        id = element["id"].as<std::string>();
        name = element["name"].as<std::string>();
        type = MetaDataOptionTypeWrapper::FromString(element["type"].as<std::string>());
        values = element["values"].as<std::vector<std::string>>(std::vector<std::string>{});
        labels = element["labels"].as<std::vector<std::string>>(std::vector<std::string>{});

        if (auto defaultValueNode = element["default"]) {
            auto strValue = YAML::Dump(defaultValueNode);
            std::istringstream iss(strValue);

            int64_t integer_value;
            double decimal_value;
            char trailing;

            if ((iss >> std::noskipws >> integer_value) && !(iss >> trailing)) {
                defaultValue = integer_value;
            } else {
                // reset and clear the stream state
                iss.clear();
                iss.seekg(0, std::ios::beg);
                if ((iss >> std::noskipws >> decimal_value) && !(iss >> trailing)) {
                    defaultValue = decimal_value;
                } else {
                    defaultValue = strValue;
                }
            }
        }

        isRequired = element["required"].as<bool>(true);
    }
} // namespace metadata