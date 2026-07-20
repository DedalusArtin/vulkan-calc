#include "ui.hpp"
#include "calc_logic.hpp"
#include "locale.hpp"
#include "state.hpp"
#include "imgui.h"
#include <cmath>
#include <numbers>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <cctype>

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
static const float BTN_GAP = 4.0f;
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
    float h = 165.0f;

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
    }

    ImGui::SetCursorScreenPos(ImVec2(pos.x + padX, y));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(10, 18, 10, 0)); // transparent
    ImGui::PushStyleColor(ImGuiCol_Text, COL_LCD_TEXT);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(10, 18, 10, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, IM_COL32(10, 18, 10, 10));

    // Calculate font-based height for input field
    pushLcdFont();
    float lcdFontH = ImGui::GetFontSize() + 6.0f;

    ImGui::SetNextItemWidth(w - padX * 2);
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
    float btnW = (availW - (BTN_COLS - 1) * BTN_GAP) / (float)BTN_COLS;
    float btnH = 44.0f;

    auto sameLine = [&]() { ImGui::SameLine(0, BTN_GAP); };
    auto btnSize = ImVec2(btnW, btnH);

    // Row 0: Blue function keys — USE UNIQUE IDs
    if (funcBtn("sin##r0c0")) calcOnButton("sin"); sameLine();
    if (funcBtn("cos##r0c1")) calcOnButton("cos"); sameLine();
    if (funcBtn("tan##r0c2")) calcOnButton("tan"); sameLine();
    if (funcBtn("log##r0c3")) calcOnButton("log"); sameLine();
    if (funcBtn("ln##r0c4"))  calcOnButton("ln");

    // Row 1: Blue function keys
    if (funcBtn("x²##r1c0"))  calcOnButton("x²"); sameLine();
    if (funcBtn("√##r1c1"))   calcOnButton("√"); sameLine();
    if (funcBtn("x^y##r1c2")) calcOnButton("x^y"); sameLine();
    if (funcBtn("π##r1c3"))   calcOnButton("π"); sameLine();
    if (funcBtn("e##r1c4"))   calcOnButton("e");

    // Row 2: 7 8 9 ÷ DEL
    if (numBtn("7##r2c0"))  calcOnButton("7"); sameLine();
    if (numBtn("8##r2c1"))  calcOnButton("8"); sameLine();
    if (numBtn("9##r2c2"))  calcOnButton("9"); sameLine();
    if (opBtn("÷##r2c3"))   calcOnButton("÷"); sameLine();
    if (delBtn("DEL##r2c4")) calcOnButton("DEL");

    // Row 3: 4 5 6 × AC
    if (numBtn("4##r3c0"))  calcOnButton("4"); sameLine();
    if (numBtn("5##r3c1"))  calcOnButton("5"); sameLine();
    if (numBtn("6##r3c2"))  calcOnButton("6"); sameLine();
    if (opBtn("×##r3c3"))   calcOnButton("×"); sameLine();
    if (acBtn("AC##r3c4"))  calcOnButton("AC");

    // Row 4: 1 2 3 ( ) -
    if (numBtn("1##r4c0"))  calcOnButton("1"); sameLine();
    if (numBtn("2##r4c1"))  calcOnButton("2"); sameLine();
    if (numBtn("3##r4c2"))  calcOnButton("3"); sameLine();
    if (styledButton("(##r4c3", COL_BTN_NUM, COL_BTN_NUM_H, COL_BTN_NUM_A, btnSize))
        calcOnButton("(");
    if (styledButton(")##r4c4", COL_BTN_NUM, COL_BTN_NUM_H, COL_BTN_NUM_A, btnSize))
        calcOnButton(")");

    // Row 5: 0 . Ans + =
    if (numBtn("0##r5c0"))  calcOnButton("0"); sameLine();
    if (numBtn(".##r5c1"))  calcOnButton("."); sameLine();
    if (toolBtn("Ans##r5c2")) calcOnButton("Ans"); sameLine();
    if (opBtn("+##r5c3"))   calcOnButton("+"); sameLine();
    if (opBtn("-##r5c4"))   calcOnButton("-");

    // Row 6: = button spans full width
    {
        float eqBtnW = availW; // full width
        if (eqBtn("=##r6c0"))
            calcOnButton("=");
    }
}

