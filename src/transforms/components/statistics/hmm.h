#pragma once
//
// Hidden Markov Model Transform for Financial Time Series Analysis
//
#include "dataframe_armadillo_utils.h"
#include "epoch_frame/aliases.h"
#include <epoch_script/transforms/core/itransform.h>
#include <arrow/array.h>
#include <arrow/array/builder_base.h>
#include <arrow/builder.h>
#include <arrow/status.h>
#include <arrow/table.h>
#include <arrow/tensor.h>
#include <arrow/type.h>
#include <arrow/util/macros.h>
#include <cmath>
#include <epoch_frame/factory/array_factory.h>
#include <epoch_frame/factory/dataframe_factory.h>

#include <armadillo>
#include <mlpack/core.hpp>
#include <mlpack/methods/gmm/gmm.hpp>
#include <mlpack/methods/hmm/hmm.hpp>

using arma::mat;
using arma::Row;
using arma::u64;
using arma::vec;
using mlpack::GaussianDistribution;

namespace epoch_script::transform {

// Concrete Gaussian HMM type alias (mlpack observations are column-oriented)
using HMMGaussian = mlpack::HMM<GaussianDistribution<>>;

/**
 * @brief Hidden Markov Model Transform for Financial Time Series
 *
 * This transform implements HMM-based regime detection and state prediction
 * for financial markets using Gaussian emission distributions.
 *
 * Template parameter N_STATES specifies the number of hidden states.
 * Use specialized versions: hmm_2, hmm_3, hmm_4, hmm_5
 *
 * Financial Applications:
 * - Market regime detection (bull/bear/sideways)
 * - Volatility state identification (low/medium/high)
 * - Trend change detection
 * - Risk state assessment
 */
template <size_t N_STATES>
class HMMTransform final : public ITransform {
public:
  static_assert(N_STATES >= 2 && N_STATES <= 5,
                "HMM supports 2-5 states");

  explicit HMMTransform(const TransformConfiguration &cfg) : ITransform(cfg) {
    // Core HMM parameters
    m_max_iterations = static_cast<size_t>(
        cfg.GetOptionValue("max_iterations",
                           epoch_script::MetaDataOptionDefinition{1000.0})
            .GetInteger());

    m_tolerance =
        cfg.GetOptionValue("tolerance",
                           epoch_script::MetaDataOptionDefinition{1e-5})
            .GetDecimal();

    // Data preprocessing options
    m_compute_zscore =
        cfg.GetOptionValue("compute_zscore",
                           epoch_script::MetaDataOptionDefinition{true})
            .GetBoolean();

    // Training options
    m_min_training_samples = static_cast<size_t>(
        cfg.GetOptionValue("min_training_samples",
                           epoch_script::MetaDataOptionDefinition{100.0})
            .GetInteger());

    m_lookback_window = static_cast<size_t>(
        cfg.GetOptionValue("lookback_window",
                           epoch_script::MetaDataOptionDefinition{0.0})
            .GetInteger());
  }

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(const epoch_frame::DataFrame &bars) const override {
    const auto cols = GetInputIds();
    if (cols.empty()) {
      throw std::runtime_error(
          "HMMTransform requires at least one input column.");
    }

    // Convert DataFrame to Armadillo matrix
    arma::mat X = utils::MatFromDataFrame(bars, cols);

    if (X.n_rows < m_min_training_samples) {
      throw std::runtime_error("Insufficient training samples for HMM");
    }

    // Split into training and prediction sets
    arma::mat training_data;
    arma::mat prediction_data;
    epoch_frame::IndexPtr prediction_index;

    if (m_lookback_window > 0 && X.n_rows > m_lookback_window) {
      // Train on first m_lookback_window bars
      training_data = X.rows(0, m_lookback_window - 1);

      // Predict on remaining bars (bars after training window)
      prediction_data = X.rows(m_lookback_window, X.n_rows - 1);

      // Index for prediction window
      prediction_index = bars.index()->iloc(
          {static_cast<int64_t>(m_lookback_window),
           static_cast<int64_t>(X.n_rows)});
    } else {
      // If no lookback specified, use all data for both training and prediction
      // (Research mode - acceptable look-ahead for exploratory analysis)
      training_data = X;
      prediction_data = X;
      prediction_index = bars.index();
    }

    // Compute preprocessing parameters from training data
    auto preprocess_params = ComputePreprocessParams(training_data);

    // Apply preprocessing to training data
    training_data = ApplyPreprocessParams(training_data, preprocess_params);

    // Train HMM model on preprocessed training data
    auto hmm = TrainHMM(training_data);

    // Apply SAME preprocessing parameters to prediction data
    prediction_data = ApplyPreprocessParams(prediction_data, preprocess_params);

    // Generate predictions on prediction data (not training data)
    return GenerateOutputs(prediction_index, hmm, prediction_data);
  }

