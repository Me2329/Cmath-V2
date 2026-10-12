#ifndef CMATH_V2_HPP_INCLUDED
#define CMATH_V2_HPP_INCLUDED

#if __cplusplus < 202002L
#error "cmath_v2 requires C++20 or newer. Compile with -std=c++20."
#endif

// C++20, header-only. Include "cmath_v2.hpp" and call math::function(...).
// Standard functions retain std::cmath behavior. Extra helpers document and
// check their own domains. Calculations return double unless stated otherwise.

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <limits>
#include <numbers>
#include <numeric>
#include <ranges>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace math {

inline constexpr char version[] = "2.2.0";
inline constexpr double pi = std::numbers::pi;
inline constexpr double tau = 2.0 * pi;
inline constexpr double e = std::numbers::e;
inline constexpr double phi = std::numbers::phi;

// Discrete operations accept whole-number types, never silently truncated floats.
template <typename T>
concept Integer = std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>;

// Familiar <cmath> functions, available through the same namespace.
using std::abs;
using std::acos;
using std::acosh;
using std::asin;
using std::asinh;
using std::atan;
using std::atan2;
using std::atanh;
using std::cbrt;
using std::ceil;
using std::copysign;
using std::cos;
using std::cosh;
using std::erf;
using std::erfc;
using std::exp;
using std::exp2;
using std::expm1;
using std::floor;
using std::fma;
using std::fmod;
using std::hypot;
using std::isfinite;
using std::isinf;
using std::isnan;
using std::lerp;
using std::lgamma;
using std::log;
using std::log10;
using std::log1p;
using std::log2;
using std::pow;
using std::remainder;
using std::round;
using std::signbit;
using std::sin;
using std::sinh;
using std::sqrt;
using std::tan;
using std::tanh;
using std::tgamma;
using std::trunc;

namespace detail {
inline void require_finite(double value, const char *message) {
    if (!std::isfinite(value)) {
        throw std::domain_error(message);
    }
}

inline double finite_result(long double value, const char *message) {
    if (!std::isfinite(value) || std::abs(value) > std::numeric_limits<double>::max()) {
        throw std::overflow_error(message);
    }
    return static_cast<double>(value);
}

template <Integer T> std::uint64_t nonnegative_count(T value, const char *message) {
    if constexpr (std::is_signed_v<T>) {
        if (value < 0)
            throw std::domain_error(message);
    }
    return static_cast<std::uint64_t>(value);
}
} // namespace detail

// Like ordinary floating-point multiplication, square/cube can produce infinity.
constexpr double square(double value) noexcept { return value * value; }
constexpr double cube(double value) noexcept { return value * value * value; }

inline double clamp(double value, double low, double high) {
    detail::require_finite(value, "clamp: value must be finite");
    detail::require_finite(low, "clamp: bounds must be finite");
    detail::require_finite(high, "clamp: bounds must be finite");
    if (low > high) {
        throw std::invalid_argument("clamp: lower bound exceeds upper bound");
    }
    return std::clamp(value, low, high);
}

// Absolute tolerance handles values near zero; relative tolerance handles scale.
// Equal infinities compare true; NaNs compare false.
inline bool approx(double a, double b, double relative_tolerance = 1e-9,
                   double absolute_tolerance = 1e-12) {
    if (!std::isfinite(relative_tolerance) || relative_tolerance < 0 ||
        !std::isfinite(absolute_tolerance) || absolute_tolerance < 0) {
        throw std::invalid_argument("approx: tolerances must be finite and nonnegative");
    }
    if (a == b)
        return true;
    if (!std::isfinite(a) || !std::isfinite(b))
        return false;
    const long double difference = std::abs(static_cast<long double>(a) - b);
    const long double scale = std::max(std::abs(a), std::abs(b));
    return difference <=
           std::max(static_cast<long double>(absolute_tolerance), relative_tolerance * scale);
}