// ============================================================
// Top bar
// ============================================================
void renderTopBar() {
    // Angle mode toggle
    const char* amStr = angleModeStr(g_state.angleMode);
    if (styledButton(amStr, COL_BTN_TOOL, COL_BTN_TOOL_H, COL_BTN_TOOL_A, ImVec2(52, 26))) {
        calcToggleAngleMode();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", T("Angle mode: DEG/RAD/GRAD",
                                  "角度模式：度/弧度/百分度",
                                  "角度モード：度/ラジアン/グラード"));
    }

    ImGui::SameLine(0, 4);

    // Advanced button
    bool advActive = g_state.showAdvanced;
    if (styledButton(T("ADV", "高级", "応用"),
                     advActive ? COL_BTN_FUNC : COL_BTN_TOOL,
                     COL_BTN_TOOL_H, COL_BTN_TOOL_A, ImVec2(56, 26))) {
        g_state.showAdvanced = !g_state.showAdvanced;
        if (g_state.showAdvanced) {
            g_state.showHistory = false;
            g_state.showConstants = false;
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", T("Fourier / Complex / Calculus",
                                  "傅里叶 / 复变函数 / 微积分",
                                  "フーリエ / 複素関数 / 微積分"));
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
        g_state.showAdvanced = true;
        g_state.showHistory = false;
        g_state.showConstants = false;
        // Focus Fourier tab (index 1)
        // Use a simple approach: open advanced and set a hint
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", T("Fourier Series", "傅里叶级数", "フーリエ級数"));
    }

    ImGui::SameLine(0, 2);

    // Complex quick button
    if (styledButton(T("C##cb", "复##cb", "複##cb"),
                     COL_BTN_TOOL, COL_BTN_TOOL_H, COL_BTN_TOOL_A, ImVec2(28, 26))) {
        g_state.showAdvanced = true;
        g_state.showHistory = false;
        g_state.showConstants = false;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", T("Complex Analysis", "复分析", "複素解析"));
    }

    ImGui::SameLine(0, 2);

    // Calculus quick button
    if (styledButton(T("∫##ib", "积##ib", "積##ib"),
                     COL_BTN_TOOL, COL_BTN_TOOL_H, COL_BTN_TOOL_A, ImVec2(28, 26))) {
        g_state.showAdvanced = true;
        g_state.showHistory = false;
        g_state.showConstants = false;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", T("Calculus", "微积分", "微積分"));
    }

    ImGui::SameLine(0, 8);

    // Extra utilities: Rand, %, !
    if (toolBtn(T("rand##tb", "随机##tb", "乱数##tb"))) calcOnButton("rand");
    ImGui::SameLine(0, 4);
    if (toolBtn("%##tb")) calcOnButton("%");
    ImGui::SameLine(0, 4);
    if (toolBtn("n!##tb")) calcOnButton("!");

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
    for (int i = 0; i <= 10; i++) {
        double t = i / 10.0;
        double x = xMin + t * (xMax - xMin), y = yMin + t * (yMax - yMin);
        auto c = (std::abs(x) < 1e-9 || std::abs(y) < 1e-9)
                     ? IM_COL32(80, 80, 110, 255)
                     : IM_COL32(30, 30, 45, 200);
        dl->AddLine(ts(x, yMin), ts(x, yMax), c);
        dl->AddLine(ts(xMin, y), ts(xMax, y), c);
    }
}

