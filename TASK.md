# EpochScript - Implementation Tasks

This file tracks high-priority implementation tasks for EpochScript compiler and runtime improvements.

---

## High Priority - Compiler Optimization

### 1. Implement Common Subexpression Elimination (CSE)

**Status:** Not Started
**Priority:** High
**Effort:** Medium (2-3 days)

#### Problem

The compiler currently creates separate transform nodes for identical transforms, even when they have the same type, parameters, and inputs. This causes redundant computation and wastes CPU cycles.

**Current behavior:**
```epochscript
# This creates THREE separate ema nodes: ema_0, ema_1, ema_2
signal1 = ema(period=20)(src.c) > 100
signal2 = ema(period=20)(src.c) > ema(period=50)(src.c)
signal3 = src.c > ema(period=20)(src.c)
```

**Expected behavior:**
```
# Should create only ONE ema(period=20) node, reused by all three signals
```

#### Solution

Add a Common Subexpression Elimination (CSE) optimization pass after AST compilation:

1. **Create `CSEOptimizer` class**
   - Location: `/src/transforms/compiler/cse_optimizer.{h,cpp}`
   - Runs after initial AST compilation, before topological sort
   - Deduplicates identical transform nodes

2. **Hash function for `AlgorithmNode`**
   - Hash based on:
     - `type` (transform name, e.g., "ema")
     - `options` (all parameters, e.g., `period=20`)
     - `inputs` (input handles, e.g., `src.c`)
     - `timeframe` (if specified)
     - `session` (if specified)
   - Use `std::hash` combination or similar

3. **Deduplication algorithm**
   ```cpp
   std::unordered_map<size_t, std::string> hash_to_node_id;

   for each AlgorithmNode node in graph:
       hash = ComputeHash(node)
       if hash exists in hash_to_node_id:
           original_id = hash_to_node_id[hash]
           // Update all references to node.id -> point to original_id
           // Mark node for removal
       else:
           hash_to_node_id[hash] = node.id

   // Remove duplicate nodes from graph
   ```

4. **Integration point**
   - Modify `AlgorithmAstCompiler::compileAST()` in `/src/transforms/compiler/ast_compiler.cpp`
   - Add CSE pass after line where `algorithms` vector is populated
   - Run before topological sort (since we're modifying the graph)

#### Benefits

- **Automatic optimization** - No user code changes required
- **Handles multiple patterns:**
  - Redundant transform calculations
  - Multi-output transform duplication (e.g., calling `bbands(...)` twice)
  - Lag operation caching (e.g., multiple `src.c[10]` accesses)
- **Significant performance improvement** - Especially for complex strategies with many indicators
- **Transparent to users** - Works like any modern compiler optimization

#### Implementation Details

**Files to create:**
- `/src/transforms/compiler/cse_optimizer.h`
- `/src/transforms/compiler/cse_optimizer.cpp`

**Files to modify:**
- `/src/transforms/compiler/ast_compiler.cpp` - Add CSE pass
- `/src/transforms/compiler/ast_compiler.h` - Add CSE member/method

**Key considerations:**
- Preserve semantic meaning (only deduplicate truly identical transforms)
- Handle edge cases:
  - Transforms with side effects (should not deduplicate)
  - Different timeframes (must remain separate)
  - Different sessions (must remain separate)
- Update reference counts correctly
- Maintain topological ordering after deduplication

#### Testing

**Unit tests:**
- Test CSE deduplicates identical transforms
- Test CSE preserves different transforms (different params, inputs, etc.)
- Test CSE handles multi-output transforms correctly
- Test CSE handles lag operations correctly
- Test topological sort still works after CSE

**Integration tests:**
- Benchmark performance on real strategies before/after CSE
- Verify output correctness (same results with fewer computations)
- Test with various strategy complexities

**Expected test cases:**
```epochscript
# Test 1: Simple deduplication
ema_20_a = ema(period=20)(src.c)
ema_20_b = ema(period=20)(src.c)
# Should create 1 node, not 2

# Test 2: Different parameters - no deduplication
ema_20 = ema(period=20)(src.c)
ema_50 = ema(period=50)(src.c)
# Should create 2 nodes

# Test 3: Multi-output deduplication
bb1 = bbands(period=20, stddev=2)(src.c)
bb2 = bbands(period=20, stddev=2)(src.c)
# Should create 1 node, not 2

# Test 4: Lag deduplication
lag_10_a = src.c[10]
lag_10_b = src.c[10]
# Should create 1 lag node, not 2
```

#### References

- **Evidence:** `/src/transforms/compiler/expression_compiler.cpp` lines 1065-1078 - `UniqueNodeId()` function simply increments counter
- **Related:** Runtime caching exists in `/src/transforms/runtime/execution/intermediate_storage.cpp` but only works within single pipeline run
- **Standard practice:** CSE is a common compiler optimization (GCC, LLVM, etc.)

#### Success Criteria

- [ ] CSE optimizer implemented and integrated into compilation pipeline
- [ ] All unit tests pass
- [ ] Performance benchmarks show reduction in transform node count
- [ ] No regression in output correctness
- [ ] Documentation updated (compiler internals, not user-facing)

---

## Future Tasks

### 2. Dead Code Elimination

**Status:** Not Started
**Priority:** Medium

Eliminate transform nodes that are computed but never used in outputs (signals, reports, executors).

### 3. Constant Propagation Enhancement

**Status:** Partially Implemented
**Priority:** Low

Extend existing constant folder to handle more complex constant expressions.

---

## Notes

- Tasks are ordered by priority and impact
- CSE optimization will provide the biggest performance win with least user impact
- Most optimizations should be transparent to users (no code changes required)
