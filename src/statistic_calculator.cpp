#include "statistic_calculator.hpp"

StatsCalculator::StatsCalculator(size_t switch_threshold)
    : threshold_(switch_threshold)
    , use_boost_(false)
    , total_added_(0)
    , sum_(0.0)
    , sum_sq_(0.0)
    , prev_median_(0.0) {
    buffer_.reserve(threshold_);
    spdlog::debug("StatsCalculator создан с порогом {}", threshold_);
}

bool StatsCalculator::add(double value, Stats& out_stats) {
    ++total_added_;

    if (!use_boost_) {
        buffer_.push_back(value);
        sum_ += value;
        sum_sq_ += value * value;

        if (buffer_.size() >= threshold_) {
            spdlog::info("Достигнут порог {} значений. Переключение на Boost.Accumulators", threshold_);
            switchToBoost();
        }

        out_stats = computeExact();
    } else {
        (*acc_)(value);
        out_stats = computeBoost();
    }

    if (std::abs(out_stats.median - prev_median_) > 1e-10) {
        prev_median_ = out_stats.median;
        return true;
    }
    return false;
}

void StatsCalculator::switchToBoost() {
    std::vector<double> probs_vec(probs.begin(), probs.end());
    acc_.emplace(ba::tag::extended_p_square::probabilities = probs_vec);
    
    for (double v : buffer_) {
        (*acc_)(v);
    }
    
    use_boost_ = true;
    buffer_.clear();
    buffer_.shrink_to_fit();
}

Stats StatsCalculator::computeExact() const {
    Stats s{};
    size_t n = buffer_.size();
    if (n == 0) return s;
    
    s.mean = sum_ / n;
    double variance = (sum_sq_ - sum_ * sum_ / n) / (n - 1);
    s.stddev = (n < 2) ? 0.0 : std::sqrt(variance);

    std::vector<double> sorted = buffer_;
    std::sort(sorted.begin(), sorted.end());

    auto quantile = [&](double p) -> double {
        if (n == 1) return sorted[0];
        double pos = p * (n - 1);
        size_t idx = static_cast<size_t>(pos);
        double frac = pos - idx;
        if (idx + 1 < n) {
            return sorted[idx] * (1 - frac) + sorted[idx + 1] * frac;
        } else {
            return sorted[idx];
        }
    };

    s.median = quantile(0.5);
    s.p90 = quantile(0.9);
    s.p95 = quantile(0.95);
    s.p99 = quantile(0.99);

    return s;
}

Stats StatsCalculator::computeBoost() const {
    Stats s;
    s.mean = ba::mean(*acc_);
    s.stddev = std::sqrt(ba::variance(*acc_));

    const auto& quantiles = ba::extended_p_square(*acc_);
    s.median = quantiles[0];
    s.p90   = quantiles[1];
    s.p95   = quantiles[2];
    s.p99   = quantiles[3];

    return s;
}