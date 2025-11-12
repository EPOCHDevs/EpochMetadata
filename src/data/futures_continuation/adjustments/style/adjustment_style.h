#pragma once
//
// Created by dewe on 12/5/23.
//

namespace epoch_script::futures {
  struct IAdjustmentStyle {
    virtual ~IAdjustmentStyle() = default;

    virtual void ComputeAdjustmentFactor(double frontValue, double backValue) = 0;

    virtual double ApplyAdjustment(double newFront) = 0;

    virtual double ApplyCumulativeAdjustment(double newFront) = 0;

    virtual double GetAdjustmentFactor() const = 0;

    virtual double GetAccumulatedAdjFactor() const = 0;
  };

struct AdjustmentStyle :  IAdjustmentStyle {
  double GetAdjustmentFactor() const final { return m_adjustmentFactor; }

  double GetAccumulatedAdjFactor() const final {
    return m_accumulatedAdjFactor;
  }

protected:
  double m_adjustmentFactor{0};
  double m_accumulatedAdjFactor{0};
};

struct PanamaCanal : AdjustmentStyle {
  void ComputeAdjustmentFactor(double frontValue,
                               double backValue) final {
    m_adjustmentFactor = backValue - frontValue;
    m_accumulatedAdjFactor += m_adjustmentFactor;
  }

  double ApplyAdjustment(double newFront) final {
    return newFront + m_adjustmentFactor;
  }

  double ApplyCumulativeAdjustment(double newFront) final {
    return newFront + m_accumulatedAdjFactor;
  }
};

struct Ratio : AdjustmentStyle {
  Ratio() {  m_adjustmentFactor = 1;
    m_accumulatedAdjFactor = 1; }

  void ComputeAdjustmentFactor(double frontValue,
                               double backValue) final {
    m_adjustmentFactor = backValue / frontValue;
    m_accumulatedAdjFactor *= m_adjustmentFactor;
  }

  double ApplyAdjustment(double newFront) final {
    return newFront * m_adjustmentFactor;
  }

  double ApplyCumulativeAdjustment(double newFront) final {
    return newFront * m_accumulatedAdjFactor;
  }
};
} // namespace epoch_script::futures