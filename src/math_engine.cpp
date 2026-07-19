#include "math_engine.hpp"
#include <algorithm>
#include <random>
#include <numbers>
#include <stdexcept>

// ============================================================
// INTEGRATOR IMPLEMENTATION
// ============================================================

// Adaptive Simpson helper
static double simpson_step(const RealFunc& f, double a, double b,
                           double fa, double fb, double fm,
                           double tol, int depth, int max_depth,
                           int& iter) {
    double m = (a + b) / 2.0;
    double h = (b - a) / 2.0;
    double fl = f((a + m) / 2.0);
    double fr = f((m + b) / 2.0);

    double S = (h / 3.0) * (fa + 4.0 * fm + fb);
    double S_left  = (h / 6.0) * (fa + 4.0 * fl + fm);
    double S_right = (h / 6.0) * (fm + 4.0 * fr + fb);
    double S2 = S_left + S_right;
    iter += 2;

    double error = std::abs(S2 - S) / 15.0;
    if (error < tol || depth >= max_depth) {
        return S2 + (S2 - S) / 15.0; // Richardson extrapolation
    }
    return simpson_step(f, a, m, fa, fm, fl, tol / 2.0, depth + 1, max_depth, iter)
         + simpson_step(f, m, b, fm, fb, fr, tol / 2.0, depth + 1, max_depth, iter);
}

IntegrationResult Integrator::adaptiveSimpson(
    const RealFunc& f, double a, double b,
    double tol, int max_depth) {
    IntegrationResult result;
    result.iterations = 2;
    double fa = f(a), fb = f(b), fm = f((a + b) / 2.0);

    if (a == b) { result.value = 0.0; result.error_estimate = 0.0; return result; }

    result.value = simpson_step(f, a, b, fa, fb, fm, tol, 0, max_depth, result.iterations);
    // Rough error estimate
    double S_simple = ((b - a) / 6.0) * (fa + 4.0 * fm + fb);
    result.error_estimate = std::abs(result.value - S_simple);
    return result;
}

IntegrationResult Integrator::romberg(
    const RealFunc& f, double a, double b,
    int n, double tol) {
    IntegrationResult result;
    result.iterations = 0;

    std::vector<std::vector<double>> R(n);
    double h = b - a;

    R[0].resize(1);
    R[0][0] = (h / 2.0) * (f(a) + f(b));
    result.iterations += 2;

    for (int k = 1; k < n; ++k) {
        h /= 2.0;
        R[k].resize(k + 1);

        // Composite trapezoidal
        double sum = 0.0;
        int steps = 1 << (k - 1);
        for (int i = 0; i < steps; ++i) {
            sum += f(a + (2.0 * i + 1.0) * h);
            result.iterations++;
        }
        R[k][0] = 0.5 * R[k-1][0] + h * sum;

        // Richardson extrapolation
        double pow4 = 1.0;
        for (int j = 1; j <= k; ++j) {
            pow4 *= 4.0;
            R[k][j] = R[k][j-1] + (R[k][j-1] - R[k-1][j-1]) / (pow4 - 1.0);
        }

        if (k > 2 && std::abs(R[k][k] - R[k-1][k-1]) < tol) {
            result.value = R[k][k];
            result.error_estimate = std::abs(R[k][k] - R[k-1][k-1]);
            return result;
        }
    }
    result.value = R[n-1][n-1];
    result.error_estimate = std::abs(R[n-1][n-1] - R[n-2][n-2]);
    return result;
}

// Gauss-Legendre weights and nodes (n=5, n=10)
// For simplicity use a generic approach
IntegrationResult Integrator::gaussLegendre(
    const RealFunc& f, double a, double b,
    int n, double tol) {
    // Subdivide into adaptive segments using 10-point Gauss-Legendre
    static const double x5[] = {0.0,
        0.5384693101056831, -0.5384693101056831,
        0.9061798459386640, -0.9061798459386640};
    static const double w5[] = {0.5688888888888889,
        0.4786286704993665, 0.4786286704993665,
        0.2369268850561891, 0.2369268850561891};

    auto segment = [&](double l, double r) -> double {
        double mid = (l + r) / 2.0, half = (r - l) / 2.0;
        double sum = 0.0;
        for (int i = 0; i < 5; ++i) {
            sum += w5[i] * f(mid + half * x5[i]);
        }
        return half * sum;
    };

    // Adaptive subdivision
    std::vector<std::pair<double,double>> segs = {{a, b}};
    double total = 0.0;
    int iter = 0;

    for (size_t i = 0; i < segs.size(); ++i) {
        double l = segs[i].first, r = segs[i].second;
        double m = (l + r) / 2.0;
        double S = segment(l, r);
        double S1 = segment(l, m);
        double S2 = segment(m, r);
        iter += 3;

        if (std::abs(S1 + S2 - S) < tol * (r - l) / (b - a) || (r - l) < 1e-12) {
            total += S1 + S2;
        } else {
            segs.push_back({l, m});
            segs.push_back({m, r});
        }
    }

    IntegrationResult result;
    result.value = total;
    result.error_estimate = tol;
    result.iterations = iter;
    return result;
}