  // Note: GetOutputMetaData() is handled in separate metadata repo

private:
  // Core HMM parameters
  size_t m_max_iterations{1000};
  double m_tolerance{1e-5};

  // Data preprocessing
  bool m_compute_zscore{true};

  // Training parameters
  size_t m_min_training_samples{100};
  size_t m_lookback_window{0}; // 0 = use all available data

  // Preprocessing parameters structure
  struct PreprocessParams {
    std::vector<double> means;
    std::vector<double> stds;
  };

  // Compute preprocessing parameters from training data
  PreprocessParams ComputePreprocessParams(const arma::mat &X) const {
    PreprocessParams params;
    if (!m_compute_zscore) {
      return params; // Return empty params if not using zscore
    }

    params.means.resize(X.n_cols);
    params.stds.resize(X.n_cols);

    for (size_t i = 0; i < X.n_cols; ++i) {
      params.means[i] = arma::mean(X.col(i));
      params.stds[i] = arma::stddev(X.col(i));
    }

    return params;
  }

  // Apply preprocessing parameters to data
  arma::mat ApplyPreprocessParams(arma::mat X,
                                  const PreprocessParams &params) const {
    if (!m_compute_zscore) {
      return X; // Return unchanged if not using zscore
    }

    for (size_t i = 0; i < X.n_cols; ++i) {
      if (params.stds[i] > 1e-10) {
        X.col(i) = (X.col(i) - params.means[i]) / params.stds[i];
      }
    }

    return X;
  }

  HMMGaussian TrainHMM(const arma::mat &X) const {
    // Number of dimensions (features)
    const size_t dimensionality = X.n_cols;

    // Initialize Gaussian HMM with N_STATES and given dimensionality
    HMMGaussian hmm(N_STATES, GaussianDistribution<>(dimensionality),
                    m_tolerance);

    // Prepare sequences (each matrix is dimensionality x T, observations in
    // columns)
    std::vector<arma::mat> sequences;
    sequences.emplace_back(X.t());

    // Unsupervised training (Baum-Welch)
    hmm.Train(sequences);
    return hmm;
  }

  epoch_frame::DataFrame GenerateOutputs(const epoch_frame::IndexPtr &index,
                                         const HMMGaussian &hmm,
                                         const arma::mat &X) const {
    const size_t T = X.n_rows;
    std::vector<std::string> output_columns;
    std::vector<arrow::ChunkedArrayPtr> output_arrays;

    // Most likely state sequence (Viterbi path)
    arma::Row<size_t> viterbi_path;
    hmm.Predict(X.t(), viterbi_path);

    // Forward-backward probabilities (state probabilities)
    arma::mat stateLogProb;
    arma::mat forwardLogProb;
    arma::mat backwardLogProb;
    arma::vec logScales;
    hmm.LogEstimate(X.t(), stateLogProb, forwardLogProb, backwardLogProb,
                    logScales);
    arma::mat state_probs = arma::exp(stateLogProb);

    // 1. State sequence (Viterbi path)
    std::vector<int64_t> state_vec(T);
    for (size_t i = 0; i < T; ++i) {
      state_vec[i] = static_cast<int64_t>(viterbi_path[i]);
    }
    output_columns.push_back(GetOutputId("state"));
    output_arrays.push_back(epoch_frame::factory::array::make_array(state_vec));

    // 2. Individual state probability columns (state_0_prob, state_1_prob, ...)
    // Generate N_STATES individual scalar columns at compile time
    for (size_t s = 0; s < N_STATES; ++s) {
      std::vector<double> prob_vec(T);
      for (size_t i = 0; i < T; ++i) {
        prob_vec[i] = state_probs(s, i);
      }
      output_columns.push_back(GetOutputId("state_" + std::to_string(s) + "_prob"));
      output_arrays.push_back(epoch_frame::factory::array::make_array(prob_vec));
    }

    return epoch_frame::make_dataframe(index, output_arrays, output_columns);
  }
};

// Specialized HMM transforms for 2-5 states
using HMM2Transform = HMMTransform<2>;
using HMM3Transform = HMMTransform<3>;
using HMM4Transform = HMMTransform<4>;
using HMM5Transform = HMMTransform<5>;

} // namespace epoch_script::transform
