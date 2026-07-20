#include "ui.hpp"
#include "calc_logic.hpp"
#include "locale.hpp"
#include "state.hpp"
#include "imgui.h"
#include <iostream>
#include <cmath>
#include <numbers>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <cctype>
#include <ctime>

// Colors
static const ImU32 COL_LCD_BG       = IM_COL32(10, 18, 10, 255);
static const ImU32 COL_LCD_TEXT     = IM_COL32(50, 220, 80, 255);
static const ImU32 COL_LCD_RESULT   = IM_COL32(80, 255, 110, 255);
static const ImU32 COL_LCD_DIM      = IM_COL32(30, 130, 50, 200);
static const ImU32 COL_LCD_ERROR    = IM_COL32(255, 50, 50, 255);
static const ImU32 COL_LCD_STATUS   = IM_COL32(40, 180, 70, 220);

static const ImU32 COL_BTN_NUM      = IM_COL32(50, 50, 60, 255);
static const ImU32 COL_BTN_NUM_H    = IM_COL32(70, 70, 85, 255);
static const ImU32 COL_BTN_NUM_A    = IM_COL32(85, 85, 100, 255);
static const ImU32 COL_BTN_OP       = IM_COL32(200, 120, 40, 255);
static const ImU32 COL_BTN_OP_H     = IM_COL32(220, 150, 60, 255);
static const ImU32 COL_BTN_OP_A     = IM_COL32(240, 170, 80, 255);
static const ImU32 COL_BTN_FUNC     = IM_COL32(30, 80, 160, 255);
static const ImU32 COL_BTN_FUNC_H   = IM_COL32(50, 110, 200, 255);
static const ImU32 COL_BTN_FUNC_A   = IM_COL32(70, 140, 230, 255);
static const ImU32 COL_BTN_DEL      = IM_COL32(180, 40, 40, 255);
static const ImU32 COL_BTN_DEL_H    = IM_COL32(210, 60, 60, 255);
static const ImU32 COL_BTN_DEL_A    = IM_COL32(240, 80, 80, 255);
static const ImU32 COL_BTN_EQ       = IM_COL32(220, 120, 30, 255);
static const ImU32 COL_BTN_EQ_H     = IM_COL32(240, 150, 50, 255);
static const ImU32 COL_BTN_EQ_A     = IM_COL32(255, 180, 70, 255);
static const ImU32 COL_BTN_AC       = IM_COL32(200, 30, 30, 255);
static const ImU32 COL_BTN_AC_H     = IM_COL32(230, 50, 50, 255);
static const ImU32 COL_BTN_AC_A     = IM_COL32(255, 70, 70, 255);
static const ImU32 COL_BTN_TOOL     = IM_COL32(40, 40, 55, 255);
static const ImU32 COL_BTN_TOOL_H   = IM_COL32(60, 60, 80, 255);
static const ImU32 COL_BTN_TOOL_A   = IM_COL32(80, 80, 100, 255);

// Button layout constants
static const float BTN_GAP = 2.0f;
static const int BTN_COLS = 5;

// ============================================================
// Helper: styled button with custom colors
// ============================================================
static bool styledButton(const char* label, ImU32 col, ImU32 colH, ImU32 colA,
                          ImVec2 size = ImVec2(0, 0)) {
    ImGui::PushStyleColor(ImGuiCol_Button, col);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colH);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, colA);
    bool clicked = ImGui::Button(label, size);
    ImGui::PopStyleColor(3);
    return clicked;
}

// ============================================================
// Small button helpers (defined before use)
// ============================================================
static bool numBtn(const char* l) { return styledButton(l, COL_BTN_NUM, COL_BTN_NUM_H, COL_BTN_NUM_A); }
static bool opBtn(const char* l)  { return styledButton(l, COL_BTN_OP, COL_BTN_OP_H, COL_BTN_OP_A); }
static bool funcBtn(const char* l){ return styledButton(l, COL_BTN_FUNC, COL_BTN_FUNC_H, COL_BTN_FUNC_A); }
static bool delBtn(const char* l) { return styledButton(l, COL_BTN_DEL, COL_BTN_DEL_H, COL_BTN_DEL_A); }
static bool acBtn(const char* l)  { return styledButton(l, COL_BTN_AC, COL_BTN_AC_H, COL_BTN_AC_A); }
static bool eqBtn(const char* l)  { return styledButton(l, COL_BTN_EQ, COL_BTN_EQ_H, COL_BTN_EQ_A); }
static bool toolBtn(const char* l){ return styledButton(l, COL_BTN_TOOL, COL_BTN_TOOL_H, COL_BTN_TOOL_A); }

// ============================================================
// Font size helpers
// ============================================================
static void pushLcdFont() {
    int idx = 0;
    switch (g_state.fontSize) {
        case 0: idx = 3; break; // small
        case 1: idx = 2; break; // medium (default)
        case 2: idx = 1; break; // large
        default: idx = 2;
    }
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[idx]);
}

static void pushResultFont() {
    int idx = 0;
    switch (g_state.fontSize) {
        case 0: idx = 2; break;  // medium
        case 1: idx = 1; break;  // large
        case 2: idx = 1; break;  // large (same)
        default: idx = 1;
    }
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[idx]);
}

// ============================================================
// Autocomplete: function names for manual input
// ============================================================
static const char* g_autocompleteFuncs[] = {
    "sin(", "cos(", "tan(", "asin(", "acos(", "atan(",
    "sinh(", "cosh(", "tanh(", "arcsin(", "arccos(", "arctan(",
    "log(", "ln(", "log10(", "log2(", "sqrt(", "cbrt(",
    "exp(", "abs(", "floor(", "ceil(", "round(",
    "pi", "e"
};
static const int g_autocompleteCount = sizeof(g_autocompleteFuncs) / sizeof(g_autocompleteFuncs[0]);

// ============================================================
// LCD Display with InputText and autocomplete
// ============================================================
void renderLCD() {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x;
    float h = 150.0f;

    // Determine flash border color
    ImU32 borderCol = COL_LCD_BG;
    if (g_state.keyFlashTimer > 0) {
        // Brighter green flash
        float t = (float)g_state.keyFlashTimer / 8.0f;
        int g = (int)(80 + 175 * t);
        borderCol = IM_COL32(30, g, 30, 200);
        g_state.keyFlashTimer--;
    } else {
        borderCol = IM_COL32(30, 80, 30, 150);
    }

    // Background (LCD panel)
    dl->AddRectFilled(pos, ImVec2(pos.x + w, pos.y + h), COL_LCD_BG, 6.0f);
    dl->AddRect(pos, ImVec2(pos.x + w, pos.y + h), borderCol, 6.0f);

    float padX = 12.0f;
    float padY = 6.0f;
    float y = pos.y + padY;

    // --- Expression line (InputText for manual input) ---
    // Sync inputBuf from display when not being edited by user
    static bool lcdWasFocused = false;

    // If we're gaining focus or were focused, don't overwrite inputBuf from display
    if (!lcdWasFocused) {
        // Not focused - keep inputBuf in sync with display
        strncpy(g_state.inputBuf, g_state.display.c_str(), sizeof(g_state.inputBuf) - 1);
        g_state.inputBuf[sizeof(g_state.inputBuf) - 1] = '\0';
        // Reset cursor position when not focused
        g_state.inputCursorPos = (int)g_state.display.size();
    }

    // Calculate font-based height for input field - enforce minimum 40px
    pushLcdFont();
    float lcdFontH = std::max(ImGui::GetFontSize() + 6.0f, 40.0f);

    // Make the entire LCD width clickable to focus the input
    static bool focusInputNextFrame = false;
    ImGui::SetCursorScreenPos(ImVec2(pos.x, y));
    ImGui::InvisibleButton("##lcdClickArea", ImVec2(w, lcdFontH));
    if (ImGui::IsItemClicked()) {
        focusInputNextFrame = true;
        g_state.inputCursorPos = (int)std::strlen(g_state.inputBuf);
    }
    // Highlight the clickable area on hover
    if (ImGui::IsItemHovered()) {
        dl->AddRectFilled(ImVec2(pos.x + 4, y), ImVec2(pos.x + w - 4, y + lcdFontH),
                          IM_COL32(20, 35, 20, 100), 4.0f);
    }

    // Draw a subtle input background to show the editable area
    dl->AddRectFilled(ImVec2(pos.x + 4, y), ImVec2(pos.x + w - 4, y + lcdFontH),
                      IM_COL32(15, 25, 15, 200), 4.0f);

    ImGui::SetCursorScreenPos(ImVec2(pos.x + padX, y + 2));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(15, 25, 15, 0)); // transparent
    ImGui::PushStyleColor(ImGuiCol_Text, COL_LCD_TEXT);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(15, 25, 15, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, IM_COL32(15, 25, 15, 10));

    ImGui::SetNextItemWidth(w - padX * 2);

    // Set keyboard focus if LCD area was clicked
    if (focusInputNextFrame) {
        ImGui::SetKeyboardFocusHere();
        focusInputNextFrame = false;
    }
    if (ImGui::InputText("##lcdInput", g_state.inputBuf, sizeof(g_state.inputBuf),
                         ImGuiInputTextFlags_EnterReturnsTrue)) {
        // Enter pressed in input field -> evaluate
        g_state.display = g_state.inputBuf;
        calcEvaluate();
    }

    // Sync display from inputBuf when it changes (user typed)
    bool lcdIsFocused = ImGui::IsItemFocused();
    if (lcdIsFocused) {
        // User is editing - sync display to inputBuf
        g_state.display = g_state.inputBuf;
        g_state.hasResult = false;
        // Cursor at end when editing in InputText (ImGui manages internal cursor)
        g_state.inputCursorPos = (int)g_state.display.size();
    }
    lcdWasFocused = lcdIsFocused;

    ImGui::PopFont();
    ImGui::PopStyleColor(4);

    // --- Autocomplete dropdown ---
    if (lcdIsFocused && g_state.inputBuf[0] != '\0') {
        // Get current word being typed (after last non-alphanum)
        std::string currentInput = g_state.inputBuf;
        std::string lastWord;
        // Find the last sequence of letters
        for (int i = (int)currentInput.size() - 1; i >= 0; i--) {
            char c = currentInput[i];
            if (std::isalpha((unsigned char)c)) {
                lastWord = currentInput.substr(i);
                break;
            }
            if (c == '(' || c == ')' || c == '+' || c == '-' ||
                c == '*' || c == '/' || c == '^' || c == ' ' || c == ',') {
                break;
            }
        }

        if (!lastWord.empty()) {
            // Find matching functions
            std::string lowerWord;
            for (char c : lastWord) lowerWord += std::tolower((unsigned char)c);

            // Count matches
            int matchCount = 0;
            int matchIndices[64];
            for (int i = 0; i < g_autocompleteCount && matchCount < 64; i++) {
                std::string funcName = g_autocompleteFuncs[i];
                std::string lowerFunc;
                for (char c : funcName) lowerFunc += std::tolower((unsigned char)c);
                if (lowerFunc.find(lowerWord) == 0) {
                    matchIndices[matchCount++] = i;
                }
            }

            if (matchCount > 0) {
                ImGui::SetNextWindowPos(ImVec2(pos.x + padX, y + lcdFontH + 2));
                ImGui::SetNextWindowSizeConstraints(ImVec2(w - padX * 2, 0),
                                                     ImVec2(w - padX * 2, 250));

                // Use a unique ID for the popup
                ImGui::PushID("##autocompletePopup");
                if (ImGui::Begin("##autocompleteList", nullptr,
                                 ImGuiWindowFlags_NoTitleBar |
                                 ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_AlwaysAutoResize |
                                 ImGuiWindowFlags_Tooltip)) {
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(20, 25, 35, 240));
                    ImGui::BeginChild("##acScroll", ImVec2(0, std::min((float)matchCount * 24.0f, 200.0f)), true);
                    for (int i = 0; i < matchCount; i++) {
                        int idx = matchIndices[i];
                        char itemLabel[64];
                        snprintf(itemLabel, sizeof(itemLabel), "%s##ac%d",
                                 g_autocompleteFuncs[idx], idx);
                        if (ImGui::Selectable(itemLabel, false)) {
                            // Replace the last word with the selected function
                            std::string funcName = g_autocompleteFuncs[idx];
                            size_t wordStart = currentInput.size() - lastWord.size();
                            std::string newInput = currentInput.substr(0, wordStart) + funcName;
                            strncpy(g_state.inputBuf, newInput.c_str(), sizeof(g_state.inputBuf) - 1);
                            g_state.inputBuf[sizeof(g_state.inputBuf) - 1] = '\0';
                            g_state.display = newInput;
                            ImGui::CloseCurrentPopup();
                        }
                    }
                    ImGui::EndChild();
                    ImGui::PopStyleColor();
                }
                ImGui::End();
                ImGui::PopID();
            }
        }
    }

    // --- Result line ---
    y = pos.y + padY + 34.0f;
    if (!g_state.result.empty()) {
        ImGui::SetCursorScreenPos(ImVec2(pos.x + padX, y));
        ImGui::PushStyleColor(ImGuiCol_Text, COL_LCD_RESULT);
        pushResultFont();
        std::string resultText = g_state.result;
        ImGui::TextUnformatted(resultText.c_str());
        ImGui::PopFont();
        ImGui::PopStyleColor();
    } else if (!g_state.error.empty()) {
        ImGui::SetCursorScreenPos(ImVec2(pos.x + padX, y));
        ImGui::PushStyleColor(ImGuiCol_Text, COL_LCD_ERROR);
        pushLcdFont();
        ImGui::TextUnformatted(g_state.error.c_str());
        ImGui::PopFont();
        ImGui::PopStyleColor();
    }

    // --- History (small, last 3 entries) ---
    y = pos.y + padY + 68.0f;
    ImGui::SetCursorScreenPos(ImVec2(pos.x + padX, y));
    ImGui::PushStyleColor(ImGuiCol_Text, COL_LCD_DIM);
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[3]); // Small font
    int count = 0;
    for (const auto& entry : g_state.history) {
        if (count >= 3) break;
        ImGui::TextColored(ImVec4(0.15f, 0.6f, 0.3f, 0.7f), "  %s = %s",
                           entry.expression.c_str(), entry.result.c_str());
        count++;
    }
    ImGui::PopFont();
    ImGui::PopStyleColor();

    // --- Status line ---
    y = pos.y + h - 20.0f;
    ImGui::SetCursorScreenPos(ImVec2(pos.x + padX, y));
    ImGui::PushStyleColor(ImGuiCol_Text, COL_LCD_STATUS);
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[3]);
    char status[64];
    snprintf(status, sizeof(status), "%s  DEC",
             angleModeStr(g_state.angleMode));
    ImGui::TextUnformatted(status);
    ImGui::SameLine();
    // Constants indicator on right
    float rightX = pos.x + w - padX;
    ImGui::SetCursorScreenPos(ImVec2(rightX - 140, y));
    ImGui::TextUnformatted(T("Font", "字体", "フォント"));
    ImGui::PopFont();
    ImGui::PopStyleColor();

    ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + h + 4));
}

// ============================================================
// Button Grid
// ============================================================
void renderButtons() {
    float availW = ImGui::GetContentRegionAvail().x;
    float availH = ImGui::GetContentRegionAvail().y;
    float btnW = (availW - (BTN_COLS - 1) * BTN_GAP) / (float)BTN_COLS;

    // Dynamic button height based on available space (leave room for bottom graph)
    int totalRows = 7;
    float graphReserveH = 180.0f;
    float btnAreaH = availH - graphReserveH - 8.0f;
    float btnH = (btnAreaH - (totalRows - 1) * BTN_GAP) / (float)totalRows;
    // Clamp to reasonable range
    if (btnH < 50.0f) btnH = 50.0f;
    if (btnH > 72.0f) btnH = 72.0f;

    auto sameLine = [&]() { ImGui::SameLine(0, BTN_GAP); };
    auto btnSize = ImVec2(btnW, btnH);

    // Macros for fixed-size buttons
    #define NUMBTN(l)   styledButton(l, COL_BTN_NUM, COL_BTN_NUM_H, COL_BTN_NUM_A, btnSize)
    #define OPBTN(l)    styledButton(l, COL_BTN_OP, COL_BTN_OP_H, COL_BTN_OP_A, btnSize)
    #define FUNCBTN(l)  styledButton(l, COL_BTN_FUNC, COL_BTN_FUNC_H, COL_BTN_FUNC_A, btnSize)
    #define DELBTN(l)   styledButton(l, COL_BTN_DEL, COL_BTN_DEL_H, COL_BTN_DEL_A, btnSize)
    #define ACBTN(l)    styledButton(l, COL_BTN_AC, COL_BTN_AC_H, COL_BTN_AC_A, btnSize)
    #define EQBTN(l)    styledButton(l, COL_BTN_EQ, COL_BTN_EQ_H, COL_BTN_EQ_A, btnSize)
    #define TOOLBTN(l)  styledButton(l, COL_BTN_TOOL, COL_BTN_TOOL_H, COL_BTN_TOOL_A, btnSize)

    // Push larger font for buttons (Fonts[4] = 20px button font)
    ImFont* btnFont = ImGui::GetIO().Fonts->Fonts[4];
    ImGui::PushFont(btnFont);

    // Row 0: Blue function keys — parentheses with func keys
    if (FUNCBTN("sin##r0c0")) calcOnButton("sin"); sameLine();
    if (FUNCBTN("cos##r0c1")) calcOnButton("cos"); sameLine();
    if (FUNCBTN("tan##r0c2")) calcOnButton("tan"); sameLine();
    if (FUNCBTN("(##r0c3"))   calcOnButton("(");  sameLine();
    if (FUNCBTN(")##r0c4"))   calcOnButton(")");

    // Row 1: Blue function keys
    if (FUNCBTN("x²##r1c0"))  calcOnButton("x²"); sameLine();
    if (FUNCBTN("√##r1c1"))   calcOnButton("√");  sameLine();
    if (FUNCBTN("x^y##r1c2")) calcOnButton("x^y"); sameLine();
    if (FUNCBTN("π##r1c3"))   calcOnButton("π");  sameLine();
    if (FUNCBTN("e##r1c4"))   calcOnButton("e");

    // Row 2: 7 8 9 ÷ DEL
    if (NUMBTN("7##r2c0"))  calcOnButton("7"); sameLine();
    if (NUMBTN("8##r2c1"))  calcOnButton("8"); sameLine();
    if (NUMBTN("9##r2c2"))  calcOnButton("9"); sameLine();
    if (OPBTN("÷##r2c3"))   calcOnButton("÷"); sameLine();
    if (DELBTN("DEL##r2c4")) calcOnButton("DEL");

    // Row 3: 4 5 6 × AC
    if (NUMBTN("4##r3c0"))  calcOnButton("4"); sameLine();
    if (NUMBTN("5##r3c1"))  calcOnButton("5"); sameLine();
    if (NUMBTN("6##r3c2"))  calcOnButton("6"); sameLine();
    if (OPBTN("×##r3c3"))   calcOnButton("×"); sameLine();
    if (ACBTN("AC##r3c4"))  calcOnButton("AC");

    // Row 4: 1 2 3 + -
    if (NUMBTN("1##r4c0"))  calcOnButton("1"); sameLine();
    if (NUMBTN("2##r4c1"))  calcOnButton("2"); sameLine();
    if (NUMBTN("3##r4c2"))  calcOnButton("3"); sameLine();
    if (OPBTN("+##r4c3"))   calcOnButton("+"); sameLine();
    if (OPBTN("-##r4c4"))   calcOnButton("-");

    // Row 5: 0 . Ans = %
    if (NUMBTN("0##r5c0"))  calcOnButton("0"); sameLine();
    if (NUMBTN(".##r5c1"))  calcOnButton("."); sameLine();
    if (TOOLBTN("Ans##r5c2")) calcOnButton("Ans"); sameLine();
    if (EQBTN("=##r5c3"))     calcOnButton("="); sameLine();
    if (TOOLBTN("%##r5c4"))   calcOnButton("%");

    // Row 6: log ln n! Rand Help
    if (FUNCBTN("log##r6c0")) calcOnButton("log"); sameLine();
    if (FUNCBTN("ln##r6c1"))  calcOnButton("ln"); sameLine();
    if (TOOLBTN("n!##r6c2"))  calcOnButton("!");  sameLine();
    if (TOOLBTN(T("Rand##r6c3", "随机##r6c3", "乱数##r6c3"))) calcOnButton("rand"); sameLine();
    if (TOOLBTN(T("Help##r6c4", "帮助##r6c4", "ヘルプ##r6c4"))) ImGui::OpenPopup("##helpPopup");

    // Cleanup macros
    #undef NUMBTN
    #undef OPBTN
    #undef FUNCBTN
    #undef DELBTN
    #undef ACBTN
    #undef EQBTN
    #undef TOOLBTN

    ImGui::PopFont();
}

