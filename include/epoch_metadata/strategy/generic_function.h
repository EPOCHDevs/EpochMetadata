//
// Created by adesola on 1/27/25.
//

#pragma once
#include <epoch_metadata/metadata_options.h>
#include <epoch_metadata/strategy/metadata.h>
#include <glaze/glaze.hpp>

CREATE_ENUM(GenericFunctionAuthor, User, Epoch);

namespace epoch_metadata::strategy {
    struct GenericFunction {
        std::string type;
        epoch_metadata::MetaDataArgDefinitionMapping args{};
        std::optional<epoch_metadata::TimeFrame> timeframe{};
        std::optional<std::vector<struct GenericFunction>> children{std::nullopt};
        std::optional<std::vector<AlgorithmNode>> algorithm{
            std::nullopt};
        std::optional<epoch_metadata::strategy::AlgorithmNode> executor{std::nullopt};
        epoch_core::GenericFunctionAuthor author{epoch_core::GenericFunctionAuthor::Epoch};

        bool operator==(const GenericFunction & other) const {
            return (type == other.type) &&
                (args == other.args) &&
                    (timeframe == other.timeframe) &&
                        (children == other.children) &&
                            (algorithm == other.algorithm) &&
                                (executor == other.executor) &&
                                    (author == other.author);
        }
    };

    template <typename T> struct TemplatedGenericFunction {
        T type;
        MetaDataArgDefinitionMapping args;
    };
    bool EqualsOptionalGenericFunction(std::optional<GenericFunction> const& lhs, std::optional<GenericFunction> const& rhs);
} // namespace epoch_stratifyx