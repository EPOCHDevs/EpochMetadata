//
// Created by Claude Code
// EpochFlow AST Compiler
//
// Compiles Python AST directly into AlgorithmNode structures.
// Optimal single-pass compilation leveraging Python's sequential execution.
//

#pragma once

#include "../parser/ast_nodes.h"
#include <epoch_metadata/strategy/metadata.h>
#include <epoch_metadata/metadata_options.h>
#include <epoch_metadata/time_frame.h>
#include <epoch_metadata/transforms/metadata.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <optional>

namespace epoch_stratifyx::epochflow
{

    // Compilation result: topologically sorted algorithms (includes executor)
    using CompilationResult = std::vector<epoch_metadata::strategy::AlgorithmNode>;

    // Type system for type checking and casting
    enum class DataType
    {
        Boolean, // B
        Integer, // I
        Decimal, // D
        Number,  // N (accepts Integer or Decimal)
        String,  // S
        Any      // A
    };

    class AlgorithmAstCompiler
    {
    public:
        // Constructor - accesses ITransformRegistry singleton internally
        AlgorithmAstCompiler() = default;

        // Main compilation entry point - returns topologically sorted algorithms
        CompilationResult compile(const std::string &source);

        // Direct AST compilation (for testing)
        CompilationResult compileAST(ModulePtr module);

        size_t getExecutorCount() const { return executor_count_; }

    private:
        // Compilation state - optimal for single-pass
        std::unordered_map<std::string, std::string> var_to_binding_;                            // variable -> "node.handle" or "component_name"
        std::vector<epoch_metadata::strategy::AlgorithmNode> algorithms_;                        // Main output (topologically sorted, includes executor)
        size_t executor_count_;                                                                  // Track executor count for validation
        std::unordered_map<std::string, size_t> node_lookup_; // Fast O(1) lookup by index (never invalidated)
        std::unordered_map<std::string, std::unordered_map<std::string, DataType>> node_output_types_;
        std::unordered_set<std::string> used_node_ids_;                                          // Track used node IDs for O(1) uniqueness checks

        // AST visitor methods (will be implemented in Phase 2)
        void visitModule(const Module &module);
        void visitStmt(const Stmt &stmt);
        void visitAssign(const Assign &assign);
        void visitExprStmt(const ExprStmt &exprStmt);

        // Expression evaluation (returns node_id.handle)
        struct ValueHandle
        {
            std::string node_id;
            std::string handle;
        };

        ValueHandle visitExpr(const Expr &expr);
        ValueHandle visitCall(const Call &call);
        ValueHandle visitAttribute(const Attribute &attr);
        ValueHandle visitName(const Name &name);
        ValueHandle visitConstant(const Constant &constant);
        ValueHandle visitBinOp(const BinOp &binOp);
        ValueHandle visitUnaryOp(const UnaryOp &unaryOp);
        ValueHandle visitCompare(const Compare &compare);
        ValueHandle visitBoolOp(const BoolOp &boolOp);
        ValueHandle visitIfExp(const IfExp &ifExp);
        ValueHandle visitSubscript(const Subscript &subscript);

        // Constructor call parsing
        struct ConstructorParseResult
        {
            std::string ctor_name;
            std::unordered_map<std::string, epoch_metadata::MetaDataOptionDefinition::T> ctor_kwargs;
            std::vector<std::pair<std::vector<ValueHandle>, std::unordered_map<std::string, ValueHandle>>> feed_steps;
        };

        bool isConstructorCall(const Expr &expr);
        ConstructorParseResult parseConstructorAndFeeds(const Call &call);
        epoch_metadata::MetaDataOptionDefinition::T parseLiteralOrPrimitive(const Expr &expr);
        void handleConstructorAssignment(const Expr &target, const Expr &value, const Assign &assign);
        void handleNonConstructorAssignment(const Expr &target, const Expr &value, const Assign &assign);

        // Input wiring - directly populates AlgorithmNode.inputs with "node_id#handle" format
        void wireInputs(
            const std::string &target_node_id,
            const std::string &component_name,
            const std::vector<ValueHandle> &args,
            const std::unordered_map<std::string, ValueHandle> &kwargs);

        // Helper methods (stubs for now)
        std::string uniqueNodeId(const std::string &base);
        void validateComponent(const std::string &componentName);
        void validateTimeframe(const std::string &timeframe);
        void validateSession(const std::string &session);

        // Validation and optimization (incremental during compilation)
        void validateAndApplyOptions(
            const std::string &node_id,
            const epoch_metadata::transforms::TransformsMetaData &comp_meta,
            std::unordered_map<std::string, epoch_metadata::MetaDataOptionDefinition::T> &kwargs,
            const Call &call);

        // Special parameter handling (following Python's SRP pattern)
        void canonicalizeTimeframe(std::unordered_map<std::string, epoch_metadata::MetaDataOptionDefinition::T> &params);
        void canonicalizeSession(std::unordered_map<std::string, epoch_metadata::MetaDataOptionDefinition::T> &params);
        void applySpecialFields(epoch_metadata::strategy::AlgorithmNode &algo,
                                const std::unordered_map<std::string, epoch_metadata::MetaDataOptionDefinition::T> &params);
        void verifySessionDependencies();

        // Timeframe resolution (post-compilation step using TimeframeResolver utility)
        void resolveTimeframes(const std::optional<epoch_metadata::TimeFrame> &baseTimeframe);

        // Materialize literal nodes
        ValueHandle materializeNumber(double value);
        ValueHandle materializeBoolean(bool value);
        ValueHandle materializeText(const std::string &value);
        ValueHandle materializeNull();

        // Attribute resolution helpers
        std::pair<std::string, std::string> attributeToTuple(const Attribute &attr);
        ValueHandle resolveHandle(const std::string &var, const std::string &handle);

        // Helper to create "node_id#handle" format
        std::string joinId(const std::string &node_id, const std::string &handle);

        // Type system helpers
        DataType getNodeOutputType(const std::string &nodeId, const std::string &handle);
        bool isTypeCompatible(DataType source, DataType target);
        std::optional<std::string> needsTypeCast(DataType source, DataType target);
        ValueHandle insertTypeCast(const ValueHandle &source, DataType sourceType, DataType targetType);
        std::string dataTypeToString(DataType type);

        // Error reporting
        [[noreturn]] void throwError(const std::string &msg, int line = 0, int col = 0);
    };

    // Convenience function (mirrors Python's compile_algorithm)
    CompilationResult compileAlgorithm(const std::string &source);

} // namespace epoch_stratifyx::epochflow
