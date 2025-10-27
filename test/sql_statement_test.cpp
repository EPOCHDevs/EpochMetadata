//
// Test cases for SqlStatement validation
//

#include "common.h"
#include "epoch_metadata/sql_statement.h"
#include "epoch_metadata/metadata_options.h"  // For glaze serialization
#include <catch2/catch_all.hpp>
#include <glaze/glaze.hpp>

using namespace epoch_metadata;

TEST_CASE("SqlStatement - Basic validation", "[SqlStatement]") {
  SECTION("Valid single-output query") {
    REQUIRE_NOTHROW(SqlStatement("SELECT * FROM self"));
    REQUIRE_NOTHROW(SqlStatement("SELECT SLOT0 FROM self"));
    REQUIRE_NOTHROW(SqlStatement("SELECT SLOT0, SLOT1, SLOT2 FROM self"));
  }

  SECTION("Valid query with WHERE clause") {
    REQUIRE_NOTHROW(SqlStatement("SELECT SLOT0 FROM self WHERE SLOT1 > 100"));
    REQUIRE_NOTHROW(SqlStatement("SELECT SLOT0, SLOT1 FROM self WHERE SLOT0 < SLOT1"));
  }

  SECTION("Empty SQL is rejected") {
    SqlStatement stmt("");
    REQUIRE_THROWS_AS(stmt.Validate(), std::runtime_error);
  }

  SECTION("Invalid SQL syntax is rejected") {
    SqlStatement stmt1("SELECT FROM");
    REQUIRE_THROWS_AS(stmt1.Validate(), std::runtime_error);
    SqlStatement stmt2("INVALID SQL");
    REQUIRE_THROWS_AS(stmt2.Validate(), std::runtime_error);
  }
}

TEST_CASE("SqlStatement - Table name validation", "[SqlStatement]") {
  SECTION("Must reference table as 'self'") {
    REQUIRE_NOTHROW(SqlStatement("SELECT * FROM self"));
    REQUIRE_NOTHROW(SqlStatement("SELECT * from self"));  // case insensitive
    REQUIRE_NOTHROW(SqlStatement("SELECT * FROM Self"));  // case insensitive
  }

  SECTION("Wrong table name is rejected") {
    SqlStatement stmt1("SELECT * FROM my_table");
    REQUIRE_THROWS_AS(stmt1.Validate(), std::runtime_error);
    SqlStatement stmt2("SELECT * FROM foo");
    REQUIRE_THROWS_AS(stmt2.Validate(), std::runtime_error);
  }

  SECTION("Missing FROM clause is rejected") {
    SqlStatement stmt("SELECT 1");
    REQUIRE_THROWS_AS(stmt.Validate(), std::runtime_error);
  }
}

TEST_CASE("SqlStatement - SLOT column validation", "[SqlStatement]") {
  SECTION("Valid SLOT columns are accepted") {
    REQUIRE_NOTHROW(SqlStatement("SELECT SLOT0 FROM self"));
    REQUIRE_NOTHROW(SqlStatement("SELECT SLOT0, SLOT1, SLOT2 FROM self"));
    REQUIRE_NOTHROW(SqlStatement("SELECT SLOT99 FROM self"));  // SLOT0-SLOT99 supported
  }

  SECTION("Invalid column names are rejected") {
    SqlStatement stmt1("SELECT price FROM self");
    REQUIRE_THROWS_AS(stmt1.Validate(), std::runtime_error);
    SqlStatement stmt2("SELECT foo, bar FROM self");
    REQUIRE_THROWS_AS(stmt2.Validate(), std::runtime_error);
  }

  SECTION("SLOT columns out of range are rejected") {
    SqlStatement stmt1("SELECT SLOT100 FROM self");
    REQUIRE_THROWS_AS(stmt1.Validate(), std::runtime_error);
    SqlStatement stmt2("SELECT SLOT999 FROM self");
    REQUIRE_THROWS_AS(stmt2.Validate(), std::runtime_error);
  }

  SECTION("Mixed valid and invalid columns are rejected") {
    SqlStatement stmt("SELECT SLOT0, invalid_col FROM self");
    REQUIRE_THROWS_AS(stmt.Validate(), std::runtime_error);
  }
}

