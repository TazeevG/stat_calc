#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include "../src/csv_reader.hpp"

namespace fs = boost::filesystem;

TEST_CASE("Интеграционный тест: два файла с известными данными", "[integration]") {
    fs::path test_dir = fs::temp_directory_path() / "integ_test";
    fs::create_directories(test_dir);

    std::ofstream(test_dir / "level.csv") <<
        "receive_ts;exchange_ts;price;quantity;side;rebuild\n"
        "100;100;10.0;1;bid;0\n"
        "300;300;30.0;1;ask;0\n";

    std::ofstream(test_dir / "trade.csv") <<
        "receive_ts;exchange_ts;price;quantity;side\n"
        "200;200;20.0;1;bid\n"
        "400;400;40.0;1;ask\n";

    fs::path out_file = test_dir / "median_result.csv";

    CSVReader reader;
    reader.processStreaming(test_dir, {"level", "trade"}, out_file);

    std::ifstream result(out_file.string());
    std::string line;

    std::getline(result, line);
    REQUIRE(line == "receive_ts;median;mean;stddev;p90;p95;p99");

    std::getline(result, line);
    auto cols = splitHeader(line);
    REQUIRE(cols[0] == "100");
    REQUIRE(cols[1] == formatDouble(10.0));

    std::getline(result, line);
    cols = splitHeader(line);
    REQUIRE(cols[0] == "200");
    REQUIRE(cols[1] == formatDouble(15.0));

    std::getline(result, line);
    cols = splitHeader(line);
    REQUIRE(cols[0] == "300");
    REQUIRE(cols[1] == formatDouble(20.0));

    std::getline(result, line);
    cols = splitHeader(line);
    REQUIRE(cols[0] == "400");
    REQUIRE(cols[1] == formatDouble(25.0));

    REQUIRE(result.peek() == EOF);

    fs::remove_all(test_dir);
}