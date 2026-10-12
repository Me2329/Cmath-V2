#ifndef CMATH_V2_EXPRESSION_HPP
#define CMATH_V2_EXPRESSION_HPP
#include "cmath_v2.hpp"
#include <charconv>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace math {
// A bounded arithmetic parser, not a C++/JavaScript evaluator. No I/O or code execution.
class Expression {
    struct Node {
        char kind = 'n';
        double number = 0;
        std::string name;
        std::vector<std::shared_ptr<Node>> children;
        double eval(double x) const {
            double r = 0;
            if (kind == 'n') return number;
            if (kind == 'x') return x;
            const double a = children[0]->eval(x);
            if (kind == '~') r = -a;
            else if (kind == 'f') {
                const double b = children.size() > 1 ? children[1]->eval(x) : 0;
                if (name == "sin") r = math::sin(a);
                else if (name == "cos") r = math::cos(a);
                else if (name == "tan") r = math::tan(a);
                else if (name == "sin_deg") r = math::sin_deg(a);
                else if (name == "cos_deg") r = math::cos_deg(a);
                else if (name == "tan_deg") r = math::tan_deg(a);
                else if (name == "asin") r = math::asin(a);
                else if (name == "acos") r = math::acos(a);
                else if (name == "atan") r = math::atan(a);
                else if (name == "asin_deg") r = math::asin_deg(a);
                else if (name == "acos_deg") r = math::acos_deg(a);
                else if (name == "atan_deg") r = math::atan_deg(a);
                else if (name == "atan2") r = math::atan2(a, b);
                else if (name == "atan2_deg") r = math::atan2_deg(a, b);
                else if (name == "sqrt") r = math::sqrt(a);
                else if (name == "cbrt") r = math::cbrt(a);
                else if (name == "abs") r = math::abs(a);
                else if (name == "exp") r = math::exp(a);
                else if (name == "ln" || (name == "log" && children.size() == 1)) r = math::log(a);
                else if (name == "log" || name == "log_base") r = math::log_base(a, b);
                else if (name == "log10") r = math::log10(a);
                else if (name == "log2") r = math::log2(a);
                else if (name == "floor") r = math::floor(a);
                else if (name == "ceil") r = math::ceil(a);
                else if (name == "round") r = math::round(a);
                else if (name == "square") r = math::square(a);
                else if (name == "cube") r = math::cube(a);
                else if (name == "radians") r = math::radians(a);
                else if (name == "degrees") r = math::degrees(a);
                else if (name == "pow") r = math::pow(a, b);
                else if (name == "hypot") r = math::hypot(a, b);
                else if (name == "min") r = std::min(a, b);
                else if (name == "max") r = std::max(a, b);
                else if (name == "percent") r = math::percent(a, b);
                else if (name == "percent_of") r = math::percent_of(a, b);
                else if (name == "root" || name == "nth_root") {
                    if (b != std::trunc(b) || std::abs(b) > 1000000)
                        throw std::domain_error("Root degree must be an integer between -1000000 and 1000000.");
                    r = math::nth_root(a, static_cast<int>(b));
                } else if (name == "factorial") {
                    if (a < 0 || a > 20 || a != std::trunc(a))
                        throw std::domain_error("factorial requires an integer from 0 to 20.");
                    r = static_cast<double>(math::factorial(static_cast<int>(a)));
                } else if (name == "clamp") r = math::clamp(a, b, children[2]->eval(x));
                else throw std::logic_error("Unrecognized expression function.");
            } else {
                const double b = children[1]->eval(x);
                switch (kind) {
                    case '+': r = a + b; break;
                    case '-': r = a - b; break;
                    case '*': r = a * b; break;
                    case '/': if (b == 0) throw std::domain_error("Division by zero."); r = a / b; break;
                    case '%': if (b == 0) throw std::domain_error("Remainder by zero."); r = std::fmod(a, b); break;
                    case '^': r = std::pow(a, b); break;
                    default: throw std::logic_error("Invalid expression node.");
                }
            }
            if (!std::isfinite(r)) throw std::domain_error("Expression is undefined or outside finite real-number range.");
            return r;
        }
    };
    struct Parser {
        std::string_view text;
        std::size_t position = 0, nodes = 0, depth = 0;
        [[noreturn]] void error(const std::string& message) const {
            throw std::invalid_argument(message + " (character " + std::to_string(position + 1) + ")");
        }
        void spaces() { while (position < text.size() && (text[position] == ' ' || text[position] == '\t' || text[position] == '\n' || text[position] == '\r')) ++position; }
        bool take(char c) { spaces(); if (position < text.size() && text[position] == c) { ++position; return true; } return false; }
        std::shared_ptr<Node> node(char kind, std::vector<std::shared_ptr<Node>> children = {}) {
            if (++nodes > 512) error("Expression is too complex; maximum 512 nodes");
            auto n = std::make_shared<Node>(); n->kind = kind; n->children = std::move(children); return n;
        }
        std::shared_ptr<Node> sum() {
            auto n = product();
            while (true) {
                if (take('+')) n = node('+', {n, product()});
                else if (take('-')) n = node('-', {n, product()});
                else return n;
            }
        }
        std::shared_ptr<Node> product() {
            auto n = unary();
            while (true) {
                if (take('*')) n = node('*', {n, unary()});
                else if (take('/')) n = node('/', {n, unary()});
                else if (take('%')) n = node('%', {n, unary()});
                else return n;
            }
        }
        std::shared_ptr<Node> unary() {
            if (++depth > 64) error("Maximum expression nesting is 64");
            std::shared_ptr<Node> n;
            if (take('+')) n = unary();
            else if (take('-')) n = node('~', {unary()});
            else { n = primary(); if (take('^')) n = node('^', {n, unary()}); }
            --depth;
            return n;
        }
        std::shared_ptr<Node> primary() {
            spaces();
            if (take('(')) { auto n = sum(); if (!take(')')) error("Expected closing parenthesis"); return n; }
            if (position == text.size()) error("Expected a number, x, or function");
            const char c = text[position];
            if ((c >= '0' && c <= '9') || c == '.') {
                double value = 0;
                const char* first = text.data() + position;
                const auto parsed = std::from_chars(first, text.data() + text.size(), value);
                if (parsed.ec != std::errc{} || parsed.ptr == first || !std::isfinite(value)) error("Invalid or overflowing number");
                position = static_cast<std::size_t>(parsed.ptr - text.data());
                auto n = node('n'); n->number = value; return n;
            }
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
                const auto start = position++;
                while (position < text.size()) {
                    const char d = text[position];
                    if (!((d >= 'a' && d <= 'z') || (d >= 'A' && d <= 'Z') || (d >= '0' && d <= '9') || d == '_')) break;
                    ++position;
                }
                const std::string name(text.substr(start, position - start));
                if (!take('(')) {
                    if (name == "x") return node('x');
                    auto n = node('n');
                    if (name == "pi") n->number = math::pi;
                    else if (name == "tau") n->number = math::tau;
                    else if (name == "e") n->number = math::e;
                    else if (name == "phi") n->number = math::phi;
                    else error("Unknown name '" + name + "'; use x, pi, tau, e, or phi");
                    return n;
                }
                static const std::unordered_map<std::string, std::size_t> arity{
                    {"sin",1},{"cos",1},{"tan",1},{"sin_deg",1},{"cos_deg",1},{"tan_deg",1},
                    {"asin",1},{"acos",1},{"atan",1},{"asin_deg",1},{"acos_deg",1},{"atan_deg",1},
                    {"atan2",2},{"atan2_deg",2},{"sqrt",1},{"cbrt",1},{"abs",1},{"exp",1},
                    {"ln",1},{"log",1},{"log_base",2},{"log10",1},{"log2",1},{"floor",1},
                    {"ceil",1},{"round",1},{"square",1},{"cube",1},{"radians",1},{"degrees",1},
                    {"pow",2},{"hypot",2},{"min",2},{"max",2},{"percent",2},{"percent_of",2},
                    {"root",2},{"nth_root",2},{"factorial",1},{"clamp",3}};
                const auto found = arity.find(name);
                if (found == arity.end()) error("Unknown function '" + name + "'");
                std::vector<std::shared_ptr<Node>> args;
                if (!take(')')) {
                    do { args.push_back(sum()); if (args.size() > 3) error("Too many function arguments"); } while (take(','));
                    if (!take(')')) error("Expected comma or closing parenthesis");
                }
                if (args.size() != found->second && !(name == "log" && args.size() == 2)) error("Wrong number of arguments for " + name);
                auto n = node('f', std::move(args)); n->name = name; return n;
            }
            error("Unexpected symbol; use explicit multiplication such as 2*x");
        }
    };
    std::string source_;
    std::shared_ptr<Node> root_;
public:
    explicit Expression(std::string source) : source_(std::move(source)) {
        if (source_.size() > 2048) throw std::invalid_argument("Expression exceeds 2048 characters.");
        Parser parser{source_}; root_ = parser.sum(); parser.spaces();
        if (parser.position != source_.size()) parser.error("Unexpected text; use * for multiplication");
    }
    double operator()(double x = 0) const {
        if (!std::isfinite(x)) throw std::domain_error("x must be finite.");
        return root_->eval(x);
    }
    const std::string& source() const noexcept { return source_; }
};
inline double evaluate(const std::string& expression, double x = 0) { return Expression(expression)(x); }
} // namespace math
#endif