IntegrationResult Integrator::monteCarlo(
    const RealFunc& f, double a, double b,
    int n_samples) {
    static std::mt19937 rng(42);
    std::uniform_real_distribution<double> dist(a, b);

    double sum = 0.0, sum2 = 0.0;
    for (int i = 0; i < n_samples; ++i) {
        double x = dist(rng);
        double fx = f(x);
        sum += fx;
        sum2 += fx * fx;
    }

    double mean = sum / n_samples;
    double variance = (sum2 / n_samples - mean * mean) / n_samples;
    double integral = mean * (b - a);

    IntegrationResult result;
    result.value = integral;
    result.error_estimate = (b - a) * std::sqrt(variance);
    result.iterations = n_samples;
    return result;
}

Complex Integrator::contourIntegrate(
    const ComplexFunc& f,
    const std::function<Complex(double)>& contour,
    double t_start, double t_end,
    int n_steps) {
    if (n_steps < 2) n_steps = 2;
    double dt = (t_end - t_start) / n_steps;

    Complex sum = 0.0;
    for (int i = 0; i < n_steps; ++i) {
        double t = t_start + (i + 0.5) * dt;
        Complex z = contour(t);
        // Derivative of contour: (z(t+dt/2) - z(t-dt/2)) / dt
        Complex zp = (contour(t + dt/2.0) - contour(t - dt/2.0)) / dt;
        sum += f(z) * zp;
    }
    return sum * dt;
}

// ============================================================
// DIFFERENTIATOR IMPLEMENTATION
// ============================================================

DerivativeResult Differentiator::derivative(const RealFunc& f, double x, double h) {
    double f1 = f(x + h), f2 = f(x - h);
    double f3 = f(x + 2*h), f4 = f(x - 2*h);
    // 5-point stencil: O(h^4)
    double val = (-f3 + 8*f1 - 8*f2 + f4) / (12.0 * h);
    double err = std::abs(f1 - f2) / (2.0 * h) * h * h;
    return {val, err};
}

DerivativeResult Differentiator::secondDerivative(const RealFunc& f, double x, double h) {
    double val = (-f(x + 2*h) + 16*f(x + h) - 30*f(x)
                  + 16*f(x - h) - f(x - 2*h)) / (12.0 * h * h);
    return {val, std::abs(val) * h * h};
}

double Differentiator::partialDerivative(
    const std::function<double(double)>& f, double x, double h) {
    return derivative(f, x, h).value;
}

Complex Differentiator::complexDerivative(
    const ComplexFunc& f, Complex z, double h) {
    // Complex step differentiation (works for analytic functions)
    return (f(z + Complex(0, h)) - f(z - Complex(0, h))) / Complex(0, 2*h);
}

// ============================================================
// COMPLEX ANALYSIS IMPLEMENTATION
// ============================================================

Complex ComplexAnalysis::cauchyIntegral(
    const ComplexFunc& f, Complex a, double radius,
    int derivative_order, int n_points) {
    // ∮ f(z)/(z-a)^(n+1) dz  → f^(n)(a) * 2πi / n!
    // Circle contour: z = a + r * exp(iθ)

    auto circle = [&](double t) -> Complex {
        return a + radius * std::exp(Complex(0, t * 2.0 * std::numbers::pi));
    };

    // Contour derivative
    auto dcircle = [&](double t) -> Complex {
        return radius * Complex(0, 2.0 * std::numbers::pi)
             * std::exp(Complex(0, t * 2.0 * std::numbers::pi));
    };

    Complex sum = 0.0;
    double dt = 1.0 / n_points;

    for (int i = 0; i < n_points; ++i) {
        double t = (i + 0.5) * dt;
        Complex z = circle(t);
        Complex denom = std::pow(z - a, derivative_order + 1.0);
        sum += f(z) / denom * dcircle(t);
    }
    sum *= dt;

    // f^(n)(a) = n! / (2πi) * ∮ f(z)/(z-a)^(n+1) dz
    Complex nfact = 1.0;
    for (int i = 2; i <= derivative_order; ++i) nfact *= i;
    return sum * nfact / (Complex(0, 2.0 * std::numbers::pi));
}

