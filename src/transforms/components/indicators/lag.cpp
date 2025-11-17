//
// Created by adesola on 1/20/25.
//
#include "lag.h"

namespace epoch_script::transform {

// Explicit template instantiations - single implementation for all types
template class TypedLag<StringType>;
template class TypedLag<NumberType>;
template class TypedLag<BooleanType>;
template class TypedLag<TimestampType>;

} // namespace epoch_script::transform