// ============================================================
// Top bar
// ============================================================
void renderTopBar() {
    // --- Language toggle button (leftmost) ---
    const char* langLabel;
    ImU32 langCol;
    switch (g_state.lang) {
        case LANG_EN: langLabel = "EN";  langCol = COL_BTN_FUNC; break;
        case LANG_ZH: langLabel = "中文"; langCol = COL_BTN_FUNC; break;
        case LANG_JA: langLabel = "日本語"; langCol = COL_BTN_FUNC; break;
        default:      langLabel = "EN";  langCol = COL_BTN_TOOL; break;
    }
    if (styledButton(langLabel, langCol, COL_BTN_TOOL_H, COL_BTN_TOOL_A, ImVec2(56, 30))) {
        calcToggleLang();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", T("Switch language: EN / 中文 / 日本語",
                                  "切换语言: EN / 中文 / 日本語",
                                  "言語切替: EN / 中文 / 日本語"));
    }

    ImGui::SameLine(0, 4);

    // Angle mode toggle
    const char* amStr = angleModeStr(g_state.angleMode);
    if (styledButton(amStr, COL_BTN_TOOL, COL_BTN_TOOL_H, COL_BTN_TOOL_A, ImVec2(56, 30))) {
        calcToggleAngleMode();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", T("Angle mode: DEG/RAD/GRAD",
                                  "角度模式：度/弧度/百分度",
                                  "角度モード：度/ラジアン/グラード"));
    }

    ImGui::SameLine(0, 4);

    // --- Xiaomi-style Mode toggle: Basic ↔ Scientific ---
    if (styledButton(g_state.isAdvanced ? T("标准##md", "标准##md", "標準##md")
                                        : T("科学##md", "科学##md", "科学##md"),
                     g_state.isAdvanced ? COL_BTN_FUNC : COL_BTN_TOOL,
                     COL_BTN_TOOL_H, COL_BTN_TOOL_A, ImVec2(52, 26))) {
        g_state.isAdvanced = !g_state.isAdvanced;
        if (g_state.isAdvanced) {
            g_state.showHistory = false;
            g_state.showConstants = false;
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", T("Switch between Basic and Scientific mode (Xiaomi style)",
                                  "切换标准/科学模式（小米计算器风格）",
                                  "標準/科学モードを切替（Xiaomi電卓風）"));
    }

    ImGui::SameLine(0, 4);

    // History button
    if (styledButton(T("HIST", "历史", "履歴"),
                     g_state.showHistory ? COL_BTN_FUNC : COL_BTN_TOOL,
                     COL_BTN_TOOL_H, COL_BTN_TOOL_A, ImVec2(56, 26))) {
        g_state.showHistory = !g_state.showHistory;
        if (g_state.showHistory) {
            g_state.showAdvanced = false;
            g_state.showConstants = false;
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", T("Calculation history (last 20)",
                                  "计算历史记录（最近20条）",
                                  "計算履歴（最新20件）"));
    }

    ImGui::SameLine(0, 4);

    // Constants panel button
    if (styledButton("CONST", COL_BTN_TOOL, COL_BTN_TOOL_H, COL_BTN_TOOL_A, ImVec2(56, 26))) {
        g_state.showConstants = !g_state.showConstants;
        if (g_state.showConstants) {
            g_state.showAdvanced = false;
            g_state.showHistory = false;
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", T("Physical constants",
                                  "物理常数",
                                  "物理定数"));
    }

    ImGui::SameLine(0, 8);

    // --- Font size selector ---
    const char* fontLabels[] = {
        T("S", "小", "小"),
        T("M", "中", "中"),
        T("L", "大", "大")
    };
    for (int i = 0; i < 3; i++) {
        if (i > 0) ImGui::SameLine(0, 2);
        bool active = (g_state.fontSize == i);
        if (styledButton(fontLabels[i],
                         active ? COL_BTN_FUNC : COL_BTN_TOOL,
                         COL_BTN_TOOL_H, COL_BTN_TOOL_A, ImVec2(28, 26))) {
            g_state.fontSize = i;
        }
    }

    ImGui::SameLine(0, 8);

    // --- Quick access to advanced features ---
    // Fourier quick button
    if (styledButton(T("F##fb", "傅##fb", "フ##fb"),
                     COL_BTN_TOOL, COL_BTN_TOOL_H, COL_BTN_TOOL_A, ImVec2(28, 26))) {
        g_state.isAdvanced = true;
        g_state.showHistory = false;
        g_state.showConstants = false;
        g_state.advTab = 1;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", T("Fourier Series", "傅里叶级数", "フーリエ級数"));
    }

    ImGui::SameLine(0, 2);

    // Complex quick button
    if (styledButton(T("C##cb", "复##cb", "複##cb"),
                     COL_BTN_TOOL, COL_BTN_TOOL_H, COL_BTN_TOOL_A, ImVec2(28, 26))) {
        g_state.isAdvanced = true;
        g_state.showHistory = false;
        g_state.showConstants = false;
        g_state.advTab = 2;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", T("Complex Analysis", "复分析", "複素解析"));
    }

    ImGui::SameLine(0, 2);

    // Calculus quick button
    if (styledButton(T("∫##ib", "积##ib", "積##ib"),
                     COL_BTN_TOOL, COL_BTN_TOOL_H, COL_BTN_TOOL_A, ImVec2(28, 26))) {
        g_state.isAdvanced = true;
        g_state.showHistory = false;
        g_state.showConstants = false;
        g_state.advTab = 0;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", T("Calculus", "微积分", "微積分"));
    }

    ImGui::SameLine(0, 2);

    // Probability quick button
    if (styledButton(T("P##pb", "概##pb", "確##pb"),
                     COL_BTN_TOOL, COL_BTN_TOOL_H, COL_BTN_TOOL_A, ImVec2(28, 26))) {
        g_state.isAdvanced = true;
        g_state.showHistory = false;
        g_state.showConstants = false;
        g_state.advTab = 4;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", T("Probability & Statistics", "概率统计", "確率統計"));
    }

    ImGui::SameLine(0, 2);

    // 3D Surface quick button
    if (styledButton(T("3D##3db", "3D##3db", "3D##3db"),
                     COL_BTN_TOOL, COL_BTN_TOOL_H, COL_BTN_TOOL_A, ImVec2(28, 26))) {
        g_state.isAdvanced = true;
        g_state.showHistory = false;
        g_state.showConstants = false;
        g_state.advTab = 5;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", T("3D Surface", "3D曲面", "3D曲面"));
    }

    ImGui::SameLine(0, 2);

    // Extension quick button
    if (styledButton(T("Ext##eb", "扩##eb", "拡##eb"),
                     COL_BTN_TOOL, COL_BTN_TOOL_H, COL_BTN_TOOL_A, ImVec2(28, 26))) {
        g_state.isAdvanced = true;
        g_state.showHistory = false;
        g_state.showConstants = false;
        g_state.advTab = 6;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", T("Extension API", "扩展插件", "拡張プラグイン"));
    }

    ImGui::SameLine(0, 8);

    ImGui::Separator();
}

// ============================================================
// History Panel
// ============================================================
void renderHistoryPanel() {
    if (!g_state.showHistory) return;

    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    ImGui::Begin(T("History##win", "计算历史##win", "計算履歴##win"), &g_state.showHistory);

    if (g_state.history.empty()) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1),
                           "%s", T("No history yet", "暂无历史记录", "履歴がありません"));
    } else {
        if (ImGui::BeginChild("##histList", ImVec2(0, 0), true)) {
            int idx = 0;
            for (const auto& entry : g_state.history) {
                char label[128];
                snprintf(label, sizeof(label), "%s = %s##hist%d",
                         entry.expression.c_str(), entry.result.c_str(), idx);

                if (ImGui::Selectable(label, false)) {
                    g_state.display.clear();
                    g_state.hasResult = false;
                    g_state.display = entry.expression;
                    g_state.error.clear();
                }
                idx++;
            }
        }
        ImGui::EndChild();
    }

    ImGui::End();
}

// ============================================================
// Constants Panel
// ============================================================
void renderConstantsPanel() {
    if (!g_state.showConstants) return;

    ImGui::SetNextWindowSize(ImVec2(320, 250), ImGuiCond_FirstUseEver);
    ImGui::Begin(T("Constants##win", "物理常数##win", "物理定数##win"), &g_state.showConstants);

    struct ConstEntry {
        const char* name;
        const char* value;
        const char* key;
    };

    const ConstEntry consts[] = {
        {"π (Pi)",   "3.141592653589793",  "pi"},
        {"e (Euler)", "2.718281828459045",  "e"},
        {"c (Light)", "299,792,458 m/s",    "c"},
        {"h (Planck)", "6.62607015e-34 J·s", "h"},
        {"G (Gravity)", "6.67430e-11 m³/kg·s²", "G"},
        {"NA (Avogadro)", "6.02214076e23 mol⁻¹", "NA"},
    };

    for (const auto& c : consts) {
        if (ImGui::Selectable(c.name, false)) {
            calcInsertConstant(c.key);
            g_state.showConstants = false;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s = %s", c.name, c.value);
        }
    }

    ImGui::End();
}

// ============================================================
// Advanced Mode Modal (Fourier / Complex / Domain Coloring)
// ============================================================
void generateFourierPlot() {
    g_state.fourierPlots.clear();
    int N = std::max(1, g_state.fourierTerms);
    auto f = [&](double x) -> double {
        auto squareWave = [](double x, int n) -> double {
            double s = 0;
            for (int k = 1; k <= n; k++)
                s += std::sin((2 * k - 1) * x) / (2 * k - 1);
            return (4 / std::numbers::pi) * s;
        };
        auto sawtoothWave = [](double x, int n) -> double {
            double s = 0;
            for (int k = 1; k <= n; k++)
                s += ((k % 2 ? 1 : -1)) * std::sin(k * x) / k;
            return (2 / std::numbers::pi) * s;
        };
        auto triangleWave = [](double x, int n) -> double {
            double s = 0;
            for (int k = 0; k < n; k++)
                s += std::pow(-1, k) * std::sin((2 * k + 1) * x) /
                     std::pow(2 * k + 1, 2);
            return (8 / (std::numbers::pi * std::numbers::pi)) * s;
        };
        switch (g_state.fourierType) {
            case 0: return squareWave(x, N);
            case 1: return sawtoothWave(x, N);
            case 2: return triangleWave(x, N);
            default: return 0;
        }
    };
    g_state.fourierPlots.push_back(
        PlotGenerator::generate(f, -10, 10, 800, "Fourier N=" + std::to_string(N)));

    if (g_state.fourierType < 3) {
        auto exact = [&](double x) -> double {
            auto ex = [](double x) -> double {
                // Normalize x to [-π, π)
                double xnorm = std::fmod(x + std::numbers::pi, 2 * std::numbers::pi);
                if (xnorm < 0) xnorm += 2 * std::numbers::pi;
                xnorm -= std::numbers::pi;
                switch (g_state.fourierType) {
                    case 0: return xnorm > 0 ? 1 : (xnorm < 0 ? -1 : 0);
                    case 1: return xnorm / std::numbers::pi;
                    case 2: return std::numbers::pi - std::abs(xnorm);
                    default: return 0;
                }
            };
            return ex(x);
        };
        g_state.fourierPlots.push_back(
            PlotGenerator::generate(exact, -10, 10, 800, "Exact"));
    }
}

void generateDomainColoring() {
    auto f = [&](Complex z) -> Complex {
        if (g_state.complexExpr == "z") return z;
        if (g_state.complexExpr == "z^2") return z * z;
        if (g_state.complexExpr == "z^3") return z * z * z;
        if (g_state.complexExpr == "1/z")
            return std::abs(z) < 1e-15 ? Complex(1e15) : Complex(1) / z;
        if (g_state.complexExpr == "sin(z)") return std::sin(z);
        if (g_state.complexExpr == "cos(z)") return std::cos(z);
        if (g_state.complexExpr == "exp(z)") return std::exp(z);
        if (g_state.complexExpr == "log(z)") return std::log(z);
        if (g_state.complexExpr == "sqrt(z)") return std::sqrt(z);
        if (g_state.complexExpr == "Gamma(z)")
            return SpecialFunctions::gamma(z);
        return z;
    };
    g_state.domainPts = PlotGenerator::domainColoring(
        f, g_state.cxMin, g_state.cxMax, g_state.cyMin, g_state.cyMax, 256, 256);
    g_state.domainDirty = false;
}

// Graph drawing helpers
static void drawGraphBg(ImDrawList* dl, ImVec2 o, ImVec2 sz) {
    dl->AddRectFilled(o, ImVec2(o.x + sz.x, o.y + sz.y), IM_COL32(8, 8, 12, 255));
    dl->AddRect(o, ImVec2(o.x + sz.x, o.y + sz.y), IM_COL32(30, 30, 45, 200), 4.0f);
}

static auto makeToScreen = [](ImVec2 o, ImVec2 sz,
                               double xMin, double xMax,
                               double yMin, double yMax) {
    return [=](double wx, double wy) -> ImVec2 {
        float nx = (float)((wx - xMin) / (xMax - xMin));
        float ny = (float)((wy - yMin) / (yMax - yMin));
        return ImVec2(o.x + nx * sz.x, o.y + (1 - ny) * sz.y);
    };
};

static void drawGrid(ImDrawList* dl, ImVec2 o, ImVec2 sz,
                     double xMin, double xMax,
                     double yMin, double yMax) {
    auto ts = makeToScreen(o, sz, xMin, xMax, yMin, yMax);

    // Compute nice step sizes for grid lines
    auto niceStep = [](double range, int targetTicks) -> double {
        if (range <= 0) return 1.0;
        double rough = range / targetTicks;
        double e = std::floor(std::log10(rough));
        double f = rough / std::pow(10, e);
        double nice;
        if (f < 1.5)      nice = 1.0;
        else if (f < 3.5) nice = 2.0;
        else if (f < 7.5) nice = 5.0;
        else              nice = 10.0;
        return nice * std::pow(10, e);
    };

    double xStep = niceStep(xMax - xMin, 8);
    double yStep = niceStep(yMax - yMin, 8);
    if (xStep <= 0) xStep = (xMax - xMin) / 8;
    if (yStep <= 0) yStep = (yMax - yMin) / 8;

    auto tc = IM_COL32(100, 100, 140, 200);
    auto tcAxis = IM_COL32(120, 120, 180, 220);

    // Draw vertical grid lines at each nice x position
    double xStart = std::ceil(xMin / xStep) * xStep;
    for (double v = xStart; v <= xMax; v += xStep) {
        auto p = ts(v, 0);
        if (p.x >= o.x && p.x <= o.x + sz.x) {
            bool isZero = std::abs(v) < 1e-12;
            ImU32 col = isZero ? tcAxis : tc;
            float thickness = isZero ? 2.5f : 1.0f;
            dl->AddLine(ImVec2(p.x, o.y), ImVec2(p.x, o.y + sz.y), col, thickness);
        }
    }

    // Draw horizontal grid lines at each nice y position
    double yStart = std::ceil(yMin / yStep) * yStep;
    for (double v = yStart; v <= yMax; v += yStep) {
        auto p = ts(0, v);
        if (p.y >= o.y && p.y <= o.y + sz.y) {
            bool isZero = std::abs(v) < 1e-12;
            ImU32 col = isZero ? tcAxis : tc;
            float thickness = isZero ? 2.5f : 1.0f;
            dl->AddLine(ImVec2(o.x, p.y), ImVec2(o.x + sz.x, p.y), col, thickness);
        }
    }
}