constexpr double radians(double angle_degrees) noexcept { return angle_degrees * (pi / 180.0); }
constexpr double degrees(double angle_radians) noexcept { return angle_radians * (180.0 / pi); }
inline double sin_deg(double angle) {
    detail::require_finite(angle, "sin_deg: angle must be finite");
    const double reduced = std::remainder(angle, 360.0);
    if (reduced == 0 || std::abs(reduced) == 180)
        return std::copysign(0.0, reduced);
    if (reduced == 90)
        return 1;
    if (reduced == -90)
        return -1;
    return std::sin(radians(reduced));
}
inline double cos_deg(double angle) {
    detail::require_finite(angle, "cos_deg: angle must be finite");
    const double reduced = std::remainder(angle, 360.0);
    if (reduced == 0)
        return 1;
    if (std::abs(reduced) == 180)
        return -1;
    if (std::abs(reduced) == 90)
        return 0;
    return std::cos(radians(reduced));
}
inline double tan_deg(double angle) {
    detail::require_finite(angle, "tan_deg: angle must be finite");
    const double reduced = std::remainder(angle, 180.0);
    if (std::abs(reduced) == 90) {
        throw std::domain_error("tan_deg: tangent is undefined at odd multiples of 90 degrees");
    }
    return std::tan(radians(reduced));
}
inline double asin_deg(double value) {
    detail::require_finite(value, "asin_deg: value must be finite");
    if (value < -1 || value > 1)
        throw std::domain_error("asin_deg: value must be in [-1, 1]");
    return degrees(std::asin(value));
}
inline double acos_deg(double value) {
    detail::require_finite(value, "acos_deg: value must be finite");
    if (value < -1 || value > 1)
        throw std::domain_error("acos_deg: value must be in [-1, 1]");
    return degrees(std::acos(value));
}
inline double atan_deg(double value) {
    detail::require_finite(value, "atan_deg: value must be finite");
    return degrees(std::atan(value));
}
// Argument order follows atan2: y first, x second. Output is in [-180, 180].
inline double atan2_deg(double y, double x) {
    detail::require_finite(y, "atan2_deg: coordinates must be finite");
    detail::require_finite(x, "atan2_deg: coordinates must be finite");
    if (x == 0 && y == 0)
        throw std::domain_error("atan2_deg: the origin has no direction");
    return degrees(std::atan2(y, x));
}

inline double log_base(double value, double base) {
    detail::require_finite(value, "log_base: value must be finite");
    detail::require_finite(base, "log_base: base must be finite");
    if (value <= 0 || base <= 0 || base == 1) {
        throw std::domain_error("log_base: value and base must be positive, and base cannot be 1");
    }
    return detail::finite_result(std::log(static_cast<long double>(value)) /
                                     std::log(static_cast<long double>(base)),
                                 "log_base: result exceeds double range");
}

// percent(25, 200) = 12.5; percent_of(12.5, 200) = 25.
inline double percent(double part, double total) {
    detail::require_finite(part, "percent: values must be finite");
    detail::require_finite(total, "percent: values must be finite");
    if (total == 0)
        throw std::domain_error("percent: total cannot be zero");
    return detail::finite_result(100.0L * part / total, "percent: result exceeds double range");
}
inline double percent_of(double percentage, double value) {
    detail::require_finite(percentage, "percent_of: values must be finite");
    detail::require_finite(value, "percent_of: values must be finite");
    return detail::finite_result(static_cast<long double>(percentage) * value / 100.0L,
                                 "percent_of: result exceeds double range");
}

// Real nth root; negative degrees mean the reciprocal root.
// Even roots of negative numbers and the zeroth root are undefined here.
template <Integer Degree> double nth_root(double value, Degree degree) {
    detail::require_finite(value, "nth_root: value must be finite");
    if (degree == 0)
        throw std::domain_error("nth_root: degree cannot be zero");
    bool reciprocal = false;
    if constexpr (std::is_signed_v<Degree>)
        reciprocal = degree < 0;
    // Unsigned arithmetic safely handles even the most negative signed integer.
    using Unsigned = std::make_unsigned_t<Degree>;
    const auto raw = static_cast<Unsigned>(degree);
    const auto magnitude = reciprocal ? static_cast<Unsigned>(Unsigned{0} - raw) : raw;
    if (value < 0 && magnitude % 2 == 0) {
        throw std::domain_error("nth_root: an even root of a negative value is not real");
    }
    if (value == 0 && reciprocal) {
        throw std::domain_error("nth_root: a negative-degree root of zero is undefined");
    }
    const long double exponent = 1.0L / static_cast<long double>(magnitude);
    long double root = std::pow(std::abs(static_cast<long double>(value)), exponent);
    if (std::signbit(value))
        root = -root;
    if (reciprocal)
        root = 1.0L / root;
    return detail::finite_result(root, "nth_root: result exceeds double range");
}

// Exact integer arithmetic, with overflow checks.
template <Integer Count> std::uint64_t factorial(Count n) {
    const auto count = detail::nonnegative_count(n, "factorial: n cannot be negative");
    if (count > 20)
        throw std::overflow_error("factorial: uint64_t supports only 0! through 20!");
    std::uint64_t result = 1;
    for (std::uint64_t i = 2; i <= count; ++i)
        result *= i;
    return result;
}

