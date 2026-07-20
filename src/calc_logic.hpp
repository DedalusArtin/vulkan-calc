#pragma once
#include "state.hpp"
#include <string>

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
