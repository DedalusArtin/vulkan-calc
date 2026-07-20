#pragma once
#include "imgui.h"
#include <string>
#include <vector>

// Render LCD display
void renderLCD();

// Render button grid
void renderButtons();

// Render history panel
void renderHistoryPanel();

// Render constants panel
void renderConstantsPanel();

// Render advanced mode modal (with all tabs)
void renderAdvancedModal();

// Main UI render function (entry point)
void renderUI();

// Render top bar
void renderTopBar();

// Render bottom function graph (sin/cos)
void renderBottomGraph();

// ============================================================
// NEW: Individual tab renderers
// ============================================================

// Probability & Statistics tab (tab index 4)
void renderProbStatsTab();

// Volume calculation tab (embedded in calculus tab or standalone)
void renderVolumeCalc();

// Surface Integral tab
void renderSurfaceIntegral();

// Derivative graph improvements (in calculus tab)
void renderDerivativeGraphExtras();

// 3D Surface tab (tab index 5)
void render3DSurfaceTab();

// Extension panel (tab index 6)
void renderExtPanel();

// Histogram drawing helper
void drawHistogram(ImDrawList* dl, ImVec2 o, ImVec2 sz,
                   const std::vector<double>& counts,
                   const std::vector<double>& data,
                   double dataMin, double dataMax,
                   int nBins);

// 3D Surface rendering helper
void draw3DSurface(ImDrawList* dl, ImVec2 o, ImVec2 sz,
                   const std::string& expr,
                   double xMin, double xMax,
                   double rotX, double rotY,
                   float zoom);
