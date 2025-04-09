//
// Created by dewe on 9/11/24.
//

#pragma once

#include "../metadata_options.h"
#include "metadata.h"
#include "registry.h"

namespace epoch_metadata::ml_kit {
inline int RegisterMLKitMetaData(const MLKitMetaData &metaData) {
  IMLKitRegistry::GetInstance().Register(metaData);
  return 0;
}

#define REGISTER_MLKIT_METADATA(FactoryMetaData)                               \
  const int REGISTRATION_MLKIT_METADATA_##FactoryMetaData =                    \
      RegisterMLKitMetaData(FactoryMetaData)

const MLKitMetaData FRAC_DIFF_SCALAR_METADATA{
    .id = "frac_diff",
    .name = "Fractional Differentiation",
    .class_ = RegistryClass::Transformer,
    .args =
        std::vector<MetaDataArg>{
            MetaDataArg{.id = "n",
                        .name = "Order of Differentiation",
                        .type = MetaDataArgType::Decimal,
                        .defaultValue = 1.0,
                        .desc = "The order of the fractional differentiation."},
            MetaDataArg{.id = "window",
                        .name = "Window",
                        .defaultValue = "10",
                        .type = MetaDataArgType::Integer,
                        .desc = "The size of the moving window."},
            MetaDataArg{.id = "random_state",
                        .name = "Random State",
                        .defaultValue = "",
                        .type = MetaDataArgType::Integer,
                        .desc = "Seed used by the random number generator.",
                        .isRequired = false},
            MetaDataArg{.id = "mode",
                        .name = "Mode",
                        .defaultValue = "Same",
                        .type = MetaDataArgType::Select,
                        .desc = "The convolution mode to use.",
                        .values = {"Same", "Valid"},
                        .isRequired = true}},
    .desc = "Performs fractional differentiation on time series data."};
REGISTER_MLKIT_METADATA(FRAC_DIFF_SCALAR_METADATA);

const MLKitMetaData PCA_SCALAR_METADATA{
    .id = "pca",
    .name = "Principal Component Analysis Scaler",
    .class_ = RegistryClass::Transformer,
    .args =
        std::vector<MetaDataArg>{
            MetaDataArg{.id = "iterated_power",
                        .name = "Iterated Power",
                        .defaultValue = "15",
                        .type = MetaDataArgType::Integer,
                        .desc = "Number of iterations for the power method."},
            MetaDataArg{.id = "n_components",
                        .name = "Component Count",
                        .defaultValue = "",
                        .type = MetaDataArgType::Integer,
                        .desc = "Number of components to keep.",
                        .isRequired = false},
            MetaDataArg{.id = "random_state",
                        .name = "Random State",
                        .defaultValue = "",
                        .type = MetaDataArgType::Integer,
                        .desc = "Seed used by the random number generator.",
                        .isRequired = false},
            MetaDataArg{.id = "solver",
                        .name = "Solver",
                        .defaultValue = "cov_dc",
                        .type = MetaDataArgType::Select,
                        .desc = "Algorithm used for the computation.",
                        .values = {"Covariance Divide and Conquer Method",
                                   "Covariance Jacobi Method"},
                        .isRequired = true},
            MetaDataArg{.id = "tol",
                        .name = "Tolerance",
                        .defaultValue = "1e-7",
                        .type = MetaDataArgType::Decimal,
                        .desc =
                            "Tolerance for singular values computed by svd."},
            MetaDataArg{
                .id = "whiten",
                .name = "Whiten",
                .defaultValue = "false",
                .type = MetaDataArgType::Boolean,
                .desc =
                    "When true, the components are multiplied by the square "
                    "root of n_samples and divided by the singular values."}},
    .desc = "Applies PCA for dimensionality reduction."};
REGISTER_MLKIT_METADATA(PCA_SCALAR_METADATA);

const MLKitMetaData MINMAX_SCALAR_METADATA{
    .id = "minmax",
    .name = "Minimum/Maximum Scaler",
    .class_ = RegistryClass::Transformer,
    .args =
        std::vector<MetaDataArg>{
            MetaDataArg{.id = "min",
                        .name = "Minimum Value",
                        .defaultValue = "0",
                        .type = MetaDataArgType::Decimal,
                        .desc = "Desired minimum value after scaling."},
            MetaDataArg{.id = "max",
                        .name = "Maximum Value",
                        .defaultValue = "1",
                        .type = MetaDataArgType::Decimal,
                        .desc = "Desired maximum value after scaling."}},
    .desc = "Scales features to a specified range."};
REGISTER_MLKIT_METADATA(MINMAX_SCALAR_METADATA);

const MLKitMetaData STANDARD_SCALAR_METADATA{
    .id = "standard",
    .name = "Standard Scaler",
    .class_ = RegistryClass::Transformer,
    .args =
        std::vector<MetaDataArg>{
            MetaDataArg{.id = "with_mean",
                        .name = "Use Mean",
                        .defaultValue = "true",
                        .type = MetaDataArgType::Boolean,
                        .desc = "Center data before scaling."},
            MetaDataArg{.id = "with_scale",
                        .name = "Use Scale",
                        .defaultValue = "true",
                        .type = MetaDataArgType::Boolean,
                        .desc = "Scale data to unit variance."}},
    .desc = "Standardizes features by removing the mean and scaling to unit "
            "variance."};
REGISTER_MLKIT_METADATA(STANDARD_SCALAR_METADATA);

const std::vector<MetaDataArg> MLKIT_XGB_METADATA_ARGS{
    MetaDataArg{.id = "n_estimators",
                .name = "Number of Gradient Boosted Trees",
                .defaultValue = "100",
                .type = MetaDataArgType::Integer,
                .desc = "Number of gradient boosted trees."},
    MetaDataArg{.id = "max_depth",
                .name = "Maximum Tree Depth",
                .defaultValue = "6",
                .type = MetaDataArgType::Integer,
                .desc = "Maximum depth of a tree."},
    MetaDataArg{.id = "max_bin",
                .name = "Maximum Number of Bins",
                .defaultValue = "256",
                .type = MetaDataArgType::Integer,
                .desc = "Maximum number of bins for histogram construction."},
    MetaDataArg{
        .id = "grow_policy",
        .name = "Tree Grow Policy",
        .defaultValue = "depthwise",
        .type = MetaDataArgType::Select,
        .desc = "Controls the way new nodes are added to the tree.",
        .values = {"depthwise", "lossguide"},
        .isRequired = true,
        .labels = {"Depth-wise", "Loss-guide"},
    },
    MetaDataArg{
        .id = "learning_rate",
        .name = "Boosting Learning Rate",
        .defaultValue = "0.1",
        .type = MetaDataArgType::Decimal,
        .desc = "Step size shrinkage used in update to prevent overfitting."},
    MetaDataArg{.id = "booster",
                .name = "Booster",
                .defaultValue = "gbtree",
                .type = MetaDataArgType::Select,
                .desc = "Specify which booster to use.",
                .values = {"gbtree", "gblinear", "dart"},
                .isRequired = false},
    MetaDataArg{.id = "n_jobs",
                .name = "Number of Parallel Threads",
                .defaultValue = "1",
                .type = MetaDataArgType::Integer,
                .desc = "Number of parallel threads used to run XGBoost."},
    MetaDataArg{.id = "gamma",
                .name = "Gamma",
                .defaultValue = "0",
                .type = MetaDataArgType::Decimal,
                .desc = "Minimum loss reduction required to make a further "
                        "partition on a leaf node."},
    MetaDataArg{.id = "min_child_weight",
                .name = "Minimum Sum of Instance Weight (Hessian)",
                .defaultValue = "1",
                .type = MetaDataArgType::Decimal,
                .desc = "Minimum sum of instance weight needed in a child."},
    MetaDataArg{
        .id = "max_delta_step",
        .name = "Maximum Delta Step",
        .defaultValue = "0",
        .type = MetaDataArgType::Decimal,
        .desc =
            "Maximum delta step we allow each tree's weight estimation to be."},
    MetaDataArg{.id = "subsample",
                .name = "Subsample Ratio",
                .defaultValue = "1",
                .type = MetaDataArgType::Decimal,
                .desc = "Subsample ratio of the training instances."},
    MetaDataArg{.id = "sampling_method",
                .name = "Sampling Method",
                .defaultValue = "uniform",
                .type = MetaDataArgType::Select,
                .desc = "Sampling method to use.",
                .values = {"uniform", "gradient_based"},
                .isRequired = false},
    MetaDataArg{.id = "colsample_bytree",
                .name = "Colsample By Tree",
                .defaultValue = "1",
                .type = MetaDataArgType::Decimal,
                .desc =
                    "Subsample ratio of columns when constructing each tree."},
    MetaDataArg{
        .id = "colsample_bylevel",
        .name = "Colsample By Level",
        .defaultValue = "1",
        .type = MetaDataArgType::Decimal,
        .desc = "Subsample ratio of columns for each split, in each level."},
    MetaDataArg{.id = "colsample_bynode",
                .name = "Colsample By Node",
                .defaultValue = "1",
                .type = MetaDataArgType::Decimal,
                .desc = "Subsample ratio of columns for each node."},
    MetaDataArg{.id = "reg_alpha",
                .name = "Alpha",
                .defaultValue = "0",
                .type = MetaDataArgType::Decimal,
                .desc = "L1 regularization term on weights."},
    MetaDataArg{.id = "reg_lambda",
                .name = "Lambda",
                .defaultValue = "1",
                .type = MetaDataArgType::Decimal,
                .desc = "L2 regularization term on weights."},
    MetaDataArg{.id = "scale_pos_weight",
                .name = "Scale Positive Weight",
                .defaultValue = "1",
                .type = MetaDataArgType::Decimal,
                .desc = "Balancing of positive and negative weights."}};

const MLKitMetaData XGB_CLASSIFIER_METADATA{
    .id = "xgb_classifier",
    .name = "XGB Classifier",
    .class_ = RegistryClass::Classifier,
    .args = MLKIT_XGB_METADATA_ARGS,
    .desc = "XGBoost classifier for classification tasks."};
REGISTER_MLKIT_METADATA(XGB_CLASSIFIER_METADATA);

const MLKitMetaData XGB_RF_CLASSIFIER_METADATA{
    .id = "xgb_rf_classifier",
    .name = "XGB Random Forest Classifier",
    .class_ = RegistryClass::Classifier,
    .args = MLKIT_XGB_METADATA_ARGS,
    .desc = "XGBoost Random Forest classifier."};
REGISTER_MLKIT_METADATA(XGB_RF_CLASSIFIER_METADATA);

const MLKitMetaData GPU_XGB_CLASSIFIER_METADATA{
    .id = "xgb_classifier_gpu",
    .name = "GPU XGB Classifier",
    .class_ = RegistryClass::Classifier,
    .args = MLKIT_XGB_METADATA_ARGS,
    .desc = "XGBoost classifier with GPU acceleration."};
REGISTER_MLKIT_METADATA(GPU_XGB_CLASSIFIER_METADATA);

const MLKitMetaData GPU_XGB_RF_CLASSIFIER_METADATA{
    .id = "xgb_rf_classifier_gpu",
    .name = "GPU XGB Random Forest Classifier",
    .class_ = RegistryClass::Classifier,
    .args = MLKIT_XGB_METADATA_ARGS,
    .desc = "XGBoost Random Forest classifier with GPU acceleration."};
REGISTER_MLKIT_METADATA(GPU_XGB_RF_CLASSIFIER_METADATA);

const MLKitMetaData CUML_LINEAR_REGRESSION_METADATA{
    .id = "cuml_linear_reg",
    .name = "GPU Linear Regression",
    .class_ = RegistryClass::RegressionEstimator,
    .args =
        std::vector<MetaDataArg>{
            MetaDataArg{
                .id = "fit_intercept",
                .name = "Fit Intercept",
                .defaultValue = "true",
                .type = MetaDataArgType::Boolean,
                .desc = "Whether to calculate the intercept for this model."},
            MetaDataArg{.id = "normalize",
                        .name = "Normalize",
                        .defaultValue = "true",
                        .type = MetaDataArgType::Boolean,
                        .desc = "This parameter is ignored when fit_intercept "
                                "is set to False."},
            MetaDataArg{.id = "algo",
                        .name = "Algorithm Type",
                        .defaultValue = "svd-jacobi",
                        .type = MetaDataArgType::Select,
                        .desc = "Algorithm to use in the computation.",
                        .values = {"svd-jacobi", "eig", "qr", "svd-qr"},
                        .isRequired = true}},
    .desc = "GPU-accelerated linear regression model."};
REGISTER_MLKIT_METADATA(CUML_LINEAR_REGRESSION_METADATA);

const MLKitMetaData CUML_LOGISTIC_REGRESSION_METADATA{
    .id = "cuml_logistic_reg",
    .name = "GPU Logistic Regression",
    .class_ = RegistryClass::RegressionEstimator,
    .args =
        std::vector<MetaDataArg>{
            MetaDataArg{.id = "penalty",
                        .name = "Penalty",
                        .defaultValue = "L2",
                        .type = MetaDataArgType::Select,
                        .desc = "Norm used in the penalization.",
                        .values = {"L1", "L2", "Elastic Net"},
                        .isRequired = false},
            MetaDataArg{.id = "tol",
                        .name = "Tolerance",
                        .defaultValue = "1e-4",
                        .type = MetaDataArgType::Decimal,
                        .desc = "Tolerance for stopping criteria."},
            MetaDataArg{.id = "c",
                        .name = "Inverse of Regularization Strength",
                        .defaultValue = "1.0",
                        .type = MetaDataArgType::Decimal,
                        .desc = "Must be a positive float; smaller values "
                                "specify stronger regularization."},
            MetaDataArg{.id = "fit_intercept",
                        .name = "Fit Intercept",
                        .defaultValue = "true",
                        .type = MetaDataArgType::Boolean,
                        .desc = "Specifies if a constant should be added to "
                                "the decision function."},
            MetaDataArg{.id = "max_iter",
                        .name = "Max Number of Iterations",
                        .defaultValue = "1000",
                        .type = MetaDataArgType::Integer,
                        .desc = "Maximum number of iterations for the solver."},
            MetaDataArg{.id = "linesearch_max_iter",
                        .name = "Max Number of Line Search Iterations",
                        .defaultValue = "50",
                        .type = MetaDataArgType::Integer,
                        .desc = "Maximum number of line search iterations."},
            MetaDataArg{.id = "l1_ratio",
                        .name = "L1 Ratio",
                        .defaultValue = "0.5",
                        .type = MetaDataArgType::NormalizedDecimal,
                        .desc = "The Elastic-Net mixing parameter, with 0 <= "
                                "l1_ratio <= 1."}},
    .desc = "GPU-accelerated logistic regression model."};
REGISTER_MLKIT_METADATA(CUML_LOGISTIC_REGRESSION_METADATA);

// Loss Functions
const MLKitMetaData DLB_MSE_METADATA{
    .id = "mse_loss",
    .name = "Mean Squared Error Loss",
    .class_ = RegistryClass::DLB_LossFunction,
    .args = std::vector<MetaDataArg>{MetaDataArg{
        .id = "reduction",
        .name = "Reduction",
        .defaultValue = "mean",
        .type = MetaDataArgType::Select,
        .desc = "Specifies the reduction to apply to the output.",
        .values = {"mean", "sum"},
        .isRequired = true}},
    .desc =
        "Measures the average squared difference between inputs and targets."};
REGISTER_MLKIT_METADATA(DLB_MSE_METADATA);

const MLKitMetaData DLB_BCE_WITH_LOGITS_METADATA{
    .id = "bce_with_logits",
    .name = "Binary Cross Entropy with Logits Loss",
    .class_ = RegistryClass::DLB_LossFunction,
    .args = std::vector<MetaDataArg>{MetaDataArg{
        .id = "reduction",
        .name = "Reduction",
        .defaultValue = "mean",
        .type = MetaDataArgType::Select,
        .desc = "Specifies the reduction to apply to the output.",
        .values = {"mean", "sum"},
        .isRequired = true}},
    .desc = "Combines a Sigmoid layer and the BCELoss in one single class."};
REGISTER_MLKIT_METADATA(DLB_BCE_WITH_LOGITS_METADATA);

const MLKitMetaData DLB_CROSS_ENTROPY_METADATA{
    .id = "cross_entropy",
    .name = "Cross Entropy Loss",
    .class_ = RegistryClass::DLB_LossFunction,
    .args =
        std::vector<MetaDataArg>{
            MetaDataArg{.id = "reduction",
                        .name = "Reduction",
                        .defaultValue = "mean",
                        .type = MetaDataArgType::Select,
                        .desc =
                            "Specifies the reduction to apply to the output.",
                        .values = {"mean", "sum"},
                        .isRequired = true},
            MetaDataArg{.id = "ignore_index",
                        .name = "Ignore Index",
                        .defaultValue = "-100",
                        .type = MetaDataArgType::Integer,
                        .desc = "Specifies a target value that is ignored and "
                                "does not contribute to the input gradient.",
                        .isRequired = false},
            MetaDataArg{.id = "label_smoothing",
                        .name = "Label Smoothing",
                        .defaultValue = "0.0",
                        .type = MetaDataArgType::Decimal,
                        .desc = "Applies label smoothing."}},
    .desc = "Combines LogSoftmax and NLLLoss in one single class."};
REGISTER_MLKIT_METADATA(DLB_CROSS_ENTROPY_METADATA);

// Optimizers
const std::vector<MetaDataArg> DLB_ADAM_OPTIONS_METADATA{
    MetaDataArg{.id = "lr",
                .name = "Learning Rate",
                .defaultValue = "0.001",
                .type = MetaDataArgType::Decimal,
                .desc = "Learning rate for the optimizer."},
    // MetaDataArgs{
    //     .id = "betas",
    //     .name = "Betas",
    //     .defaultValue = "(0.9, 0.999)",
    //     .type = MetaDataArgType::Decimal,
    //     .desc = "Coefficients used for computing running averages of gradient
    //     and its square."
    // },
    MetaDataArg{
        .id = "eps",
        .name = "Epsilon",
        .defaultValue = "1e-8",
        .type = MetaDataArgType::Decimal,
        .desc =
            "Term added to the denominator to improve numerical stability."},
    MetaDataArg{.id = "weight_decay",
                .name = "Weight Decay",
                .defaultValue = "0",
                .type = MetaDataArgType::Decimal,
                .desc = "Weight decay (L2 penalty)."},
    MetaDataArg{.id = "amsgrad",
                .name = "Use AMSGrad",
                .defaultValue = "false",
                .type = MetaDataArgType::Boolean,
                .desc =
                    "Whether to use the AMSGrad variant of this algorithm."}};

const MLKitMetaData DLB_ADAM_METADATA{
    .id = "adam",
    .name = "Adam Optimizer",
    .class_ = RegistryClass::DLB_Optimizer,
    .args = DLB_ADAM_OPTIONS_METADATA,
    .desc = "Optimizer that implements the Adam algorithm."};
REGISTER_MLKIT_METADATA(DLB_ADAM_METADATA);

const MLKitMetaData DLB_ADAMW_METADATA{
    .id = "adamw",
    .name = "AdamW Optimizer",
    .class_ = RegistryClass::DLB_Optimizer,
    .args = DLB_ADAM_OPTIONS_METADATA,
    .desc = "Adam optimizer with decoupled weight decay."};
REGISTER_MLKIT_METADATA(DLB_ADAMW_METADATA);

const MLKitMetaData DLB_ADAGRAD_METADATA{
    .id = "adagrad",
    .name = "Adagrad Optimizer",
    .class_ = RegistryClass::DLB_Optimizer,
    .args =
        std::vector<MetaDataArg>{
            MetaDataArg{.id = "lr",
                        .name = "Learning Rate",
                        .defaultValue = "0.01",
                        .type = MetaDataArgType::Decimal,
                        .desc = "Learning rate for the optimizer."},
            MetaDataArg{.id = "lr_decay",
                        .name = "Learning Rate Decay",
                        .defaultValue = "0",
                        .type = MetaDataArgType::Decimal,
                        .desc = "Learning rate decay over each update."},
            MetaDataArg{.id = "weight_decay",
                        .name = "Weight Decay",
                        .defaultValue = "0",
                        .type = MetaDataArgType::Decimal,
                        .desc = "Weight decay (L2 penalty)."},
            MetaDataArg{.id = "eps",
                        .name = "Epsilon",
                        .defaultValue = "1e-10",
                        .type = MetaDataArgType::Decimal,
                        .desc = "Term added to the denominator to improve "
                                "numerical stability."}},
    .desc = "Optimizer that implements the Adagrad algorithm."};
REGISTER_MLKIT_METADATA(DLB_ADAGRAD_METADATA);

const MLKitMetaData DLB_SGD_METADATA{
    .id = "sgd",
    .name = "Stochastic Gradient Descent Optimizer",
    .class_ = RegistryClass::DLB_Optimizer,
    .args =
        std::vector<MetaDataArg>{
            MetaDataArg{.id = "lr",
                        .name = "Learning Rate",
                        .defaultValue = "0.1",
                        .type = MetaDataArgType::Decimal,
                        .desc = "Learning rate for the optimizer."},
            MetaDataArg{.id = "momentum",
                        .name = "Momentum",
                        .defaultValue = "0",
                        .type = MetaDataArgType::Decimal,
                        .desc = "Momentum factor."},
            MetaDataArg{.id = "weight_decay",
                        .name = "Weight Decay",
                        .defaultValue = "0",
                        .type = MetaDataArgType::Decimal,
                        .desc = "Weight decay (L2 penalty)."},
            MetaDataArg{.id = "dampening",
                        .name = "Dampening",
                        .defaultValue = "0",
                        .type = MetaDataArgType::Decimal,
                        .desc = "Dampening for momentum."},
            MetaDataArg{.id = "nesterov",
                        .name = "Enable Nesterov",
                        .defaultValue = "false",
                        .type = MetaDataArgType::Boolean,
                        .desc = "Enables Nesterov momentum."}},
    .desc = "Implements stochastic gradient descent."};
REGISTER_MLKIT_METADATA(DLB_SGD_METADATA);

const MLKitMetaData DLB_LBFGS_METADATA{
    .id = "lbfgs",
    .name = "L-BFGS Optimizer",
    .class_ = RegistryClass::DLB_Optimizer,
    .args =
        std::vector<MetaDataArg>{
            MetaDataArg{.id = "lr",
                        .name = "Learning Rate",
                        .defaultValue = "1",
                        .type = MetaDataArgType::Decimal,
                        .desc = "Learning rate for the optimizer."},
            MetaDataArg{
                .id = "max_iter",
                .name = "Max Iterations",
                .defaultValue = "20",
                .type = MetaDataArgType::Integer,
                .desc = "Maximum number of iterations per optimization step."},
            MetaDataArg{.id = "max_eval",
                        .name = "Max Evaluations",
                        .defaultValue = "25",
                        .type = MetaDataArgType::Integer,
                        .desc = "Maximum number of function evaluations per "
                                "optimization step."},
            MetaDataArg{.id = "tolerance_grad",
                        .name = "Tolerance Gradient",
                        .defaultValue = "1e-5",
                        .type = MetaDataArgType::Decimal,
                        .desc =
                            "Termination tolerance on first order optimality."},
            MetaDataArg{.id = "tolerance_change",
                        .name = "Tolerance Change",
                        .defaultValue = "1e-9",
                        .type = MetaDataArgType::Decimal,
                        .desc = "Termination tolerance on function "
                                "value/parameter changes."},
            MetaDataArg{.id = "history_size",
                        .name = "History Size",
                        .defaultValue = "100",
                        .type = MetaDataArgType::Integer,
                        .desc = "Update history size."}},
    .desc = "Implements L-BFGS optimizer."};
REGISTER_MLKIT_METADATA(DLB_LBFGS_METADATA);

// LR Scheduler
const MLKitMetaData DLB_STEP_LR_SCHEDULER_METADATA{
    .id = "step_lr",
    .name = "Step LR Scheduler",
    .class_ = RegistryClass::DLB_LRScheduler,
    .args =
        std::vector<MetaDataArg>{
            MetaDataArg{.id = "step_size",
                        .name = "Step Size",
                        .defaultValue = "30",
                        .type = MetaDataArgType::Integer,
                        .desc = "Period of learning rate decay."},
            MetaDataArg{.id = "gamma",
                        .name = "Gamma",
                        .defaultValue = "0.1",
                        .type = MetaDataArgType::Decimal,
                        .desc =
                            "Multiplicative factor of learning rate decay."}},
    .desc = "Decays the learning rate of each parameter group by gamma every "
            "step_size epochs."};
REGISTER_MLKIT_METADATA(DLB_STEP_LR_SCHEDULER_METADATA);

const std::vector<MetaDataArg> DLB_MODEL_ARGS{
    MetaDataArg{.id = "max_epochs",
                .name = "Max Epochs",
                .defaultValue = "10",
                .type = MetaDataArgType::Integer,
                .desc = "Maximum number of training epochs."},
    MetaDataArg{.id = "batch_size",
                .name = "Batch Size",
                .defaultValue = "32",
                .type = MetaDataArgType::Integer,
                .desc = "Number of samples per batch."}};

const MLKitMetaData DLB_MODEL_REGRESSION_METADATA{
    .id = "deep_learning_reg",
    .name = "Deep Learning Regression Model",
    .class_ = RegistryClass::RegressionEstimator,
    .args = DLB_MODEL_ARGS,
    .desc = "Deep learning model for regression tasks."};
REGISTER_MLKIT_METADATA(DLB_MODEL_REGRESSION_METADATA);

const MLKitMetaData DLB_MODEL_CLASSIFIER_METADATA{
    .id = "deep_learning_classifier",
    .name = "Deep Learning Classifier Model",
    .class_ = RegistryClass::Classifier,
    .args = DLB_MODEL_ARGS,
    .desc = "Deep learning model for classification tasks."};
REGISTER_MLKIT_METADATA(DLB_MODEL_CLASSIFIER_METADATA);

const MLKitMetaData KFOLD_CV_METADATA{
    .id = "k_fold",
    .name = "K-Fold Cross-Validator",
    .class_ = RegistryClass::CrossValidator,
    .args = std::vector<MetaDataArg>{MetaDataArg{
        .id = "n_splits",
        .name = "Number of Splits",
        .defaultValue = "5",
        .type = MetaDataArgType::Integer,
        .desc = "Number of folds. Must be at least 2."}},
    .desc = "Splits dataset into k consecutive folds."};
REGISTER_MLKIT_METADATA(KFOLD_CV_METADATA);

// Metrics
const MLKitMetaData DLB_R2_METRIC_METADATA{
    .id = "r2_score",
    .name = "R2 Score",
    .class_ = RegistryClass::Metric,
    .args = {},
    .desc = "Coefficient of determination regression score function."};
REGISTER_MLKIT_METADATA(DLB_R2_METRIC_METADATA);

const MLKitMetaData DLB_MSE_METRIC_METADATA{
    .id = "mean_squared_error",
    .name = "Mean Squared Error",
    .class_ = RegistryClass::Metric,
    .args = {},
    .desc = "Mean squared error regression loss."};
REGISTER_MLKIT_METADATA(DLB_MSE_METRIC_METADATA);

const MLKitMetaData DLB_NMSE_METRIC_METADATA{
    .id = "negative_mean_squared_error",
    .name = "Negative Mean Squared Error",
    .class_ = RegistryClass::Metric,
    .args = {},
    .desc = "Negative mean squared error regression loss."};
REGISTER_MLKIT_METADATA(DLB_NMSE_METRIC_METADATA);

const MLKitMetaData DLB_ACCURACY_METRIC_METADATA{
    .id = "accuracy",
    .name = "Accuracy",
    .class_ = RegistryClass::Metric,
    .args = std::vector<MetaDataArg>{MetaDataArg{
        .id = "task",
        .name = "Task",
        .defaultValue = "Binary",
        .type = MetaDataArgType::Select,
        .desc = "Type of classification task.",
        .values = {"Binary", "Multiclass", "Multilabel"},
        .isRequired = true}},
    .desc = "Accuracy metric for classification tasks."};
REGISTER_MLKIT_METADATA(DLB_ACCURACY_METRIC_METADATA);
} // namespace epoch_metadata::ml_kit
