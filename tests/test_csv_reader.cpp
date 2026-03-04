#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include "../src/csv_reader.hpp"
#include <catch2/catch_approx.hpp>
using Catch::Approx;

namespace fs = boost::filesystem;

// Вспомогательная функция для создания временного файла с содержимым
fs::path createTempFile(const std::string& content) {
    fs::path tmp = fs::temp_directory_path() / fs::unique_path();
    std::ofstream(tmp.string()) << content;
    return tmp;
}

TEST_CASE("CSVReader::findFiles фильтрация по маскам", "[csv]") {
    // Создаём временную структуру
    fs::path test_dir = fs::temp_directory_path() / "csv_test";
    fs::create_directories(test_dir);
    fs::create_directories(test_dir / "sub");

    std::vector<std::string> files = {
        "btcusdt_level_2024.csv",
        "btcusdt_trade_2024.csv",
        "ethusdt_level.csv",
        "ethusdt_trade.csv",
        "other.csv",
        "sub/level_extra.csv"
    };
    for (const auto& f : files) {
        std::ofstream(test_dir / f).close();
    }

    CSVReader reader;

    SECTION("Пустая маска – все CSV") {
        auto result = reader.findFiles(test_dir, {}, ".csv");
        REQUIRE(result.size() == 6);
    }

    SECTION("Одна маска 'level'") {
        auto result = reader.findFiles(test_dir, {"level"}, ".csv");
        REQUIRE(result.size() == 3); // level_2024, ethusdt_level, sub/level_extra
    }

    SECTION("Несколько масок") {
        auto result = reader.findFiles(test_dir, {"level", "trade"}, ".csv");
        REQUIRE(result.size() == 5); // все кроме other.csv
    }

    SECTION("Регистронезависимость (если не Linux, то через tolower)") {
        auto result = reader.findFiles(test_dir, {"LEVEL"}, ".csv");
        REQUIRE(result.size() == 3);
    }

    SECTION("Расширение .csv уже в маске") {
        auto result = reader.findFiles(test_dir, {"level.csv"}, ".csv");
        REQUIRE(result.size() == 3);
    }

    fs::remove_all(test_dir);
}

TEST_CASE("CSVReader::initFile успешная инициализация", "[csv]") {
    auto file = createTempFile(
        "receive_ts;exchange_ts;price;quantity;side;rebuild\n"
        "1716810808593627;1716810808574000;68480.10000000;10.10900000;bid;1\n"
        "1716810808663260;1716810808661000;68480.20000000;0.01100000;ask;0\n"
    );

    CSVReader reader;
    auto state = reader.initFile(file);
    REQUIRE(state != nullptr);
    REQUIRE(state->has_data == true);
    REQUIRE(state->current_event.timestamp == 1716810808593627ULL);
    REQUIRE(state->current_event.price == Approx(68480.1));

    fs::remove(file);
}

TEST_CASE("CSVReader::initFile ошибки", "[csv]") {
    CSVReader reader;

    SECTION("Неверный заголовок") {
        auto file = createTempFile("wrong;columns\n1;2\n");
        auto state = reader.initFile(file);
        REQUIRE(state == nullptr);
        fs::remove(file);
    }

    SECTION("Нет колонки price") {
        auto file = createTempFile("receive_ts;quantity\n1;2\n");
        auto state = reader.initFile(file);
        REQUIRE(state == nullptr);
        fs::remove(file);
    }

    SECTION("Пустой файл") {
        auto file = createTempFile("");
        auto state = reader.initFile(file);
        REQUIRE(state == nullptr);
        fs::remove(file);
    }

    SECTION("Битая первая строка данных") {
        auto file = createTempFile("receive_ts;price\nnot_a_number;100\n");
        auto state = reader.initFile(file);
        REQUIRE(state == nullptr);
        fs::remove(file);
    }

    SECTION("Недостаточно колонок в первой строке") {
        auto file = createTempFile("receive_ts;price\n12345\n");
        auto state = reader.initFile(file);
        REQUIRE(state == nullptr);
        fs::remove(file);
    }
}

TEST_CASE("CSVReader::FileState::parseNextLine", "[csv]") {
    auto file = createTempFile(
        "receive_ts;price\n"
        "100;10.0\n"
        "200;20.0\n"
        "invalid;30.0\n"
    );

    CSVReader reader;
    auto state = reader.initFile(file);
    REQUIRE(state != nullptr);
    REQUIRE(state->current_event.timestamp == 100);
    REQUIRE(state->current_event.price == 10.0);

    SECTION("Чтение следующей корректной строки") {
        bool ok = state->parseNextLine();
        REQUIRE(ok == true);
        REQUIRE(state->current_event.timestamp == 200);
        REQUIRE(state->current_event.price == 20.0);
    }

    SECTION("Ошибка при чтении битой строки") {
        state->parseNextLine();
        bool ok = state->parseNextLine();
        REQUIRE(ok == false);
        REQUIRE(state->has_data == false);
    }

    fs::remove(file);
}