static void drawAxesLabels(ImDrawList* dl, ImVec2 o, ImVec2 sz,
                           double xMin, double xMax,
                           double yMin, double yMax) {
    auto ts = makeToScreen(o, sz, xMin, xMax, yMin, yMax);

    // Same niceStep as drawGrid
    auto niceStep = [](double range, int targetTicks) -> double {
        if (range <= 0) return 1.0;
        double rough = range / targetTicks;
        double e = std::floor(std::log10(rough));
        double f = rough / std::pow(10, e);
        double nice;
        if (f < 1.5)      nice = 1.0;
        else if (f < 3.5) nice = 2.0;
        else if (f < 7.5) nice = 5.0;
        else              nice = 10.0;
        return nice * std::pow(10, e);
    };

    double xs = niceStep(xMax - xMin, 8);
    double ys = niceStep(yMax - yMin, 8);
    if (xs <= 0) xs = (xMax - xMin) / 8;
    if (ys <= 0) ys = (yMax - yMin) / 8;
    auto tc = IM_COL32(100, 100, 140, 200);
    double xS = std::ceil(xMin / xs) * xs;
    double yS = std::ceil(yMin / ys) * ys;
    for (double v = xS; v <= xMax; v += xs) {
        auto p = ts(v, 0);
        if (p.x >= o.x && p.x <= o.x + sz.x) {
            dl->AddLine(ImVec2(p.x, o.y + sz.y - 3),
                        ImVec2(p.x, o.y + sz.y), tc);
            char b[32];
            snprintf(b, sizeof(b), "%.10g", v);
            char* e = b + strlen(b) - 1;
            while (e > b && *e == '0') e--;
            if (*e == '.') *e = 0;
            else *(e + 1) = 0;
            float tw = ImGui::CalcTextSize(b).x;
            dl->AddText(ImVec2(p.x - tw / 2, o.y + sz.y + 2), tc, b);
        }
    }
    for (double v = yS; v <= yMax; v += ys) {
        auto p = ts(0, v);
        if (p.y >= o.y && p.y <= o.y + sz.y) {
            dl->AddLine(ImVec2(o.x, p.y), ImVec2(o.x + 3, p.y), tc);
            char b[32];
            snprintf(b, sizeof(b), "%.10g", v);
            char* e = b + strlen(b) - 1;
            while (e > b && *e == '0') e--;
            if (*e == '.') *e = 0;
            else *(e + 1) = 0;
            dl->AddText(ImVec2(o.x + 4, p.y - 7), tc, b);
        }
    }
    auto po = ts(0, 0);
    if (po.x >= o.x && po.x <= o.x + sz.x && po.y >= o.y && po.y <= o.y + sz.y)
        dl->AddText(ImVec2(po.x - 6, po.y + 2), IM_COL32(120, 120, 160, 255), "O");
    auto px = ts(xMax, 0);
    if (px.x >= o.x)
        dl->AddText(ImVec2(px.x - 6, o.y + sz.y + 2),
                    IM_COL32(120, 120, 160, 255), "x");
    auto py = ts(0, yMax);
    if (py.y >= o.y)
        dl->AddText(ImVec2(o.x + 4, py.y - 14),
                    IM_COL32(120, 120, 160, 255), "y");
}

static void drawIntegralFill(ImDrawList* dl, ImVec2 o, ImVec2 sz,
                              double xMin, double xMax,
                              double yMin, double yMax,
                              double intA, double intB,
                              const std::string& expr) {
    if (intB <= intA) return;
    if (intA >= xMax || intB <= xMin) return;

    double a = std::max(intA, xMin);
    double b = std::min(intB, xMax);

    // Create a local evaluator for sampling
    ExpressionEvaluator eval;
    eval.setExpression(expr);
    if (!eval.valid()) return;

    auto ts = makeToScreen(o, sz, xMin, xMax, yMin, yMax);

    // Sample 200 points in [a, b]
    int nSamples = 200;
    std::vector<ImVec2> pts;
    pts.reserve(nSamples + 2);

    // Start at bottom (y=0 baseline), go along curve, come back
    pts.push_back(ts(a, 0));

    for (int i = 0; i <= nSamples; i++) {
        double t = (double)i / nSamples;
        double x = a + t * (b - a);
        double y = eval.evaluate(x);
        if (!std::isfinite(y)) y = 0;
        // Clamp to visible range
        if (y < yMin) y = yMin;
        if (y > yMax) y = yMax;
        pts.push_back(ts(x, y));
    }

    pts.push_back(ts(b, 0));

    // Draw vertical line fill (preventing color overlap)
    ImU32 fillCol = IM_COL32(255, 180, 60, 80);
    ImU32 borderCol = IM_COL32(255, 180, 60, 200);

    if (pts.size() >= 3) {
        // Draw vertical lines from baseline (y=0) to curve, every 3 pixels
        float stepPx = 3.0f;
        for (float px = 0; px < (b - a) * (sz.x / (xMax - xMin)); px += stepPx) {
            double x = a + px / (sz.x / (xMax - xMin));
            double y = eval.evaluate(x);
            if (!std::isfinite(y)) y = 0;
            if (y < yMin) y = yMin;
            if (y > yMax) y = yMax;
            ImVec2 top = ts(x, y);
            ImVec2 bot = ts(x, 0);
            dl->AddLine(top, bot, fillCol, 1.5f);
        }
        // Draw outline along the curve
        for (size_t i = 2; i < pts.size() - 1; i++) {
            dl->AddLine(pts[i-1], pts[i], borderCol, 2.0f);
        }
        // Also draw the baseline
        dl->AddLine(pts.front(), pts.back(), IM_COL32(255, 180, 60, 100), 1.0f);
    }
}

static void drawPlot(ImDrawList* dl, ImVec2 o, ImVec2 sz,
                     double xMin, double xMax,
                     double yMin, double yMax,
                     const std::vector<PlotData>& plots,
                     const std::vector<ImU32>& colors) {
    auto ts = makeToScreen(o, sz, xMin, xMax, yMin, yMax);
    for (size_t pi = 0; pi < plots.size(); pi++) {
        auto& p = plots[pi];
        auto col = pi < colors.size() ? colors[pi] : IM_COL32(80, 160, 255, 255);
        for (size_t i = 1; i < p.points.size(); i++) {
            auto a = ts(p.points[i - 1].x, p.points[i - 1].y);
            auto b = ts(p.points[i].x, p.points[i].y);
            dl->AddLine(a, b, col, 2);
        }
    }
}

// ============================================================
// Chrome-style Tab Bar for Advanced Mode
// ============================================================
void renderChromeTabBar() {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float availW = ImGui::GetContentRegionAvail().x;

    const char* tabNames[] = {
        T("Calculus", "微积分", "微積分"),
        T("Fourier", "傅里叶", "フーリエ"),
        T("Complex", "复变", "複素"),
        T("Domain", "域着色", "領域"),
        T("ProbStat", "概率统计", "確率統計"),
        T("3D Surf", "3D曲面", "3D曲面"),
        T("Ext", "扩展", "拡張")
    };
    int tabCount = 7;
    float tabH = 32.0f;

    // Tab bar background strip
    dl->AddRectFilled(pos, ImVec2(pos.x + availW, pos.y + tabH), IM_COL32(15, 15, 25, 255));

    float x = pos.x;
    for (int i = 0; i < tabCount; i++) {
        float textW = ImGui::CalcTextSize(tabNames[i]).x;
        float tabW = textW + 20.0f;
        bool selected = (g_state.advTab == i);

        ImVec2 tabPos(x, pos.y + 2);
        ImVec2 tabEnd(x + tabW, pos.y + tabH);

        // Chrome-style tab: rounded top corners
        ImU32 bgCol = selected ? IM_COL32(30, 30, 48, 255) : IM_COL32(20, 20, 34, 200);
        dl->AddRectFilled(tabPos, tabEnd, bgCol, 6.0f, ImDrawFlags_RoundCornersTop);

        // Active tab bottom accent line
        if (selected) {
            dl->AddLine(ImVec2(tabPos.x + 2, tabEnd.y - 1),
                       ImVec2(tabEnd.x - 2, tabEnd.y - 1),
                       IM_COL32(80, 200, 255, 255), 2.0f);
        }

        // Invisible button for click interaction
        char btnId[32];
        snprintf(btnId, sizeof(btnId), "##chromeTab%d", i);
        ImGui::SetCursorScreenPos(tabPos);
        ImGui::InvisibleButton(btnId, ImVec2(tabW, tabH - 2));
        bool hovered = ImGui::IsItemHovered();
        if (ImGui::IsItemActivated()) {
            g_state.advTab = i;
        }

        // Hover highlight
        if (hovered && !selected) {
            dl->AddRectFilled(tabPos, tabEnd, IM_COL32(30, 30, 50, 100), 6.0f, ImDrawFlags_RoundCornersTop);
        }

        // Draw tab label text (Font 3 = 13px fits in 32px height)
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[3]);
        float tw = ImGui::CalcTextSize(tabNames[i]).x;
        ImVec2 textPos(x + (tabW - tw) / 2, pos.y + 8);
        ImU32 textCol = selected ? IM_COL32(230, 230, 245, 255) : IM_COL32(150, 150, 170, 255);
        dl->AddText(textPos, textCol, tabNames[i]);
        ImGui::PopFont();

        x += tabW;
    }

    // Advance cursor below tab bar
    ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + tabH + 4));
}

