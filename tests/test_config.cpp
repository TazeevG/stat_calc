#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <cstdio>
#include "../src/config_parser.hpp"

namespace fs = boost::filesystem;

TEST_CASE("Config: чтение корректного TOML", "[config]") {
    
    char tmpname[] = "/tmp/config_test_XXXXXX";
    int fd = mkstemp(tmpname);
    close(fd);

    std::ofstream(tmpname) << R"(
        [main]
        input = "/data/in"
        output = "/data/out"
        filename_mask = ["level", "trade"]
    )";

    Config cfg(tmpname);
    REQUIRE(cfg.getInputDir() == "/data/in");
    REQUIRE(cfg.getOutputDir() == "/data/out");
    REQUIRE(cfg.getFilenameMasks() == std::vector<std::string>({"level", "trade"}));

    std::remove(tmpname);
}

TEST_CASE("Config: отсутствует обязательный input", "[config]") {
    char tmpname[] = "/tmp/config_test_XXXXXX";
    int fd = mkstemp(tmpname);
    close(fd);

    std::ofstream(tmpname) << "[main]\noutput = '/out'\n";

    REQUIRE_THROWS_AS(Config(tmpname), std::runtime_error);

    std::remove(tmpname);
}

TEST_CASE("Config: неверный тип input", "[config]") {
    char tmpname[] = "/tmp/config_test_XXXXXX";
    int fd = mkstemp(tmpname);
    close(fd);

    std::ofstream(tmpname) << "[main]\ninput = 123\n";

    REQUIRE_THROWS_AS(Config(tmpname), std::runtime_error);

    std::remove(tmpname);
}

TEST_CASE("Config: отсутствует секция main", "[config]") {
    char tmpname[] = "/tmp/config_test_XXXXXX";
    int fd = mkstemp(tmpname);
    close(fd);

    std::ofstream(tmpname) << "[other]\nkey = 'value'\n";

    REQUIRE_THROWS_AS(Config(tmpname), std::runtime_error);

    std::remove(tmpname);
}

TEST_CASE("Config: output по умолчанию", "[config]") {
    char tmpname[] = "/tmp/config_test_XXXXXX";
    int fd = mkstemp(tmpname);
    close(fd);

    std::ofstream(tmpname) << "[main]\ninput = '/in'\n";

    Config cfg(tmpname);
    REQUIRE(cfg.getOutputDir() == fs::current_path() / "output");

    std::remove(tmpname);
}

TEST_CASE("Config: filename_mask как строка", "[config]") {
    char tmpname[] = "/tmp/config_test_XXXXXX";
    int fd = mkstemp(tmpname);
    close(fd);

    std::ofstream(tmpname) << "[main]\ninput = '/in'\nfilename_mask = 'level'\n";

    Config cfg(tmpname);
    REQUIRE(cfg.getFilenameMasks() == std::vector<std::string>({"level"}));

    std::remove(tmpname);
}