template <Integer N, Integer K> std::uint64_t binomial(N input_n, K input_k) {
    const auto n = detail::nonnegative_count(input_n, "binomial: n cannot be negative");
    auto k = detail::nonnegative_count(input_k, "binomial: k cannot be negative");
    if (k > n)
        throw std::domain_error("binomial: k cannot exceed n");
    k = std::min(k, n - k);
    std::uint64_t result = 1;
    for (std::uint64_t i = 1; i <= k; ++i) {
        std::uint64_t numerator = n - k + i;
        std::uint64_t denominator = i;
        const auto common = std::gcd(numerator, denominator);
        numerator /= common;
        denominator /= common;
        const auto remaining = std::gcd(result, denominator);
        result /= remaining;
        denominator /= remaining;
        // Integrality of n-choose-k makes the remaining denominator exactly one.
        if (denominator != 1 || result > std::numeric_limits<std::uint64_t>::max() / numerator) {
            throw std::overflow_error("binomial: result exceeds uint64_t range");
        }
        result *= numerator;
    }
    return result;
}

// Statistics accept const-iterable arithmetic containers: arrays, vectors,
// spans, and initializer lists. Empty/nonfinite data is rejected, except sum({})=0.
template <typename Range>
concept NumericRange = std::ranges::input_range<const Range> &&
                       std::is_arithmetic_v<std::ranges::range_value_t<const Range>>;

namespace detail {
struct Statistics {
    std::size_t count = 0;
    long double total = 0;
    long double correction = 0;
    long double origin = 0;
    long double running_mean = 0;
    long double m2 = 0;
    double minimum = std::numeric_limits<double>::infinity();
    double maximum = -std::numeric_limits<double>::infinity();
};

template <bool Moments = true, NumericRange Range>
Statistics summarize(const Range &values, std::vector<long double> *copy = nullptr) {
    Statistics result;
    for (const auto item : values) {
        const long double value = static_cast<long double>(item);
        if (!std::isfinite(value) || std::abs(value) > std::numeric_limits<double>::max()) {
            throw std::domain_error("statistics: each value must be finite and fit in double");
        }
        if (copy != nullptr)
            copy->push_back(value);
        // Neumaier compensated sum and Welford's online second moment.
        const long double next = result.total + value;
        if (std::abs(result.total) >= std::abs(value)) {
            result.correction += (result.total - next) + value;
        } else {
            result.correction += (value - next) + result.total;
        }
        result.total = next;
        ++result.count;
        if constexpr (Moments) {
            // Subtract the first value before Welford updates so a large common
            // integer offset does not erase a small but meaningful variance.
            if (result.count == 1)
                result.origin = value;
            const long double centered = value - result.origin;
            const long double delta = centered - result.running_mean;
            result.running_mean += delta / result.count;
            result.m2 += delta * (centered - result.running_mean);
        }
        result.minimum = std::min(result.minimum, static_cast<double>(value));
        result.maximum = std::max(result.maximum, static_cast<double>(value));
    }
    return result;
}

inline void require_count(std::size_t count, std::size_t minimum) {
    if (count < minimum)
        throw std::domain_error("statistics: not enough data values");
}

template <NumericRange Range> std::vector<long double> sorted_values(const Range &values) {
    std::vector<long double> sorted;
    for (const auto value : values) {
        const long double number = static_cast<long double>(value);
        if (!std::isfinite(number) || std::abs(number) > std::numeric_limits<double>::max()) {
            throw std::domain_error("statistics: each value must be finite and fit in double");
        }
        sorted.push_back(number);
    }
    require_count(sorted.size(), 1);
    std::sort(sorted.begin(), sorted.end());
    return sorted;
}

// Linear interpolation at q*(n-1), with no rounding to double before interpolation.
inline double sorted_quantile(const std::vector<long double> &sorted, double q) {
    const long double position = static_cast<long double>(q) * (sorted.size() - 1);
    const auto lower = static_cast<std::size_t>(position);
    if (lower >= sorted.size() - 1)
        return static_cast<double>(sorted.back());
    return finite_result(std::lerp(sorted[lower], sorted[lower + 1], position - lower),
                         "quantile: result exceeds double range");
}
} // namespace detail

template <NumericRange Range> double sum(const Range &values) {
    const auto s = detail::summarize<false>(values);
    return detail::finite_result(s.total + s.correction, "sum: result exceeds double range");
}

template <NumericRange Range> double mean(const Range &values) {
    const auto s = detail::summarize<false>(values);
    detail::require_count(s.count, 1);
    return detail::finite_result((s.total + s.correction) / s.count,
                                 "mean: result exceeds double range");
}

template <NumericRange Range> double median(const Range &values) {
    const auto sorted = detail::sorted_values(values);
    const auto middle = sorted.size() / 2;
    return detail::finite_result(
        sorted.size() % 2 ? sorted[middle] : std::midpoint(sorted[middle - 1], sorted[middle]),
        "median: result exceeds double range");
}