TEST_CASE("SqlStatement - RESULT column validation", "[SqlStatement]") {
  SECTION("Output columns with RESULT prefix - no numOutputs specified") {
    // When numOutputs=0, just validate RESULT prefix
    REQUIRE_NOTHROW(SqlStatement("SELECT SLOT0 as RESULT0 FROM self"));
    REQUIRE_NOTHROW(SqlStatement("SELECT SLOT0 as RESULT1 FROM self"));
    REQUIRE_NOTHROW(SqlStatement("SELECT SLOT0 as RESULT0, SLOT1 as RESULT1 FROM self"));
  }

  SECTION("Output columns without RESULT prefix - no numOutputs specified") {
    // When numOutputs=0, columns without RESULT prefix should fail
    SqlStatement stmt1("SELECT SLOT0 as output FROM self");
    REQUIRE_THROWS_AS(stmt1.Validate(), std::runtime_error);
    SqlStatement stmt2("SELECT SLOT0 as col1 FROM self");
    REQUIRE_THROWS_AS(stmt2.Validate(), std::runtime_error);
  }

  SECTION("Specific number of outputs - validates RESULT0 to RESULTN-1") {
    // numOutputs=2 expects exactly RESULT0 and RESULT1
    REQUIRE_NOTHROW(SqlStatement("SELECT SLOT0 as RESULT0, SLOT1 as RESULT1 FROM self", 2));
  }

  SECTION("Wrong number of outputs is rejected") {
    // Expects 2 outputs but query returns 1
    SqlStatement stmt1("SELECT SLOT0 as RESULT0 FROM self", 2);
    REQUIRE_THROWS_AS(stmt1.Validate(), std::runtime_error);

    // Expects 2 outputs but query returns 3
    SqlStatement stmt2("SELECT SLOT0 as RESULT0, SLOT1 as RESULT1, SLOT2 as RESULT2 FROM self", 2);
    REQUIRE_THROWS_AS(stmt2.Validate(), std::runtime_error);
  }

  SECTION("Missing required RESULT column") {
    // Expects RESULT0 and RESULT1 but gets RESULT0 and RESULT2
    SqlStatement stmt("SELECT SLOT0 as RESULT0, SLOT1 as RESULT2 FROM self", 2);
    REQUIRE_THROWS_AS(stmt.Validate(), std::runtime_error);
  }

  SECTION("Multi-output queries (2-4 outputs)") {
    // 2 outputs
    REQUIRE_NOTHROW(SqlStatement(
        "SELECT SLOT0 as RESULT0, SLOT1 as RESULT1 FROM self", 2));

    // 3 outputs
    REQUIRE_NOTHROW(SqlStatement(
        "SELECT SLOT0 as RESULT0, SLOT1 as RESULT1, SLOT2 as RESULT2 FROM self", 3));

    // 4 outputs
    REQUIRE_NOTHROW(SqlStatement(
        "SELECT SLOT0 as RESULT0, SLOT1 as RESULT1, SLOT2 as RESULT2, SLOT3 as RESULT3 FROM self", 4));
  }
}

TEST_CASE("SqlStatement - Complex queries", "[SqlStatement]") {
  SECTION("Queries with aggregations") {
    REQUIRE_NOTHROW(SqlStatement("SELECT SUM(SLOT0) as RESULT0 FROM self"));
    REQUIRE_NOTHROW(SqlStatement("SELECT AVG(SLOT0) as RESULT0, MAX(SLOT1) as RESULT1 FROM self", 2));
  }

  SECTION("Queries with expressions") {
    REQUIRE_NOTHROW(SqlStatement("SELECT SLOT0 * 2 as RESULT0 FROM self"));
    REQUIRE_NOTHROW(SqlStatement("SELECT SLOT0 + SLOT1 as RESULT0 FROM self"));
    REQUIRE_NOTHROW(SqlStatement("SELECT CASE WHEN SLOT0 > 0 THEN 1 ELSE 0 END as RESULT0 FROM self"));
  }

  SECTION("Queries with GROUP BY") {
    REQUIRE_NOTHROW(SqlStatement("SELECT SLOT0, COUNT(*) as RESULT0 FROM self GROUP BY SLOT0"));
  }

  SECTION("Queries with ORDER BY") {
    REQUIRE_NOTHROW(SqlStatement("SELECT SLOT0 as RESULT0 FROM self ORDER BY SLOT0"));
  }

  SECTION("Queries with LIMIT") {
    REQUIRE_NOTHROW(SqlStatement("SELECT SLOT0 as RESULT0 FROM self LIMIT 10"));
  }
}

