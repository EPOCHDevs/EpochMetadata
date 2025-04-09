#pragma once
#include "../registry.h"
#include "epoch_metadata/transforms/metadata.h"

namespace metadata::transforms{
    using ITransformRegistry = IMetaDataRegistry<TransformsMetaData>;
}