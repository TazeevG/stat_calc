#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../src/csv_reader.hpp" 

using Catch::Approx;

TEST_CASE("StatsCalculator: точный расчёт (до порога)", "[stats]") {
    StatsCalculator calc(100);
    Stats stats;

    SECTION("Пустой набор – первое добавление") {
        bool changed = calc.add(42.0, stats);
        REQUIRE(changed == true);
        REQUIRE(stats.median == 42.0);
        REQUIRE(stats.mean == 42.0);
        REQUIRE(stats.stddev == 0.0);
        REQUIRE(stats.p90 == 42.0);
        REQUIRE(stats.p95 == 42.0);
        REQUIRE(stats.p99 == 42.0);
    }

    SECTION("Два значения") {
        calc.add(10.0, stats);
        calc.add(20.0, stats);
        REQUIRE(stats.median == 15.0);
        REQUIRE(stats.mean == 15.0);
        REQUIRE(stats.stddev == Approx(7.0710678));
    }

    SECTION("Нечётное количество") {
        std::vector<double> data = {5, 1, 3, 2, 4};
        for (double v : data) calc.add(v, stats);
        REQUIRE(stats.median == 3.0);
        REQUIRE(stats.mean == 3.0);
        REQUIRE(stats.stddev == Approx(1.5811388));
    }

    SECTION("Чётное количество") {
        std::vector<double> data = {1, 2, 3, 4, 5, 6};
        for (double v : data) calc.add(v, stats);
        REQUIRE(stats.median == 3.5);
        REQUIRE(stats.mean == 3.5);
    }

    SECTION("Квантили для 1..10") {
        std::vector<double> data = {1,2,3,4,5,6,7,8,9,10};
        for (double v : data) calc.add(v, stats);
        REQUIRE(stats.p90 == Approx(9.1));
        REQUIRE(stats.p95 == Approx(9.55));
        REQUIRE(stats.p99 == Approx(9.91));
    }

    SECTION("Отрицательные числа") {
        std::vector<double> data = {-10, -5, 0, 5, 10};
        for (double v : data) calc.add(v, stats);
        REQUIRE(stats.median == 0.0);
        REQUIRE(stats.mean == 0.0);
    }
}

TEST_CASE("StatsCalculator: add возвращает true только при изменении медианы", "[stats]") {
    StatsCalculator calc;
    Stats stats;
    bool changed;

    changed = calc.add(100, stats);
    REQUIRE(changed == true);
    
    changed = calc.add(200, stats);
    REQUIRE(changed == true);

    changed = calc.add(300, stats);
    REQUIRE(changed == true);

    changed = calc.add(200, stats);
    REQUIRE(changed == false);
}