//
// Created by Claude Code
// EpochFlow Python Parser - Tree-sitter Integration
//
// Parses Python source code into C++ AST nodes using tree-sitter.
//

#pragma once

#include "ast_nodes.h"

// Suppress unused parameter warnings from cpp-tree-sitter header
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <cpp-tree-sitter.h>
#pragma GCC diagnostic pop

#include <string>
#include <stdexcept>
#include <optional>

namespace epochflow {

class PythonParseError : public std::runtime_error {
public:
    PythonParseError(const std::string& msg, int line, int col)
        : std::runtime_error(msg + " (line " + std::to_string(line) +
                             ", col " + std::to_string(col) + ")")
        , lineno(line)
        , col_offset(col) {}

    int lineno;
    int col_offset;
};

class PythonParser {
public:
    PythonParser();
    ~PythonParser() = default;

    // Parse Python source code into AST
    ModulePtr parse(const std::string& source);

private:
    std::optional<ts::Parser> parser_;

    // Convert tree-sitter nodes to our AST
    ModulePtr parseModule(const ts::Node& node, std::string_view source);
    StmtPtr parseStatement(const ts::Node& node, std::string_view source);
    ExprPtr parseExpression(const ts::Node& node, std::string_view source);

    // Specific node type parsers
    ExprPtr parseCall(const ts::Node& node, std::string_view source);
    ExprPtr parseAttribute(const ts::Node& node, std::string_view source);
    ExprPtr parseName(const ts::Node& node, std::string_view source);
    ExprPtr parseConstant(const ts::Node& node, std::string_view source);
    ExprPtr parseBinaryOp(const ts::Node& node, std::string_view source);
    ExprPtr parseCompare(const ts::Node& node, std::string_view source);
    ExprPtr parseBoolOp(const ts::Node& node, std::string_view source);
    ExprPtr parseUnaryOp(const ts::Node& node, std::string_view source);
    ExprPtr parseIfExp(const ts::Node& node, std::string_view source);
    ExprPtr parseSubscript(const ts::Node& node, std::string_view source);
    ExprPtr parseTuple(const ts::Node& node, std::string_view source);
    ExprPtr parseList(const ts::Node& node, std::string_view source);
    ExprPtr parseDict(const ts::Node& node, std::string_view source);

    StmtPtr parseAssignment(const ts::Node& node, std::string_view source);
    StmtPtr parseExprStmt(const ts::Node& node, std::string_view source);

    // Helper functions
    std::string getNodeText(const ts::Node& node, std::string_view source);
    BinOpType parseBinOpType(const std::string& opText);
    UnaryOpType parseUnaryOpType(const std::string& opText);
    void throwError(const std::string& msg, const ts::Node& node);
};

} // namespace epochflow
