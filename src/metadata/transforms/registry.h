#pragma once
#include "../registry.h"
#include "metadata.h"

namespace metadata::transforms{
    using ITransformRegistry = IMetaDataRegistry<TransformsMetaData>;
}