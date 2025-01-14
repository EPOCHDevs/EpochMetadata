//
// Created by dewe on 9/12/24.
//

#pragma once
#include "enum_wrapper.h"
#include <unordered_set>
#include <variant>
#include <yaml-cpp/yaml.h>
#include <glaze/glaze.hpp>


CREATE_ENUM(MetaDataOptionType, Integer, Decimal, Boolean, Select);

namespace metadata {
    class MetaDataOptionDefinition {
    public:
        using T = std::variant<double, int64_t, bool, std::string>;

        explicit MetaDataOptionDefinition(auto &&value) : m_optionsVariant(std::forward<decltype(value)>(value)) {}

        auto GetVariant() const {
            return m_optionsVariant;
        }

        double GetNumericValue() const;

        auto GetDecimal() const {
            return GetValueByType<double>();
        }

        auto GetInteger() const {
            return GetValueByType<int64_t>();
        }

        auto GetBoolean() const {
            return GetValueByType<bool>();
        }

        template<class T>
        T GetSelectOption() const {
            return EnumWrapper<T>::type::FromString(GetValueByType<std::string>());
        }

        std::string GetSelectOption() const {
            return GetValueByType<std::string>();
        }

        void AssertType(MetaDataOptionType const &argType,
                        std::unordered_set<std::string> const &selections = {}) const;

        template<class T>
        void AssertType() const {
            AssertWithTraceFromStream(std::holds_alternative<T>(m_optionsVariant), "Wrong type! Expected: "
                    << typeid(T).name()
                    << ", but got: "
                    << typeid(m_optionsVariant).name());
        }

    private:
        T m_optionsVariant;

        template<class T>
        T GetValueByType() const {
            std::stringstream errorStreamer;
            try {
                return std::get<T>(m_optionsVariant);
            } catch (std::bad_variant_access const &) {
                errorStreamer << "Error: Bad variant access.\n"
                              << "Expected type: " << typeid(T).name() << std::endl;
            } catch (...) {
                errorStreamer << "Error: An unknown error occurred." << std::endl;
            }
            throw std::runtime_error(errorStreamer.str());
        }
    };

    using MetaDataArgDefinitionMapping = std::unordered_map<std::string, MetaDataOptionDefinition::T>;

    struct MetaDataOption {
        std::string id;
        std::string name;
        MetaDataOptionType type;
        std::optional<MetaDataOptionDefinition::T> defaultValue{std::nullopt};
        bool isRequired{true};
        std::vector<std::string> values{};
        std::vector<std::string> labels{};

        void decode(YAML::Node const&);
        YAML::Node encode() const{
            return {};
        }
    };

    using MetaDataOptionList = std::vector<MetaDataOption>;

    MetaDataOptionDefinition::T CreateMetaDataArgDefinition(YAML::Node const &, MetaDataOption const &);
}

namespace YAML {
    template<>
    struct convert<metadata::MetaDataOption> {
        static bool decode(const Node &node, metadata::MetaDataOption &t) {
            t.decode(node);
            return true;
        }
    };
}