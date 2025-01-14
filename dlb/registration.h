//
// Created by dewe on 9/12/24.
//

#pragma once

#include "registry.h"
#include "metadata.h"

namespace stratifyx::metadata::dlb {
    inline int RegisterDLBMetaData(const DLBMetaData& metaData)
    {
        DLBRegistry::GetInstance().Register(metaData);
        return 0;
    }

    auto Merge(auto args1, auto const& args2) {
        args1.insert(args1.end(), args2.begin(), args2.end());
        return args1;
    }

#define REGISTER_DLB_METADATA(FactoryMetaData) const int REGISTER_DLB_METADATA_##FactoryMetaData = RegisterDLBMetaData(FactoryMetaData)

    const  std::vector SEQ_NODE_OPTIONS{
        MetaDataArgs{.id = "weight_init_type", .name = "Weight Init Type", .type= MetaDataArgType::Decimal, .defaultValue = "", .isRequired=true, .values= {"orthogonal", "xavier_uniform", "xavier_normal", "constant"} },
          MetaDataArgs{.id = "weight_init_gain", .name = "Weight Init Gain", .type= MetaDataArgType::Decimal},
          MetaDataArgs{.id = "bias_init", .name = "Random State", .type= MetaDataArgType::Decimal, .defaultValue="", .isRequired=false},
          MetaDataArgs{.id = "activation", .name = "Mode", .type= MetaDataArgType::Select, .defaultValue = "relu", .isRequired=true, .values = {"tanh", "relu", "leaky_relu", "sigmoid"} },
          MetaDataArgs{.id = "flatten", .name = "Flatten", .type= MetaDataArgType::Boolean, .defaultValue = "false", .isRequired=false}
    };

    const std::vector DLB_LINEAR_METADATA_ARGS{
        MetaDataArgs{.id = "dim", .name = "Dim", .type= MetaDataArgType::Integer},
        MetaDataArgs{.id = "new_bias", .name = "Add New Bias", .type= MetaDataArgType::Boolean, .defaultValue = "false", .isRequired=false},
      };
    const DLBMetaData DLB_FCNN_METADATA{
        .id = "FCNN",
        .name = "Fully Connected Neural Network",
        .nodeType = NodeConnectionType::OneToOne,
        .args = Merge(SEQ_NODE_OPTIONS, DLB_LINEAR_METADATA_ARGS),
        .isArgsList = true
    };
    REGISTER_DLB_METADATA(DLB_FCNN_METADATA);

    std::vector DLB_RNN_OPTIONS{
        MetaDataArgs{.id = "hidden_size", .name = "Hidden State Size", .type= MetaDataArgType::Integer},
        MetaDataArgs{.id = "num_layers", .name = "Num Layers", .type= MetaDataArgType::Integer, .defaultValue = "1"},
        MetaDataArgs{.id = "drop_out", .name = "Drop out", .type= MetaDataArgType::Decimal},
        MetaDataArgs{.id = "bidirectional", .name = "Bi-Directional", .type= MetaDataArgType::Boolean},
        MetaDataArgs{.id = "bias", .name = "Bias", .type= MetaDataArgType::Boolean},
        MetaDataArgs{.id = "nonlinearity", .name = "Non Linearity", .type= MetaDataArgType::Select, .defaultValue = "", .isRequired=false, .values = {"tanh", "relu"}},
      };

    const DLBMetaData DLB_GRU_METADATA{
        .id = "GRU",
        .name = "Gated Recurrent Unit",
        .nodeType = NodeConnectionType::OneToOne,
        .args = Merge(SEQ_NODE_OPTIONS, DLB_RNN_OPTIONS)
    };
    REGISTER_DLB_METADATA(DLB_GRU_METADATA);

    const DLBMetaData DLB_RNN_METADATA{
        .id = "RNN",
        .name = "Recurrent Neural Network",
        .nodeType = NodeConnectionType::OneToOne,
        .args = Merge(SEQ_NODE_OPTIONS, DLB_RNN_OPTIONS)
    };
    REGISTER_DLB_METADATA(DLB_RNN_METADATA);

    const DLBMetaData DLB_LSTM_METADATA{
        .id = "LSTM",
        .name = "Long Short-Term Memory",
        .nodeType = NodeConnectionType::OneToOne,
        .args = Merge(Merge(SEQ_NODE_OPTIONS, DLB_RNN_OPTIONS), std::vector{MetaDataArgs{
            MetaDataArgs{.id = "proj_size", .name = "Proj Size", .type= MetaDataArgType::Integer, .defaultValue = "0", .isRequired=false},
        }})
    };
    REGISTER_DLB_METADATA(DLB_LSTM_METADATA);

    const DLBMetaData DLB_LINEAR_METADATA{
        .id = "Linear",
        .name = "Linear",
        .nodeType = NodeConnectionType::OneToOne,
        .args = DLB_LINEAR_METADATA_ARGS
    };
    REGISTER_DLB_METADATA(DLB_LINEAR_METADATA);

    std::vector DLB_EMBEDDING_OPTIONS{
        MetaDataArgs{.id = "in_features", .name = "Size", .type= MetaDataArgType::Integer},
        MetaDataArgs{.id = "padding_idx", .name = "Padding Index", .type= MetaDataArgType::Integer, .defaultValue = "", .isRequired = false},
        MetaDataArgs{.id = "max_norm", .name = "Max Norm", .type= MetaDataArgType::Integer, .defaultValue = "", .isRequired = false},
        MetaDataArgs{.id = "norm_type", .name = "Norm Type", .type= MetaDataArgType::Decimal, .defaultValue = "", .isRequired = false},
        MetaDataArgs{.id = "scale_grad_by_freq", .name = "Scale Grad By Freq", .type= MetaDataArgType::Boolean, .defaultValue = "", .isRequired = false},
                MetaDataArgs{.id = "sparse", .name = "Sparse", .type= MetaDataArgType::Boolean, .defaultValue = "", .isRequired = false}
    };

    const DLBMetaData DLB_EMBEDDING_METADATA{
        .id = "Embedding",
        .name = "Embedding",
        .nodeType = NodeConnectionType::OneToOne,
        .args = Merge(SEQ_NODE_OPTIONS, DLB_EMBEDDING_OPTIONS)
    };
    REGISTER_DLB_METADATA(DLB_EMBEDDING_METADATA);

    const DLBMetaData DLB_EMBEDDING_SEQ_METADATA{
        .id = "Embeddings",
        .name = "Embeddings",
        .nodeType = NodeConnectionType::OneToOne,
        .args = Merge(SEQ_NODE_OPTIONS, DLB_EMBEDDING_OPTIONS),
        .isArgsList = true
    };
    REGISTER_DLB_METADATA(DLB_EMBEDDING_SEQ_METADATA);

    const DLBMetaData DLB_CONCAT_METADATA{
        .id = "Concat",
        .name = "Concat",
        .nodeType = NodeConnectionType::ManyToOne,
        .args = {}
    };
    REGISTER_DLB_METADATA(DLB_CONCAT_METADATA);
}  // namespace stratifyx::metadata::dlb