// Population variance by default. Pass true for sample variance (n-1).
template <NumericRange Range> double variance(const Range &values, bool sample = false) {
    const auto s = detail::summarize(values);
    detail::require_count(s.count, sample ? 2 : 1);
    return detail::finite_result(std::max(0.0L, s.m2) / (s.count - (sample ? 1 : 0)),
                                 "variance: result exceeds double range");
}

template <NumericRange Range> double stddev(const Range &values, bool sample = false) {
    const auto s = detail::summarize(values);
    detail::require_count(s.count, sample ? 2 : 1);
    return detail::finite_result(std::sqrt(std::max(0.0L, s.m2) / (s.count - (sample ? 1 : 0))),
                                 "stddev: result exceeds double range");
}

template <NumericRange Range> double min(const Range &values) {
    const auto s = detail::summarize<false>(values);
    detail::require_count(s.count, 1);
    return s.minimum;
}

template <NumericRange Range> double max(const Range &values) {
    const auto s = detail::summarize<false>(values);
    detail::require_count(s.count, 1);
    return s.maximum;
}

inline double sum(std::initializer_list<double> v) { return sum<std::initializer_list<double>>(v); }
inline double mean(std::initializer_list<double> v) {
    return mean<std::initializer_list<double>>(v);
}
inline double median(std::initializer_list<double> v) {
    return median<std::initializer_list<double>>(v);
}
inline double min(std::initializer_list<double> v) { return min<std::initializer_list<double>>(v); }
inline double max(std::initializer_list<double> v) { return max<std::initializer_list<double>>(v); }
inline double variance(std::initializer_list<double> v, bool sample = false) {
    return variance<std::initializer_list<double>>(v, sample);
}
inline double stddev(std::initializer_list<double> v, bool sample = false) {
    return stddev<std::initializer_list<double>>(v, sample);
}

// q is a fraction from 0 to 1. Uses linear interpolation, not nearest-rank.
template <NumericRange Range> double quantile(const Range &values, double q) {
    if (!std::isfinite(q) || q < 0 || q > 1) {
        throw std::domain_error("quantile: q must be finite and in [0, 1]");
    }
    return detail::sorted_quantile(detail::sorted_values(values), q);
}
inline double quantile(std::initializer_list<double> values, double q) {
    return quantile<std::initializer_list<double>>(values, q);
}

// percentile(data, 50) is the median; percentile(data, 90) is the 90th percentile.
template <NumericRange Range> double percentile(const Range &values, double p) {
    if (!std::isfinite(p) || p < 0 || p > 100) {
        throw std::domain_error("percentile: p must be finite and in [0, 100]");
    }
    return quantile(values, p / 100.0);
}
inline double percentile(std::initializer_list<double> values, double p) {
    return percentile<std::initializer_list<double>>(values, p);
}

struct Summary {
    std::size_t count;
    double sum;
    double mean;
    double median;
    double min;
    double max;
    double variance;
    double stddev;
};

// Read the input once and return its statistics together. A sorted copy supports
// the median, so input stays unchanged. All returned fields must fit in double.
template <NumericRange Range> Summary describe(const Range &values, bool sample = false) {
    std::vector<long double> sorted;
    const auto s = detail::summarize(values, &sorted);
    detail::require_count(s.count, sample ? 2 : 1);
    std::sort(sorted.begin(), sorted.end());
    const long double total = s.total + s.correction;
    const long double variance_value = std::max(0.0L, s.m2) / (s.count - (sample ? 1 : 0));
    return {
        s.count,
        detail::finite_result(total, "describe: sum exceeds double range"),
        detail::finite_result(total / s.count, "describe: mean exceeds double range"),
        detail::sorted_quantile(sorted, 0.5),
        s.minimum,
        s.maximum,
        detail::finite_result(variance_value, "describe: variance exceeds double range"),
        detail::finite_result(std::sqrt(variance_value), "describe: stddev exceeds double range")};
}
inline Summary describe(std::initializer_list<double> values, bool sample = false) {
    return describe<std::initializer_list<double>>(values, sample);
}

