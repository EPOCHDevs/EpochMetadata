#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "epoch_script/transforms/core/transform_configuration.h"

namespace epoch_script::chart_metadata {
    /**
     * @brief Manages Y-axis assignment and configuration for chart series
     *
     * This class is responsible for:
     * - Tracking which series use which axis
     * - Creating new axes for indicators
     * - Managing axis heights and positions
     */
    class AxisManager {
    public:
        struct AxisInfo {
            uint8_t index;
            uint8_t top;
            uint8_t height;
        };

        AxisManager();

        /**
         * @brief Assign an axis to a transform based on its plot kind and inputs
         * @param cfg The transform configuration
         * @param timeframe The timeframe for this chart
         * @param priceInputs Set of price-related input keys
         * @param volumeInput The volume input key
         * @param outputHandlesToSeriesId Map of output handles to series indices
         * @return Axis index and optional linkedTo series ID
         */
        std::pair<uint8_t, std::optional<std::string>> AssignAxis(
            const epoch_script::transform::TransformConfiguration &cfg,
            const std::string &timeframe,
            const std::unordered_set<std::string> &priceInputs,
            const std::string &volumeInput,
            const std::unordered_map<std::string, int64_t> &outputHandlesToSeriesId);

        /**
         * @brief Get all configured axes for a timeframe
         */
        std::vector<AxisInfo> GetAxes(const std::string &timeframe) const;

        /**
         * @brief Register a series with its assigned axis
         */
        void RegisterSeries(const std::string &timeframe, const std::string &seriesId,
                            uint8_t axisIndex);

        /**
         * @brief Get the series ID at a specific index for a timeframe
         * @param timeframe The timeframe string
         * @param index The index in the series order
         * @return Series ID or empty string if not found
         */
        std::string GetSeriesIdAtIndex(const std::string &timeframe,
                                       size_t index) const;

        /**
         * @brief Initialize base axes for a timeframe (price and volume)
         * @param timeframe The timeframe to initialize axes for
         */
        void InitializeBaseAxes(const std::string &timeframe);

    private:
        // Track axes per timeframe
        std::unordered_map<std::string, std::vector<AxisInfo>> m_axes;

        // Track series to axis mapping per timeframe
        std::unordered_map<std::string, std::unordered_map<std::string, uint8_t>>
            m_seriesAxisMap;

        // Track series order per timeframe
        std::unordered_map<std::string, std::vector<std::string>> m_seriesOrder;

        // Track indicator type to axis mapping per timeframe (for axis sharing)
        std::unordered_map<std::string, std::unordered_map<std::string, uint8_t>>
            m_indicatorTypeToAxis;

        /**
         * @brief Determine axis assignment based on transform inputs
         * @param cfg The transform configuration
         * @param timeframe The timeframe string
         * @param priceInputs Set of price input keys
         * @param volumeInput Volume input key
         * @param outputHandlesToSeriesId Mapping of output handles to series IDs
         * @param linkedTo Output parameter for linked series ID
         * @return Assigned axis index
         */
        uint8_t DetermineAxisFromInputs(
            const epoch_script::transform::TransformConfiguration &cfg,
            const std::string &timeframe,
            const std::unordered_set<std::string> &priceInputs,
            const std::string &volumeInput,
            const std::unordered_map<std::string, int64_t> &outputHandlesToSeriesId,
            std::optional<std::string> &linkedTo);

        /**
         * @brief Create a new indicator axis for the timeframe
         * @param timeframe The timeframe string
         * @return New axis index
         */
        uint8_t CreateIndicatorAxis(const std::string &timeframe);

        /**
         * @brief Find or create axis for an indicator type
         * @param timeframe The timeframe string
         * @param descriptiveName The full descriptive name (e.g., "RSI 14", "RSI 14
         * (SMA)")
         * @return Axis index
         */
        uint8_t FindOrCreateIndicatorAxis(const std::string &timeframe,
                                          const std::string &descriptiveName);

        /**
         * @brief Recalculate axis heights after adding new axes
         */
        void RecalculateAxisHeights(const std::string &timeframe);

        /**
         * @brief Determine if transform should get its own axis based on plot kind
         */
        static bool RequiresOwnAxis(      const epoch_script::transform::TransformConfiguration &cfg);

        /**
         * @brief Check if transform has price-related inputs
         */
        static bool
        HasPriceInputs(      const epoch_script::transform::TransformConfiguration &cfg,

                       const std::unordered_set<std::string> &priceInputs);

        /**
         * @brief Check if transform has volume input
         */
        static bool HasVolumeInput(      const epoch_script::transform::TransformConfiguration &cfg,
                                   const std::string &volumeInput);
    };
} // namespace epoch_script::chart_metadata