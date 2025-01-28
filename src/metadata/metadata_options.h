//
// Created by dewe on 9/12/24.
//

#pragma once
#include <epoch_lab_shared/enum_wrapper.h>
#include <unordered_set>
#include <variant>
#include <yaml-cpp/yaml.h>
#include <glaze/glaze.hpp>


CREATE_ENUM(MetaDataOptionType, Integer, Decimal, Boolean, Select);

namespace metadata {
    struct MetaDataArgRef {
        std::string refName{};
    };

    class MetaDataOptionDefinition {
    public:
        using T = std::variant<double, int64_t, bool, std::string, MetaDataArgRef>;

        MetaDataOptionDefinition()=default;

        template<typename K> requires std::is_constructible_v<T, K>
        MetaDataOptionDefinition(K &&value) : m_optionsVariant(std::forward<K>(value)) {}

        [[nodiscard]] auto GetVariant() const {
            return m_optionsVariant;
        }

        template<class K>
        [[nodiscard]] bool IsType() const {
            return std::holds_alternative<K>(m_optionsVariant);
        }

        [[nodiscard]] double GetNumericValue() const;

        [[nodiscard]] auto GetDecimal() const {
            return GetValueByType<double>();
        }

        [[nodiscard]] auto GetInteger() const {
            if (IsType<int64_t>()) { return GetValueByType<int64_t>(); }
            return static_cast<int64_t>(GetValueByType<double>());
        }

        [[nodiscard]] auto GetBoolean() const {
            return GetValueByType<bool>();
        }

        std::string GetRef() const {
            return GetValueByType<MetaDataArgRef>().refName;
        }

        template<class T>
        T GetSelectOption() const {
            return EnumWrapper<T>::type::FromString(GetValueByType<std::string>());
        }

        std::string GetSelectOption() const {
            return GetValueByType<std::string>();
        }

        size_t GetHash() const;

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
        T m_optionsVariant{};

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

    using MetaDataArgDefinitionMapping = std::unordered_map<std::string, MetaDataOptionDefinition>;

    struct SelectOption {
        std::string name{};
        std::string value{};

        void decode(YAML::Node const &node) {
            name = node["name"].as<std::string>();
            value = node["value"].as<std::string>();
        }
    };

    struct MetaDataOption {
        std::string id;
        std::string name;
        MetaDataOptionType type;
        std::optional<MetaDataOptionDefinition> defaultValue{std::nullopt};
        bool isRequired{true};
        std::vector<SelectOption> selectOption{};

        void decode(YAML::Node const&);

        YAML::Node encode() const{
            return {};
        }
    };

    using MetaDataOptionList = std::vector<MetaDataOption>;

    MetaDataOptionDefinition CreateMetaDataArgDefinition(YAML::Node const &, MetaDataOption const &);
}

namespace YAML {
    template<>
    struct convert<metadata::MetaDataOption> {
        static bool decode(const Node &node, metadata::MetaDataOption &t) {
            t.decode(node);
            return true;
        }
    };

    template<>
    struct convert<metadata::SelectOption> {
        static bool decode(const Node &node, metadata::SelectOption &t) {
            t.decode(node);
            return true;
        }
    };

}

namespace glz {
    template<>
    struct meta<metadata::MetaDataOptionDefinition> {
        static constexpr auto read = [](metadata::MetaDataOptionDefinition &x,
                                        const glz::json_t &input) {
            if (input.is_null()) {
                return;
            } else if (input.is_number()) {
                x = metadata::MetaDataOptionDefinition{input.get<double>()};
            } else if (input.is_boolean()) {
                x = metadata::MetaDataOptionDefinition{input.get<bool>()};
            } else if (input.is_string()) {
                x = metadata::MetaDataOptionDefinition{input.get<std::string>()};
            } else {
                throw std::runtime_error("Unknown type for MetaDataOptionDefinition");
            }
        };

        static constexpr auto write = [](
                const metadata::MetaDataOptionDefinition &x) -> auto {
            return x.GetVariant();
        };

        static constexpr auto value = glz::custom<read, write>;
    };
}