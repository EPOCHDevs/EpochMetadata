#pragma once
#include "../../core/registry.h"
#include "metadata.h"

namespace epochflow::transforms {
using ITransformRegistry = IMetaDataRegistry<TransformsMetaData>;
}