static void drawAxesLabels(ImDrawList* dl, ImVec2 o, ImVec2 sz,
                           double xMin, double xMax,
                           double yMin, double yMax) {
    auto ts = makeToScreen(o, sz, xMin, xMax, yMin, yMax);
    int nTicks = 6;
    auto nice = [](double v) -> double {
        double e = std::floor(std::log10(v));
        double f = v / std::pow(10, e);
        if (f < 1.5) return 1 * std::pow(10, e);
        if (f < 3.5) return 2 * std::pow(10, e);
        if (f < 7.5) return 5 * std::pow(10, e);
        return 10 * std::pow(10, e);
    };
    double xs = nice((xMax - xMin) / nTicks);
    double ys = nice((yMax - yMin) / nTicks);
    if (xs <= 0) xs = (xMax - xMin) / nTicks;
    if (ys <= 0) ys = (yMax - yMin) / nTicks;
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

void renderAdvancedModal() {
    if (!g_state.showAdvanced) return;

    ImGui::SetNextWindowSize(ImVec2(680, 520), ImGuiCond_FirstUseEver);
    ImGui::Begin(T("Advanced Mode##adv", "高级模式##adv", "応用モード##adv"),
                 &g_state.showAdvanced,
                 ImGuiWindowFlags_NoResize);

    // Tabs inside the advanced window
    static int advTab = 0;
    const char* tabNames[] = {
        T("Calculus##t0", "微积分##t0", "微積分##t0"),
        T("Fourier##t1", "傅里叶##t1", "フーリエ##t1"),
        T("Complex##t2", "复变函数##t2", "複素関数##t2"),
        T("Domain##t3", "域着色##t3", "領域彩色##t3")
    };

    for (int i = 0; i < 4; i++) {
        if (i > 0) ImGui::SameLine();
        if (ImGui::RadioButton(tabNames[i], &advTab, i)) {}
    }
    ImGui::Separator();

    // ---- Calculus Tab ----
    if (advTab == 0) {
        ImGui::Text("%s", T("Expression f(x) =", "表达式 f(x) =", "式 f(x) ="));

        char buf[1024];
        strncpy(buf, g_state.display.empty() ? "sin(x)" : g_state.display.c_str(),
                sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = 0;
        ImGui::PushItemWidth(-1);
        if (ImGui::InputText("##advExpr", buf, sizeof(buf),
                             ImGuiInputTextFlags_EnterReturnsTrue)) {
            g_state.display = buf;
        }
        ImGui::PopItemWidth();

        ImGui::SetNextItemWidth(-1);
        ImGui::InputDouble("x =##advX", &g_state.evalX, 0.1, 1, "%.4f");

        // Evaluate button
        if (ImGui::Button(T("Evaluate f(x)##adv", "计算 f(x)##adv", "計算 f(x)##adv"))) {
            g_state.evaluator.setExpression(g_state.display);
            if (g_state.evaluator.valid()) {
                double v = g_state.evaluator.evaluate(g_state.evalX);
                std::ostringstream oss;
                oss << v;
                g_state.result = oss.str();
                g_state.error.clear();
            } else {
                g_state.error = g_state.evaluator.lastError();
            }
        }
        ImGui::SameLine();

        // Integral
        if (ImGui::Button(T("∫ f(x) dx##adv", "定积分 ∫##adv", "定積分 ∫##adv"))) {
            auto r = Integrator::adaptiveSimpson(
                [&](double x) {
                    g_state.evaluator.setExpression(g_state.display);
                    return g_state.evaluator.evaluate(x);
                },
                g_state.intA, g_state.intB);
            std::ostringstream o;
            o << "∫[" << r.value << "]";
            g_state.intResult = o.str();
            g_state.result = std::to_string(r.value);
            g_state.plotMode = CalcState::PLOT_INTEG;
            g_state.plots = PlotGenerator::generateMultiple(
                {[&](double x) {
                    g_state.evaluator.setExpression(g_state.display);
                    return g_state.evaluator.evaluate(x);
                }},
                g_state.plotXMin, g_state.plotXMax, 500);
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
                    g_state.evaluator.setExpression(g_state.display);
                    return g_state.evaluator.evaluate(x);
                }},
                g_state.plotXMin, g_state.plotXMax, 500);
            auto deriv = [&](double x) {
                return Differentiator::derivative(
                    [&](double t) {
                        g_state.evaluator.setExpression(g_state.display);
                        return g_state.evaluator.evaluate(t);
                    },
                    x).value;
            };
            g_state.plots.push_back(
                PlotGenerator::generate(deriv, g_state.plotXMin, g_state.plotXMax,
                                        500, "f'(x)"));
        }

        // Result text
        if (!g_state.result.empty()) {
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.3f, 1),
                               "f(%.4f) = %s", g_state.evalX, g_state.result.c_str());
        }
        if (!g_state.error.empty()) {
            ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "%s", g_state.error.c_str());
        }
        if (!g_state.intResult.empty()) {
            ImGui::TextColored(ImVec4(0.6f, 0.4f, 0.1f, 1), "%s",
                               g_state.intResult.c_str());
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
            }
        }
        ImGui::EndChild();
    }

    // ---- Fourier Tab ----
    if (advTab == 1) {
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
    if (advTab == 2) {
        ImGui::TextUnformatted(T("Complex Analysis##ct", "复分析##ct", "複素解析##ct"));

        if (ImGui::Button(T("Cauchy ∮##cb", "柯西积分 ∮##cb", "コーシー積分 ∮##cb"))) {
            Complex a(g_state.complexRe, g_state.complexIm);
            auto f = [&](Complex z) -> Complex {
                g_state.evaluator.setExpression(g_state.display);
                return Complex(g_state.evaluator.evaluate(z.real()), 0);
            };
            auto cr = ComplexAnalysis::cauchyIntegral(f, a, g_state.cauchyRadius);
            g_state.complexResult = "Cauchy ∮ = " + to_string(cr, 4);
            g_state.result = to_string(cr, 4);
        }
        ImGui::SameLine();
        if (ImGui::Button(T("Residue##cb", "留数##cb", "留数##cb"))) {
            Complex a(g_state.complexRe, g_state.complexIm);
            auto f = [&](Complex z) -> Complex {
                g_state.evaluator.setExpression(g_state.display);
                return Complex(g_state.evaluator.evaluate(z.real()), 0);
            };
            auto res = ComplexAnalysis::residue(f, a);
            g_state.complexResult = "Res(f, " + to_string(a, 4) + ") = " +
                                    to_string(res, 4);
        }
        ImGui::SameLine();
        if (ImGui::Button("Γ(z)##cb")) {
            g_state.result = "Γ(0.5) = " +
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
    if (advTab == 3) {
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

    ImGui::End();
}

// ============================================================
// Main UI Render — adaptive to window size
// ============================================================
void renderUI() {
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

    // Top bar
    renderTopBar();

    // LCD Display
    renderLCD();

    // Button Grid
    renderButtons();

    // Extra row: constants shortcuts
    ImGui::Separator();
    float availW = ImGui::GetContentRegionAvail().x;
    float btnW = (availW - 5 * BTN_GAP) / 6.0f;
    ImVec2 smBtn(btnW, 32);
    if (styledButton("π##extra0", COL_BTN_TOOL, COL_BTN_TOOL_H, COL_BTN_TOOL_A, smBtn))
        calcOnButton("π");
    ImGui::SameLine(0, BTN_GAP);
    if (styledButton("e##extra1", COL_BTN_TOOL, COL_BTN_TOOL_H, COL_BTN_TOOL_A, smBtn))
        calcOnButton("e");
    ImGui::SameLine(0, BTN_GAP);
    if (styledButton(T("rand##extra2", "随机##extra2", "乱数##extra2"),
                     COL_BTN_TOOL, COL_BTN_TOOL_H, COL_BTN_TOOL_A, smBtn))
        calcOnButton("rand");
    ImGui::SameLine(0, BTN_GAP);
    if (styledButton("%##extra3", COL_BTN_TOOL, COL_BTN_TOOL_H, COL_BTN_TOOL_A, smBtn))
        calcOnButton("%");
    ImGui::SameLine(0, BTN_GAP);
    if (styledButton("n!##extra4", COL_BTN_TOOL, COL_BTN_TOOL_H, COL_BTN_TOOL_A, smBtn))
        calcOnButton("!");
    ImGui::SameLine(0, BTN_GAP);
    if (styledButton(T("Help##extra5", "帮助##extra5", "ヘルプ##extra5"),
                     COL_BTN_TOOL, COL_BTN_TOOL_H, COL_BTN_TOOL_A, smBtn)) {
        ImGui::OpenPopup("##helpPopup");
    }

    // Help popup
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

    // Panels
    renderHistoryPanel();
    renderConstantsPanel();
    renderAdvancedModal();
}
