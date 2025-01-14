//
// Created by dewe on 9/12/24.
//

#pragma once
#include "../registry.h"
#include "metadata.h"


namespace stratifyx::metadata::strategy {
    namespace trade_signal {
        using Registry = IMetaDataRegistry<TradeSignalMetaData>;
    }

    namespace stop_loss {
        using Registry = IMetaDataRegistry<AlgorithmMetaData, 0>;
    }

    namespace take_profit {
        using Registry = IMetaDataRegistry<AlgorithmMetaData, 1>;
    }

    namespace position_sizer {
        using Registry = IMetaDataRegistry<AlgorithmMetaData, 2>;
    }

    namespace futures_continuation {
        using Registry = IMetaDataRegistry<AlgorithmBaseMetaData, 0>;
    }

    namespace commission {
        using Registry = IMetaDataRegistry<AlgorithmBaseMetaData, 1>;
    }

    namespace slippage {
        using Registry = IMetaDataRegistry<AlgorithmBaseMetaData, 2>;
    }
}