Complex ComplexAnalysis::residue(const ComplexFunc& f, Complex pole, double tol) {
    // Res(f, a) = lim_{z→a} (z-a) * f(z)
    auto shifted = [&](const Complex& z) -> Complex {
        return (z - pole) * f(z);
    };
    return shifted(pole + tol);
}

Complex ComplexAnalysis::laurentCoefficient(
    const ComplexFunc& f, Complex a, int n,
    double radius, int n_points) {
    // a_n = 1/(2πi) ∮ f(z)/(z-a)^(n+1) dz
    auto circle = [&](double t) -> Complex {
        return a + radius * std::exp(Complex(0, t * 2.0 * std::numbers::pi));
    };
    auto dcircle = [&](double t) -> Complex {
        return radius * Complex(0, 2.0 * std::numbers::pi)
             * std::exp(Complex(0, t * 2.0 * std::numbers::pi));
    };

    Complex sum = 0.0;
    double dt = 1.0 / n_points;
    for (int i = 0; i < n_points; ++i) {
        double t = (i + 0.5) * dt;
        Complex z = circle(t);
        sum += f(z) / std::pow(z - a, n + 1) * dcircle(t);
    }
    return sum * dt / (Complex(0, 2.0 * std::numbers::pi));
}

// ============================================================
// SPECIAL FUNCTIONS
// ============================================================

double SpecialFunctions::gamma(double x) {
    // Stirling's approximation + reflection for negative values
    if (x < 0.5) {
        return std::numbers::pi / (std::sin(std::numbers::pi * x) * gamma(1.0 - x));
    }

    // Lanczos approximation
    static const double p[] = {
        676.5203681218851, -1259.1392167224028, 771.32342877765313,
        -176.61502916214059, 12.507343278686905, -0.13857109526572012,
        9.9843695780195716e-6, 1.5056327351493116e-7
    };

    x -= 1.0;
    double g = 0.99999999999980993;
    for (int i = 0; i < 8; ++i) g += p[i] / (x + i + 1.0);

    double t = x + 7.5;
    return std::sqrt(2.0 * std::numbers::pi) * std::pow(t, x + 0.5)
           * std::exp(-t) * g;
}

double SpecialFunctions::erf(double x) {
    // Horner's method for the error function
    double t = 1.0 / (1.0 + 0.5 * std::abs(x));
    double tau = t * std::exp(-x * x - 1.26551223
        + t * (1.00002368 + t * (0.37409196 + t * (0.09678418
        + t * (-0.18628806 + t * (0.27886807 + t * (-1.13520398
        + t * (1.48851587 + t * (-0.82215223 + t * 0.17087277)))))))));
    return x >= 0 ? (1.0 - tau) : (tau - 1.0);
}

double SpecialFunctions::besselJ(int n, double x) {
    if (n < 0 || x < 0) return std::numeric_limits<double>::quiet_NaN();
    if (x == 0) return (n == 0) ? 1.0 : 0.0;

    // Use series for small x, asymptotic for large
    if (x < n) {
        double sum = 0.0;
        double term = 1.0 / std::tgamma(n + 1) * std::pow(x / 2.0, n);
        for (int k = 0; k < 100; ++k) {
            sum += term;
            if (std::abs(term) < 1e-15) break;
            term *= -x * x / (4.0 * (k + 1) * (k + n + 1));
        }
        return sum;
    }

    // Asymptotic + recurrence
    std::vector<double> j(2*n + 5, 0.0);
    double start = 2*n + 4;
    j[start] = 0.0;
    j[start-1] = 1.0;

    for (int k = (int)start - 2; k >= 0; --k) {
        j[k] = 2.0 * (k + 1) / x * j[k+1] - j[k+2];
    }

    double norm = j[0];
    for (int k = 2; k <= (int)start; k += 2) {
        norm += 2 * j[k];
    }

    return j[n] / norm;
}

