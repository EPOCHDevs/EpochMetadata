#pragma once

#include <string>
#include <vector>
#include <optional>
#include <stdexcept>
#include <algorithm>
#include <regex>

// Forward declaration to avoid including duckdb.hpp in header
namespace duckdb {
  class Connection;
  class PreparedStatement;
}

namespace epoch_script
{

/**
 * @brief Validated SQL statement with strict naming conventions
 *
 * SqlStatement enforces:
 * - Table reference must be "FROM self"
 * - Input columns use SLOT naming: SLOT0, SLOT1, SLOT2, ..., SLOTN
 * - Output columns use RESULT naming: RESULT0, RESULT1, RESULT2, ..., RESULTN (for multi-output)
 *
 * Validation uses DuckDB PREPARE to catch syntax and semantic errors at construction time.
 *
 * Example:
 * ```cpp
 * // Valid single-output query
 * SqlStatement stmt1("SELECT * FROM self WHERE SLOT0 > 100");
 *
 * // Valid multi-output query
 * SqlStatement stmt2("SELECT RESULT0, RESULT1 FROM (SELECT SLOT0 as RESULT0, SLOT1 as RESULT1 FROM self)", {"RESULT0", "RESULT1"});
 * ```
 */
class SqlStatement
{
public:
  /**
   * @brief Default constructor (creates empty, invalid SQL statement)
   */
  SqlStatement() = default;

  /**
   * @brief Construct SQL statement (validation must be called separately)
   *
   * @param sql The SQL query string
   * @param numOutputs Optional number of expected output columns (validates RESULT0 to RESULT(N-1))
   *                   If not specified (0), validates all outputs have RESULT prefix
   */
  SqlStatement(std::string sql, int numOutputs = 0)
      : m_sql(std::move(sql)), m_numOutputs(numOutputs)
  {
  }

  /**
   * @brief Validate SQL statement
   *
   * @param numOutputs Optional number of expected output columns (validates RESULT0 to RESULT(N-1))
   *                   If not specified, uses the numOutputs from constructor
   * @throws std::runtime_error if SQL is invalid or doesn't meet naming conventions
   */
  void Validate(int numOutputs = -1)
  {
    if (numOutputs >= 0)
    {
      m_numOutputs = numOutputs;
    }
    ValidateInternal();
  }

  /**
   * @brief Get the validated SQL string
   * @return The SQL query string
   */
  [[nodiscard]] std::string const &GetSql() const { return m_sql; }

  /**
   * @brief Set the SQL string (for deserialization)
   * @param sql The SQL query string
   */
  void SetSql(std::string sql) { m_sql = std::move(sql); Validate(); }

  /**
   * @brief Get the number of expected output columns
   * @return Number of expected outputs (0 if not specified)
   */
  [[nodiscard]] int GetNumOutputs() const
  {
    return m_numOutputs;
  }

  /**
   * @brief Equality comparison
   */
  bool operator==(const SqlStatement &other) const
  {
    return m_sql == other.m_sql && m_numOutputs == other.m_numOutputs;
  }

private:
  std::string m_sql;
  int m_numOutputs = 0;

  /**
   * @brief Internal validation using DuckDB PREPARE
   */
  void ValidateInternal();

  /**
   * @brief Validate SQL using DuckDB PREPARE statement
   *
   * Uses a static shared DuckDB connection with a "self" table containing SLOT0-SLOT99 columns
   * and attempts to PREPARE the user's SQL statement.
   */
  void ValidateWithDuckDB();

  /**
   * @brief Validate that required output columns exist in the query result
   *
   * Uses PreparedStatement metadata to check the result schema without executing the query.
   */
  void ValidateOutputColumns(duckdb::PreparedStatement &preparedStmt);

  /**
   * @brief Helper to join column names for error messages
   */
  static std::string JoinColumns(const std::vector<std::string> &columns);
};

} // namespace epoch_script
