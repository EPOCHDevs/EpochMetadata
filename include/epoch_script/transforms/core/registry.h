#pragma once
#include "../../core/registry.h"
#include "metadata.h"

namespace epoch_script::transforms {
using ITransformRegistry = IMetaDataRegistry<TransformsMetaData>;
}