double SpecialFunctions::besselY(int n, double x) {
    if (n < 0 || x <= 0) return std::numeric_limits<double>::quiet_NaN();
    // Y_n(x) = (J_n(x) * cos(nπ) - J_{-n}(x)) / sin(nπ)
    double jn = besselJ(n, x);
    double j_n = (n % 2 == 0) ? besselJ(n, x) : -besselJ(n, x);
    return (jn * std::cos(n * std::numbers::pi) - j_n)
           / std::sin(n * std::numbers::pi);
}

Complex SpecialFunctions::gamma(const Complex& z) {
    if (z.real() < 0.5) {
        return std::numbers::pi / (std::sin(std::numbers::pi * z) * gamma(1.0 - z));
    }

    static const double p[] = {
        676.5203681218851, -1259.1392167224028, 771.32342877765313,
        -176.61502916214059, 12.507343278686905, -0.13857109526572012,
        9.9843695780195716e-6, 1.5056327351493116e-7
    };

    Complex zz = z - 1.0;
    Complex g = 0.99999999999980993;
    for (int i = 0; i < 8; ++i) g += p[i] / (zz + Complex(i + 1, 0));

    Complex t = zz + 7.5;
    return std::sqrt(2.0 * std::numbers::pi) * std::pow(t, zz + 0.5)
           * std::exp(-t) * g;
}

// ============================================================
// PLOT GENERATOR
// ============================================================

PlotData PlotGenerator::generate(
    const RealFunc& f, double x_min, double x_max,
    int n_points, const std::string& label) {
    PlotData data;
    data.x_min = x_min;
    data.x_max = x_max;
    data.y_min = std::numeric_limits<double>::infinity();
    data.y_max = -std::numeric_limits<double>::infinity();
    data.label = label;
    data.points.reserve(n_points);

    double dx = (x_max - x_min) / (n_points - 1);
    for (int i = 0; i < n_points; ++i) {
        double x = x_min + i * dx;
        double y = f(x);
        if (std::isfinite(y)) {
            data.points.push_back({x, y});
            if (y < data.y_min) data.y_min = y;
            if (y > data.y_max) data.y_max = y;
        }
    }

    if (data.points.empty()) {
        data.y_min = -1.0;
        data.y_max = 1.0;
    }
    // Add some padding
    double y_range = data.y_max - data.y_min;
    if (y_range < 1e-15) y_range = 1.0;
    data.y_min -= 0.1 * y_range;
    data.y_max += 0.1 * y_range;

    return data;
}

std::vector<PlotData> PlotGenerator::generateMultiple(
    const std::vector<RealFunc>& funcs,
    double x_min, double x_max,
    int n_points,
    const std::vector<std::string>& labels) {
    std::vector<PlotData> results;
    for (size_t i = 0; i < funcs.size(); ++i) {
        std::string lbl = (i < labels.size()) ? labels[i] : "";
        results.push_back(generate(funcs[i], x_min, x_max, n_points, lbl));
    }
    return results;
}

std::vector<PlotGenerator::DomainColorPoint> PlotGenerator::domainColoring(
    const ComplexFunc& f,
    double x_min, double x_max,
    double y_min, double y_max,
    int x_res, int y_res) {
    std::vector<DomainColorPoint> points;
    points.reserve(x_res * y_res);

    double dx = (x_max - x_min) / x_res;
    double dy = (y_max - y_min) / y_res;

    for (int iy = 0; iy < y_res; ++iy) {
        double y = y_min + iy * dy;
        for (int ix = 0; ix < x_res; ++ix) {
            double x = x_min + ix * dx;
            Complex z(x, y);
            Complex fz = f(z);

            double mag = std::abs(fz);
            double arg = std::arg(fz); // [-π, π]

            // Hue from argument, saturation fixed, brightness from magnitude
            double hue = (arg / (2.0 * std::numbers::pi) + 0.5);
            double lightness = 0.3 + 0.7 * std::atan(mag) / (std::numbers::pi / 2.0);

            // HSL to RGB
            double c = (1.0 - std::abs(2.0 * lightness - 1.0)) * 0.9;
            double hp = hue * 6.0;
            double xc = c * (1.0 - std::abs(std::fmod(hp, 2.0) - 1.0));
            double m = lightness - c / 2.0;

            float r, g, b;
            int hi = static_cast<int>(hp) % 6;
            switch (hi) {
                case 0: r = c; g = xc; b = 0; break;
                case 1: r = xc; g = c; b = 0; break;
                case 2: r = 0; g = c; b = xc; break;
                case 3: r = 0; g = xc; b = c; break;
                case 4: r = xc; g = 0; b = c; break;
                default: r = c; g = 0; b = xc; break;
            }
            r += m; g += m; b += m;
            if (r > 1) r = 1; if (g > 1) g = 1; if (b > 1) b = 1;

            points.push_back({x, y, r, g, b});
        }
    }
    return points;
}