// ============================================================
// Advanced Tab Content (inline — shared by modal and Chrome mode)
// ============================================================
void renderAdvancedTabContent() {
    // ---- Calculus Tab ----
    if (g_state.advTab == 0) {
        ImGui::Text("%s", T("Expression f(x) =", "表达式 f(x) =", "式 f(x) ="));

        char buf[1024];
        strncpy(buf, g_state.advExpr.empty() ? "sin(x)" : g_state.advExpr.c_str(),
                sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = 0;
        ImGui::PushItemWidth(-1);
        ImGui::InputText("##advExpr", buf, sizeof(buf));
        // Sync on every frame so button clicks always use current input
        g_state.advExpr = buf;
        ImGui::PopItemWidth();

        ImGui::SetNextItemWidth(-1);
        ImGui::InputDouble("x =##advX", &g_state.evalX, 0.1, 1, "%.4f");

        // Evaluate button
        if (ImGui::Button(T("Evaluate f(x)##adv", "计算 f(x)##adv", "計算 f(x)##adv"))) {
            g_state.evaluator.setExpression(g_state.advExpr);
            if (g_state.evaluator.valid()) {
                double v = g_state.evaluator.evaluate(g_state.evalX);
                std::ostringstream oss;
                oss << v;
                g_state.advResult = oss.str();
                g_state.error.clear();
            } else {
                g_state.error = g_state.evaluator.lastError();
            }
        }
        ImGui::SameLine();

        // Draw Graph button (independent, no integral needed)
        if (ImGui::Button(T("Draw Graph##advPlot", "绘制图像##advPlot", "描画##advPlot"))) {
            g_state.plotMode = CalcState::PLOT_FUNC;
            g_state.plots = PlotGenerator::generateMultiple(
                {[&](double x) {
                    g_state.evaluator.setExpression(g_state.advExpr);
                    return g_state.evaluator.evaluate(x);
                }},
                g_state.plotXMin, g_state.plotXMax, 500);
            g_state.showIntegralFill = false;
            g_state.advPlotDirty = true;
        }
        ImGui::SameLine();

        // Integral
        if (ImGui::Button(T("∫ f(x) dx##adv", "定积分 ∫##adv", "定積分 ∫##adv"))) {
            g_state.showIntegralFill = true;
            auto r = Integrator::adaptiveSimpson(
                [&](double x) {
                    g_state.evaluator.setExpression(g_state.advExpr);
                    return g_state.evaluator.evaluate(x);
                },
                g_state.intA, g_state.intB);
            std::ostringstream o;
            // Display: ∫[a,b] f(x) dx = result
            o << "∫[" << g_state.intA << ", " << g_state.intB << "] f(x) dx = " << r.value;
            g_state.advIntegralResult = o.str();
            g_state.plotMode = CalcState::PLOT_INTEG;
            g_state.plots = PlotGenerator::generateMultiple(
                {[&](double x) {
                    g_state.evaluator.setExpression(g_state.advExpr);
                    return g_state.evaluator.evaluate(x);
                }},
                g_state.plotXMin, g_state.plotXMax, 500);
            g_state.advPlotDirty = true;
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(60);
        ImGui::InputDouble(T("Low##al", "下限##al", "下端##al"), &g_state.intA, 0, 0, "%.3f");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(60);
        ImGui::InputDouble(T("Upp##au", "上限##au", "上端##au"), &g_state.intB, 0, 0, "%.3f");

        // Derivative
        if (ImGui::Button("d/dx##adv")) {
            g_state.plotMode = CalcState::PLOT_DERIV;
            g_state.plots = PlotGenerator::generateMultiple(
                {[&](double x) {
                    g_state.evaluator.setExpression(g_state.advExpr);
                    return g_state.evaluator.evaluate(x);
                }},
                g_state.plotXMin, g_state.plotXMax, 500);
            auto deriv = [&](double x) {
                return Differentiator::derivative(
                    [&](double t) {
                        g_state.evaluator.setExpression(g_state.advExpr);
                        return g_state.evaluator.evaluate(t);
                    },
                    x).value;
            };
            g_state.plots.push_back(
                PlotGenerator::generate(deriv, g_state.plotXMin, g_state.plotXMax,
                                        500, "f'(x)"));
            // Compute derivative at current evalX
            double derivVal = deriv(g_state.evalX);
            std::ostringstream do2;
            do2 << "f'(" << g_state.evalX << ") = " << derivVal;
            g_state.advDerivResult = do2.str();
        }

        // Result text
        if (!g_state.advResult.empty()) {
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.3f, 1),
                               "f(%.4f) = %s", g_state.evalX, g_state.advResult.c_str());
        }
        if (!g_state.error.empty()) {
            ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "%s", g_state.error.c_str());
        }
        if (!g_state.advIntegralResult.empty()) {
            ImGui::TextColored(ImVec4(0.6f, 0.4f, 0.1f, 1), "%s",
                               g_state.advIntegralResult.c_str());
        }
        if (!g_state.advDerivResult.empty()) {
            ImGui::TextColored(ImVec4(0.8f, 0.3f, 0.6f, 1), "%s",
                               g_state.advDerivResult.c_str());
        }

        // Plot
        ImVec2 gs(ImGui::GetContentRegionAvail().x, 200);
        if (ImGui::BeginChild("##advPlot", gs, true, ImGuiWindowFlags_NoScrollbar)) {
            auto* dl = ImGui::GetWindowDrawList();
            auto o = ImGui::GetCursorScreenPos();
            auto sz = ImGui::GetContentRegionAvail();
            if (sz.x > 0 && sz.y > 0) {
                float mg = 50;
                ImVec2 po(o.x + mg, o.y), ps(sz.x - mg, sz.y - mg);
                double xMin = g_state.plotXMin, xMax = g_state.plotXMax;
                double yMin = -5, yMax = 5;
                if (g_state.plotMode == CalcState::PLOT_DERIV && g_state.plots.size() >= 2) {
                    yMin = 1e30;
                    yMax = -1e30;
                    for (auto& p : g_state.plots) {
                        if (!p.points.empty()) {
                            if (p.y_min < yMin) yMin = p.y_min;
                            if (p.y_max > yMax) yMax = p.y_max;
                        }
                    }
                    double pad = (yMax - yMin) * 0.1;
                    yMin -= pad;
                    yMax += pad;
                }
                drawGraphBg(dl, po, ps);
                drawGrid(dl, po, ps, xMin, xMax, yMin, yMax);
                drawPlot(dl, po, ps, xMin, xMax, yMin, yMax, g_state.plots,
                         {IM_COL32(80, 200, 255, 255), IM_COL32(255, 180, 80, 255)});
                drawAxesLabels(dl, po, ps, xMin, xMax, yMin, yMax);

                // Draw integral area fill if applicable
                if (g_state.showIntegralFill && !g_state.advExpr.empty()) {
                    drawIntegralFill(dl, po, ps, xMin, xMax, yMin, yMax,
                                     g_state.intA, g_state.intB, g_state.advExpr);
                    // Legend for integral area
                    float legX = po.x + 8;
                    float legY = po.y + 8;
                    dl->AddRectFilled(ImVec2(legX, legY), ImVec2(legX + 130, legY + 22),
                                      IM_COL32(10, 10, 18, 200), 4.0f);
                    dl->AddRectFilled(ImVec2(legX + 4, legY + 6), ImVec2(legX + 20, legY + 16),
                                      IM_COL32(255, 180, 60, 120), 2.0f);
                    dl->AddRect(ImVec2(legX + 4, legY + 6), ImVec2(legX + 20, legY + 16),
                                IM_COL32(255, 180, 60, 200), 2.0f);
                    char legBuf[64];
                    snprintf(legBuf, sizeof(legBuf), "∫ [%.2f, %.2f]", g_state.intA, g_state.intB);
                    dl->AddText(ImVec2(legX + 24, legY + 3),
                                IM_COL32(200, 200, 220, 220), legBuf);
                }

                // Click on plot to expand to large viewer
                ImGui::SetCursorScreenPos(po);
                ImGui::InvisibleButton("##expandPlot", ps);
                if (ImGui::IsItemClicked()) {
                    g_state.showLargePlot = true;
                    g_state.largePlotXMin = xMin; g_state.largePlotXMax = xMax;
                    g_state.largePlotYMin = yMin; g_state.largePlotYMax = yMax;
                    g_state.largePlotDirty = true;
                    g_state.largePlotData = g_state.plots;
                }
                // Show hint on hover
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s",
                        T("Click to expand plot", "点击放大图像", "クリックして拡大"));
                }
                
                // Explicit "Enlarge" button below the plot
                ImGui::SetCursorScreenPos(ImVec2(po.x, po.y + ps.y + 4));
                if (ImGui::Button(T("[+] 放大##expBtn", "[+] 放大##expBtn", "[+] 拡大##expBtn"), ImVec2(80, 24))) {
                    g_state.showLargePlot = true;
                    g_state.largePlotXMin = xMin; g_state.largePlotXMax = xMax;
                    g_state.largePlotYMin = yMin; g_state.largePlotYMax = yMax;
                    g_state.largePlotDirty = true;
                    g_state.largePlotData = g_state.plots;
                }
            }
        }
        ImGui::EndChild();

        // ---- Derivative Graph Improvements ----
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1, 1), "=== %s ===",
                           T("Derivative Graph", "导数图像", "導関数グラフ"));
        if (ImGui::Button(T("f'(x)##der1", "f'(x)##der1", "f'(x)##der1"))) {
            g_state.plotMode = CalcState::PLOT_DERIV;
            g_state.plots = PlotGenerator::generateMultiple(
                {[&](double x) {
                    g_state.evaluator.setExpression(g_state.advExpr);
                    return g_state.evaluator.evaluate(x);
                }},
                g_state.plotXMin, g_state.plotXMax, 500);
            auto deriv = [&](double x) {
                return Differentiator::derivative(
                    [&](double t) {
                        g_state.evaluator.setExpression(g_state.advExpr);
                        return g_state.evaluator.evaluate(t);
                    },
                    x).value;
            };
            g_state.plots.push_back(
                PlotGenerator::generate(deriv, g_state.plotXMin, g_state.plotXMax,
                                        500, "f'(x)"));
            // Find zero crossings and extremes
            g_state.derivZeroPts = findZeroCrossings(deriv, g_state.plotXMin, g_state.plotXMax, 500);
            g_state.derivExtremePts = findLocalExtremes(
                [&](double x) {
                    g_state.evaluator.setExpression(g_state.advExpr);
                    return g_state.evaluator.evaluate(x);
                },
                g_state.plotXMin, g_state.plotXMax, 500);
        }
        ImGui::SameLine();
        if (ImGui::Button(T("f''(x)##der2", "f''(x)##der2", "f''(x)##der2"))) {
            g_state.plotMode = CalcState::PLOT_DERIV;
            g_state.plots = PlotGenerator::generateMultiple(
                {[&](double x) {
                    g_state.evaluator.setExpression(g_state.advExpr);
                    return g_state.evaluator.evaluate(x);
                }},
                g_state.plotXMin, g_state.plotXMax, 500);
            auto f = [&](double x) {
                g_state.evaluator.setExpression(g_state.advExpr);
                return g_state.evaluator.evaluate(x);
            };
            auto deriv1 = [&](double x) {
                return Differentiator::derivative(f, x).value;
            };
            auto deriv2 = [&](double x) {
                return Differentiator::derivative(deriv1, x).value;
            };
            g_state.plots.push_back(
                PlotGenerator::generate(deriv1, g_state.plotXMin, g_state.plotXMax,
                                        500, "f'(x)"));
            g_state.plots.push_back(
                PlotGenerator::generate(deriv2, g_state.plotXMin, g_state.plotXMax,
                                        500, "f''(x)"));
            g_state.derivZeroPts = findZeroCrossings(deriv1, g_state.plotXMin, g_state.plotXMax, 500);
            g_state.derivExtremePts = findLocalExtremes(f, g_state.plotXMin, g_state.plotXMax, 500);
        }
        // Show derivative zero points and extremes info
        if (!g_state.derivZeroPts.empty()) {
            std::string info = T("Zeros of f'(x): ", "f'(x)的零点: ", "f'(x)の零点: ");
            std::ostringstream oss;
            for (size_t i = 0; i < g_state.derivZeroPts.size() && i < 5; i++) {
                if (i > 0) oss << ", ";
                oss.precision(3);
                oss << std::fixed << g_state.derivZeroPts[i];
            }
            if (g_state.derivZeroPts.size() > 5) oss << "...";
            ImGui::TextColored(ImVec4(0.4f, 0.6f, 1, 1), "%s%s", info.c_str(), oss.str().c_str());
        }
        if (!g_state.derivExtremePts.empty()) {
            std::string info = T("Extreme points: ", "极值点: ", "極値点: ");
            std::ostringstream oss;
            for (size_t i = 0; i < g_state.derivExtremePts.size() && i < 5; i++) {
                if (i > 0) oss << ", ";
                oss.precision(3);
                oss << std::fixed << g_state.derivExtremePts[i];
            }
            if (g_state.derivExtremePts.size() > 5) oss << "...";
            ImGui::TextColored(ImVec4(1, 0.6f, 0.3f, 1), "%s%s", info.c_str(), oss.str().c_str());
        }

        // ---- Volume Calculation ----
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1, 1), "=== %s ===",
                           T("Volume Calculation", "物体体积计算", "体積計算"));

        const char* volNames[] = {
            T("Revolution##vt0", "旋转体##vt0", "回転体##vt0"),
            T("Sphere##vt1", "球体##vt1", "球体##vt1"),
            T("Cylinder##vt2", "圆柱##vt2", "円柱##vt2"),
            T("Cone##vt3", "圆锥##vt3", "円錐##vt3"),
            T("Box##vt4", "长方体##vt4", "直方体##vt4")
        };
        for (int i = 0; i < 5; i++) {
            if (i > 0) ImGui::SameLine();
            if (ImGui::RadioButton(volNames[i], &g_state.volType, i)) {}
        }

        if (g_state.volType == 0) {
            // Revolution
            ImGui::Text("%s", T("f(x) = expression entered above", "f(x) = 上方输入的表达式", "f(x) = 上で入力した式"));
            ImGui::Text("%s", T("Around x-axis: V = π∫[f(x)]²dx", "绕x轴: V = π∫[f(x)]²dx", "x軸周り: V = π∫[f(x)]²dx"));
            ImGui::SetNextItemWidth(80);
            ImGui::InputDouble(T("a (lower)##va", "a (下限)##va", "a (下限)##va"), &g_state.revA, 0, 0, "%.3f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(80);
            ImGui::InputDouble(T("b (upper)##vb", "b (上限)##vb", "b (上限)##vb"), &g_state.revB, 0, 0, "%.3f");
            ImGui::SameLine();
            if (ImGui::Button(T("Calc V##vc", "计算体积##vc", "体積計算##vc"))) {
                // V = π∫[f(x)]²dx
                auto r = Integrator::adaptiveSimpson(
                    [&](double x) {
                        g_state.evaluator.setExpression(g_state.advExpr);
                        double fv = g_state.evaluator.evaluate(x);
                        return fv * fv;
                    },
                    g_state.revA, g_state.revB);
                std::ostringstream oss;
                oss.precision(6);
                oss << "V = π * " << r.value << " = " << (std::numbers::pi * r.value);
                g_state.volResult = oss.str();
                g_state.volFormula = "V = π∫[" + g_state.advExpr + "]² dx";
            }
        } else if (g_state.volType == 1) {
            // Sphere
            ImGui::Text("V = 4/3 · π · r³");
            ImGui::SetNextItemWidth(80);
            ImGui::InputDouble(T("r##vs", "半径 r##vs", "半径 r##vs"), &g_state.sphereR, 0, 0, "%.2f");
            ImGui::SameLine();
            if (ImGui::Button(T("Calc V##vsb", "计算体积##vsb", "体積計算##vsb"))) {
                double v = sphereVolume(g_state.sphereR);
                std::ostringstream oss;
                oss.precision(6);
                oss << "V = 4/3·π·(" << g_state.sphereR << ")³ = " << v;
                g_state.volResult = oss.str();
                g_state.volFormula = "V = 4/3 · π · r³";
            }
        } else if (g_state.volType == 2) {
            // Cylinder
            ImGui::Text("V = π · r² · h");
            ImGui::SetNextItemWidth(80);
            ImGui::InputDouble(T("r##vc1", "半径 r##vc1", "半径 r##vc1"), &g_state.cylR, 0, 0, "%.2f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(80);
            ImGui::InputDouble(T("h##vc2", "高度 h##vc2", "高さ h##vc2"), &g_state.cylH, 0, 0, "%.2f");
            ImGui::SameLine();
            if (ImGui::Button(T("Calc V##vcb", "计算体积##vcb", "体積計算##vcb"))) {
                double v = cylinderVolume(g_state.cylR, g_state.cylH);
                std::ostringstream oss;
                oss.precision(6);
                oss << "V = π·(" << g_state.cylR << ")²·" << g_state.cylH << " = " << v;
                g_state.volResult = oss.str();
                g_state.volFormula = "V = π · r² · h";
            }
        } else if (g_state.volType == 3) {
            // Cone
            ImGui::Text("V = 1/3 · π · r² · h");
            ImGui::SetNextItemWidth(80);
            ImGui::InputDouble(T("r##vco1", "半径 r##vco1", "半径 r##vco1"), &g_state.coneR, 0, 0, "%.2f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(80);
            ImGui::InputDouble(T("h##vco2", "高度 h##vco2", "高さ h##vco2"), &g_state.coneH, 0, 0, "%.2f");
            ImGui::SameLine();
            if (ImGui::Button(T("Calc V##vcob", "计算体积##vcob", "体積計算##vcob"))) {
                double v = coneVolume(g_state.coneR, g_state.coneH);
                std::ostringstream oss;
                oss.precision(6);
                oss << "V = 1/3·π·(" << g_state.coneR << ")²·" << g_state.coneH << " = " << v;
                g_state.volResult = oss.str();
                g_state.volFormula = "V = 1/3 · π · r² · h";
            }
        } else if (g_state.volType == 4) {
            // Box
            ImGui::Text("V = a · b · c");
            ImGui::SetNextItemWidth(60);
            ImGui::InputDouble("a##vb1", &g_state.boxA, 0, 0, "%.2f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(60);
            ImGui::InputDouble("b##vb2", &g_state.boxB, 0, 0, "%.2f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(60);
            ImGui::InputDouble("c##vb3", &g_state.boxC, 0, 0, "%.2f");
            ImGui::SameLine();
            if (ImGui::Button(T("Calc V##vbb", "计算体积##vbb", "体積計算##vbb"))) {
                double v = boxVolume(g_state.boxA, g_state.boxB, g_state.boxC);
                std::ostringstream oss;
                oss.precision(6);
                oss << "V = " << g_state.boxA << "·" << g_state.boxB << "·" << g_state.boxC << " = " << v;
                g_state.volResult = oss.str();
                g_state.volFormula = "V = a · b · c";
            }
        }
        if (!g_state.volFormula.empty()) {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 1, 1), "%s", T("Formula: ", "公式: ", "公式: "));
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1), "%s", g_state.volFormula.c_str());
        }
        if (!g_state.volResult.empty()) {
            ImGui::TextColored(ImVec4(0.2f, 1, 0.4f, 1), "V = %s", g_state.volResult.c_str());
        }

        // ---- Surface Integral (Monte Carlo) ----
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1, 1), "=== %s ===",
                           T("Surface Integral (MC)", "曲面积分(蒙特卡洛)", "面積分(モンテカルロ)"));

        const char* surfNames[] = {
            T("Sphere##st0", "球面##st0", "球面##st0"),
            T("Cylinder##st1", "柱面##st1", "円柱面##st1"),
            T("Plane##st2", "平面##st2", "平面##st2"),
            T("Custom##st3", "自定义##st3", "カスタム##st3")
        };
        for (int i = 0; i < 4; i++) {
            if (i > 0) ImGui::SameLine();
            if (ImGui::RadioButton(surfNames[i], &g_state.surfType, i)) {}
        }

        ImGui::Text("f(x,y,z) =");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200);
        ImGui::InputText("##surfFunc", g_state.surfFunc, sizeof(g_state.surfFunc));

        ImGui::SetNextItemWidth(60);
        ImGui::InputDouble(T("u min##su1", "u 最小##su1", "u 最小##su1"), &g_state.surfA, 0, 0, "%.2f");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(60);
        ImGui::InputDouble(T("u max##su2", "u 最大##su2", "u 最大##su2"), &g_state.surfB, 0, 0, "%.2f");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(60);
        ImGui::InputDouble(T("v min##sv1", "v 最小##sv1", "v 最小##sv1"), &g_state.surfC, 0, 0, "%.2f");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(60);
        ImGui::InputDouble(T("v max##sv2", "v 最大##sv2", "v 最大##sv2"), &g_state.surfD, 0, 0, "%.2f");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        ImGui::SliderInt(T("Samples##sms", "采样数##sms", "サンプル数##sms"), &g_state.mcSamples, 100, 50000);

        if (ImGui::Button(T("Compute ∬f dS##scb", "计算曲面积分##scb", "面積分を計算##scb"))) {
            // Define parametric surfaces using std::function (assignable)
            std::function<double(double,double)> px, py, pz;
            px = [](double u, double v) -> double { return 0; };
            py = [](double u, double v) -> double { return 0; };
            pz = [](double u, double v) -> double { return 0; };

            switch (g_state.surfType) {
                case 0: // Sphere: r=1, u=θ[0,π], v=φ[0,2π]
                    px = [](double u, double v) { return std::sin(u) * std::cos(v); };
                    py = [](double u, double v) { return std::sin(u) * std::sin(v); };
                    pz = [](double u, double v) { return std::cos(u); };
                    break;
                case 1: // Cylinder: r=1, h=2
                    px = [](double u, double v) { return std::cos(u); };
                    py = [](double u, double v) { return std::sin(u); };
                    pz = [](double u, double v) { return v; };
                    break;
                case 2: // Plane: z=0
                    px = [](double u, double v) { return u; };
                    py = [](double u, double v) { return v; };
                    pz = [](double u, double v) { return 0; };
                    break;
                case 3: // Custom
                    px = [](double u, double v) { return u; };
                    py = [](double u, double v) { return v; };
                    pz = [&](double u, double v) { return eval3DFunc(g_state.surfFunc, u, v); };
                    break;
            }

            double result = monteCarloSurfaceIntegral(
                [](double x, double y, double z) { return x*x + y*y + z*z; },
                px, py, pz,
                g_state.surfA, g_state.surfB,
                g_state.surfC, g_state.surfD,
                g_state.mcSamples);

            std::ostringstream oss;
            oss.precision(6);
            oss << "∬ f dS ≈ " << std::fixed << result
                << "  (MC samples: " << g_state.mcSamples << ")";
            g_state.surfResult = oss.str();
        }
        if (!g_state.surfResult.empty()) {
            ImGui::TextColored(ImVec4(0.2f, 1, 0.4f, 1), "%s", g_state.surfResult.c_str());
        }
    }

    // ---- Fourier Tab ----
    if (g_state.advTab == 1) {
        if (ImGui::RadioButton(T("Square##ft", "方波##ft", "方形波##ft"), &g_state.fourierType, 0))
            generateFourierPlot();
        ImGui::SameLine();
        if (ImGui::RadioButton(T("Sawtooth##ft", "锯齿波##ft", "鋸波##ft"), &g_state.fourierType, 1))
            generateFourierPlot();
        ImGui::SameLine();
        if (ImGui::RadioButton(T("Triangle##ft", "三角波##ft", "三角波##ft"), &g_state.fourierType, 2))
            generateFourierPlot();
        ImGui::SameLine();
        if (ImGui::Button(T("Redraw##fr", "重绘##fr", "再描画##fr"))) generateFourierPlot();

        ImGui::SetNextItemWidth(200);
        if (ImGui::SliderInt(T("N Terms##fn", "项数 N##fn", "項数 N##fn"),
                             &g_state.fourierTerms, 1, 50)) {
            generateFourierPlot();
        }

        ImVec2 gs(ImGui::GetContentRegionAvail().x, 350);
        if (ImGui::BeginChild("##fourierPlot", gs, true, ImGuiWindowFlags_NoScrollbar)) {
            auto* dl = ImGui::GetWindowDrawList();
            auto o = ImGui::GetCursorScreenPos();
            auto sz = ImGui::GetContentRegionAvail();
            double xM = -10, xX = 10, yM = -2, yX = 2;
            if (!g_state.fourierPlots.empty()) {
                yM = g_state.fourierPlots[0].y_min;
                yX = g_state.fourierPlots[0].y_max;
            }
            float mg = 50;
            ImVec2 po(o.x + mg, o.y), ps(sz.x - mg, sz.y - mg);
            if (ps.x > 0 && ps.y > 0) {
                drawGraphBg(dl, po, ps);
                drawGrid(dl, po, ps, xM, xX, yM, yX);
                for (auto& p : g_state.fourierPlots) {
                    bool ex = p.label.find("Exact") != std::string::npos;
                    auto col = ex ? IM_COL32(255, 160, 80, 255)
                                  : IM_COL32(80, 200, 255, 255);
                    auto ts = makeToScreen(po, ps, xM, xX, yM, yX);
                    for (size_t i = 1; i < p.points.size(); i++)
                        dl->AddLine(ts(p.points[i - 1].x, p.points[i - 1].y),
                                    ts(p.points[i].x, p.points[i].y), col, ex ? 1 : 2);
                }
                drawAxesLabels(dl, po, ps, xM, xX, yM, yX);

                // Click to enlarge
                ImGui::SetCursorScreenPos(po);
                ImGui::InvisibleButton("##expandFourierPlot", ps);
                if (ImGui::IsItemClicked()) {
                    g_state.showLargePlot = true;
                    g_state.largePlotXMin = xM; g_state.largePlotXMax = xX;
                    g_state.largePlotYMin = yM; g_state.largePlotYMax = yX;
                    g_state.largePlotDirty = true;
                    g_state.largePlotData = g_state.fourierPlots;
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s",
                        T("Click to expand plot", "点击放大图像", "クリックして拡大"));
                }
                // Explicit enlarge button
                ImGui::SetCursorScreenPos(ImVec2(po.x, po.y + ps.y + 4));
                if (ImGui::Button(T("[+] 放大##fourierExp", "[+] 放大##fourierExp", "[+] 拡大##fourierExp"), ImVec2(80, 24))) {
                    g_state.showLargePlot = true;
                    g_state.largePlotXMin = xM; g_state.largePlotXMax = xX;
                    g_state.largePlotYMin = yM; g_state.largePlotYMax = yX;
                    g_state.largePlotDirty = true;
                    g_state.largePlotData = g_state.fourierPlots;
                }
            }
        }
        ImGui::EndChild();
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1, 1), "█ %s",
                           T("Fourier approx.", "傅里叶逼近", "フーリエ近似"));
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1, 0.6f, 0.3f, 1), "█ %s",
                           T("Exact", "精确", "正確"));
    }

    // ---- Complex Tab ----
    if (g_state.advTab == 2) {
        ImGui::TextUnformatted(T("Complex Analysis##ct", "复分析##ct", "複素解析##ct"));

        if (ImGui::Button(T("Cauchy ∮##cb", "柯西积分 ∮##cb", "コーシー積分 ∮##cb"))) {
            Complex a(g_state.complexRe, g_state.complexIm);
            auto f = [&](Complex z) -> Complex {
                g_state.evaluator.setExpression(g_state.advExpr);
                return Complex(g_state.evaluator.evaluate(z.real()), 0);
            };
            auto cr = ComplexAnalysis::cauchyIntegral(f, a, g_state.cauchyRadius);
            g_state.complexResult = "Cauchy ∮ = " + to_string(cr, 4);
            g_state.advResult = to_string(cr, 4);
        }
        ImGui::SameLine();
        if (ImGui::Button(T("Residue##cb", "留数##cb", "留数##cb"))) {
            Complex a(g_state.complexRe, g_state.complexIm);
            auto f = [&](Complex z) -> Complex {
                g_state.evaluator.setExpression(g_state.advExpr);
                return Complex(g_state.evaluator.evaluate(z.real()), 0);
            };
            auto res = ComplexAnalysis::residue(f, a);
            g_state.complexResult = "Res(f, " + to_string(a, 4) + ") = " +
                                    to_string(res, 4);
        }
        ImGui::SameLine();
        if (ImGui::Button("Γ(z)##cb")) {
            g_state.advResult = "Γ(0.5) = " +
                                std::to_string(SpecialFunctions::gamma(0.5));
        }

        ImGui::SetNextItemWidth(60);
        ImGui::InputDouble("Re(a)##ci", &g_state.complexRe, 0, 0, "%.2f");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(60);
        ImGui::InputDouble("Im(a)##ci", &g_state.complexIm, 0, 0, "%.2f");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(60);
        ImGui::InputDouble("R=##ci", &g_state.cauchyRadius, 0, 0, "%.2f");

        if (!g_state.complexResult.empty())
            ImGui::TextColored(ImVec4(0.4f, 0.6f, 0.1f, 1), "%s",
                               g_state.complexResult.c_str());
    }

    // ---- Domain Coloring Tab ----
    if (g_state.advTab == 3) {
        ImGui::TextUnformatted("f(z) =");
        ImGui::SameLine();
        char buf[128];
        strncpy(buf, g_state.complexExpr.c_str(), sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = 0;
        if (ImGui::InputText("##ce", buf, sizeof(buf))) {
            g_state.complexExpr = buf;
            g_state.domainDirty = true;
        }
        ImGui::SameLine();
        if (ImGui::Button(T("Render##dr", "渲染##dr", "レンダリング##dr")))
            generateDomainColoring();

        auto pr = [&](const char* n, const char* e) {
            ImGui::SameLine();
            if (ImGui::SmallButton(n)) {
                g_state.complexExpr = e;
                generateDomainColoring();
            }
        };
        pr("z##db0", "z");
        pr("z²##db1", "z^2");
        pr("z³##db2", "z^3");
        pr("1/z##db3", "1/z");
        pr("sin(z)##db4", "sin(z)");
        pr("cos(z)##db5", "cos(z)");
        pr("exp(z)##db6", "exp(z)");
        pr("Γ(z)##db7", "Gamma(z)");

        ImGui::SetNextItemWidth(60);
        ImGui::InputDouble("x min##dd", &g_state.cxMin);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(60);
        ImGui::InputDouble("x max##dd", &g_state.cxMax);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(60);
        ImGui::InputDouble("y min##dd", &g_state.cyMin);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(60);
        ImGui::InputDouble("y max##dd", &g_state.cyMax);
        ImGui::SameLine();
        if (ImGui::Button(T("Update##du", "更新##du", "更新##du")))
            generateDomainColoring();

        ImVec2 gs(ImGui::GetContentRegionAvail().x, 300);
        if (ImGui::BeginChild("##dp", gs, true, ImGuiWindowFlags_NoScrollbar)) {
            auto* dl = ImGui::GetWindowDrawList();
            auto o = ImGui::GetCursorScreenPos();
            auto sz = ImGui::GetContentRegionAvail();
            dl->AddRectFilled(o, ImVec2(o.x + sz.x, o.y + sz.y),
                              IM_COL32(10, 10, 14, 255));
            if (!g_state.domainPts.empty()) {
                float dx = sz.x / (float)(g_state.cxMax - g_state.cxMin);
                float dy = sz.y / (float)(g_state.cyMax - g_state.cyMin);
                for (auto& pt : g_state.domainPts) {
                    float sx = o.x + (float)((pt.x - g_state.cxMin) /
                                              (g_state.cxMax - g_state.cxMin)) * sz.x;
                    float sy = o.y + (float)(1 - (pt.y - g_state.cyMin) /
                                               (g_state.cyMax - g_state.cyMin)) * sz.y;
                    dl->AddRectFilled(
                        ImVec2(sx, sy), ImVec2(sx + dx, sy + dy),
                        IM_COL32((int)(pt.r * 255), (int)(pt.g * 255),
                                 (int)(pt.b * 255), 255));
                }
            } else {
                ImGui::SetCursorPos(
                    ImVec2(gs.x / 2 - 60, gs.y / 2 - 10));
                ImGui::TextUnformatted(
                    T("Press Render", "点击渲染", "レンダリングをクリック"));
            }
        }
        ImGui::EndChild();
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.8f, 1), "%s",
                           T("Hue=arg(f(z)) Bright=|f(z)|",
                             "色相=辐角 亮度=模长",
                             "色相=偏角 輝度=絶対値"));
    }

    // ---- Probability & Statistics Tab ----
    if (g_state.advTab == 4) {
        if (ImGui::BeginChild("##probContent", ImVec2(0, -1), true)) {
            // --- Permutation & Combination ---
            ImGui::TextColored(ImVec4(0.3f, 0.8f, 1, 1), "=== %s ===",
                               T("Permutations & Combinations", "排列与组合", "順列と組合せ"));
            ImGui::SetNextItemWidth(80);
            ImGui::InputDouble(T("n##pc1", "n##pc1", "n##pc1"), (double*)&g_state.probN, 0, 0, "%.0f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(80);
            ImGui::InputDouble(T("k##pc2", "k##pc2", "k##pc2"), (double*)&g_state.probK, 0, 0, "%.0f");
            ImGui::SameLine();
            if (ImGui::Button(T("P(n,k)##pc3", "P(n,k)##pc3", "P(n,k)##pc3"))) {
                if (g_state.probN >= g_state.probK && g_state.probK >= 0) {
                    auto r = permutationLL(g_state.probN, g_state.probK);
                    g_state.probPermResult = "P(" + std::to_string(g_state.probN) + "," +
                                             std::to_string(g_state.probK) + ") = " + std::to_string(r);
                } else {
                    g_state.probPermResult = T("Invalid n,k", "无效参数", "無効なパラメータ");
                }
            }
            ImGui::SameLine();
            if (ImGui::Button(T("C(n,k)##pc4", "C(n,k)##pc4", "C(n,k)##pc4"))) {
                if (g_state.probN >= g_state.probK && g_state.probK >= 0) {
                    auto r = combinationLL(g_state.probN, g_state.probK);
                    g_state.probCombResult = "C(" + std::to_string(g_state.probN) + "," +
                                             std::to_string(g_state.probK) + ") = " + std::to_string(r);
                } else {
                    g_state.probCombResult = T("Invalid n,k", "无效参数", "無効なパラメータ");
                }
            }
            ImGui::SameLine();
            if (ImGui::Button(T("n! ##pc5", "n! ##pc5", "n! ##pc5"))) {
                auto r = factorialLL(g_state.probN);
                g_state.probFactResult = std::to_string(g_state.probN) + "! = " + std::to_string(r);
            }
            if (!g_state.probPermResult.empty())
                ImGui::TextColored(ImVec4(0.2f, 1, 0.4f, 1), "%s", g_state.probPermResult.c_str());
            if (!g_state.probCombResult.empty())
                ImGui::TextColored(ImVec4(0.2f, 1, 0.4f, 1), "%s", g_state.probCombResult.c_str());
            if (!g_state.probFactResult.empty())
                ImGui::TextColored(ImVec4(0.2f, 1, 0.4f, 1), "%s", g_state.probFactResult.c_str());

            ImGui::Separator();

            // --- Binomial Distribution ---
            ImGui::TextColored(ImVec4(0.3f, 0.8f, 1, 1), "=== %s ===",
                               T("Binomial", "二项分布", "二項分布"));
            ImGui::SetNextItemWidth(60);
            ImGui::InputDouble(T("n##bd1", "n##bd1", "n##bd1"), (double*)&g_state.binomN, 0, 0, "%.0f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(60);
            ImGui::InputDouble(T("k##bd2", "k##bd2", "k##bd2"), (double*)&g_state.binomK, 0, 0, "%.0f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(80);
            ImGui::InputDouble(T("p##bd3", "p##bd3", "p##bd3"), &g_state.binomP, 0.1, 0.5, "%.4f");
            ImGui::SameLine();
            if (ImGui::Button(T("P(X=k)##bd4", "P(X=k)##bd4", "P(X=k)##bd4"))) {
                double p = binomialProb(g_state.binomN, g_state.binomK, g_state.binomP);
                std::ostringstream oss;
                oss.precision(6);
                oss << "P(X=" << g_state.binomK << ") = " << std::fixed << p
                    << " = " << (p * 100) << "%";
                g_state.binomResult = oss.str();
            }
            if (!g_state.binomResult.empty())
                ImGui::TextColored(ImVec4(0.2f, 1, 0.4f, 1), "%s", g_state.binomResult.c_str());

            // --- Normal Distribution ---
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.3f, 0.8f, 1, 1), "=== %s ===",
                               T("Normal Distribution", "正态分布", "正規分布"));
            ImGui::SetNextItemWidth(60);
            ImGui::InputDouble("x##nd1", &g_state.normX, 0, 0, "%.3f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(60);
            ImGui::InputDouble("μ##nd2", &g_state.normMu, 0, 0, "%.3f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(60);
            ImGui::InputDouble("σ##nd3", &g_state.normSigma, 0, 0, "%.3f");
            ImGui::SameLine();
            if (ImGui::Button(T("Φ(x)##nd4", "CDF Φ(x)##nd4", "CDF Φ(x)##nd4"))) {
                double z = (g_state.normX - g_state.normMu) / g_state.normSigma;
                double cdf = normalCDF(z);
                std::ostringstream oss;
                oss.precision(6);
                oss << "Φ(" << g_state.normX << ") = " << std::fixed << cdf
                    << " = " << (cdf * 100) << "%";
                g_state.normCdfResult = oss.str();
            }
            ImGui::SameLine();
            if (ImGui::Button(T("Inv Φ(p)##nd5", "逆Φ(p)##nd5", "逆Φ(p)##nd5"))) {
                double inv = normalInv(g_state.normX);
                double x = g_state.normMu + inv * g_state.normSigma;
                std::ostringstream oss;
                oss.precision(6);
                oss << "Φ⁻¹(" << g_state.normX << ") = " << std::fixed << x;
                g_state.normInvResult = oss.str();
            }
            if (!g_state.normCdfResult.empty())
                ImGui::TextColored(ImVec4(0.2f, 1, 0.4f, 1), "%s", g_state.normCdfResult.c_str());
            if (!g_state.normInvResult.empty())
                ImGui::TextColored(ImVec4(0.2f, 1, 0.4f, 1), "%s", g_state.normInvResult.c_str());

            // --- Poisson Distribution ---
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.3f, 0.8f, 1, 1), "=== %s ===",
                               T("Poisson", "泊松分布", "ポアソン分布"));
            ImGui::SetNextItemWidth(80);
            ImGui::InputDouble("λ##pd1", &g_state.poisLambda, 0, 0, "%.3f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(60);
            ImGui::InputDouble("k##pd2", (double*)&g_state.poisK, 0, 0, "%.0f");
            ImGui::SameLine();
            if (ImGui::Button(T("P(X=k)##pd3", "P(X=k)##pd3", "P(X=k)##pd3"))) {
                double p = poissonProb(g_state.poisLambda, g_state.poisK);
                std::ostringstream oss;
                oss.precision(6);
                oss << "P(X=" << g_state.poisK << ") = " << std::fixed << p
                    << " = " << (p * 100) << "%";
                g_state.poisResult = oss.str();
            }
            if (!g_state.poisResult.empty())
                ImGui::TextColored(ImVec4(0.2f, 1, 0.4f, 1), "%s", g_state.poisResult.c_str());

            // --- Descriptive Statistics ---
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.3f, 0.8f, 1, 1), "=== %s ===",
                               T("Descriptive Stats", "描述性统计", "記述統計"));
            ImGui::Text("%s", T("Enter data (comma separated):",
                               "输入数据（逗号分隔）:", "データ（カンマ区切り）:"));
            ImGui::SetNextItemWidth(-1);
            ImGui::InputTextMultiline("##statsData", g_state.statsDataBuf,
                                      sizeof(g_state.statsDataBuf),
                                      ImVec2(0, 50));
            if (ImGui::Button(T("Calculate##stb", "计算统计##stb", "統計計算##stb"))) {
                auto data = parseDataList(g_state.statsDataBuf);
                if (data.size() >= 2) {
                    auto s = calcDescriptiveStats(data);
                    std::ostringstream oss;
                    oss.precision(4);
                    oss << std::fixed;
                    oss << "数据个数: " << s.count << "\n";
                    oss << "总和: " << s.sum << "\n";
                    oss << "平均值: " << s.mean << "\n";
                    oss << "中位数: " << s.median << "\n";
                    oss << "众数: " << s.modeStr << "\n";
                    oss << "方差: " << s.variance << "\n";
                    oss << "标准差: " << s.stddev << "\n";
                    oss << "最小值: " << s.minVal << "\n";
                    oss << "最大值: " << s.maxVal << "\n";
                    oss << "范围: " << s.range;
                    g_state.statsResult = oss.str();
                    // Store histogram data
                    g_state.histData = data;
                    g_state.histDirty = true;
                } else if (data.size() == 1) {
                    g_state.statsResult = T("Need at least 2 data points",
                                           "至少需要2个数据点", "少なくとも2つのデータ点が必要");
                } else {
                    g_state.statsResult = T("No valid data entered",
                                           "没有有效数据", "有効なデータがありません");
                }
            }
            if (!g_state.statsResult.empty()) {
                ImGui::BeginChild("##statsRes", ImVec2(0, 140), true);
                ImGui::TextColored(ImVec4(0.2f, 1, 0.4f, 1), "%s", g_state.statsResult.c_str());
                ImGui::EndChild();
            }

            // --- Histogram ---
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.3f, 0.8f, 1, 1), "=== %s ===",
                               T("Histogram", "统计柱状图", "ヒストグラム"));
            if (!g_state.histData.empty()) {
                ImGui::SetNextItemWidth(200);
                if (ImGui::SliderInt(T("Bins##hb", "分组数##hb", "区間数##hb"),
                                     &g_state.histNBins, 3, 30)) {
                    g_state.histDirty = true;
                }
                if (g_state.histDirty) {
                    calcHistogram(g_state.histData, g_state.histNBins,
                                  g_state.histCounts, g_state.histMin, g_state.histMax);
                    g_state.histDirty = false;
                }
                ImVec2 hsz(ImGui::GetContentRegionAvail().x, 180);
                if (hsz.x > 50 && hsz.y > 50) {
                    if (ImGui::BeginChild("##histCanvas", hsz, true, ImGuiWindowFlags_NoScrollbar)) {
                        auto* dl = ImGui::GetWindowDrawList();
                        auto o = ImGui::GetCursorScreenPos();
                        auto sz = ImGui::GetContentRegionAvail();
                        drawHistogram(dl, o, sz, g_state.histCounts, g_state.histData,
                                     g_state.histMin, g_state.histMax, g_state.histNBins);
                    }
                    ImGui::EndChild();
                }
            } else {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "%s",
                                   T("Enter data above and calculate first",
                                     "请先在上方输入数据并计算", "上でデータを入力して計算してください"));
            }
        }
        ImGui::EndChild();
    }

    // ---- 3D Surface Tab ----
    if (g_state.advTab == 5) {
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1, 1), "=== %s ===",
                           T("3D Surface Plot", "3D曲面图", "3Dサーフェス"));

        ImGui::Text("f(x,y) =");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(300);
        if (ImGui::InputText("##func3d", g_state.func3D, sizeof(g_state.func3D))) {
            g_state.surf3DDirty = true;
        }
        ImGui::SameLine();
        if (ImGui::Button(T("Update##3du", "更新##3du", "更新##3du"))) {
            g_state.surf3DDirty = true;
        }

        // Rotation and zoom controls
        ImGui::SetNextItemWidth(150);
        if (ImGui::SliderFloat(T("Rot X##3rx", "旋转 X##3rx", "回転 X##3rx"),
                               &g_state.rot3DX, -180, 180, "%.1f°")) {
            g_state.surf3DDirty = true;
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(150);
        if (ImGui::SliderFloat(T("Rot Y##3ry", "旋转 Y##3ry", "回転 Y##3ry"),
                               &g_state.rot3DY, -180, 180, "%.1f°")) {
            g_state.surf3DDirty = true;
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        if (ImGui::SliderFloat(T("Zoom##3dz", "缩放##3dz", "ズーム##3dz"),
                               &g_state.zoom3D, 0.1f, 5.0f, "%.1f")) {
            g_state.surf3DDirty = true;
        }

        // Range controls
        ImGui::SetNextItemWidth(80);
        ImGui::InputDouble(T("Min##3dm", "最小##3dm", "最小##3dm"), &g_state.range3DMin, 0, 0, "%.1f");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        ImGui::InputDouble(T("Max##3dx", "最大##3dx", "最大##3dx"), &g_state.range3DMax, 0, 0, "%.1f");
        ImGui::SameLine();
        if (ImGui::Button(T("Reset##3dr", "重置##3dr", "リセット##3dr"))) {
            g_state.rot3DX = 30.0f; g_state.rot3DY = -30.0f;
            g_state.zoom3D = 1.0f;
            g_state.range3DMin = -5.0; g_state.range3DMax = 5.0;
            g_state.surf3DDirty = true;
        }
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.7f, 1), "%s",
                           T("z: color(red=high, blue=low)",
                             "z: 颜色(红=高, 蓝=低)",
                             "z: 色(赤=高, 青=低)"));
        ImGui::SameLine();
        if (ImGui::SmallButton(T("Enlarge##3de", "放大##3de", "拡大##3de"))) {
            g_state.showLarge3D = true;
            g_state.large3DRotX = g_state.rot3DX;
            g_state.large3DRotY = g_state.rot3DY;
            g_state.large3DZoom = g_state.zoom3D;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s",
                T("Enlarge 3D view to full screen",
                  "放大3D视图至全屏",
                  "3D表示を全画面に拡大"));
        }

        // Reset dirty flag and get expression
        g_state.surf3DDirty = false;
        std::string funcExpr = g_state.func3D;

        // Draw the 3D surface
        ImVec2 gs(ImGui::GetContentRegionAvail().x, 0);
        float minH = 320.0f;
        if (gs.y < minH) gs.y = minH;
        if (ImGui::BeginChild("##surf3dPlot", gs, true, ImGuiWindowFlags_NoScrollbar)) {
            auto* dl = ImGui::GetWindowDrawList();
            auto o = ImGui::GetCursorScreenPos();
            auto sz = ImGui::GetContentRegionAvail();
            if (sz.x > 50 && sz.y > 50) {
                draw3DSurface(dl, o, sz, funcExpr,
                             g_state.range3DMin, g_state.range3DMax,
                             g_state.rot3DX, g_state.rot3DY, g_state.zoom3D);
            }

            // Invisible button for orbit control
            ImGui::SetCursorScreenPos(o);
            ImGui::InvisibleButton("##surfDrag", sz);
            if (ImGui::IsItemHovered()) {
                // Mouse wheel zoom
                float wheel = ImGui::GetIO().MouseWheel;
                if (wheel != 0.0f) {
                    g_state.zoom3D *= (1.0f + wheel * 0.1f);
                    if (g_state.zoom3D < 0.1f) g_state.zoom3D = 0.1f;
                    if (g_state.zoom3D > 5.0f) g_state.zoom3D = 5.0f;
                    g_state.surf3DDirty = true;
                }
                // Left-click drag to rotate
                if (ImGui::IsMouseDown(0)) {
                    ImVec2 mp = ImGui::GetIO().MousePos;
                    if (!g_state.surfIsDragging) {
                        g_state.surfIsDragging = true;
                        g_state.surfDragLastX = mp.x;
                        g_state.surfDragLastY = mp.y;
                    } else {
                        float dx = mp.x - g_state.surfDragLastX;
                        float dy = mp.y - g_state.surfDragLastY;
                        if (dx != 0 || dy != 0) {
                            g_state.rot3DY += dx * 0.3f;
                            g_state.rot3DX += dy * 0.3f;
                            g_state.surfDragLastX = mp.x;
                            g_state.surfDragLastY = mp.y;
                            g_state.surf3DDirty = true;
                        }
                    }
                } else {
                    g_state.surfIsDragging = false;
                }
            } else {
                g_state.surfIsDragging = false;
            }
        }
        ImGui::EndChild();
        
        // Enlarge button for 3D view
        ImGui::SameLine();
        if (ImGui::Button(T("[+] 全屏##3dzoom", "[+] 全屏##3dzoom", "[+] 全画面##3dzoom"), ImVec2(80, 28))) {
            g_state.showLarge3D = true;
        }
    }

    // ---- Extension Tab ----
    if (g_state.advTab == 6) {
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1, 1), "=== %s ===",
                           T("Extension API", "扩展插件", "拡張プラグイン"));

        // Extension selector
        if (g_state.extensions.empty()) {
            initExtensions();
        }

        const char* extNames[64];
        int nExt = (int)g_state.extensions.size();
        for (int i = 0; i < nExt && i < 64; i++) {
            extNames[i] = g_state.extensions[i].name;
        }
        ImGui::SetNextItemWidth(200);
        ImGui::Combo(T("Extension##es", "扩展##es", "拡張##es"),
                     &g_state.extSelected, extNames, nExt);

        if (g_state.extSelected >= 0 && g_state.extSelected < nExt) {
            auto& ext = g_state.extensions[g_state.extSelected];
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.8f, 1), "%s", ext.desc);
            ImGui::SetNextItemWidth(300);
            ImGui::InputText("##extInput", g_state.extInput, sizeof(g_state.extInput));
            ImGui::SameLine();
            if (ImGui::Button(T("Run##extr", "运行##extr", "実行##extr"))) {
                if (g_state.extInput[0] != '\0') {
                    g_state.extResult = ext.func(g_state.extInput);
                } else if (ext.exampleInput) {
                    g_state.extResult = ext.func(ext.exampleInput);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button(T("Example##exte", "示例##exte", "例##exte"))) {
                if (ext.exampleInput) {
                    strncpy(g_state.extInput, ext.exampleInput, sizeof(g_state.extInput) - 1);
                    g_state.extInput[sizeof(g_state.extInput) - 1] = '\0';
                    g_state.extResult = ext.func(ext.exampleInput);
                }
            }
            ImGui::Separator();
            // Result area
            if (!g_state.extResult.empty()) {
                ImVec2 rsz(0, 120);
                if (ImGui::BeginChild("##extRes", rsz, true)) {
                    ImGui::TextColored(ImVec4(0.2f, 1, 0.4f, 1), "%s", g_state.extResult.c_str());
                }
                ImGui::EndChild();
            }
        }

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.6f, 1), "%s",
                           T("Extensions use a simple C function pointer API. "
                             "Add new .h/.cpp files to add custom extensions.",
                             "扩展使用简单的C函数指针API。添加.h/.cpp文件即可自定义扩展。",
                             "拡張はシンプルなC関数ポインタAPIを使用します。.h/.cppファイルを追加してカスタム拡張を追加します。"));

        ImGui::Separator();
        // ============================================================
        // Currency Converter (enhanced, inside extension tab)
        // ============================================================
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1, 1), "=== %s ===",
                           T("Currency Converter", "货币换算", "為替換算"));

        // Source currency Combo
        ImGui::SetNextItemWidth(120);
        ImGui::Combo(T("From##ecf", "从##ecf", "元##ecf"),
                     &g_state.exchangeFromIdx, g_currencyCodes, g_currencyCount);
        ImGui::SameLine();

        // Swap button
        if (ImGui::Button(T("⇄##ecs", "⇄##ecs", "⇄##ecs"))) {
            std::swap(g_state.exchangeFromIdx, g_state.exchangeToIdx);
        }
        ImGui::SameLine();

        // Target currency Combo
        ImGui::SetNextItemWidth(120);
        ImGui::Combo(T("To##ect", "至##ect", "先##ect"),
                     &g_state.exchangeToIdx, g_currencyCodes, g_currencyCount);

        // Amount input
        ImGui::SetNextItemWidth(120);
        ImGui::InputText("##ecAmount", g_state.exchangeAmountBuf,
                         sizeof(g_state.exchangeAmountBuf));
        ImGui::SameLine();

        // Convert button
        if (ImGui::Button(T("Convert##ecb", "换算##ecb", "換算##ecb"))) {
            if (!g_state.exchangeRates.empty()) {
                try {
                    double amt = std::stod(g_state.exchangeAmountBuf);
                    g_state.exchangeAmount = amt;
                    std::string from = g_currencyCodes[g_state.exchangeFromIdx];
                    std::string to = g_currencyCodes[g_state.exchangeToIdx];
                    g_state.exchangeResult = convertCurrency(amt, from, to);
                    // Add to history
                    std::ostringstream h;
                    h.precision(4);
                    h << std::fixed << amt << " " << from << " = " << g_state.exchangeResult << " " << to;
                    g_state.exchangeHistory.push_back(h.str());
                    if (g_state.exchangeHistory.size() > 10)
                        g_state.exchangeHistory.erase(g_state.exchangeHistory.begin());
                } catch (...) {
                    g_state.exchangeRateInfo = T("Invalid amount", "无效金额", "無効な金額");
                }
            } else {
                g_state.exchangeRateInfo = T(
                    "No exchange rates. Click 'Fetch Rates' first.",
                    "没有汇率数据，请先点击'获取最新汇率'",
                    "為替レートがありません。'レート取得'をクリックしてください。");
            }
        }

        // Result display
        if (g_state.exchangeResult != 0.0) {
            char resBuf[128];
            snprintf(resBuf, sizeof(resBuf), "%.4f %s = %.4f %s",
                     g_state.exchangeAmount,
                     g_currencyCodes[g_state.exchangeFromIdx],
                     g_state.exchangeResult,
                     g_currencyCodes[g_state.exchangeToIdx]);
            ImGui::TextColored(ImVec4(0.2f, 1, 0.4f, 1), "%s", resBuf);

            // Show rate
            if (!g_state.exchangeRates.empty()) {
                std::string from = g_currencyCodes[g_state.exchangeFromIdx];
                std::string to = g_currencyCodes[g_state.exchangeToIdx];
                auto itF = g_state.exchangeRates.find(from);
                auto itT = g_state.exchangeRates.find(to);
                if (itF != g_state.exchangeRates.end() && itT != g_state.exchangeRates.end()) {
                    double rate = itT->second / itF->second;
                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.8f, 1),
                                       "1 %s = %.6f %s", from.c_str(), rate, to.c_str());
                }
            }
        }

        // Fetch rates button + timestamp
        if (ImGui::Button(T("Fetch Latest Rates##ecf", "获取最新汇率##ecf", "レート取得##ecf"))) {
            fetchExchangeRates();
        }
        ImGui::SameLine();
        if (ImGui::Button(T("Load Cache##ecc", "加载缓存##ecc", "キャッシュ読込##ecc"))) {
            if (loadExchangeRatesCache()) {
                g_state.exchangeRateInfo += T(" (loaded)", " (已加载)", " (読込完了)");
            } else {
                g_state.exchangeRateInfo = T("No cache found", "无缓存数据", "キャッシュがありません");
            }
        }

        // Timestamp + source info
        if (g_state.exchangeRateTime > 0) {
            char timeBuf[64];
            struct tm* t = localtime(&g_state.exchangeRateTime);
            strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", t);
            std::string info = T("Rates updated: ", "汇率更新时间: ", "レート更新: ");
            info += timeBuf;
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.7f, 1), "%s", info.c_str());
        }
        if (!g_state.exchangeRateInfo.empty()) {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.7f, 1), "%s", g_state.exchangeRateInfo.c_str());
        }
        ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.6f, 1), "%s",
                           T("Data: open.er-api.com", "数据来源: open.er-api.com", "データ: open.er-api.com"));

        // Trend chart button
        ImGui::SameLine();
        if (ImGui::Button(T("Draw Trend##ect", "绘制走势图##ect", "トレンド描画##ect"))) {
            g_state.exchangeTrendVisible = !g_state.exchangeTrendVisible;
            if (g_state.exchangeTrendVisible) {
                // Generate simulated 30-day trend data
                g_state.exchangeTrendData.clear();
                if (!g_state.exchangeRates.empty()) {
                    std::string from = g_currencyCodes[g_state.exchangeFromIdx];
                    std::string to = g_currencyCodes[g_state.exchangeToIdx];
                    auto itF = g_state.exchangeRates.find(from);
                    auto itT = g_state.exchangeRates.find(to);
                    if (itF != g_state.exchangeRates.end() && itT != g_state.exchangeRates.end()) {
                        double baseRate = itT->second / itF->second;
                        // Generate 30 days of simulated data with sine wave variation
                        for (int d = 0; d < 30; d++) {
                            double variation = 0.02 * std::sin(d * 0.5) +
                                               0.015 * std::cos(d * 0.3) +
                                               0.01 * std::sin(d * 0.7 + 1.0);
                            double trend = 0.001 * d / 30.0; // slight trend
                            g_state.exchangeTrendData.push_back(baseRate * (1.0 + variation + trend));
                        }
                    }
                }
                if (g_state.exchangeTrendData.empty()) {
                    // Fallback dummy data
                    for (int d = 0; d < 30; d++)
                        g_state.exchangeTrendData.push_back(7.2 + 0.3 * std::sin(d * 0.3));
                }
            }
        }

        // Trend chart
        if (g_state.exchangeTrendVisible && !g_state.exchangeTrendData.empty()) {
            ImVec2 trendSz(ImGui::GetContentRegionAvail().x, 180);
            if (trendSz.x > 50 && trendSz.y > 50) {
                if (ImGui::BeginChild("##ecTrend", trendSz, true, ImGuiWindowFlags_NoScrollbar)) {
                    auto* dl = ImGui::GetWindowDrawList();
                    auto o = ImGui::GetCursorScreenPos();
                    auto sz = ImGui::GetContentRegionAvail();

                    // Background
                    dl->AddRectFilled(o, ImVec2(o.x + sz.x, o.y + sz.y), IM_COL32(8, 8, 14, 255));
                    dl->AddRect(o, ImVec2(o.x + sz.x, o.y + sz.y), IM_COL32(40, 40, 60, 200), 2.0f);

                    float mg = 50.0f;
                    float px = o.x + mg;
                    float py = o.y + 10.0f;
                    float pw = sz.x - mg - 10.0f;
                    float ph = sz.y - mg - 10.0f;
                    if (pw > 20 && ph > 20) {
                        // Find min/max
                        double mn = g_state.exchangeTrendData[0], mx = g_state.exchangeTrendData[0];
                        for (double v : g_state.exchangeTrendData) {
                            if (v < mn) mn = v;
                            if (v > mx) mx = v;
                        }
                        double pad = (mx - mn) * 0.1;
                        if (pad < 0.001) pad = 0.1;
                        mn -= pad;
                        mx += pad;

                        auto toScr = [&](int idx, double val) -> ImVec2 {
                            float fx = px + (float)idx / 29.0f * pw;
                            float fy = py + ph - (float)((val - mn) / (mx - mn)) * ph;
                            return ImVec2(fx, fy);
                        };

                        // Grid lines
                        for (int i = 0; i <= 5; i++) {
                            float y = py + ph * (float)i / 5.0f;
                            dl->AddLine(ImVec2(px, y), ImVec2(px + pw, y), IM_COL32(30, 30, 50, 150));
                            double val = mn + (mx - mn) * (1.0 - (double)i / 5.0);
                            char lb[32];
                            snprintf(lb, sizeof(lb), "%.4f", val);
                            dl->AddText(ImVec2(px - ImGui::CalcTextSize(lb).x - 4, y - 7),
                                        IM_COL32(100, 100, 140, 180), lb);
                        }

                        // X-axis day labels
                        for (int d = 0; d < 30; d += 7) {
                            ImVec2 lp = toScr(d, mn);
                            char db[8];
                            snprintf(db, sizeof(db), "D%d", d + 1);
                            dl->AddText(ImVec2(lp.x - 8, py + ph + 4),
                                        IM_COL32(100, 100, 140, 180), db);
                        }

                        // Y-axis label
                        std::string pair = std::string(g_currencyCodes[g_state.exchangeFromIdx]) + "/" +
                                          g_currencyCodes[g_state.exchangeToIdx];
                        dl->AddText(ImVec2(px + 4, py + 2),
                                    IM_COL32(120, 120, 180, 200), pair.c_str());

                        // Draw the trend line
                        ImU32 lineCol = IM_COL32(80, 200, 255, 220);
                        ImU32 fillCol = IM_COL32(80, 200, 255, 40);
                        std::vector<ImVec2> linePts;
                        linePts.reserve(30);
                        for (int d = 0; d < 30; d++) {
                            linePts.push_back(toScr(d, g_state.exchangeTrendData[d]));
                        }
                        // Draw fill under the line
                        if (linePts.size() >= 2) {
                            std::vector<ImVec2> fillPts;
                            fillPts.push_back(ImVec2(linePts[0].x, py + ph));
                            for (auto& p : linePts)
                                fillPts.push_back(p);
                            fillPts.push_back(ImVec2(linePts.back().x, py + ph));
                            dl->AddConvexPolyFilled(fillPts.data(), (int)fillPts.size(), fillCol);
                        }
                        // Draw the line
                        for (size_t i = 1; i < linePts.size(); i++) {
                            dl->AddLine(linePts[i-1], linePts[i], lineCol, 2.0f);
                        }

                        // Min/Max markers
                        int minIdx = 0, maxIdx = 0;
                        for (int d = 0; d < 30; d++) {
                            if (g_state.exchangeTrendData[d] < g_state.exchangeTrendData[minIdx]) minIdx = d;
                            if (g_state.exchangeTrendData[d] > g_state.exchangeTrendData[maxIdx]) maxIdx = d;
                        }
                        ImVec2 minP = toScr(minIdx, g_state.exchangeTrendData[minIdx]);
                        ImVec2 maxP = toScr(maxIdx, g_state.exchangeTrendData[maxIdx]);
                        dl->AddCircleFilled(minP, 4, IM_COL32(100, 255, 100, 220));
                        dl->AddText(ImVec2(minP.x - 8, minP.y - 16),
                                    IM_COL32(100, 255, 100, 220), "min");
                        dl->AddCircleFilled(maxP, 4, IM_COL32(255, 100, 100, 220));
                        dl->AddText(ImVec2(maxP.x - 10, maxP.y + 4),
                                    IM_COL32(255, 100, 100, 220), "max");

                        // Current value marker
                        ImVec2 curP = toScr(29, g_state.exchangeTrendData[29]);
                        dl->AddCircleFilled(curP, 5, lineCol);
                        char cvb[16];
                        snprintf(cvb, sizeof(cvb), "%.4f", g_state.exchangeTrendData[29]);
                        dl->AddText(ImVec2(curP.x + 6, curP.y - 8), lineCol, cvb);
                    }
                }
                ImGui::EndChild();
            }
        }

        // Conversion history
        if (!g_state.exchangeHistory.empty()) {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.4f, 0.6f, 0.8f, 1), "%s",
                               T("Conversion History", "换算记录", "換算履歴"));
            ImGui::BeginChild("##ecHistory", ImVec2(0, 100), true);
            for (auto& entry : g_state.exchangeHistory) {
                ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.5f, 1), "%s", entry.c_str());
            }
            ImGui::EndChild();
        }
    }

    // ---- Volume Calculation (embedded in calculus or standalone section) ----
    // This section is shown directly in the calculus tab below the derivative stuff
}

