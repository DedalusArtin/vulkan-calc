#pragma once
#include "state.hpp"
#include <string>
#include <vector>
#include <functional>

// Calculator button press
void calcOnButton(const std::string& label);

// Evaluate current expression
void calcEvaluate();

// Clear all
void calcClear();

// Delete last character
void calcDelete();

// Toggle angle mode DEG/RAD/GRAD
void calcToggleAngleMode();

// Insert Ans value
void calcInsertAns();

// Insert a named constant
void calcInsertConstant(const std::string& name);

// Insert random number
void calcInsertRandom();

// Factorial: replace n! pattern in expression
void calcApplyFactorial(std::string& expr);

// Percentage: replace % with /100
void calcApplyPercent(std::string& expr);

// Apply angle mode conversion to trig functions
std::string applyAngleMode(const std::string& expr, AngleMode mode);

// Keyboard input
void calcKeyboardInput(int key, int mods);

// Preprocess expression: replace function aliases and constants
void calcPreprocessExpression(std::string& expr);

// Set keyboard flash feedback
void calcTriggerKeyFlash();

// ============================================================
// NEW: Probability & Statistics functions
// ============================================================

// Factorial (long long)
long long factorialLL(long long n);

// Permutation P(n,k) = n! / (n-k)!
long long permutationLL(long long n, long long k);

// Combination C(n,k) = n! / (k! * (n-k)!)
long long combinationLL(long long n, long long k);

// Binomial probability: P(X=k) = C(n,k) * p^k * (1-p)^(n-k)
double binomialProb(long long n, long long k, double p);

// Standard normal CDF Φ(x) using approximation
double normalCDF(double x);

// Inverse normal (quantile function) using rational approximation
double normalInv(double p);

// Poisson distribution: P(X=k) = e^(-λ) * λ^k / k!
double poissonProb(double lambda, long long k);

// Descriptive statistics on a vector of doubles
struct DescriptiveStats {
    long long count = 0;
    double sum = 0;
    double mean = 0;
    double median = 0;
    double variance = 0;
    double stddev = 0;
    double minVal = 0;
    double maxVal = 0;
    double range = 0;
    std::vector<double> modeVals;
    std::string modeStr;
};
DescriptiveStats calcDescriptiveStats(const std::vector<double>& data);

// Parse comma-separated string into double vector
std::vector<double> parseDataList(const char* str);

// Generate histogram bins
void calcHistogram(const std::vector<double>& data, int nBins,
                   std::vector<double>& counts,
                   double& dataMin, double& dataMax);

// ============================================================
// NEW: Volume calculation functions
// ============================================================

// Sphere volume: V = 4/3 * π * r³
double sphereVolume(double r);

// Cylinder volume: V = π * r² * h
double cylinderVolume(double r, double h);

// Cone volume: V = 1/3 * π * r² * h
double coneVolume(double r, double h);

// Box volume: V = a * b * c
double boxVolume(double a, double b, double c);

// ============================================================
// NEW: Surface integral (Monte Carlo) functions
// ============================================================

// Monte Carlo surface integral over a parametric surface
// f(x,y,z) is the scalar field
// x(u,v), y(u,v), z(u,v) is the parameterization
// u in [uMin, uMax], v in [vMin, vMax]
double monteCarloSurfaceIntegral(
    std::function<double(double,double,double)> field,
    std::function<double(double,double)> px,
    std::function<double(double,double)> py,
    std::function<double(double,double)> pz,
    double uMin, double uMax,
    double vMin, double vMax,
    int nSamples);

// Evaluate a scalar field expression
double evalField(const std::string& expr, double x, double y, double z);

// ============================================================
// NEW: Derivative graph helpers
// ============================================================

// Find zero crossings of a function in an interval
std::vector<double> findZeroCrossings(const std::function<double(double)>& f,
                                       double xMin, double xMax, int steps);

// Find local extremes using derivative sign changes
std::vector<double> findLocalExtremes(const std::function<double(double)>& f,
                                       double xMin, double xMax, int steps);

// ============================================================
// NEW: 3D Surface helpers
// ============================================================

// Evaluate f(x,y) expression for 3D surface
double eval3DFunc(const std::string& expr, double x, double y);

// ============================================================
// NEW: Extension API helpers
// ============================================================

// Built-in extension: Currency conversion (USD↔CNY)
const char* extCurrency(const char* input);

// Built-in extension: BMI Calculator
const char* extBMI(const char* input);

// Built-in extension: Temperature conversion
const char* extTemperature(const char* input);

// Initialize built-in extensions
void initExtensions();