// Solve a*x*x + b*x + c = 0. Returns two roots (including a repeated root),
// one root for a linear equation, or none for an inconsistent constant equation.
// The all-zero equation has infinitely many solutions and throws domain_error.
inline std::vector<std::complex<double>> solve_quadratic(double a, double b, double c) {
    detail::require_finite(a, "solve_quadratic: coefficients must be finite");
    detail::require_finite(b, "solve_quadratic: coefficients must be finite");
    detail::require_finite(c, "solve_quadratic: coefficients must be finite");
    const auto checked = [](long double value) {
        return detail::finite_result(value, "solve_quadratic: root exceeds double range");
    };
    if (a == 0) {
        if (b != 0)
            return {{checked(-static_cast<long double>(c) / b), 0.0}};
        if (c != 0)
            return {};
        throw std::domain_error("solve_quadratic: infinitely many solutions");
    }
    // Power-of-two scaling preserves the input coefficients exactly on binary
    // floating-point systems. Arbitrary division can turn a double root complex.
    int exponent = 0;
    std::frexp(std::max({std::abs(a), std::abs(b), std::abs(c)}), &exponent);
    const long double aa = std::scalbn(static_cast<long double>(a), -exponent);
    const long double bb = std::scalbn(static_cast<long double>(b), -exponent);
    const long double cc = std::scalbn(static_cast<long double>(c), -exponent);
    const long double product = (4.0L * aa) * cc;
    const long double correction = std::fma(-4.0L * aa, cc, product);
    const long double discriminant = std::fma(bb, bb, -product) + correction;
    if (discriminant < 0) {
        const double real = checked(-bb / (2 * aa));
        const double imaginary = checked(std::sqrt(-discriminant) / (2 * std::abs(aa)));
        return {{real, imaginary}, {real, -imaginary}};
    }
    if (discriminant == 0) {
        const double root = checked(-bb / (2 * aa));
        return {{root, 0.0}, {root, 0.0}};
    }
    // Avoid subtracting nearly equal terms in the usual quadratic formula.
    const long double q = -0.5L * (bb + std::copysign(std::sqrt(discriminant), bb));
    double first = checked(q / aa);
    double second = checked(cc / q);
    if (first > second)
        std::swap(first, second);
    return {{first, 0.0}, {second, 0.0}};
}

inline double circle_area(double radius) {
    detail::require_finite(radius, "circle_area: radius must be finite");
    if (radius < 0)
        throw std::domain_error("circle_area: radius cannot be negative");
    return detail::finite_result(std::numbers::pi_v<long double> * radius * radius,
                                 "circle_area: result exceeds double range");
}

inline double circle_circumference(double radius) {
    detail::require_finite(radius, "circle_circumference: radius must be finite");
    if (radius < 0)
        throw std::domain_error("circle_circumference: radius cannot be negative");
    return detail::finite_result(2 * std::numbers::pi_v<long double> * radius,
                                 "circle_circumference: result exceeds double range");
}

inline double triangle_area(double a, double b, double c) {
    detail::require_finite(a, "triangle_area: sides must be finite");
    detail::require_finite(b, "triangle_area: sides must be finite");
    detail::require_finite(c, "triangle_area: sides must be finite");
    std::array<long double, 3> sides{a, b, c};
    std::sort(sides.begin(), sides.end(), std::greater<>{});
    const auto [x, y, z] = sides;
    if (z <= 0 || z <= x - y) {
        throw std::domain_error("triangle_area: sides must form a nondegenerate triangle");
    }
    // Rearranged Heron's formula reduces cancellation for thin triangles.
    const long double product = (x + (y + z)) * (z - (x - y)) * (z + (x - y)) * (x + (y - z));
    return detail::finite_result(std::sqrt(product) / 4,
                                 "triangle_area: result exceeds double range");
}

inline double distance(double x1, double y1, double x2, double y2) {
    for (double value : {x1, y1, x2, y2}) {
        detail::require_finite(value, "distance: coordinates must be finite");
    }
    return detail::finite_result(
        std::hypot(static_cast<long double>(x2) - x1, static_cast<long double>(y2) - y1),
        "distance: result exceeds double range");
}

