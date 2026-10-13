# cmath_v2 — version 2.1.0

A C++20 math toolbox in **one header**. It keeps familiar `<cmath>` functions and
adds checked helpers for statistics, equations, geometry, and numerical calculus.
No extra installation or linking flags are needed. The implementation targets
ordinary floating-point calculations, with additional precision in intermediate
calculations on your Fedora machine.

## 1. Run the example

Open `cmath_v2_examples.cpp` in VS Code and press **Ctrl+Shift+B**.
Your global setup compiles it, opens the terminal, and prints the program's output.
Keep that `.cpp` tab active when pressing the shortcut. **F5** uses the debugger.

Your own `main.cpp` is unchanged. The new example is separate so you can experiment.
To use this library in another project, copy **cmath_v2.hpp** beside its `.cpp` file.
The global compiler setup does not automatically make this project header available everywhere.

Terminal alternative, from this folder:

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic cmath_v2_examples.cpp -o cmath_v2_examples
./cmath_v2_examples
```

## 2. Write your first program

```cpp
#include "cmath_v2.hpp"
#include <iostream>

int main() {
    double radius = 3;
    double area = math::circle_area(radius);

    std::cout << "Area: " << area << '\n';
    std::cout << "15% of 200: " << math::percent_of(15, 200) << '\n';
    std::cout << "Average: " << math::mean({10, 20, 30}) << '\n';
    std::cout << "sin(30 degrees): " << math::sin_deg(30) << '\n';
}
```

- `#include "cmath_v2.hpp"` makes the library's definitions available to your code.
  Quotes tell the compiler to look for the header beside your source file first.
- `math::` is the library's namespace: it identifies which toolbox a function belongs to.
- The values inside parentheses are the **arguments**. `percent_of(15, 200)` means 15% of 200.
- Most functions **return a number**. Store it in a variable, pass it to another
  function, or print it with `std::cout`. Calling `math::sqrt(25);` alone prints nothing.
- Use `double` for results with fractions. Use `auto` if you want C++ to infer the result type.
- Keep `math::` and `std::` prefixes instead of importing both entire namespaces;
  they share names such as `sqrt`, `min`, and `max`.

To read a radius from the keyboard:

```cpp
double radius;
std::cout << "Radius: ";
if (std::cin >> radius) {
    std::cout << math::circle_area(radius) << '\n';
}
```

Place this inside `main()` and use the error handling shown below if inputs may be invalid.

## 3. Everyday tools

| Function | Meaning / example |
| --- | --- |
| `math::sqrt(25)` | Square root: 5 |
| `math::pow(2, 10)` | 2 raised to the 10th power: 1024 |
| `math::square(5)`, `math::cube(3)` | 25 and 27 |
| `math::nth_root(-27, 3)` | Real cube root: approximately -3 |
| `math::nth_root(16, -2)` | Reciprocal square root: 0.25 |
| `math::log_base(1024, 2)` | Logarithm in base 2: 10 |
| `math::percent(25, 200)` | 25 as a percentage of 200: 12.5 |
| `math::percent_of(15, 200)` | 15% of 200: 30 |
| `math::clamp(12, 0, 10)` | Keep a value within a range: 10 |
| `math::approx(0.1 + 0.2, 0.3)` | Approximate equality: true |
| `math::factorial(5)` | 5!: 120 |
| `math::binomial(10, 3)` | 10 choose 3: 120 |
| `math::pi`, `math::tau`, `math::e`, `math::phi` | Constants |

`factorial`, `binomial`, and the degree of `nth_root` require integer types.
Negative factorial/combination counts throw; fractional arguments fail to compile
instead of silently truncating. `factorial` supports 0 through 20. Combinations
support results fitting in `std::uint64_t`; overflowing results throw.

`approx(a, b, relative_tolerance, absolute_tolerance)` defaults to `1e-9` and `1e-12`.
Equal infinities compare true, NaNs false. Floating-point approximations are normal;
use this helper instead of expecting all computed decimals to compare exactly.

## 4. Angles

`math::sin`, `cos`, and `tan` use **radians**, like standard C++.
Use `sin_deg`, `cos_deg`, and `tan_deg` to work directly in **degrees**.
The degree helpers reduce large angles before conversion and handle cardinal angles.
Tangent at an exact odd multiple of 90 degrees throws because it is undefined.

```cpp
double sine = math::sin_deg(30);           // approximately 0.5
double angle = math::asin_deg(0.5);        // approximately 30 degrees
double direction = math::atan2_deg(1, 1); // 45 degrees (y first, then x)
```

`asin_deg` and `acos_deg` accept values in [-1, 1]. `atan_deg` takes any finite value.
`atan2_deg(y, x)` gives a direction in [-180, 180] and rejects the origin (0, 0).
Use `radians(degrees)` and `degrees(radians)` to convert units explicitly.

## 5. Statistics

Pass an inline list, `std::vector`, array, or `std::span` of arithmetic values:

```cpp
#include <vector>

// Inside main():
std::vector<int> scores{10, 20, 30, 40, 50};
auto stats = math::describe(scores);

std::cout << stats.count << '\n';  // 5
std::cout << stats.mean << '\n';   // 30
std::cout << stats.median << '\n'; // 30
std::cout << math::percentile(scores, 90) << '\n'; // 46
```

`describe` returns a `math::Summary` containing `count`, `sum`, `mean`, `median`,
`min`, `max`, `variance`, and `stddev`. The input is read once, and a copy is sorted
for the median. The original container is not changed.

