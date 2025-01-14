#pragma once
#include "../registry.h"
#include "metadata.h"

namespace stratifyx::metadata::transforms{
    using ITransformRegistry = IMetaDataRegistry<TransformsMetaData>;
}