// ============================================================
// Advanced Mode Modal (floating window — backward compatible)
// ============================================================
void renderAdvancedModal() {
    if (!g_state.showAdvanced) return;

    ImGui::SetNextWindowSize(ImVec2(720, 600), ImGuiCond_FirstUseEver);
    ImGui::Begin(T("Advanced Mode##adv", "高级模式##adv", "応用モード##adv"),
                 &g_state.showAdvanced);

    // Inline tab buttons (kept for backward compat with the old modal)
    const char* tabNames[] = {
        T("Calculus##t0", "微积分##t0", "微積分##t0"),
        T("Fourier##t1", "傅里叶##t1", "フーリエ##t1"),
        T("Complex##t2", "复变函数##t2", "複素関数##t2"),
        T("Domain##t3", "域着色##t3", "領域彩色##t3"),
        T("ProbStats##t4", "概率统计##t4", "確率統計##t4"),
        T("3DSurface##t5", "3D曲面##t5", "3D曲面##t5"),
        T("Ext##t6", "扩展##t6", "拡張##t6")
    };
    int tabCount = 7;

    for (int i = 0; i < tabCount; i++) {
        if (i > 0) ImGui::SameLine();
        bool sel = (g_state.advTab == i);
        if (sel) {
            ImGui::PushStyleColor(ImGuiCol_Button, COL_BTN_FUNC);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, COL_BTN_FUNC_H);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, COL_BTN_FUNC_A);
        }
        if (ImGui::Button(tabNames[i])) {
            g_state.advTab = i;
        }
        if (sel) ImGui::PopStyleColor(3);
    }
    ImGui::Separator();

    // Render tab content using the shared function
    renderAdvancedTabContent();

    ImGui::End();
}