inline double rectangle_area(double width, double height) {
    detail::require_finite(width, "rectangle_area: dimensions must be finite");
    detail::require_finite(height, "rectangle_area: dimensions must be finite");
    if (width < 0 || height < 0)
        throw std::domain_error("rectangle_area: dimensions cannot be negative");
    return detail::finite_result(static_cast<long double>(width) * height,
                                 "rectangle_area: overflow");
}
inline double sphere_volume(double radius) {
    detail::require_finite(radius, "sphere_volume: radius must be finite");
    if (radius < 0)
        throw std::domain_error("sphere_volume: radius cannot be negative");
    return detail::finite_result((4.0L / 3.0L) * std::numbers::pi_v<long double> * radius * radius *
                                     radius,
                                 "sphere_volume: overflow");
}
inline double sphere_surface_area(double radius) {
    detail::require_finite(radius, "sphere_surface_area: radius must be finite");
    if (radius < 0)
        throw std::domain_error("sphere_surface_area: radius cannot be negative");
    return detail::finite_result(4.0L * std::numbers::pi_v<long double> * radius * radius,
                                 "sphere_surface_area: overflow");
}
inline double cylinder_volume(double radius, double height) {
    detail::require_finite(radius, "cylinder_volume: dimensions must be finite");
    detail::require_finite(height, "cylinder_volume: dimensions must be finite");
    if (radius < 0 || height < 0)
        throw std::domain_error("cylinder_volume: dimensions cannot be negative");
    return detail::finite_result(std::numbers::pi_v<long double> * radius * radius * height,
                                 "cylinder_volume: overflow");
}
struct LinearSolution2 {
    double x;
    double y;
};
// a*x+b*y=c and d*x+e*y=f. Singular systems have no unique solution and throw.
inline LinearSolution2 solve_linear2(double a, double b, double c, double d, double coefficient_e,
                                     double f) {
    for (const double v : {a, b, c, d, coefficient_e, f})
        detail::require_finite(v, "solve_linear2: coefficients must be finite");
    int first_exp = 0, second_exp = 0;
    std::frexp(std::max({std::abs(a), std::abs(b), std::abs(c)}), &first_exp);
    std::frexp(std::max({std::abs(d), std::abs(coefficient_e), std::abs(f)}), &second_exp);
    const long double aa = std::scalbn(static_cast<long double>(a), -first_exp),
                      bb = std::scalbn(static_cast<long double>(b), -first_exp),
                      cc = std::scalbn(static_cast<long double>(c), -first_exp);
    const long double dd = std::scalbn(static_cast<long double>(d), -second_exp),
                      ee = std::scalbn(static_cast<long double>(coefficient_e), -second_exp),
                      ff = std::scalbn(static_cast<long double>(f), -second_exp);
    const auto difference = [](long double x, long double y, long double z, long double w) {
        const long double p = z * w;
        return std::fma(x, y, -p) + std::fma(-z, w, p);
    };
    const long double determinant = difference(aa, ee, bb, dd);
    if (determinant == 0)
        throw std::domain_error("solve_linear2: system has no unique solution");
    return {detail::finite_result(difference(cc, ee, bb, ff) / determinant,
                                  "solve_linear2: x overflow"),
            detail::finite_result(difference(aa, ff, cc, dd) / determinant,
                                  "solve_linear2: y overflow")};
}

/// Estimate f'(x) with a central difference. A zero step selects
/// cbrt(machine epsilon) * max(1, |x|); a supplied step must be positive.
/// Uses the actual floating-point sample spacing and wider intermediates.
/// Requires finite arguments and callback values, and two distinct finite
/// samples on opposite sides of x. Accuracy depends on the function and step;
/// this is a numerical estimate, not symbolic differentiation.
template <class F> [[nodiscard]] double derivative(F &&f, double x, double step = 0.0) {
    if (!std::isfinite(x) || !std::isfinite(step) || step < 0.0) {
        throw std::invalid_argument("derivative: x must be finite and step finite and nonnegative");
    }
    if (step == 0.0) {
        step = std::cbrt(std::numeric_limits<double>::epsilon()) * std::max(1.0, std::abs(x));
    }
    const double left = x - step;
    const double right = x + step;
    if (!std::isfinite(left) || !std::isfinite(right) || !(left < x && x < right)) {
        throw std::invalid_argument(
            "derivative: step must produce distinct finite samples around x");
    }
    const double left_value = static_cast<double>(std::invoke(f, left));
    if (!std::isfinite(left_value)) {
        throw std::domain_error("derivative: callback returned a nonfinite value");
    }
    const double right_value = static_cast<double>(std::invoke(f, right));
    if (!std::isfinite(right_value)) {
        throw std::domain_error("derivative: callback returned a nonfinite value");
    }
    const long double slope = (static_cast<long double>(right_value) - left_value) /
                              (static_cast<long double>(right) - left);
    if (!std::isfinite(slope) || std::abs(slope) > std::numeric_limits<double>::max()) {
        throw std::overflow_error("derivative: result is outside the finite double range");
    }
    return static_cast<double>(slope);
}

