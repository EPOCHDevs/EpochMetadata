#include "epoch_metadata/sql_statement.h"
#include <duckdb.hpp>
#include <mutex>

namespace epoch_metadata
{

namespace
{
  // Static shared DuckDB connection for SQL validation
  // This avoids creating a new database for each validation
  struct ValidationConnection
  {
    duckdb::DuckDB db;
    duckdb::Connection con;
    std::mutex mutex;

    ValidationConnection() : db(nullptr), con(db)
    {
      // Create a dummy "self" table with timestamp and SLOT columns
      // Use enough SLOT columns to accommodate most use cases (SLOT0-SLOT99)
      // We'll create a table with 100 SLOT columns of type DOUBLE plus timestamp
      std::string createTableSQL = "CREATE TABLE self (timestamp TIMESTAMP";
      for (int i = 0; i < 100; ++i)
      {
        createTableSQL += ", SLOT" + std::to_string(i) + " DOUBLE";
      }
      createTableSQL += ")";

      auto createResult = con.Query(createTableSQL);
      if (createResult->HasError())
      {
        throw std::runtime_error("Failed to create validation table: " + createResult->GetError());
      }
    }
  };

  ValidationConnection& GetValidationConnection()
  {
    static ValidationConnection validationConn;
    return validationConn;
  }
}

void SqlStatement::ValidateInternal()
{
  // 1. Check for empty SQL
  if (m_sql.empty())
  {
    throw std::runtime_error("SQL statement cannot be empty");
  }

  // 2. Validate "FROM self" requirement (case-insensitive)
  std::regex fromSelfRegex(R"(\bFROM\s+self\b)", std::regex_constants::icase);
  if (!std::regex_search(m_sql, fromSelfRegex))
  {
    throw std::runtime_error(
        "SQL statement must reference table as 'self' (e.g., 'FROM self'). "
        "Input columns are named SLOT0, SLOT1, SLOT2, etc.");
  }

  // 3. Use DuckDB to validate SQL syntax and semantics
  // DuckDB will validate:
  // - Table 'self' exists (enforced by dummy table in ValidationConnection)
  // - Columns SLOT0-SLOT99 exist (enforced by dummy table schema)
  // - SQL syntax is correct
  ValidateWithDuckDB();
}

void SqlStatement::ValidateWithDuckDB()
{
  try
  {
    // Get the shared validation connection
    auto& validationConn = GetValidationConnection();

    // Lock the connection for thread-safe validation
    std::lock_guard<std::mutex> lock(validationConn.mutex);

    // Prepare the user's SQL statement
    // PREPARE validates syntax and semantics without executing the query
    auto prepareResult = validationConn.con.Prepare(m_sql);
    if (prepareResult->HasError())
    {
      throw std::runtime_error("SQL validation failed: " + prepareResult->GetError());
    }

    // Always validate output columns
    ValidateOutputColumns(*prepareResult);

  }
  catch (const duckdb::Exception &e)
  {
    throw std::runtime_error("DuckDB validation error: " + std::string(e.what()));
  }
  catch (const std::exception &e)
  {
    throw std::runtime_error("SQL validation error: " + std::string(e.what()));
  }
}

void SqlStatement::ValidateOutputColumns(duckdb::PreparedStatement &preparedStmt)
{
  try
  {
    // Get the column names from the prepared statement (no execution needed)
    auto columnNames = preparedStmt.GetNames();
    std::vector<std::string> resultColumns(columnNames.begin(), columnNames.end());

    if (m_numOutputs > 0)
    {
      // For multi-output queries, validate that required RESULT columns and timestamp exist
      // Additional columns are allowed

      // Validate that all RESULT0, RESULT1, ..., RESULT(N-1) columns exist
      for (int i = 0; i < m_numOutputs; ++i)
      {
        std::string expectedCol = "RESULT" + std::to_string(i);
        auto it = std::find(resultColumns.begin(), resultColumns.end(), expectedCol);
        if (it == resultColumns.end())
        {
          throw std::runtime_error(
              "SQL query result missing required column: " + expectedCol +
              ". Available columns: " + JoinColumns(resultColumns));
        }
      }

      // Validate that timestamp column exists
      auto timestampIt = std::find(resultColumns.begin(), resultColumns.end(), "timestamp");
      if (timestampIt == resultColumns.end())
      {
        throw std::runtime_error(
            "SQL query result missing required 'timestamp' column for timeseries index. "
            "Available columns: " + JoinColumns(resultColumns));
      }
    }
    else
    {
      // For single-output queries (numOutputs == 0), allow any column names
      // No validation needed - SQL can return any columns including timestamp
    }
  }
  catch (const duckdb::Exception &e)
  {
    throw std::runtime_error("Output column validation error: " + std::string(e.what()));
  }
}

std::string SqlStatement::JoinColumns(const std::vector<std::string> &columns)
{
  if (columns.empty())
    return "(none)";

  std::string result = columns[0];
  for (size_t i = 1; i < columns.size(); ++i)
  {
    result += ", " + columns[i];
  }
  return result;
}

} // namespace epoch_metadata
