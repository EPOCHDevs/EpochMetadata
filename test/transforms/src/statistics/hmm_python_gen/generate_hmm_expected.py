#!/usr/bin/env python3
"""
Generate HMM expected outputs using hmmlearn for 2/3/4-state Gaussian HMMs.

Outputs (per N in {2,3,4}) into hmm_test_data/:
- hmm_input_{N}.csv
- hmm_expected_{N}.csv

Input columns:
- index (UTC ISO8601, e.g. 2020-01-01T00:00:00Z)
- x, y, z (three features)

Columns in expected:
- index (same as input)
- state (int)
- prob_state_0..prob_state_{N-1} (double)
- trans_to_state_0..trans_to_state_{N-1} (double)
- state_persistence (double)
"""

import os
import numpy as np
import pandas as pd

try:
    from hmmlearn.hmm import GaussianHMM
except Exception as e:
    raise SystemExit("Please install hmmlearn: pip install hmmlearn") from e


def make_series() -> pd.DataFrame:
    rng = np.random.default_rng(42)
    # Three regimes with different means/volatilities
    seg1 = rng.normal(loc=-2.0, scale=0.5, size=60)
    seg2 = rng.normal(loc=0.0, scale=0.7, size=70)
    seg3 = rng.normal(loc=3.0, scale=0.6, size=80)
    x = np.concatenate([seg1, seg2, seg3]).astype(float)

    # Create additional correlated features
    noise_y = rng.normal(0.0, 0.3, size=x.shape[0])
    noise_z = rng.normal(0.0, 0.4, size=x.shape[0])
    y = 0.7 * x + noise_y
    z = -0.3 * x + 0.5 * y + noise_z

    # UTC timestamp index
    dt_index = pd.date_range("2020-01-01", periods=len(x), freq="D", tz="UTC")
    index_col = dt_index.strftime("%Y-%m-%dT%H:%M:%SZ")

    return pd.DataFrame({
        "index": index_col,
        "x": x,
        "y": y,
        "z": z,
    })


def zscore(df: pd.DataFrame) -> pd.DataFrame:
    # preserve index column, z-score feature columns
    out = pd.DataFrame({"index": df["index"].values})
    for col in [c for c in df.columns if c != "index"]:
        s = df[col]
        mu = s.mean()
        sd = s.std(ddof=1)
        if sd > 1e-10:
            s = (s - mu) / sd
        out[col] = s.values
    return out


def generate_for_states(df_in: pd.DataFrame, states: int,
                        max_iter: int = 1000, tol: float = 1e-5,
                        compute_zscore: bool = True) -> pd.DataFrame:
    df = zscore(df_in) if compute_zscore else df_in.copy()
    features = [c for c in df.columns if c != "index"]
    X = df[features].values  # shape (T, D)

    model = GaussianHMM(n_components=states, covariance_type="full",
                        tol=tol, n_iter=max_iter, random_state=123)
    model.fit(X)

    state_seq = model.predict(X)  # shape (T,)
    posteriors = model.predict_proba(X)  # shape (T, states)
    A = model.transmat_  # shape (states, states)

    # Transition-to probabilities from current state at each time
    trans_to = np.take(A, state_seq, axis=0)  # shape (T, states)
    persistence = trans_to[np.arange(len(state_seq)), state_seq]

    # Build expected dataframe (preserve index)
    out = pd.DataFrame({"index": df["index"].values, "state": state_seq.astype(int)})
    for s in range(states):
        out[f"prob_state_{s}"] = posteriors[:, s]
    for s in range(states):
        out[f"trans_to_state_{s}"] = trans_to[:, s]
    out["state_persistence"] = persistence

    return out


def main():
    script_dir = os.path.abspath(os.path.dirname(__file__))
    out_dir = os.path.join(script_dir, "hmm_test_data")
    os.makedirs(out_dir, exist_ok=True)

    df_in = make_series()

    for states in (2, 3, 4):
        expected = generate_for_states(df_in, states)
        input_path = os.path.join(out_dir, f"hmm_input_{states}.csv")
        expected_path = os.path.join(out_dir, f"hmm_expected_{states}.csv")

        df_in.to_csv(input_path, index=False)
        expected.to_csv(expected_path, index=False)
        print(f"Wrote {input_path} and {expected_path}")


if __name__ == "__main__":
    main()


