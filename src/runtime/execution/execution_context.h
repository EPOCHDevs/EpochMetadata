#pragma once
#include "iintermediate_storage.h"
#include "thread_safe_logger.h"
// Removed: #include <model/asset/asset.h> - not needed here

namespace epoch_flow::runtime {

struct ExecutionContext {
  std::unique_ptr<IIntermediateStorage> cache;
  ILoggerPtr logger;
};

} // namespace epoch_flow::runtime