/// Approximate the definite integral using composite Simpson's rule.
/// intervals must be even and at least 2. Reversed bounds negate the result;
/// equal finite bounds return zero without invoking f. Requires finite bounds
/// and callback values. Uses compensated long-double accumulation and stable
/// interpolation of sample positions. Very close bounds may share rounded
/// sample positions. Smooth functions give the best accuracy; no adaptive
/// error estimate is provided. Results too small for double may underflow.
template <class F>
[[nodiscard]] double integrate(F &&f, double a, double b, std::size_t intervals = 1000) {
    if (!std::isfinite(a) || !std::isfinite(b)) {
        throw std::invalid_argument("integrate: bounds must be finite");
    }
    if (intervals < 2 || intervals % 2 != 0) {
        throw std::invalid_argument("integrate: intervals must be even and at least 2");
    }
    if (a == b) {
        return 0.0;
    }
    const bool reversed = b < a;
    if (reversed) {
        std::swap(a, b);
    }
    long double sum = 0.0L;
    long double compensation = 0.0L;
    const auto add_sample = [&](double x, long double weight) {
        const double value = static_cast<double>(std::invoke(f, x));
        if (!std::isfinite(value)) {
            throw std::domain_error("integrate: callback returned a nonfinite value");
        }
        // Neumaier compensation also handles a term larger than the running sum.
        const long double term = weight * static_cast<long double>(value);
        const long double next = sum + term;
        if (std::abs(sum) >= std::abs(term)) {
            compensation += (sum - next) + term;
        } else {
            compensation += (term - next) + sum;
        }
        sum = next;
    };
    add_sample(a, 1.0L);
    for (std::size_t i = 1; i < intervals; ++i) {
        const long double fraction =
            static_cast<long double>(i) / static_cast<long double>(intervals);
        const double x = static_cast<double>(
            std::lerp(static_cast<long double>(a), static_cast<long double>(b), fraction));
        add_sample(x, i % 2 == 0 ? 2.0L : 4.0L);
    }
    add_sample(b, 1.0L);
    const long double width = static_cast<long double>(b) - a;
    long double area =
        (sum + compensation) * (width / (3.0L * static_cast<long double>(intervals)));
    if (reversed) {
        area = -area;
    }
    if (!std::isfinite(area) || std::abs(area) > std::numeric_limits<double>::max()) {
        throw std::overflow_error("integrate: result is outside the finite double range");
    }
    return static_cast<double>(area);
}

/// Find a root by bisection, assuming f is continuous over the supplied bounds.
/// Accepts reversed bounds and returns an exact endpoint root immediately.
/// Otherwise endpoint values must have opposite signs. All arguments and
/// sampled values must be finite; tolerance must be positive and the iteration
/// limit nonzero. Stops at a sampled zero or when both bracket endpoints are
/// within the absolute tolerance of the returned midpoint (accounting for its
/// rounding). Throws runtime_error if the iteration limit
/// or floating-point resolution prevents convergence. A sign change alone
/// cannot establish a root for a discontinuous function.
template <class F>
[[nodiscard]] double find_root(F &&f, double a, double b, double tolerance = 1e-10,
                               std::size_t max_iterations = 200) {
    if (!std::isfinite(a) || !std::isfinite(b) || !std::isfinite(tolerance) || tolerance <= 0.0 ||
        max_iterations == 0) {
        throw std::invalid_argument(
            "find_root: finite bounds, positive finite tolerance, and nonzero iterations required");
    }
    if (b < a) {
        std::swap(a, b);
    }
    const auto evaluate = [&](double x) {
        const double value = static_cast<double>(std::invoke(f, x));
        if (!std::isfinite(value)) {
            throw std::domain_error("find_root: callback returned a nonfinite value");
        }
        return value;
    };
    double left_value = evaluate(a);
    if (left_value == 0.0) {
        return a;
    }
    const double right_value = evaluate(b);
    if (right_value == 0.0) {
        return b;
    }
    if (std::signbit(left_value) == std::signbit(right_value)) {
        throw std::invalid_argument("find_root: endpoint values must bracket a root");
    }
    for (std::size_t iteration = 0; iteration < max_iterations; ++iteration) {
        const double middle = std::midpoint(a, b);
        const double middle_value = evaluate(middle);
        const long double radius =
            std::max(static_cast<long double>(middle) - a, static_cast<long double>(b) - middle);
        if (middle_value == 0.0 || radius <= tolerance) {
            return middle;
        }
        if (middle == a || middle == b) {
            throw std::runtime_error("find_root: tolerance cannot be reached at double precision");
        }
        if (std::signbit(left_value) == std::signbit(middle_value)) {
            a = middle;
            left_value = middle_value;
        } else {
            b = middle;
        }
    }
    throw std::runtime_error("find_root: maximum iterations reached without convergence");
}

/// Result of adaptive quadrature. estimated_error is an absolute numerical
/// estimate, not a rigorous bound. evaluations counts actual calls to f.
struct IntegrationResult {
    double value;
    double estimated_error;
    std::size_t evaluations;
};