// ============================================================
// Bottom Function Graph WITH user input field
// ============================================================
void renderBottomGraphWithInput() {
    float availW = ImGui::GetContentRegionAvail().x;
    float availH = ImGui::GetContentRegionAvail().y;

    ImDrawList* dl = ImGui::GetWindowDrawList();

    // === Function Input Row ===
    ImVec2 cp = ImGui::GetCursorScreenPos();

    // Color marker (green square)
    dl->AddRectFilled(ImVec2(cp.x, cp.y + 4), ImVec2(cp.x + 12, cp.y + 20),
                      IM_COL32(80, 220, 120, 255), 2.0f);

    // Expression input field
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[3]);
    ImGui::SetCursorScreenPos(ImVec2(cp.x + 18, cp.y));
    ImGui::SetNextItemWidth(availW - 95);
    bool enterPressed = ImGui::InputText("##funcInput", g_state.graphFunc,
                                          sizeof(g_state.graphFunc),
                                          ImGuiInputTextFlags_EnterReturnsTrue);

    ImGui::SameLine(0, 4);
    bool drawClicked = styledButton(T("Draw##fd", "绘制##fd", "描画##fd"),
                                    COL_BTN_FUNC, COL_BTN_FUNC_H, COL_BTN_FUNC_A,
                                    ImVec2(60, 24));
    ImGui::PopFont();

    bool needRedraw = enterPressed || drawClicked;

    // === Generate Plot Data from User Expression ===
    static bool plotGenerated = false;
    static std::vector<PlotData> plots;
    static std::string lastExpr;
    static bool exprError = false;
    static std::string exprErrorMsg;

    bool exprChanged = (lastExpr != g_state.graphFunc);
    if (!plotGenerated || needRedraw || exprChanged) {
        lastExpr = g_state.graphFunc;
        plotGenerated = true;
        exprError = false;
        plots.clear();

        g_state.evaluator.setExpression(g_state.graphFunc);
        if (g_state.evaluator.valid()) {
            auto f = [&](double x) -> double {
                g_state.evaluator.setExpression(g_state.graphFunc);
                return g_state.evaluator.evaluate(x);
            };
            plots.push_back(PlotGenerator::generate(f, -10, 10, 500,
                                                     g_state.graphFunc));
        } else {
            exprError = true;
            exprErrorMsg = g_state.evaluator.lastError();
        }
    }

    // === Graph Drawing ===
    float graphH = availH - 32.0f;
    if (graphH < 80.0f) graphH = 80.0f;
    if (graphH > 210.0f) graphH = 210.0f;

    ImVec2 graphPos = ImGui::GetCursorScreenPos();
    ImVec2 graphSize = ImVec2(availW, graphH);

    float mg = 45.0f;
    ImVec2 plotOrigin(graphPos.x + mg, graphPos.y + 4.0f);
    ImVec2 plotSize(graphSize.x - mg - 4.0f, graphSize.y - mg - 4.0f);

    if (plotSize.x > 50 && plotSize.y > 50) {
        double xMin = -10, xMax = 10;
        double yMin = -1.5, yMax = 1.5;

        if (!plots.empty() && !plots[0].points.empty()) {
            yMin = plots[0].y_min;
            yMax = plots[0].y_max;
        }
        double yPad = (yMax - yMin) * 0.1;
        if (yPad < 0.01) yPad = 0.1;
        yMin -= yPad;
        yMax += yPad;

        drawGraphBg(dl, plotOrigin, plotSize);
        drawGrid(dl, plotOrigin, plotSize, xMin, xMax, yMin, yMax);
        if (!plots.empty()) {
            drawPlot(dl, plotOrigin, plotSize, xMin, xMax, yMin, yMax, plots,
                     {IM_COL32(80, 220, 120, 255)});
        }
        drawAxesLabels(dl, plotOrigin, plotSize, xMin, xMax, yMin, yMax);
    }

    // Error message overlay
    if (exprError) {
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[3]);
        ImGui::SetCursorScreenPos(ImVec2(graphPos.x + 10, graphPos.y + 10));
        ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "%s", exprErrorMsg.c_str());
        ImGui::PopFont();
    }

    // Legend (show expression being plotted, same color as line)
    float legX = graphPos.x + mg;
    float legY = graphPos.y + 6.0f;
    dl->AddRectFilled(ImVec2(legX, legY), ImVec2(legX + 120, legY + 24),
                      IM_COL32(10, 10, 18, 200), 4.0f);
    dl->AddLine(ImVec2(legX + 4, legY + 10), ImVec2(legX + 24, legY + 10),
                IM_COL32(80, 220, 120, 255), 2);
    char legLabel[280];
    snprintf(legLabel, sizeof(legLabel), "y = %s", g_state.graphFunc);
    dl->AddText(ImVec2(legX + 28, legY + 4), IM_COL32(160, 220, 160, 255), legLabel);

    // Advance cursor
    ImGui::SetCursorScreenPos(ImVec2(graphPos.x, graphPos.y + graphSize.y + 2));
}