TEST_CASE("SqlStatement - Getter methods", "[SqlStatement]") {
  SECTION("GetSql returns the SQL string") {
    std::string sql = "SELECT SLOT0 FROM self";
    SqlStatement stmt(sql);
    REQUIRE(stmt.GetSql() == sql);
  }

  SECTION("GetNumOutputs returns the number of outputs") {
    SqlStatement stmt1("SELECT SLOT0 as RESULT0 FROM self");
    REQUIRE(stmt1.GetNumOutputs() == 0);  // Not specified

    SqlStatement stmt2("SELECT SLOT0 as RESULT0, SLOT1 as RESULT1 FROM self", 2);
    REQUIRE(stmt2.GetNumOutputs() == 2);
  }
}

TEST_CASE("SqlStatement - Equality comparison", "[SqlStatement]") {
  SECTION("Equal statements") {
    SqlStatement stmt1("SELECT SLOT0 FROM self");
    SqlStatement stmt2("SELECT SLOT0 FROM self");
    REQUIRE(stmt1 == stmt2);
  }

  SECTION("Different SQL strings") {
    SqlStatement stmt1("SELECT SLOT0 FROM self");
    SqlStatement stmt2("SELECT SLOT1 FROM self");
    REQUIRE_FALSE(stmt1 == stmt2);
  }

  SECTION("Different numOutputs") {
    SqlStatement stmt1("SELECT SLOT0 as RESULT0, SLOT1 as RESULT1 FROM self", 2);
    SqlStatement stmt2("SELECT SLOT0 as RESULT0, SLOT1 as RESULT1 FROM self", 0);  // 0 means not specified
    REQUIRE_FALSE(stmt1 == stmt2);
  }
}

TEST_CASE("SqlStatement - Default constructor", "[SqlStatement]") {
  SECTION("Default constructed SqlStatement is empty") {
    SqlStatement stmt;
    REQUIRE(stmt.GetSql().empty());
    REQUIRE(stmt.GetNumOutputs() == 0);
  }
}

TEST_CASE("SqlStatement - SetSql method", "[SqlStatement]") {
  SECTION("SetSql validates the new SQL") {
    SqlStatement stmt;
    REQUIRE_NOTHROW(stmt.SetSql("SELECT SLOT0 as RESULT0 FROM self"));
    REQUIRE(stmt.GetSql() == "SELECT SLOT0 as RESULT0 FROM self");
  }

  SECTION("SetSql rejects invalid SQL") {
    SqlStatement stmt;
    REQUIRE_THROWS_AS(stmt.SetSql("INVALID SQL"), std::runtime_error);
  }
}

TEST_CASE("SqlStatement - Glaze JSON serialization", "[SqlStatement]") {
  SECTION("Serialize and deserialize SQL statement") {
    SqlStatement original("SELECT SLOT0 as RESULT0 FROM self");
    std::string json = glz::write_json(original).value_or("");
    REQUIRE_FALSE(json.empty());

    SqlStatement deserialized;
    auto result = glz::read_json(deserialized, json);
    REQUIRE_FALSE(result);
    REQUIRE(deserialized.GetSql() == original.GetSql());
  }

  SECTION("Serialization format is plain string") {
    SqlStatement stmt("SELECT SLOT0 as RESULT0 FROM self");
    std::string json = glz::write_json(stmt).value_or("");
    // Should serialize as plain string, not as object
    REQUIRE(json.find("SELECT SLOT0 as RESULT0 FROM self") != std::string::npos);
  }

  SECTION("Invalid SQL on deserialization") {
    std::string invalidJson = R"("SELECT * FROM wrong_table")";
    SqlStatement stmt;
    // Deserialization will throw because validation happens in SetSql
    REQUIRE_NOTHROW(glz::read_json(stmt, invalidJson));
    REQUIRE_THROWS_AS(stmt.Validate(), std::runtime_error);

  }
}

TEST_CASE("SqlStatement - Edge cases", "[SqlStatement]") {
  SECTION("SQL with comments") {
    REQUIRE_NOTHROW(SqlStatement("-- Comment\nSELECT SLOT0 FROM self"));
    REQUIRE_NOTHROW(SqlStatement("SELECT SLOT0 FROM self -- inline comment"));
  }

  SECTION("SQL with extra whitespace") {
    REQUIRE_NOTHROW(SqlStatement("  SELECT   SLOT0   FROM   self  "));
  }

  SECTION("Case insensitive keywords") {
    REQUIRE_NOTHROW(SqlStatement("select slot0 from self"));
    REQUIRE_NOTHROW(SqlStatement("SeLeCt SlOt0 FrOm SeLf"));
  }

  SECTION("Subqueries") {
    REQUIRE_NOTHROW(SqlStatement(
        "SELECT * FROM (SELECT SLOT0 as RESULT0 FROM self) sub"));
  }
}
