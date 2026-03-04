#pragma once

#include <vector>
#include <algorithm>
#include <cmath>
#include <optional>
#include <array>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <spdlog/spdlog.h>

namespace ba = boost::accumulators;

struct Stats {
    double median;
    double mean;
    double stddev;
    double p90;
    double p95;
    double p99;
};

class StatsCalculator {
public:
    static constexpr std::array<double, 4> probs = {0.5, 0.9, 0.95, 0.99};

    explicit StatsCalculator(size_t switch_threshold = 100);
    
    bool add(double value, Stats& out_stats);

private:
    size_t threshold_;
    bool use_boost_;
    size_t total_added_;
    std::vector<double> buffer_;
    double sum_;
    double sum_sq_;
    
    using Accumulator = ba::accumulator_set<double, 
                          ba::features<ba::tag::mean,
                                       ba::tag::variance,
                                       ba::tag::extended_p_square>>;
    
    std::optional<Accumulator> acc_;
    double prev_median_;

    void switchToBoost();
    Stats computeExact() const;
    Stats computeBoost() const;
};