// ============================================================
// Large Plot Viewer — full-screen expandable plot
// ============================================================
void renderLargePlotView() {
    auto& io = ImGui::GetIO();
    ImVec2 winPos(0, 0);
    ImVec2 winSize = io.DisplaySize;

    ImGui::SetNextWindowPos(winPos);
    ImGui::SetNextWindowSize(winSize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::Begin("##largePlot", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration);

    auto* dl = ImGui::GetWindowDrawList();

    // Background
    dl->AddRectFilled(winPos, ImVec2(winPos.x + winSize.x, winPos.y + winSize.y),
                      IM_COL32(5, 5, 10, 240));

    // Close button (X) top-right
    float btnS = 36.0f;
    ImVec2 closePos(winPos.x + winSize.x - btnS - 16, winPos.y + 12);
    ImGui::SetCursorScreenPos(closePos);
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(40, 40, 60, 200));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(80, 40, 40, 230));
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(200, 200, 220, 255));
    if (ImGui::Button("✕##lpClose", ImVec2(btnS, btnS))) {
        g_state.showLargePlot = false;
    }
    ImGui::PopStyleColor(3);

    // ESC to close
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        g_state.showLargePlot = false;
    }

    // Function expression display
    std::string exprLabel = "f(x) = " + g_state.advExpr;
    ImGui::SetCursorScreenPos(ImVec2(winPos.x + 20, winPos.y + 16));
    ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1), "%s", exprLabel.c_str());

    // Plot area
    float margin = 80.0f;
    ImVec2 plotOrigin(winPos.x + margin, winPos.y + margin + 10);
    ImVec2 plotSize(winSize.x - 2.0f * margin, winSize.y - 2.0f * margin - 10);
    if (plotSize.x < 50) plotSize.x = 50;
    if (plotSize.y < 50) plotSize.y = 50;

    // Invisible button for mouse interaction
    ImGui::SetCursorScreenPos(plotOrigin);
    ImGui::InvisibleButton("##lpArea", plotSize);
    bool hovered = ImGui::IsItemHovered();

    // Handle scroll zoom (centered on mouse)
    if (hovered) {
        float wheel = io.MouseWheel;
        if (wheel != 0.0f) {
            ImVec2 mouse = io.MousePos;
            double mx = (mouse.x - plotOrigin.x) / plotSize.x; // 0-1 normalized
            double my = (mouse.y - plotOrigin.y) / plotSize.y;

            // World-coordinate center of zoom
            double cx = g_state.largePlotXMin + mx * (g_state.largePlotXMax - g_state.largePlotXMin);
            double cy = g_state.largePlotYMax - my * (g_state.largePlotYMax - g_state.largePlotYMin);

            double factor = (wheel > 0) ? 0.85 : 1.18;  // zoom in/out
            double halfW = (g_state.largePlotXMax - g_state.largePlotXMin) * factor * 0.5;
            double halfH = (g_state.largePlotYMax - g_state.largePlotYMin) * factor * 0.5;

            g_state.largePlotXMin = cx - halfW;
            g_state.largePlotXMax = cx + halfW;
            g_state.largePlotYMin = cy - halfH;
            g_state.largePlotYMax = cy + halfH;
            g_state.largePlotDirty = true;
        }
    }

    // Drag to pan using ImGui's built-in drag detection
    // This avoids issues with stale mouse-down from the enlarge button click
    if (hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 5.0f)) {
        ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left, 5.0f);
        double dx = -dragDelta.x / plotSize.x * (g_state.largePlotXMax - g_state.largePlotXMin);
        double dy =  dragDelta.y / plotSize.y * (g_state.largePlotYMax - g_state.largePlotYMin);

        g_state.largePlotXMin += dx;
        g_state.largePlotXMax += dx;
        g_state.largePlotYMin += dy;
        g_state.largePlotYMax += dy;
        g_state.largePlotDirty = true;
        ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
    }

    // Draw graph
    if (plotSize.x > 0 && plotSize.y > 0) {
        drawGraphBg(dl, plotOrigin, plotSize);
        drawGrid(dl, plotOrigin, plotSize,
                 g_state.largePlotXMin, g_state.largePlotXMax,
                 g_state.largePlotYMin, g_state.largePlotYMax);

        // Use stored data if available (e.g. from Fourier tab), otherwise generate
        std::vector<PlotData> largePlots;
        if (!g_state.largePlotData.empty()) {
            largePlots = g_state.largePlotData;
        } else {
            g_state.evaluator.setExpression(g_state.advExpr);
            auto f = [&](double x) -> double {
                g_state.evaluator.setExpression(g_state.advExpr);
                return g_state.evaluator.evaluate(x);
            };
            largePlots.push_back(
                PlotGenerator::generate(f, g_state.largePlotXMin, g_state.largePlotXMax, 800));
        }

        drawPlot(dl, plotOrigin, plotSize,
                 g_state.largePlotXMin, g_state.largePlotXMax,
                 g_state.largePlotYMin, g_state.largePlotYMax,
                 largePlots, {IM_COL32(80, 200, 255, 255), IM_COL32(255, 180, 80, 255)});
        drawAxesLabels(dl, plotOrigin, plotSize,
                       g_state.largePlotXMin, g_state.largePlotXMax,
                       g_state.largePlotYMin, g_state.largePlotYMax);
    }

    // Hint text
    std::string hint = T("Drag to pan · Scroll to zoom · ESC to close",
                         "拖拽平移 · 滚轮缩放 · ESC 关闭",
                         "ドラッグで移動 · スクロールで拡大縮小 · ESCで閉じる");
    ImVec2 hintSz = ImGui::CalcTextSize(hint.c_str());
    ImGui::SetCursorScreenPos(ImVec2(winPos.x + winSize.x - hintSz.x - 20,
                                      winPos.y + winSize.y - hintSz.y - 16));
    ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.6f, 0.7f), "%s", hint.c_str());

    // Re-center button
    ImGui::SetCursorScreenPos(ImVec2(winPos.x + 20, winPos.y + winSize.y - 36));
    if (ImGui::Button(T("Reset View##lpReset", "重置视图##lpReset", "ビューリセット##lpReset"),
                       ImVec2(100, 26))) {
        g_state.largePlotXMin = -10; g_state.largePlotXMax = 10;
        g_state.largePlotYMin = -10; g_state.largePlotYMax = 10;
        g_state.largePlotDirty = true;
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

// ============================================================
// Large 3D Viewer — full-screen expandable 3D surface
// ============================================================
void renderLarge3DView() {
    auto& io = ImGui::GetIO();
    ImVec2 winPos(0, 0);
    ImVec2 winSize = io.DisplaySize;

    ImGui::SetNextWindowPos(winPos);
    ImGui::SetNextWindowSize(winSize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::Begin("##large3D", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration |
                 ImGuiWindowFlags_NoBringToFrontOnFocus);

    auto* dl = ImGui::GetWindowDrawList();

    // Background
    dl->AddRectFilled(winPos, ImVec2(winPos.x + winSize.x, winPos.y + winSize.y),
                      IM_COL32(5, 5, 10, 240));

    // Close button (X) top-right
    float btnS = 36.0f;
    ImVec2 closePos(winPos.x + winSize.x - btnS - 16, winPos.y + 12);
    ImGui::SetCursorScreenPos(closePos);
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(40, 40, 60, 200));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(80, 40, 40, 230));
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(200, 200, 220, 255));
    if (ImGui::Button("✕##l3dClose", ImVec2(btnS, btnS))) {
        g_state.showLarge3D = false;
    }
    ImGui::PopStyleColor(3);

    // ESC to close
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        g_state.showLarge3D = false;
    }

    // Function expression display
    std::string exprLabel = "f(x,y) = " + std::string(g_state.func3D);
    ImGui::SetCursorScreenPos(ImVec2(winPos.x + 20, winPos.y + 16));
    ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1), "%s", exprLabel.c_str());

    // 3D drawing area (large)
    float margin = 60.0f;
    float topMargin = 80.0f;
    ImVec2 plotOrigin(winPos.x + margin, winPos.y + topMargin);
    ImVec2 plotSize(winSize.x - 2.0f * margin, winSize.y - topMargin - margin);
    if (plotSize.x < 50) plotSize.x = 50;
    if (plotSize.y < 50) plotSize.y = 50;

    // Draw the 3D surface
    std::string funcExpr = g_state.func3D;
    draw3DSurface(dl, plotOrigin, plotSize, funcExpr,
                  g_state.range3DMin, g_state.range3DMax,
                  g_state.large3DRotX, g_state.large3DRotY, g_state.large3DZoom);

    // Invisible button for mouse interaction
    ImGui::SetCursorScreenPos(plotOrigin);
    ImGui::InvisibleButton("##l3dArea", plotSize);
    bool hovered = ImGui::IsItemHovered();

    // Mouse wheel zoom
    if (hovered) {
        float wheel = io.MouseWheel;
        if (wheel != 0.0f) {
            g_state.large3DZoom *= (1.0f + wheel * 0.1f);
            if (g_state.large3DZoom < 0.1f) g_state.large3DZoom = 0.1f;
            if (g_state.large3DZoom > 10.0f) g_state.large3DZoom = 10.0f;
        }

        // Left-click drag to rotate
        if (ImGui::IsMouseDown(0)) {
            ImVec2 mp = io.MousePos;
            if (!g_state.large3DDragging) {
                g_state.large3DDragging = true;
                g_state.large3DDragLastX = mp.x;
                g_state.large3DDragLastY = mp.y;
            } else {
                float dx = mp.x - g_state.large3DDragLastX;
                float dy = mp.y - g_state.large3DDragLastY;
                if (dx != 0 || dy != 0) {
                    g_state.large3DRotY += dx * 0.3f;
                    g_state.large3DRotX += dy * 0.3f;
                    g_state.large3DDragLastX = mp.x;
                    g_state.large3DDragLastY = mp.y;
                }
            }
        } else {
            g_state.large3DDragging = false;
        }
    } else {
        g_state.large3DDragging = false;
    }

    // Hint text
    std::string hint = T("Drag to rotate · Scroll to zoom · ESC to close",
                         "拖拽旋转 · 滚轮缩放 · ESC 关闭",
                         "ドラッグで回転 · スクロールで拡大縮小 · ESCで閉じる");
    ImVec2 hintSz = ImGui::CalcTextSize(hint.c_str());
    ImGui::SetCursorScreenPos(ImVec2(winPos.x + winSize.x - hintSz.x - 20,
                                      winPos.y + winSize.y - hintSz.y - 16));
    ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.6f, 0.7f), "%s", hint.c_str());

    ImGui::End();
    ImGui::PopStyleVar();
}

// ============================================================
// Main UI Render — Xiaomi-style split layout + Advanced mode
// ============================================================
void renderUI() {
    // Initialize built-in extensions on first call
    static bool extsInitialized = false;
    if (!extsInitialized) {
        initExtensions();
        extsInitialized = true;
    }

    // Get actual window size from GLFW
    float winW = ImGui::GetIO().DisplaySize.x;
    float winH = ImGui::GetIO().DisplaySize.y;

    // Use full window
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(winW, winH));

    ImGui::Begin("##vcalc", nullptr,
                 ImGuiWindowFlags_NoTitleBar |
                 ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoCollapse |
                 ImGuiWindowFlags_NoScrollbar);

    // Top bar (full width)
    renderTopBar();

    if (g_state.isAdvanced) {
        // ========== ADVANCED MODE ==========
        // Chrome-style tab bar at top
        renderChromeTabBar();

        // Tab content fills the remaining area
        if (ImGui::BeginChild("##advContent", ImVec2(0, 0), false)) {
            renderAdvancedTabContent();
        }
        ImGui::EndChild();
    } else {
        // ========== STANDARD MODE (Xiaomi style) ==========
        // LCD Display
        renderLCD();

        // Split: calculator buttons (55% left) + function graph (45% right)
        float leftW = winW * 0.55f;
        if (leftW < 260.0f) leftW = 260.0f;

        // Left column: button grid
        ImGui::BeginChild("##calcLeft", ImVec2(leftW, 0), false);
        renderButtons();
        ImGui::EndChild();

        ImGui::SameLine(0, 4);

        // Right column: function input + graph
        ImGui::BeginChild("##calcRight", ImVec2(0, 0), false);
        renderBottomGraphWithInput();
        ImGui::EndChild();
    }

    // Help popup (opened from Row 6 Help button)
    if (ImGui::BeginPopup("##helpPopup")) {
        ImGui::TextUnformatted(T("Keyboard Shortcuts", "键盘快捷键", "キーボードショートカット"));
        ImGui::Separator();
        ImGui::TextUnformatted(T("0-9:  Digits", "0-9：数字", "0-9：数字"));
        ImGui::TextUnformatted(T("+ - * /:  Operators", "+ - * /：运算符", "+ - * /：演算子"));
        ImGui::TextUnformatted(T("Enter/=: Calculate", "Enter/=：计算结果", "Enter/=：計算"));
        ImGui::TextUnformatted(T("Backspace: Delete", "Backspace：删除", "Backspace：削除"));
        ImGui::TextUnformatted(T("ESC: Clear All", "ESC：清空", "ESC：クリア"));
        ImGui::TextUnformatted(T("s: sin(   c: cos(   t: tan(",
                                  "s：sin(  c：cos(  t：tan(",
                                  "s：sin(  c：cos(  t：tan("));
        ImGui::TextUnformatted(T("l: log(   n: ln(   r: sqrt(",
                                  "l：log(  n：ln(  r：sqrt(",
                                  "l：log(  n：ln(  r：sqrt("));
        ImGui::TextUnformatted(T("p: π (pi)   e: Euler's number",
                                  "p：π  e：自然常数",
                                  "p：π  e：ネイピア数"));
        ImGui::TextUnformatted(T("^: Power    .: Decimal point",
                                  "^：乘方  .：小数点",
                                  "^：累乗  .：小数点"));
        ImGui::TextUnformatted(T("( ): Parentheses", "( )：括号", "( )：括弧"));
        ImGui::EndPopup();
    }

    ImGui::End(); // Main window

    // Panels (floating)
    renderHistoryPanel();
    renderConstantsPanel();
    renderAdvancedModal();

    // Large plot viewer (overlay)
    if (g_state.showLargePlot) {
        renderLargePlotView();
    }

    // Large 3D viewer (overlay)
    if (g_state.showLarge3D) {
        renderLarge3DView();
    }
}