Individual functions are also available: `sum`, `mean`, `median`, `min`, `max`,
`variance`, and `stddev`. Write `math::mean({2, 4, 6})` for a quick average.

- Population variance is the default. Use `math::variance(scores, true)` or
  `math::describe(scores, true)` for sample statistics with an n-1 denominator.
- `quantile(data, q)` takes q from 0 to 1; `percentile(data, p)` takes p from 0 to 100.
  Both sort a copy and linearly interpolate at position q*(n-1). This is not the
  nearest-rank convention; for `{0, 10, 20, 30}`, the 25th percentile is 7.5.
- Empty input throws except `sum({})`, which returns zero. Sample variance needs
  at least two values. Nonfinite inputs are rejected.
- Calculations use wider intermediate values and compensated sums. Large integer
  offsets are removed before computing variance. Results are still `double`, so
  precision is finite. `describe` throws if any field, including the sum, overflows;
  use `mean` alone when you need a finite average of values whose sum is too large.

## 6. Solve equations

```cpp
auto roots = math::solve_quadratic(1, -5, 6); // x*x - 5*x + 6 = 0
for (const auto root : roots) {
    std::cout << root.real() << '\n'; // 2, then 3
}
```

The arguments are a, b, c from **a*x*x + b*x + c = 0**.
The result is a `std::vector<std::complex<double>>`:

- `.real()` and `.imag()` give the two parts of each root.
- Printing `root` directly displays `(real,imaginary)`.
- `solve_quadratic(1, 0, 1)` returns +i and -i.
- Repeated roots appear twice. Real roots are returned in ascending order.
- If a is zero, the equation is linear and returns one root when b is nonzero.
- A nonzero constant equation returns no roots; all-zero coefficients throw
  because there are infinitely many solutions.

Power-of-two scaling and a corrected discriminant preserve exact repeated roots
more reliably. A stable quadratic formula avoids subtracting almost equal numbers.

## 7. Numerical calculus

You can pass a normal function. Define it **outside** `main()`:

```cpp
double squared(double x) {
    return x * x;
}
```

Then, **inside** `main()`:

```cpp
std::cout << math::derivative(squared, 3) << '\n';   // approximately 6
std::cout << math::integrate(squared, 0, 1) << '\n'; // approximately 1/3

auto integral = math::integrate_adaptive(squared, 0, 1, 1e-10);
std::cout << integral.value << '\n';
std::cout << integral.estimated_error << '\n';
std::cout << integral.evaluations << '\n';
```

Pass `squared` without parentheses: you are giving the library a function to call.
You can also use a **lambda**, a small function defined on the spot:

```cpp
double root = math::find_root([](double x) { return x * x - 2; }, 0, 2);
std::cout << root << '\n'; // approximately 1.41421356237
```

How these functions work:

- `derivative(f, x, step=0)` compares f just to the left and right of x. A zero
  step chooses one automatically. f must be smooth and defined at both samples.
- `integrate(f, a, b, intervals=1000)` uses composite Simpson's rule. The number
  of intervals must be even and at least two. Reversing the bounds negates the result.
- `integrate_adaptive(f, a, b, tolerance=1e-8, max_depth=20)` subdivides where its
  Simpson error estimate is too large, reusing samples. It returns value, estimated
  absolute error, and callback count. Depth can be 0 through 50 (0 allows only
  the initial five-point estimate); a 1,000,000-evaluation ceiling limits work.
- `find_root(f, a, b, tolerance=1e-10, max_iterations=200)` repeatedly halves a
  bracket. f must be continuous, with opposite signs at the endpoints or an exact
  endpoint root. The tolerance is an absolute **x-position** tolerance: the returned
  midpoint is within that distance of both bracket endpoints, unless f returned zero.

These are numerical approximations, not symbolic algebra. The adaptive error is
an estimate, not a proven bound; unsampled narrow peaks, rapid oscillations, and
rounding inside your function can fool it. Use smooth deterministic functions and
finite inputs. Nonconvergence, unresolvable sample positions, nonfinite values,
or work limits produce exceptions rather than a silent success.

## 8. Geometry and error handling

```cpp
math::circle_area(3);                 // approximately 28.2743
math::circle_circumference(3);        // approximately 18.8496
math::triangle_area(3, 4, 5);          // 6 (three side lengths)
math::distance(0, 0, 3, 4);            // 5
```

Radii cannot be negative. Triangle sides must describe a nondegenerate triangle.
Put calls with potentially invalid inputs inside a try/catch:

```cpp
try {
    std::cout << math::nth_root(-16, 2) << '\n';
} catch (const std::exception& error) {
    std::cerr << "Math error: " << error.what() << '\n';
}
```

Extra checked helpers report invalid domains and overflow using standard exceptions.
Re-exported standard functions retain standard behavior: `math::sqrt(-1.0)` can
return NaN instead of throwing. `square`, `cube`, and angle conversion use ordinary
floating-point arithmetic; overflowing results can become infinity. Extremely small
results can underflow to zero. This is not an arbitrary-precision library.

## Tests and changes

Version 2.1.0 fixes repeated quadratic roots, median precision for large integers,
and variance near large common offsets. It adds summary/percentile statistics,
inverse degree trigonometry, base logarithms, percentage helpers, strict whole-number
arguments, and adaptive integration with an error estimate.

The included `tests/cmath_v2_tests.cpp` contains 242 runtime checks and compile-time
argument checks. Open it in VS Code and press Ctrl+Shift+B to run it, or compile:

```bash
g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror tests/cmath_v2_tests.cpp -o cmath_v2_tests
./cmath_v2_tests
```