// ============================================================
// EXPRESSION EVALUATOR
// ============================================================

ExpressionEvaluator::ExpressionEvaluator() {}

void ExpressionEvaluator::setExpression(const std::string& expr) {
    m_expr = expr;
    m_tokens.clear();
    m_valid = false;
    m_error.clear();

    // Simple tokenizer
    std::string s;
    for (char c : expr) {
        if (c != ' ') s += c;
    }

    size_t i = 0;
    while (i < s.length()) {
        char c = s[i];

        // Numbers
        if (std::isdigit(c) || (c == '.' && i + 1 < s.length() && std::isdigit(s[i+1]))) {
            size_t end = i;
            while (end < s.length() && (std::isdigit(s[end]) || s[end] == '.')) end++;
            double val = std::stod(s.substr(i, end - i));
            m_tokens.push_back({NUMBER, val, ""});
            i = end;
            continue;
        }

        // Variable x
        if (c == 'x' || c == 'z') {
            m_tokens.push_back({VARIABLE, 0, ""});
            i++;
            continue;
        }

        // Constants
        if (c == 'p' && s.substr(i, 4) == "pi") {
            m_tokens.push_back({CONSTANT, std::numbers::pi, "pi"});
            i += 2; continue;
        }
        if (c == 'e' && (i + 1 >= s.length() || !std::isalpha(s[i+1]))) {
            m_tokens.push_back({CONSTANT, std::numbers::e, "e"});
            i++; continue;
        }

        // Functions
        static const struct { const char* name; int len; TokenType type; } funcs[] = {
            {"sin", 3, FUNC_SIN}, {"cos", 3, FUNC_COS}, {"tan", 3, FUNC_TAN},
            {"log", 3, FUNC_LOG}, {"ln", 2, FUNC_LN},
            {"exp", 3, FUNC_EXP}, {"sqrt", 4, FUNC_SQRT},
            {"abs", 3, FUNC_ABS},
        };
        bool matched = false;
        for (const auto& f : funcs) {
            if (s.substr(i, f.len) == f.name) {
                m_tokens.push_back({f.type, 0, f.name});
                i += f.len;
                matched = true;
                break;
            }
        }
        if (matched) continue;

        // Operators
        switch (c) {
            case '+': m_tokens.push_back({OP_PLUS, 0, "+"}); break;
            case '-': m_tokens.push_back({OP_MINUS, 0, "-"}); break;
            case '*': m_tokens.push_back({OP_MULT, 0, "*"}); break;
            case '/': m_tokens.push_back({OP_DIV, 0, "/"}); break;
            case '^': m_tokens.push_back({OP_POW, 0, "^"}); break;
            case '(': m_tokens.push_back({PAREN_OPEN, 0, "("}); break;
            case ')': m_tokens.push_back({PAREN_CLOSE, 0, ")"}); break;
            default:
                m_error = "Unexpected character: ";
                m_error += c;
                return;
        }
        i++;
    }
    m_valid = true;
}

