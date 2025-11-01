//
// Created by adesola on 10/31/25.
//

#pragma once
#include "epoch_frame/array.h"
#include "epoch_frame/factory/dataframe_factory.h"
#include <epochflow/transforms/core/itransform.h>
#include "string_enums.h"
#include <arrow/compute/api.h>

namespace epochflow::transform {

// ============================================================================
// String Case Transformations
// ============================================================================
class StringCaseTransform : public ITransform {
public:
  explicit StringCaseTransform(const TransformConfiguration &config);

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override;

private:
  epoch_core::StringCaseOp m_operation;
};

// ============================================================================
// String Trimming
// ============================================================================
class StringTrimTransform : public ITransform {
public:
  explicit StringTrimTransform(const TransformConfiguration &config);

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override;

private:
  epoch_core::StringTrimOp m_operation;
  std::string m_trim_chars; // Empty string = trim whitespace
};

// ============================================================================
// String Padding
// ============================================================================
class StringPadTransform : public ITransform {
public:
  explicit StringPadTransform(const TransformConfiguration &config);

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override;

private:
  epoch_core::StringPadOp m_operation;
  int64_t m_width;
  std::string m_pad_string;
};

// ============================================================================
// String Containment Checks (returns boolean)
// ============================================================================
class StringContainsTransform : public ITransform {
public:
  explicit StringContainsTransform(const TransformConfiguration &config);

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override;

private:
  epoch_core::StringContainsOp m_operation;
  std::string m_pattern;
};

// ============================================================================
// String Character Type Checks (returns boolean)
// ============================================================================
class StringCheckTransform : public ITransform {
public:
  explicit StringCheckTransform(const TransformConfiguration &config);

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override;

private:
  epoch_core::StringCheckOp m_operation;
};

// ============================================================================
// String Replace
// ============================================================================
class StringReplaceTransform : public ITransform {
public:
  explicit StringReplaceTransform(const TransformConfiguration &config);

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override;

private:
  std::string m_pattern;
  std::string m_replacement;
};

// ============================================================================
// String Length (returns integer)
// ============================================================================
class StringLengthTransform : public ITransform {
public:
  explicit StringLengthTransform(const TransformConfiguration &config);

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override;
};

// ============================================================================
// String Reverse
// ============================================================================
class StringReverseTransform : public ITransform {
public:
  explicit StringReverseTransform(const TransformConfiguration &config);

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override;
};

} // namespace epochflow::transform