/// Integrate a smooth, finite-valued function with adaptive Simpson's rule.
/// Refines where the estimated absolute error exceeds the local tolerance and
/// reuses every sampled value. Returns the Richardson-corrected estimate.
/// Reversed bounds negate value; equal bounds return {0, 0, 0} without calls.
/// tolerance must be finite and positive. max_depth may be 0 through 50;
/// zero allows the initial five-point estimate but no recursive refinement.
/// At most 1,000,000 callback evaluations are allowed. Throws runtime_error
/// when depth, evaluation, or floating-point resolution limits stop refinement;
/// rejects nonfinite callback values and results, and propagates f's exceptions.
/// Smooth functions are assumed: narrow unsampled peaks, singularities, and
/// oscillations can fool the error estimate. Callback rounding is not included
/// in that estimate. The callback should be deterministic for a given x.
template <class F>
[[nodiscard]] IntegrationResult
integrate_adaptive(F &&f, double a, double b, double tolerance = 1e-8, std::size_t max_depth = 20) {
    if (!std::isfinite(a) || !std::isfinite(b) || !std::isfinite(tolerance) || tolerance <= 0.0 ||
        max_depth > 50) {
        throw std::invalid_argument("integrate_adaptive: finite bounds, positive finite tolerance, "
                                    "and max_depth <= 50 required");
    }
    if (a == b) {
        return {0.0, 0.0, 0};
    }
    const bool reversed = b < a;
    if (reversed) {
        std::swap(a, b);
    }
    constexpr std::size_t evaluation_limit = 1'000'000;
    std::size_t evaluations = 0;
    const auto evaluate = [&](double x) {
        if (evaluations >= evaluation_limit) {
            throw std::runtime_error("integrate_adaptive: evaluation limit reached");
        }
        ++evaluations;
        const long double value = static_cast<long double>(std::invoke(f, x));
        if (!std::isfinite(value) || std::abs(value) > std::numeric_limits<double>::max()) {
            throw std::domain_error(
                "integrate_adaptive: callback values must be finite and fit in double");
        }
        return static_cast<double>(value);
    };
    const auto panel = [](double left, double right, double fl, double fm, double fr) {
        const long double width = static_cast<long double>(right) - left;
        const long double value = (width / 6.0L) * (static_cast<long double>(fl) + 4.0L * fm + fr);
        if (!std::isfinite(value)) {
            throw std::overflow_error("integrate_adaptive: intermediate area overflow");
        }
        return value;
    };
    struct Estimate {
        long double value;
        long double error;
    };
    const auto refine = [&](auto &&self, double left, double middle, double right, double fl,
                            double fm, double fr, long double whole, long double budget,
                            std::size_t depth) -> Estimate {
        const double left_middle = std::midpoint(left, middle);
        const double right_middle = std::midpoint(middle, right);
        if (!(left < left_middle && left_middle < middle && middle < right_middle &&
              right_middle < right)) {
            throw std::runtime_error(
                "integrate_adaptive: double precision cannot resolve further sample positions");
        }
        // Each refinement needs exactly two new samples; all others are cached.
        if (evaluation_limit - evaluations < 2) {
            throw std::runtime_error("integrate_adaptive: evaluation limit reached");
        }
        const double flm = evaluate(left_middle);
        const double frm = evaluate(right_middle);
        const long double left_area = panel(left, middle, fl, flm, fm);
        const long double right_area = panel(middle, right, fm, frm, fr);
        const long double refined = left_area + right_area;
        const long double correction = (refined - whole) / 15.0L;
        const long double estimated_error = std::abs(correction);
        if (!std::isfinite(refined) || !std::isfinite(correction)) {
            throw std::overflow_error("integrate_adaptive: intermediate area overflow");
        }
        if (estimated_error <= budget) {
            return {refined + correction, estimated_error};
        }
        if (depth >= max_depth) {
            throw std::runtime_error(
                "integrate_adaptive: maximum depth reached before estimated tolerance");
        }
        const long double child_budget = budget / 2.0L;
        if (child_budget == 0.0L) {
            throw std::runtime_error("integrate_adaptive: tolerance is below working precision");
        }
        const Estimate first =
            self(self, left, left_middle, middle, fl, flm, fm, left_area, child_budget, depth + 1);
        const Estimate second = self(self, middle, right_middle, right, fm, frm, fr, right_area,
                                     child_budget, depth + 1);
        return {first.value + second.value, first.error + second.error};
    };
    const double middle = std::midpoint(a, b);
    if (!(a < middle && middle < b)) {
        throw std::runtime_error(
            "integrate_adaptive: double precision cannot resolve the interval midpoint");
    }
    const double fa = evaluate(a);
    const double fb = evaluate(b);
    const double fm = evaluate(middle);
    const Estimate result =
        refine(refine, a, middle, b, fa, fm, fb, panel(a, b, fa, fm, fb), tolerance, 0);
    const double value =
        detail::finite_result(reversed ? -result.value : result.value,
                              "integrate_adaptive: result exceeds finite double range");
    const double error = detail::finite_result(
        result.error, "integrate_adaptive: error estimate exceeds finite double range");
    return {value, error, evaluations};
}

} // namespace math

#endif // CMATH_V2_HPP_INCLUDED