double ExpressionEvaluator::evaluate(double x) {
    if (!m_valid) return 0.0;

    // Shunting-yard to RPN
    struct ShuntToken {
        bool is_op; // true = operator, false = value
        double val;
        TokenType op;
    };

    std::vector<ShuntToken> output;
    std::vector<TokenType> op_stack;

    auto op_prec = [](TokenType t) -> int {
        switch (t) {
            case OP_PLUS: case OP_MINUS: return 1;
            case OP_MULT: case OP_DIV: return 2;
            case OP_POW: return 3;
            case FUNC_SIN: case FUNC_COS: case FUNC_TAN:
            case FUNC_LOG: case FUNC_LN: case FUNC_EXP:
            case FUNC_SQRT: case FUNC_ABS: return 4;
            default: return 0;
        }
    };

    auto is_unary = [&](const Token& tok, size_t idx) -> bool {
        if (tok.type == OP_MINUS || tok.type == OP_PLUS) {
            if (idx == 0) return true;
            TokenType prev = m_tokens[idx-1].type;
            return prev == OP_PLUS || prev == OP_MINUS || prev == OP_MULT
                || prev == OP_DIV || prev == OP_POW || prev == PAREN_OPEN;
        }
        return false;
    };

    for (size_t i = 0; i < m_tokens.size(); ++i) {
        const auto& tok = m_tokens[i];
        switch (tok.type) {
            case NUMBER:
                output.push_back({false, tok.value, NUMBER});
                break;
            case VARIABLE:
                output.push_back({false, x, VARIABLE});
                break;
            case CONSTANT:
                output.push_back({false, tok.value, CONSTANT});
                break;
            case FUNC_SIN: case FUNC_COS: case FUNC_TAN:
            case FUNC_LOG: case FUNC_LN: case FUNC_EXP:
            case FUNC_SQRT: case FUNC_ABS:
                op_stack.push_back(tok.type);
                break;
            case OP_PLUS: case OP_MINUS: {
                if (is_unary(tok, i)) {
                    // Unary minus/plus
                    TokenType actual_op = (tok.type == OP_MINUS) ? FUNC_ABS : FUNC_ABS;
                    // We'll handle unary minus by negating the next value
                    output.push_back({false, 0.0, NUMBER}); // push 0
                    op_stack.push_back(OP_MINUS); // subtract
                    continue;
                }
                [[fallthrough]];
            }
            default: {
                while (!op_stack.empty()) {
                    TokenType top = op_stack.back();
                    if (top == PAREN_OPEN) break;
                    int top_prec = op_prec(top);
                    int cur_prec = op_prec(tok.type);
                    bool left_assoc = (tok.type != OP_POW);
                    if (top_prec > cur_prec || (top_prec == cur_prec && left_assoc)) {
                        output.push_back({true, 0, top});
                        op_stack.pop_back();
                    } else break;
                }
                op_stack.push_back(tok.type);
                break;
            }
            case PAREN_OPEN:
                op_stack.push_back(PAREN_OPEN);
                break;
            case PAREN_CLOSE: {
                while (!op_stack.empty() && op_stack.back() != PAREN_OPEN) {
                    output.push_back({true, 0, op_stack.back()});
                    op_stack.pop_back();
                }
                if (!op_stack.empty() && op_stack.back() == PAREN_OPEN)
                    op_stack.pop_back();
                // If function token before '('
                if (!op_stack.empty() && op_prec(op_stack.back()) == 4) {
                    output.push_back({true, 0, op_stack.back()});
                    op_stack.pop_back();
                }
                break;
            }
        }
    }

    while (!op_stack.empty()) {
        output.push_back({true, 0, op_stack.back()});
        op_stack.pop_back();
    }

    // Evaluate RPN
    std::vector<double> stack;
    for (const auto& tok : output) {
        if (!tok.is_op) {
            stack.push_back(tok.val);
        } else {
            if (stack.empty()) return 0.0;
            double b = stack.back(); stack.pop_back();
            double a = 0.0;
            bool is_unary_op = false;

            switch (tok.op) {
                case FUNC_SIN: stack.push_back(std::sin(b)); is_unary_op = true; break;
                case FUNC_COS: stack.push_back(std::cos(b)); is_unary_op = true; break;
                case FUNC_TAN: stack.push_back(std::tan(b)); is_unary_op = true; break;
                case FUNC_LOG: stack.push_back(std::log10(b)); is_unary_op = true; break;
                case FUNC_LN:  stack.push_back(std::log(b)); is_unary_op = true; break;
                case FUNC_EXP: stack.push_back(std::exp(b)); is_unary_op = true; break;
                case FUNC_SQRT: stack.push_back(std::sqrt(b)); is_unary_op = true; break;
                case FUNC_ABS: stack.push_back(std::abs(b)); is_unary_op = true; break;
                default: {
                    if (stack.empty()) return 0.0;
                    a = stack.back(); stack.pop_back();
                    switch (tok.op) {
                        case OP_PLUS:  stack.push_back(a + b); break;
                        case OP_MINUS: stack.push_back(a - b); break;
                        case OP_MULT:  stack.push_back(a * b); break;
                        case OP_DIV:   stack.push_back(a / b); break;
                        case OP_POW:   stack.push_back(std::pow(a, b)); break;
                        default: return 0.0;
                    }
                }
            }
        }
    }

    return stack.empty() ? 0.0 : stack.back();
}

Complex ExpressionEvaluator::evaluateComplex(const Complex& z) {
    // For complex evaluation, evaluate at x=z.real() and provide imaginary path
    // Full complex expression evaluation would need a separate parser
    return Complex(evaluate(z.real()), 0);
}
