//
// Created by Claude Code
// EpochFlow AST Node Definitions
//
// Mirrors Python's ast module structure for representing parsed Python syntax.
//

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <variant>

namespace epochflow {

// Forward declarations
struct ASTNode;
struct Expr;
struct Stmt;

// Base node type
struct ASTNode {
    int lineno = 0;
    int col_offset = 0;
    virtual ~ASTNode() = default;
};

// Expression nodes
struct Expr : ASTNode {};

struct Name : Expr {
    std::string id;
    explicit Name(std::string name) : id(std::move(name)) {}
};

struct Constant : Expr {
    using Value = std::variant<int, double, std::string, bool, std::monostate>; // monostate for None
    Value value;

    explicit Constant(Value val) : value(std::move(val)) {}
};

struct Attribute : Expr {
    std::unique_ptr<Expr> value;  // base object
    std::string attr;             // attribute name

    Attribute(std::unique_ptr<Expr> val, std::string attribute)
        : value(std::move(val)), attr(std::move(attribute)) {}
};

struct Call : Expr {
    std::unique_ptr<Expr> func;                    // function being called
    std::vector<std::unique_ptr<Expr>> args;       // positional arguments
    std::vector<std::pair<std::string, std::unique_ptr<Expr>>> keywords;  // keyword arguments

    Call(std::unique_ptr<Expr> function) : func(std::move(function)) {}
};

// Binary operators
enum class BinOpType {
    Add, Sub, Mult, Div, Mod, Pow,
    Lt, Gt, LtE, GtE, Eq, NotEq,
    And, Or
};

struct BinOp : Expr {
    BinOpType op;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;

    BinOp(BinOpType operation, std::unique_ptr<Expr> l, std::unique_ptr<Expr> r)
        : op(operation), left(std::move(l)), right(std::move(r)) {}
};

struct Compare : Expr {
    std::unique_ptr<Expr> left;
    std::vector<BinOpType> ops;
    std::vector<std::unique_ptr<Expr>> comparators;

    Compare(std::unique_ptr<Expr> l) : left(std::move(l)) {}
};

struct BoolOp : Expr {
    BinOpType op;  // And or Or
    std::vector<std::unique_ptr<Expr>> values;

    explicit BoolOp(BinOpType operation) : op(operation) {}
};

// Unary operators
enum class UnaryOpType {
    Not, USub, UAdd
};

struct UnaryOp : Expr {
    UnaryOpType op;
    std::unique_ptr<Expr> operand;

    UnaryOp(UnaryOpType operation, std::unique_ptr<Expr> expr)
        : op(operation), operand(std::move(expr)) {}
};

// Ternary expression (IfExp)
struct IfExp : Expr {
    std::unique_ptr<Expr> test;
    std::unique_ptr<Expr> body;
    std::unique_ptr<Expr> orelse;

    IfExp(std::unique_ptr<Expr> condition, std::unique_ptr<Expr> true_val, std::unique_ptr<Expr> false_val)
        : test(std::move(condition)), body(std::move(true_val)), orelse(std::move(false_val)) {}
};

// List expression
struct List : Expr {
    std::vector<std::unique_ptr<Expr>> elts;
};

// Tuple expression
struct Tuple : Expr {
    std::vector<std::unique_ptr<Expr>> elts;
};

// Dictionary expression
struct Dict : Expr {
    std::vector<std::unique_ptr<Expr>> keys;
    std::vector<std::unique_ptr<Expr>> values;
};

// Subscript (for lag operator: src.c[1])
struct Subscript : Expr {
    std::unique_ptr<Expr> value;
    std::unique_ptr<Expr> slice;

    Subscript(std::unique_ptr<Expr> val, std::unique_ptr<Expr> idx)
        : value(std::move(val)), slice(std::move(idx)) {}
};

// Statement nodes
struct Stmt : ASTNode {};

struct Assign : Stmt {
    std::vector<std::unique_ptr<Expr>> targets;
    std::unique_ptr<Expr> value;

    explicit Assign(std::unique_ptr<Expr> val) : value(std::move(val)) {}
};

struct ExprStmt : Stmt {
    std::unique_ptr<Expr> value;

    explicit ExprStmt(std::unique_ptr<Expr> expr) : value(std::move(expr)) {}
};

// Module (top-level)
struct Module : ASTNode {
    std::vector<std::unique_ptr<Stmt>> body;
};

// Helper type aliases
using ASTNodePtr = std::unique_ptr<ASTNode>;
using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;
using ModulePtr = std::unique_ptr<Module>;

} // namespace epochflow
