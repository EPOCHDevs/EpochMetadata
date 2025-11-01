//
// Created by adesola on 10/31/25.
//

#include "string_operations.h"
#include "epoch_frame/series.h"
#include <arrow/compute/api.h>

namespace epoch_metadata::transform {

// ============================================================================
// String Case Transformations
// ============================================================================

StringCaseTransform::StringCaseTransform(const TransformConfiguration &config)
    : ITransform(config),
      m_operation(config.GetOptionValue("operation").GetSelectOption<epoch_core::StringCaseOp>()) {
}

epoch_frame::DataFrame StringCaseTransform::TransformData(
    epoch_frame::DataFrame const &bars) const {

  using namespace epoch_frame;

  Series input = bars[GetInputId()];
  Array result;

  switch (m_operation) {
    case epoch_core::StringCaseOp::upper:
      result = input.str().utf8_upper();
      break;
    case epoch_core::StringCaseOp::lower:
      result = input.str().utf8_lower();
      break;
    case epoch_core::StringCaseOp::capitalize:
      result = input.str().utf8_capitalize();
      break;
    case epoch_core::StringCaseOp::title:
      result = input.str().utf8_title();
      break;
    case epoch_core::StringCaseOp::swapcase:
      result = input.str().utf8_swapcase();
      break;
    case epoch_core::StringCaseOp::Null:
      AssertFromStream(false, "Invalid string case operation");
      break;
  }

  return make_dataframe(bars.index(), {result.as_chunked_array()}, {GetOutputId()});
}

// ============================================================================
// String Trimming
// ============================================================================

StringTrimTransform::StringTrimTransform(const TransformConfiguration &config)
    : ITransform(config),
      m_operation(config.GetOptionValue("operation").GetSelectOption<epoch_core::StringTrimOp>()),
      m_trim_chars(config.GetOptionValue("trim_chars").GetString()) {
}

epoch_frame::DataFrame StringTrimTransform::TransformData(
    epoch_frame::DataFrame const &bars) const {

  using namespace epoch_frame;

  Series input = bars[GetInputId()];
  Array result;

  if (!m_trim_chars.empty()) {
    // Custom trim characters
    arrow::compute::TrimOptions options(m_trim_chars);
    switch (m_operation) {
      case epoch_core::StringTrimOp::trim:
        result = input.str().utf8_trim(options);
        break;
      case epoch_core::StringTrimOp::trim_left:
        result = input.str().utf8_ltrim(options);
        break;
      case epoch_core::StringTrimOp::trim_right:
        result = input.str().utf8_rtrim(options);
        break;
      case epoch_core::StringTrimOp::Null:
        AssertFromStream(false, "Invalid trim operation");
        break;
    }
  } else {
    // Default whitespace trimming
    switch (m_operation) {
      case epoch_core::StringTrimOp::trim:
        result = input.str().utf8_trim_whitespace();
        break;
      case epoch_core::StringTrimOp::trim_left:
        result = input.str().utf8_ltrim_whitespace();
        break;
      case epoch_core::StringTrimOp::trim_right:
        result = input.str().utf8_rtrim_whitespace();
        break;
      case epoch_core::StringTrimOp::Null:
        AssertFromStream(false, "Invalid trim operation");
        break;
    }
  }

  return make_dataframe(bars.index(), {result.as_chunked_array()}, {GetOutputId()});
}

// ============================================================================
// String Padding
// ============================================================================

StringPadTransform::StringPadTransform(const TransformConfiguration &config)
    : ITransform(config),
      m_operation(config.GetOptionValue("operation").GetSelectOption<epoch_core::StringPadOp>()),
      m_width(config.GetOptionValue("width").GetInteger()),
      m_pad_string(config.GetOptionValue("pad_string").GetString()) {
  AssertFromStream(m_width >= 0, "Pad width must be non-negative");
}

epoch_frame::DataFrame StringPadTransform::TransformData(
    epoch_frame::DataFrame const &bars) const {

  using namespace epoch_frame;

  Series input = bars[GetInputId()];
  arrow::compute::PadOptions options(m_width, m_pad_string);
  Array result;

  switch (m_operation) {
    case epoch_core::StringPadOp::pad_left:
      result = input.str().utf8_lpad(options);
      break;
    case epoch_core::StringPadOp::pad_right:
      result = input.str().utf8_rpad(options);
      break;
    case epoch_core::StringPadOp::center:
      result = input.str().utf8_center(options);
      break;
    case epoch_core::StringPadOp::Null:
      AssertFromStream(false, "Invalid pad operation");
      break;
  }

  return make_dataframe(bars.index(), {result.as_chunked_array()}, {GetOutputId()});
}

// ============================================================================
// String Containment Checks
// ============================================================================

StringContainsTransform::StringContainsTransform(const TransformConfiguration &config)
    : ITransform(config),
      m_operation(config.GetOptionValue("operation").GetSelectOption<epoch_core::StringContainsOp>()),
      m_pattern(config.GetOptionValue("pattern").GetString()) {
}

epoch_frame::DataFrame StringContainsTransform::TransformData(
    epoch_frame::DataFrame const &bars) const {

  using namespace epoch_frame;

  Series input = bars[GetInputId()];
  arrow::compute::MatchSubstringOptions options(m_pattern);
  Array result;

  switch (m_operation) {
    case epoch_core::StringContainsOp::starts_with:
      result = input.str().starts_with(options);
      break;
    case epoch_core::StringContainsOp::ends_with:
      result = input.str().ends_with(options);
      break;
    case epoch_core::StringContainsOp::contains:
      result = input.str().match_substring(options);
      break;
    case epoch_core::StringContainsOp::Null:
      AssertFromStream(false, "Invalid contains operation");
      break;
  }

  return make_dataframe(bars.index(), {result.as_chunked_array()}, {GetOutputId()});
}

// ============================================================================
// String Character Type Checks
// ============================================================================

StringCheckTransform::StringCheckTransform(const TransformConfiguration &config)
    : ITransform(config),
      m_operation(config.GetOptionValue("operation").GetSelectOption<epoch_core::StringCheckOp>()) {
}

epoch_frame::DataFrame StringCheckTransform::TransformData(
    epoch_frame::DataFrame const &bars) const {

  using namespace epoch_frame;

  Series input = bars[GetInputId()];
  Array result;

  switch (m_operation) {
    case epoch_core::StringCheckOp::is_alpha:
      result = input.str().utf8_is_alpha();
      break;
    case epoch_core::StringCheckOp::is_digit:
      result = input.str().utf8_is_digit();
      break;
    case epoch_core::StringCheckOp::is_alnum:
      result = input.str().utf8_is_alnum();
      break;
    case epoch_core::StringCheckOp::is_numeric:
      result = input.str().utf8_is_numeric();
      break;
    case epoch_core::StringCheckOp::is_decimal:
      result = input.str().utf8_is_decimal();
      break;
    case epoch_core::StringCheckOp::is_upper:
      result = input.str().utf8_is_upper();
      break;
    case epoch_core::StringCheckOp::is_lower:
      result = input.str().utf8_is_lower();
      break;
    case epoch_core::StringCheckOp::is_title:
      result = input.str().utf8_is_title();
      break;
    case epoch_core::StringCheckOp::is_space:
      result = input.str().utf8_is_space();
      break;
    case epoch_core::StringCheckOp::is_printable:
      result = input.str().utf8_is_printable();
      break;
    case epoch_core::StringCheckOp::is_ascii:
      result = input.str().string_is_ascii();
      break;
    case epoch_core::StringCheckOp::Null:
      AssertFromStream(false, "Invalid check operation");
      break;
  }

  return make_dataframe(bars.index(), {result.as_chunked_array()}, {GetOutputId()});
}

// ============================================================================
// String Replace
// ============================================================================

StringReplaceTransform::StringReplaceTransform(const TransformConfiguration &config)
    : ITransform(config) {
  m_pattern = config.GetOptionValue("pattern").GetString();
  m_replacement = config.GetOptionValue("replacement").GetString();
}

epoch_frame::DataFrame StringReplaceTransform::TransformData(
    epoch_frame::DataFrame const &bars) const {

  using namespace epoch_frame;

  Series input = bars[GetInputId()];
  arrow::compute::ReplaceSubstringOptions options(m_pattern, m_replacement);
  Array result = input.str().replace_substring(options);

  return make_dataframe(bars.index(), {result.as_chunked_array()}, {GetOutputId()});
}

// ============================================================================
// String Length
// ============================================================================

StringLengthTransform::StringLengthTransform(const TransformConfiguration &config)
    : ITransform(config) {
}

epoch_frame::DataFrame StringLengthTransform::TransformData(
    epoch_frame::DataFrame const &bars) const {

  using namespace epoch_frame;

  Series input = bars[GetInputId()];
  Array result = input.str().utf8_length();

  return make_dataframe(bars.index(), {result.as_chunked_array()}, {GetOutputId()});
}

// ============================================================================
// String Reverse
// ============================================================================

StringReverseTransform::StringReverseTransform(const TransformConfiguration &config)
    : ITransform(config) {
}

epoch_frame::DataFrame StringReverseTransform::TransformData(
    epoch_frame::DataFrame const &bars) const {

  using namespace epoch_frame;

  Series input = bars[GetInputId()];
  Array result = input.str().utf8_reverse();

  return make_dataframe(bars.index(), {result.as_chunked_array()}, {GetOutputId()});
}

} // namespace epoch_metadata::transform