// ============================================================
// Histogram drawing helper
// ============================================================
void drawHistogram(ImDrawList* dl, ImVec2 o, ImVec2 sz,
                   const std::vector<double>& counts,
                   const std::vector<double>& data,
                   double dataMin, double dataMax,
                   int nBins)
{
    if (counts.empty() || nBins < 1) return;

    // Background
    dl->AddRectFilled(o, ImVec2(o.x + sz.x, o.y + sz.y), IM_COL32(8, 8, 14, 255));
    dl->AddRect(o, ImVec2(o.x + sz.x, o.y + sz.y), IM_COL32(40, 40, 60, 200), 2.0f);

    float margin = 40.0f;
    float plotX = o.x + margin;
    float plotY = o.y;
    float plotW = sz.x - margin - 10.0f;
    float plotH = sz.y - margin;

    if (plotW < 10 || plotH < 10) return;

    // Find max count
    double maxCount = 0;
    for (double c : counts) if (c > maxCount) maxCount = c;
    if (maxCount <= 0) maxCount = 1;

    double binWidth = (dataMax - dataMin) / nBins;
    float barW = plotW / (float)nBins;

    ImU32 barCol = IM_COL32(60, 140, 220, 220);
    ImU32 barColH = IM_COL32(80, 180, 255, 220);
    ImU32 axisCol = IM_COL32(100, 100, 140, 200);
    ImU32 textCol = IM_COL32(140, 140, 180, 200);

    // Draw bars
    for (int i = 0; i < nBins && i < (int)counts.size(); i++) {
        float barH = (float)(counts[i] / maxCount) * plotH;
        float x1 = plotX + i * barW + 1;
        float x2 = plotX + (i + 1) * barW - 1;
        float y1 = plotY + plotH - barH;
        float y2 = plotY + plotH;

        dl->AddRectFilled(ImVec2(x1, y1), ImVec2(x2, y2), barCol, 2.0f);
        dl->AddRect(ImVec2(x1, y1), ImVec2(x2, y2), barColH, 2.0f);

        // Value label on top
        char buf[32];
        snprintf(buf, sizeof(buf), "%.0f", counts[i]);
        float tw = ImGui::CalcTextSize(buf).x;
        dl->AddText(ImVec2(x1 + (barW - tw) / 2, y1 - 14), textCol, buf);
    }

    // X-axis line
    dl->AddLine(ImVec2(plotX, plotY + plotH), ImVec2(plotX + plotW, plotY + plotH), axisCol);

    // X-axis labels
    for (int i = 0; i <= nBins && i <= 10; i++) {
        int idx = i * nBins / 10;
        if (idx > nBins) idx = nBins;
        double val = dataMin + idx * binWidth;
        char buf[32];
        snprintf(buf, sizeof(buf), "%.2g", val);
        float x = plotX + idx * barW;
        dl->AddLine(ImVec2(x, plotY + plotH), ImVec2(x, plotY + plotH + 4), axisCol);
        float tw = ImGui::CalcTextSize(buf).x;
        dl->AddText(ImVec2(x - tw / 2, plotY + plotH + 6), textCol, buf);
    }

    // Y-axis label
    dl->AddText(ImVec2(o.x + 4, o.y + 4), textCol, "freq");

    // Cumulative frequency curve (as overlay)
    if (!data.empty() && nBins > 1) {
        double total = (double)data.size();
        double cumSum = 0;
        ImVec2 prevPt(0, 0);
        bool first = true;
        for (int i = 0; i < nBins && i < (int)counts.size(); i++) {
            cumSum += counts[i];
            double frac = cumSum / total;
            float x = plotX + (i + 0.5f) * barW;
            float y = plotY + plotH * (float)(1.0 - frac);
            if (!first) {
                dl->AddLine(prevPt, ImVec2(x, y), IM_COL32(255, 200, 80, 200), 2.0f);
            }
            prevPt = ImVec2(x, y);
            first = false;
        }
    }
}

// ============================================================
// 3D Surface drawing helper — pseudo-3D isometric projection
// ============================================================
void draw3DSurface(ImDrawList* dl, ImVec2 o, ImVec2 sz,
                   const std::string& expr,
                   double xMin, double xMax,
                   double rotX, double rotY,
                   float zoom)
{
    int res = g_state.gridRes;
    double xRange = xMax - xMin;
    if (xRange <= 0) xRange = 1;

    // Evaluate grid
    std::vector<std::vector<double>> zGrid(res + 1, std::vector<double>(res + 1, 0));
    double zMin = 1e30, zMax = -1e30;

    for (int i = 0; i <= res; i++) {
        for (int j = 0; j <= res; j++) {
            double x = xMin + (double)i / res * xRange;
            double y = xMin + (double)j / res * xRange;
            double z = eval3DFunc(expr, x, y);
            zGrid[i][j] = z;
            if (z < zMin) zMin = z;
            if (z > zMax) zMax = z;
        }
    }
    if (zMax == zMin) { zMin -= 0.5; zMax += 0.5; }
    double zRange = zMax - zMin;

    // Rotation matrix (degrees → radians)
    double rx = rotX * std::numbers::pi / 180.0;
    double ry = rotY * std::numbers::pi / 180.0;
    double cx = std::cos(rx), sx = std::sin(rx);
    double cy = std::cos(ry), sy = std::sin(ry);

    // Isometric projection: center of canvas
    float cX = o.x + sz.x / 2;
    float cY = o.y + sz.y / 2;
    float scale = std::min(sz.x, sz.y) * 0.35f * zoom;

    auto project = [&](double wx, double wy, double wz) -> ImVec2 {
        // Apply rotation Y then X
        double x1 = wx * cy - wz * sy;
        double z1 = wx * sy + wz * cy;
        double y1 = wy;
        double z2 = z1 * cx - y1 * sx;
        // Isometric-like: use x1 and z2 as screen coordinates
        float sx2 = cX + (float)x1 * scale;
        float sy2 = cY - (float)z2 * scale;
        return ImVec2(sx2, sy2);
    };

    ImU32 gridLineCol = IM_COL32(60, 80, 120, 150);
    ImU32 gridLineHi = IM_COL32(100, 140, 200, 200);

    // Draw surface (back-to-front ordering for simple painter's algorithm)
    // We use wireframe with colored faces
    for (int i = 0; i < res; i++) {
        for (int j = 0; j < res; j++) {
            double x = xMin + (double)i / res * xRange;
            double y = xMin + (double)j / res * xRange;

            double z00 = zGrid[i][j];
            double z10 = zGrid[i+1][j];
            double z01 = zGrid[i][j+1];
            double z11 = zGrid[i+1][j+1];

            ImVec2 p00 = project(x, y, z00);
            ImVec2 p10 = project(x + xRange/res, y, z10);
            ImVec2 p01 = project(x, y + xRange/res, z01);
            ImVec2 p11 = project(x + xRange/res, y + xRange/res, z11);

            // Color based on height (z value)
            float t0 = (float)((z00 - zMin) / zRange);
            float t1 = (float)((z11 - zMin) / zRange);
            float t = (t0 + t1) * 0.5f;
            if (t < 0) t = 0; if (t > 1) t = 1;

            int r = (int)(30 + 200 * t);
            int g = (int)(60 + 150 * (1 - std::abs(t - 0.5) * 2));
            int b = (int)(200 - 150 * t);
            ImU32 faceCol = IM_COL32(r, g, b, 180);

            // Draw filled quad as two triangles
            dl->AddTriangleFilled(p00, p10, p11, faceCol);
            dl->AddTriangleFilled(p00, p11, p01, faceCol);
        }
    }

    // Draw grid lines on top
    for (int i = 0; i <= res; i += std::max(1, res / 8)) {
        for (int j = 0; j <= res; j += std::max(1, res / 8)) {
            double x = xMin + (double)i / res * xRange;
            double y = xMin + (double)j / res * xRange;
            double z = zGrid[i][j];

            // Line to neighbor
            if (i < res) {
                double z2 = zGrid[i+1][j];
                ImVec2 p1 = project(x, y, z);
                ImVec2 p2 = project(x + xRange/res, y, z2);
                dl->AddLine(p1, p2, i % 4 == 0 ? gridLineHi : gridLineCol, 1.0f);
            }
            if (j < res) {
                double z2 = zGrid[i][j+1];
                ImVec2 p1 = project(x, y, z);
                ImVec2 p2 = project(x, y + xRange/res, z2);
                dl->AddLine(p1, p2, j % 4 == 0 ? gridLineHi : gridLineCol, 1.0f);
            }
        }
    }

    // Draw coordinate axes
    ImU32 axisCol = IM_COL32(255, 100, 100, 220);
    ImU32 axisColY = IM_COL32(100, 255, 100, 220);
    ImU32 axisColZ = IM_COL32(100, 100, 255, 220);

    double axisLen = xRange * 0.5;
    ImVec2 origin = project(0, 0, 0);
    ImVec2 xEnd = project(axisLen, 0, 0);
    ImVec2 yEnd = project(0, axisLen, 0);
    ImVec2 zEnd = project(0, 0, axisLen);

    dl->AddLine(origin, xEnd, axisCol, 2.0f);
    dl->AddLine(origin, yEnd, axisColY, 2.0f);
    dl->AddLine(origin, zEnd, axisColZ, 2.0f);
    dl->AddText(xEnd, axisCol, "x");
    dl->AddText(yEnd, axisColY, "y");
    dl->AddText(zEnd, axisColZ, "z");
}

// ============================================================
// ============================================================
// Boot Screen — Artistic 3D animation + elegant "开 始" button
// ============================================================
void renderBootScreen() {
    auto& io = ImGui::GetIO();
    float dt = io.DeltaTime;
    if (dt > 0.1f) dt = 0.016f;

    g_state.bootTimer += dt;
    float bootT = g_state.bootTimer;

    // Full-screen window
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::SetNextWindowBgAlpha(1.0f);
    ImGui::Begin("##bootScreen", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse |
                 ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDecoration);

    auto* dl = ImGui::GetWindowDrawList();
    ImVec2 wp = ImGui::GetWindowPos();
    ImVec2 ws = ImGui::GetWindowSize();
    float cX = wp.x + ws.x / 2;
    float cY = wp.y + ws.y / 2;

    // ── Deep gradient background ──
    for (int i = 0; i < 60; i++) {
        float t = (float)i / 60.0f;
        int r = (int)(4 + 2 * t);
        int g = (int)(4 + 8 * t);
        int b = (int)(20 + 25 * t);
        dl->AddRectFilled(
            ImVec2(wp.x, wp.y + (i * ws.y) / 60),
            ImVec2(wp.x + ws.x, wp.y + ((i + 1) * ws.y) / 60),
            IM_COL32(r, g, b, 255));
    }

    // ── Star field ──
    static struct { float x, y, speed, alpha; } stars[80];
    static bool starsInit = false;
    if (!starsInit) {
        srand(42);
        for (int i = 0; i < 80; i++) {
            stars[i].x = (float)(rand() % 10000) / 10000.0f;
            stars[i].y = (float)(rand() % 10000) / 10000.0f;
            stars[i].speed = 0.15f + (float)(rand() % 1000) / 10000.0f;
            stars[i].alpha = 0.2f + (float)(rand() % 800) / 1000.0f;
        }
        starsInit = true;
    }
    for (int i = 0; i < 80; i++) {
        float sx = wp.x + stars[i].x * ws.x;
        float sy = wp.y + stars[i].y * ws.y + std::fmod(bootT * stars[i].speed * 20, ws.y);
        if (sy > wp.y + ws.y) sy -= ws.y;
        float sz = 1.0f + stars[i].speed * 1.5f;
        int a = (int)(80 * stars[i].alpha * (0.7f + 0.3f * std::sin(bootT * 0.5f + i)));
        dl->AddCircleFilled(ImVec2(sx, sy), sz, IM_COL32(180, 220, 255, a), 6);
    }

    // ── Title with zoom + fade ──
    float titleAppear = std::min(1.0f, bootT / 1.2f);
    float titleScale = 1.0f - 0.15f * (1.0f - titleAppear);
    int titleAlpha = (int)(255 * titleAppear);
    ImFont* tf = io.Fonts->Fonts.Size > 1 ? io.Fonts->Fonts[1] : io.FontDefault;
    float titleSize = tf->FontSize * titleScale;
    const char* title = "VulkanCalc";
    ImVec2 tsz = tf->CalcTextSizeA(titleSize, FLT_MAX, 0, title);
    float titleY = wp.y + 30 + 20 * (1.0f - titleAppear);
    dl->AddText(tf, titleSize, ImVec2(cX - tsz.x / 2, titleY),
                IM_COL32(180, 220, 255, titleAlpha), title);
    dl->AddCircleFilled(ImVec2(cX, titleY + tsz.y / 2),
                        tsz.x / 2 + 20, IM_COL32(60, 120, 255, (int)(15 * titleAppear)), 30);

    // ── Subtitle (delayed) ──
    float subAppear = std::min(1.0f, (bootT - 0.6f) / 1.0f);
    if (subAppear > 0) {
        ImFont* sf = io.FontDefault;
        const char* sub = "3D 智能科学计算器";
        ImVec2 ssz = sf->CalcTextSizeA(sf->FontSize, FLT_MAX, 0, sub);
        dl->AddText(sf, sf->FontSize, ImVec2(cX - ssz.x / 2, titleY + tsz.y + 8),
                    IM_COL32(140, 200, 255, (int)(180 * subAppear)), sub);
    }

    // ── 3D Surface ──
    float rotX = 25.0f + 10.0f * std::sin(bootT * 0.6f);
    float rotY = bootT * 35.0f;
    float surfW = ws.x * 0.7f, surfH = ws.y * 0.42f;
    float surfX = wp.x + (ws.x - surfW) / 2, surfY = titleY + tsz.y + 55;
    if (surfW > 50 && surfH > 50 && bootT > 0.3f) {
        static int funcIdx = 0;
        if (bootT > 4.0f && funcIdx == 0) funcIdx = 1;
        if (bootT > 8.0f && funcIdx == 1) funcIdx = 2;
        const char* exprs[] = {"sin(sqrt(x^2+y^2))", "sin(x)*cos(y)", "x*x/8 - y*y/8"};
        draw3DSurface(dl, ImVec2(surfX, surfY), ImVec2(surfW, surfH),
                      exprs[funcIdx], -5.0, 5.0, rotX, rotY, 1.1f);
    }

    // ── Separator ──
    float lineY = surfY + surfH + 25;
    float lineW = ws.x * 0.3f;
    int lAlpha = (int)(60 * std::min(1.0f, (bootT - 1.0f) / 0.5f));
    if (lAlpha > 0)
        dl->AddLine(ImVec2(cX - lineW / 2, lineY), ImVec2(cX + lineW / 2, lineY),
                    IM_COL32(80, 160, 255, lAlpha), 1.0f);

    // ── "开 始" button with gradient + glow ──
    float btnAppear = 2.2f;
    if (bootT > btnAppear) {
        float bt = bootT - btnAppear;
        float breathe = (std::sin(bt * 1.8f) + 1.0f) * 0.5f;
        float easeIn = std::min(1.0f, bt / 0.6f);
        float alpha = 0.4f + 0.6f * breathe;
        float pulse = 1.0f + 0.03f * breathe;
        float btnW = 220.0f * pulse, btnH = 60.0f * pulse;
        float btnY = lineY + 20;
        dl->AddCircleFilled(ImVec2(cX, btnY + btnH / 2), btnW / 2 + 40 + 15 * breathe,
                            IM_COL32(40, 120, 255, (int)(15 * alpha * easeIn)), 50);
        float gs = btnH / 8;
        for (int i = 0; i < 8; i++) {
            float y0 = btnY + i * gs, y1 = y0 + gs;
            float t = (float)i / 8.0f;
            ImU32 col = IM_COL32((int)(50+30*(1-t)), (int)(100+80*(1-t)),
                                 (int)(200+55*(1-t)), (int)(220*easeIn*alpha));
            dl->AddRectFilled(ImVec2(cX-btnW/2, y0), ImVec2(cX+btnW/2, y1), col, 14.0f);
            if (i > 0 && i < 7)
                dl->AddRectFilled(ImVec2(cX-btnW/2, y0), ImVec2(cX+btnW/2, y1), col);
        }
        dl->AddRect(ImVec2(cX-btnW/2, btnY), ImVec2(cX+btnW/2, btnY+btnH),
                    IM_COL32(100, 200, 255, (int)(200*easeIn*alpha)), 14.0f, 0, 2.0f);
        ImFont* bf = io.FontDefault;
        if (io.Fonts->Fonts.Size > 4) bf = io.Fonts->Fonts[4];
        else if (io.Fonts->Fonts.Size > 1) bf = io.Fonts->Fonts[1];
        float bfs = bf->FontSize * 1.4f;
        ImVec2 btsz = bf->CalcTextSizeA(bfs, FLT_MAX, 0, "开  始");
        dl->AddText(bf, bfs, ImVec2(cX-btsz.x/2, btnY+(btnH-btsz.y)/2),
                    IM_COL32(255,255,255,(int)(255*easeIn)), "开  始");
        ImVec2 mouse = io.MousePos;
        bool hovered = (mouse.x >= cX-btnW/2 && mouse.x <= cX+btnW/2 &&
                        mouse.y >= btnY && mouse.y <= btnY+btnH);
        if (hovered) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            dl->AddRectFilled(ImVec2(cX-btnW/2, btnY), ImVec2(cX+btnW/2, btnY+btnH),
                              IM_COL32(255,255,255,25), 14.0f);
        }
        if (ImGui::IsMouseClicked(0) && hovered) g_state.showBoot = false;

        float hintAppear = std::min(1.0f, (bt - 0.8f) / 0.5f);
        if (hintAppear > 0) {
            const char* hint = T("Press Enter or click to start",
                                 "按 Enter 或点击进入",
                                 "Enter またはクリックで開始");
            ImVec2 hsz = ImGui::CalcTextSize(hint);
            dl->AddText(io.FontDefault, io.FontDefault->FontSize,
                        ImVec2(cX - hsz.x/2, btnY + btnH + 14),
                        IM_COL32(120, 160, 200, (int)(120 * hintAppear)), hint);
        }
    }

    // ── Version text ──
    dl->AddText(io.FontDefault, io.FontDefault->FontSize,
                ImVec2(wp.x + ws.x - 60, wp.y + ws.y - 22),
                IM_COL32(80, 100, 130, 80), "v2.0.0");

    if (ImGui::IsKeyPressed(ImGuiKey_Enter) ||
        ImGui::IsKeyPressed(ImGuiKey_KeypadEnter) ||
        ImGui::IsKeyPressed(ImGuiKey_Space))
        g_state.showBoot = false;

    ImGui::End();
}
