#include "vulkan_renderer.hpp"
#include "math_engine.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <numbers>
#include <vector>
#include <cstring>
#include <algorithm>
#include <map>

// ============================================================
// Language
// ============================================================
enum Lang { LANG_EN, LANG_ZH, LANG_JA };
static Lang g_lang = LANG_EN;
static const char* T(const char* en, const char* zh, const char* ja) {
    switch (g_lang) {
        case LANG_ZH: return zh;
        case LANG_JA: return ja;
        default: return en;
    }
}

// ============================================================
// State
// ============================================================
struct CalcState {
    ExpressionEvaluator evaluator;
    std::string expr = "sin(x)";
    std::string result;
    std::string error;

    // Evaluation
    double evalX = 0;
    bool exprHasX = true;

    // Integration
    double intA = 0, intB = 6.2831853;
    std::string intResult;

    // Complex
    double complexRe = 0, complexIm = 0;
    double cauchyRadius = 1.0;
    std::string complexResult;

    // Plot mode
    enum PlotMode { PLOT_NONE, PLOT_FUNC, PLOT_DERIV, PLOT_INTEG } plotMode = PLOT_NONE;
    std::vector<PlotData> plots; // plot[0]=f, plot[1]=f' (if deriv)
    std::vector<PlotData> integPlot; // for integration shading
    double plotXMin = -10, plotXMax = 10, plotYMin = -10, plotYMax = 10;

    // Graph tab
    double gxMin = -10, gxMax = 10, gyMin = -10, gyMax = 10;
    std::vector<PlotData> graphPlots;

    // Fourier
    int fourierTerms = 5;
    int fourierType = 0;
    std::vector<PlotData> fourierPlots;

    // Complex domain
    std::string complexExpr = "z";
    double cxMin = -3, cxMax = 3, cyMin = -3, cyMax = 3;
    std::vector<PlotGenerator::DomainColorPoint> domainPts;
    bool domainDirty = true;

    // Mode
    enum Mode { TAB_CALC, TAB_FOURIER, TAB_COMPLEX } mode = TAB_CALC;
};

static CalcState g_state;

// ============================================================
// Helpers
// ============================================================
std::string fmt(double v, int prec = 6) {
    if (!std::isfinite(v)) return "NaN";
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(prec) << v;
    auto s = oss.str();
    while (s.size() > 1 && s.back() == '0') s.pop_back();
    if (s.back() == '.') s.pop_back();
    return s;
}

bool hasVarX(const std::string& expr) {
    return expr.find('x') != std::string::npos;
}

// ============================================================
// Plot helpers
// ============================================================
void drawGraphBg(ImDrawList* dl, ImVec2 o, ImVec2 sz) {
    dl->AddRectFilled(o, ImVec2(o.x+sz.x, o.y+sz.y), IM_COL32(8,8,12,255));
    dl->AddRect(o, ImVec2(o.x+sz.x, o.y+sz.y), IM_COL32(30,30,45,200), 4.0f);
}

auto makeToScreen = [](ImVec2 o, ImVec2 sz, double xMin, double xMax, double yMin, double yMax) {
    return [=](double wx, double wy) -> ImVec2 {
        float nx = (float)((wx - xMin) / (xMax - xMin));
        float ny = (float)((wy - yMin) / (yMax - yMin));
        return ImVec2(o.x + nx*sz.x, o.y + (1-ny)*sz.y);
    };
};

void drawGrid(ImDrawList* dl, ImVec2 o, ImVec2 sz,
              double xMin, double xMax, double yMin, double yMax) {
    auto ts = makeToScreen(o, sz, xMin, xMax, yMin, yMax);
    for (int i = 0; i <= 10; i++) {
        double t = i/10.0;
        double x = xMin + t*(xMax-xMin), y = yMin + t*(yMax-yMin);
        auto c = (std::abs(x)<1e-9||std::abs(y)<1e-9) ? IM_COL32(80,80,110,255) : IM_COL32(30,30,45,200);
        dl->AddLine(ts(x,yMin), ts(x,yMax), c); dl->AddLine(ts(xMin,y), ts(xMax,y), c);
    }
}

void drawAxesLabels(ImDrawList* dl, ImVec2 o, ImVec2 sz,
                    double xMin, double xMax, double yMin, double yMax) {
    auto ts = makeToScreen(o, sz, xMin, xMax, yMin, yMax);
    int nTicks = 6;
    auto nice = [](double v) -> double {
        double e = std::floor(std::log10(v));
        double f = v / std::pow(10,e);
        if (f < 1.5) return 1*std::pow(10,e); if (f < 3.5) return 2*std::pow(10,e);
        if (f < 7.5) return 5*std::pow(10,e); return 10*std::pow(10,e);
    };
    double xs = nice((xMax-xMin)/nTicks), ys = nice((yMax-yMin)/nTicks);
    if (xs <= 0) xs = (xMax-xMin)/nTicks; if (ys <= 0) ys = (yMax-yMin)/nTicks;
    auto tc = IM_COL32(100,100,140,200);
    auto lab = [&](double v, double x, double y, int flags) {
        char b[32]; snprintf(b,sizeof(b),"%.10g",v);
        char* e = b+strlen(b)-1; while(e>b&&*e=='0')*e--=0; if(*e=='.')*e=0;
        auto p = ts(x,y); float tw = ImGui::CalcTextSize(b).x;
        if (flags&1) dl->AddText(ImVec2(p.x-tw/2, o.y+sz.y+2), tc, b);
        if (flags&2) dl->AddText(ImVec2(o.x+4, p.y-7), tc, b);
    };
    double xS = std::ceil(xMin/xs)*xs, yS = std::ceil(yMin/ys)*ys;
    for (double v = xS; v <= xMax; v += xs) { auto p = ts(v,0);
        if (p.x>=o.x&&p.x<=o.x+sz.x) { dl->AddLine(ImVec2(p.x,o.y+sz.y-3), ImVec2(p.x,o.y+sz.y), tc); lab(v,v,0,1); }}
    for (double v = yS; v <= yMax; v += ys) { auto p = ts(0,v);
        if (p.y>=o.y&&p.y<=o.y+sz.y) { dl->AddLine(ImVec2(o.x,p.y), ImVec2(o.x+3,p.y), tc); lab(v,0,v,2); }}
    // Origin & axes labels
    auto po = ts(0,0);
    if (po.x>=o.x&&po.x<=o.x+sz.x&&po.y>=o.y&&po.y<=o.y+sz.y) dl->AddText(ImVec2(po.x-6,po.y+2), IM_COL32(120,120,160,255), "O");
    auto px = ts(xMax,0); if (px.x>=o.x) dl->AddText(ImVec2(px.x-6,o.y+sz.y+2), IM_COL32(120,120,160,255), "x");
    auto py = ts(0,yMax); if (py.y>=o.y) dl->AddText(ImVec2(o.x+4,py.y-14), IM_COL32(120,120,160,255), "y");
}

void drawPlot(ImDrawList* dl, ImVec2 o, ImVec2 sz,
              double xMin, double xMax, double yMin, double yMax,
              const std::vector<PlotData>& plots,
              const std::vector<ImU32>& colors) {
    auto ts = makeToScreen(o, sz, xMin, xMax, yMin, yMax);
    for (size_t pi = 0; pi < plots.size(); pi++) {
        auto& p = plots[pi];
        auto col = pi < colors.size() ? colors[pi] : IM_COL32(80,160,255,255);
        for (size_t i = 1; i < p.points.size(); i++) {
            auto a = ts(p.points[i-1].x, p.points[i-1].y);
            auto b = ts(p.points[i].x, p.points[i].y);
            dl->AddLine(a, b, col, 2);
        }
    }
}

// ============================================================
// Fourier
// ============================================================
double squareWave(double x, int n) { double s=0; for(int k=1;k<=n;k++) s+=std::sin((2*k-1)*x)/(2*k-1); return (4/std::numbers::pi)*s; }
double sawtoothWave(double x, int n) { double s=0; for(int k=1;k<=n;k++) s+=((k%2?1:-1))*std::sin(k*x)/k; return (2/std::numbers::pi)*s; }
double triangleWave(double x, int n) { double s=0; for(int k=0;k<n;k++) s+=std::pow(-1,k)*std::sin((2*k+1)*x)/std::pow(2*k+1,2); return (8/(std::numbers::pi*std::numbers::pi))*s; }

void generateFourierPlot() {
    g_state.fourierPlots.clear(); int N=std::max(1,g_state.fourierTerms);
    auto f=[&](double x)->double{switch(g_state.fourierType){case 0:return squareWave(x,N);case 1:return sawtoothWave(x,N);case 2:return triangleWave(x,N);default:return 0;}};
    g_state.fourierPlots.push_back(PlotGenerator::generate(f,-10,10,800,"Fourier N="+std::to_string(N)));
    if(g_state.fourierType<3){auto e=[&](double x)->double{switch(g_state.fourierType){case 0:return std::fmod(x+std::numbers::pi,2*std::numbers::pi)>std::numbers::pi?1:-1;case 1:return std::fmod(x+std::numbers::pi,2*std::numbers::pi)/std::numbers::pi-1;case 2:return std::numbers::pi-std::abs(std::fmod(x+std::numbers::pi,2*std::numbers::pi)-std::numbers::pi);default:return 0;}};
    g_state.fourierPlots.push_back(PlotGenerator::generate(e,-10,10,800,"Exact"));}
}

// ============================================================
// Domain coloring
// ============================================================
void generateDomainColoring() {
    auto f=[&](Complex z)->Complex{
        if(g_state.complexExpr=="z")return z; if(g_state.complexExpr=="z^2")return z*z; if(g_state.complexExpr=="z^3")return z*z*z;
        if(g_state.complexExpr=="1/z")return std::abs(z)<1e-15?Complex(1e15):Complex(1)/z;
        if(g_state.complexExpr=="sin(z)")return std::sin(z); if(g_state.complexExpr=="cos(z)")return std::cos(z);
        if(g_state.complexExpr=="exp(z)")return std::exp(z); if(g_state.complexExpr=="log(z)")return std::log(z);
        if(g_state.complexExpr=="sqrt(z)")return std::sqrt(z); if(g_state.complexExpr=="Gamma(z)")return SpecialFunctions::gamma(z);
        return z;
    };
    g_state.domainPts=PlotGenerator::domainColoring(f,g_state.cxMin,g_state.cxMax,g_state.cyMin,g_state.cyMax,256,256);
    g_state.domainDirty=false;
}

// ============================================================
// Main
// ============================================================
static void check_vk_result(VkResult err) {
    if(err!=VK_SUCCESS){std::cerr<<"VkResult "<<err<<"\n";if(err<0)abort();}
}

int main() {
    VulkanRenderer renderer;
    if(!renderer.init(1280,900,"VulkanCalc")){std::cerr<<"Failed to init\n";return 1;}
    GLFWwindow* w = renderer.window();
    IMGUI_CHECKVERSION(); ImGui::CreateContext(); ImGuiIO& io=ImGui::GetIO();
    io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard;

    // Font
    ImFont* fd = io.Fonts->AddFontDefault();
    const char* cjk="/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc";
    ImFont* fcjk = io.Fonts->AddFontFromFileTTF(cjk,16.0f,nullptr,io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
    if(!fcjk) fcjk=io.Fonts->AddFontFromFileTTF(cjk,16.0f,nullptr,io.Fonts->GetGlyphRangesJapanese());
    io.FontDefault = fcjk ? fcjk : fd;

    // Style — Windows 11 acrylic dark
    ImGui::StyleColorsDark(); auto& st=ImGui::GetStyle();
    st.WindowRounding=8;st.FrameRounding=5;st.ChildRounding=6;st.PopupRounding=6;
    st.WindowBorderSize=0;st.FrameBorderSize=0;st.ChildBorderSize=0;
    st.WindowPadding=ImVec2(12,12);st.FramePadding=ImVec2(8,5);st.ItemSpacing=ImVec2(6,6);
    auto c=[&st](ImGuiCol idx,float r,float g,float b,float a=1){st.Colors[idx]=ImVec4(r,g,b,a);};
    c(ImGuiCol_WindowBg,0.08f,0.08f,0.12f,0.92f); c(ImGuiCol_ChildBg,0.06f,0.06f,0.10f,0.85f);
    c(ImGuiCol_PopupBg,0.12f,0.12f,0.18f,0.95f); c(ImGuiCol_Text,0.92f,0.92f,0.95f,1);
    c(ImGuiCol_Border,0.15f,0.15f,0.22f,0.5f); c(ImGuiCol_FrameBg,0.10f,0.10f,0.15f,0.8f);
    c(ImGuiCol_FrameBgHovered,0.15f,0.15f,0.22f,0.85f); c(ImGuiCol_FrameBgActive,0.18f,0.18f,0.25f,0.9f);
    c(ImGuiCol_Button,0.14f,0.14f,0.20f,0.9f); c(ImGuiCol_ButtonHovered,0.22f,0.22f,0.30f,0.95f);
    c(ImGuiCol_ButtonActive,0.30f,0.30f,0.40f,1); c(ImGuiCol_Header,0.18f,0.18f,0.25f,0.8f);
    c(ImGuiCol_Separator,0.15f,0.15f,0.22f,0.5f); c(ImGuiCol_TitleBg,0.06f,0.06f,0.10f,0.95f);
    c(ImGuiCol_TitleBgActive,0.08f,0.08f,0.14f,0.95f);

    ImGui_ImplGlfw_InitForVulkan(w,true);
    ImGui_ImplVulkan_InitInfo vi{}; vi.Instance=renderer.m_instance; vi.PhysicalDevice=renderer.m_physicalDevice;
    vi.Device=renderer.m_device; vi.QueueFamily=renderer.m_graphicsQueueFamily; vi.Queue=renderer.m_graphicsQueue;
    vi.DescriptorPool=renderer.m_descriptorPool; vi.RenderPass=renderer.m_renderPass; vi.Subpass=0;
    vi.MinImageCount=2; vi.ImageCount=(uint32_t)renderer.m_swapchainImages.size(); vi.MSAASamples=VK_SAMPLE_COUNT_1_BIT;
    vi.CheckVkResultFn=check_vk_result; ImGui_ImplVulkan_Init(&vi); ImGui_ImplVulkan_CreateFontsTexture();
    g_state.expr="sin(x)";

    // ============================================================
    // MAIN LOOP
    // ============================================================
    while(!glfwWindowShouldClose(w)) {
        glfwPollEvents();
        int fbw,fbh; glfwGetFramebufferSize(w,&fbw,&fbh);
        if(fbw>0&&fbh>0&&(fbw!=(int)renderer.m_width||fbh!=(int)renderer.m_height)) {
            renderer.m_width=fbw; renderer.m_height=fbh; renderer.recreateSwapchain(); }
        uint32_t ii;
        auto rv=vkAcquireNextImageKHR(renderer.m_device,renderer.m_swapchain,UINT64_MAX,renderer.m_imageSemaphore,VK_NULL_HANDLE,&ii);
        if(rv==VK_ERROR_OUT_OF_DATE_KHR){renderer.recreateSwapchain();continue;}
        vkWaitForFences(renderer.m_device,1,&renderer.m_fence,VK_TRUE,UINT64_MAX);
        vkResetFences(renderer.m_device,1,&renderer.m_fence);
        ImGui_ImplVulkan_NewFrame(); ImGui_ImplGlfw_NewFrame(); ImGui::NewFrame();

        // ---- Main window ----
        ImGui::SetNextWindowPos(ImVec2(0,0));
        ImGui::SetNextWindowSize(ImVec2((float)renderer.m_width,(float)renderer.m_height));
        ImGui::Begin("##mw",nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoCollapse);

        // ---- Top bar ----
        const char* lns[]={"EN","中文","日本語"};
        int li=(int)g_lang; ImGui::SetNextItemWidth(64);
        if(ImGui::BeginCombo("##lang",lns[li])){for(int i=0;i<3;i++){bool s=i==li;if(ImGui::Selectable(lns[i],&s))g_lang=(Lang)i;if(s)ImGui::SetItemDefaultFocus();}ImGui::EndCombo();}
        if(ImGui::IsItemHovered())ImGui::SetTooltip("%s",T("Language","语言","言語"));
        ImGui::SameLine(); ImGui::TextUnformatted("|"); ImGui::SameLine();
        // Tabs
        const char* tbs[]={T("Calculator","计算器","電卓"),T("Fourier","傅里叶","フーリエ"),T("Complex","复变","複素")};
        for(int i=0;i<3;i++){
            bool act=g_state.mode==i;
            if(act)ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.25f,0.40f,0.65f,0.9f));
            else ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.12f,0.12f,0.18f,0.85f));
            if(ImGui::Button(tbs[i],ImVec2(0,28))){g_state.mode=(CalcState::Mode)i;if(i==1)generateFourierPlot();if(i==2&&g_state.domainDirty)generateDomainColoring();}
            ImGui::PopStyleColor(); if(i<2)ImGui::SameLine();
        }
        ImGui::Separator();

        // ============================================================
        // TAB: CALCULATOR (dual-pane layout)
        // ============================================================
        if(g_state.mode==CalcState::TAB_CALC) {
            float aw=ImGui::GetContentRegionAvail().x, ah=ImGui::GetContentRegionAvail().y;
            float leftW = aw*0.38f; if(leftW<320)leftW=320; if(leftW>aw-100)leftW=aw-100;
            float rightW = aw-leftW-8;

            // ---- LEFT PANEL: Input + Buttons ----
            ImGui::BeginChild("##left",ImVec2(leftW,0),false);

            // Expression input (large)
            ImGui::TextUnformatted("f(x) =");
            char buf[1024]; strncpy(buf,g_state.expr.c_str(),sizeof(buf)-1); buf[sizeof(buf)-1]=0;
            ImGui::PushItemWidth(-1);
            if(ImGui::InputText("##expr",buf,sizeof(buf),ImGuiInputTextFlags_EnterReturnsTrue)) {
                g_state.expr=buf; g_state.exprHasX=hasVarX(g_state.expr);
            }
            if(ImGui::IsItemDeactivatedAfterEdit()){g_state.expr=buf;g_state.exprHasX=hasVarX(g_state.expr);}
            ImGui::PopItemWidth();

            // x= value slider
            ImGui::SetNextItemWidth(-1);
            ImGui::SetNextItemWidth(-1);
            ImGui::InputDouble(T("x =","x =","x ="),&g_state.evalX,0.1,1,"%.4f");
            if(ImGui::IsItemHovered())ImGui::SetTooltip("%s",T("Drag to change x value","拖动改变 x 值","ドラッグしてx値を変更"));

            // Evaluate on Enter + show result
            g_state.evaluator.setExpression(g_state.expr);
            if(g_state.evaluator.valid()) {
                double v=g_state.evaluator.evaluate(g_state.evalX);
                g_state.result=fmt(v); g_state.error.clear();
            } else g_state.error=g_state.evaluator.lastError();
            ImGui::TextColored(ImVec4(0.2f,0.8f,0.3f,1),"f(%.4f) = %s",g_state.evalX,g_state.result.c_str());
            if(!g_state.error.empty()) ImGui::TextColored(ImVec4(1,0.3f,0.3f,1),"%s",g_state.error.c_str());

            ImGui::Separator();

            // ---- Button grid ----
            auto btn=[](const char* l,ImVec4 c=ImVec4(0.14f,0.14f,0.20f,0.9f))->bool{
                ImGui::PushStyleColor(ImGuiCol_Button,c);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImVec4(c.x+0.08f,c.y+0.08f,c.z+0.08f,0.95f));
                bool r=ImGui::Button(l,ImVec2(56,36)); ImGui::PopStyleColor(2); return r;
            };
            auto press=[&](const std::string& s){g_state.expr+=s;};
            auto nc=ImVec4(0.10f,0.10f,0.18f,0.9f), oc=ImVec4(0.22f,0.16f,0.16f,0.9f);
            auto ac=ImVec4(0.12f,0.22f,0.18f,0.9f); // sage green for advanced

            // Row 0: Functions
            if(btn("sin",ac)) press("sin("); ImGui::SameLine();
            if(btn("cos",ac)) press("cos("); ImGui::SameLine();
            if(btn("tan",ac)) press("tan("); ImGui::SameLine();
            if(btn("log",ac)) press("log("); ImGui::SameLine();
            if(btn("ln",ac))  press("ln(");
            // Row 1
            if(btn("exp",ac)) press("exp("); ImGui::SameLine();
            if(btn("sqrt",ac)) press("sqrt("); ImGui::SameLine();
            if(btn("abs",ac)) press("abs("); ImGui::SameLine();
            if(btn("π",ac))   press(std::to_string(std::numbers::pi)); ImGui::SameLine();
            if(btn("e",ac))   press(std::to_string(std::numbers::e));
            // Row 2-5: number pad
            if(btn("7",nc))press("7");ImGui::SameLine();if(btn("8",nc))press("8");ImGui::SameLine();if(btn("9",nc))press("9");ImGui::SameLine();if(btn("/",oc))press("/");ImGui::SameLine();if(btn("^",oc))press("^");
            if(btn("4",nc))press("4");ImGui::SameLine();if(btn("5",nc))press("5");ImGui::SameLine();if(btn("6",nc))press("6");ImGui::SameLine();if(btn("*",oc))press("*");ImGui::SameLine();if(btn("(",ImVec4(0.18f,0.18f,0.26f,0.9f)))press("(");
            if(btn("1",nc))press("1");ImGui::SameLine();if(btn("2",nc))press("2");ImGui::SameLine();if(btn("3",nc))press("3");ImGui::SameLine();if(btn("-",oc))press("-");ImGui::SameLine();if(btn(")",ImVec4(0.18f,0.18f,0.26f,0.9f)))press(")");
            if(btn("0",nc))press("0");ImGui::SameLine();if(btn(".",nc))press(".");ImGui::SameLine();if(btn("+",oc))press("+");ImGui::SameLine();
            auto dc=ImVec4(0.28f,0.12f,0.12f,0.9f);
            if(btn("DEL",dc)){if(!g_state.expr.empty())g_state.expr.pop_back();}ImGui::SameLine();
            if(btn("CLR",dc)){g_state.expr.clear();g_state.result.clear();g_state.error.clear();}

            ImGui::Separator();

            // ---- Calculus tools ----
            ImGui::TextColored(ImVec4(0.6f,0.6f,0.8f,1),"%s",T("Calculus","微积分","微積分"));

            // ∫ Integral
            if(btn("∫ f(x) dx",ac)) {
                auto r=Integrator::adaptiveSimpson([&](double x){g_state.evaluator.setExpression(g_state.expr);return g_state.evaluator.evaluate(x);},g_state.intA,g_state.intB);
                std::ostringstream o; o<<"∫["<<fmt(g_state.intA)<<","<<fmt(g_state.intB)<<"] f(x) dx = "<<fmt(r.value);
                g_state.intResult=o.str(); g_state.result=fmt(r.value);
                // Generate integration plot (f + shaded area)
                g_state.plotMode=CalcState::PLOT_INTEG;
                g_state.plots=PlotGenerator::generateMultiple(
                    {[&](double x){g_state.evaluator.setExpression(g_state.expr);return g_state.evaluator.evaluate(x);}},
                    g_state.plotXMin,g_state.plotXMax,500);
            }
            if(ImGui::IsItemHovered())ImGui::SetTooltip("%s",T("Definite integral","定积分","定積分"));
            ImGui::SameLine();

            // ∫ input fields
            ImGui::SetNextItemWidth(56); ImGui::InputDouble(T("Low","下限","下端"),&g_state.intA,0,0,"%.3f");
            ImGui::SameLine(); ImGui::SetNextItemWidth(56);
            ImGui::InputDouble(T("Upp","上限","上端"),&g_state.intB,0,0,"%.3f");
            if(ImGui::IsItemHovered())ImGui::SetTooltip("%s",T("Integration limits","积分上下限","積分範囲"));

            // d/dx
            if(btn("d/dx",ac)) {
                g_state.plotMode=CalcState::PLOT_DERIV;
                // Generate f(x) and f'(x)
                g_state.plots=PlotGenerator::generateMultiple(
                    {[&](double x){g_state.evaluator.setExpression(g_state.expr);return g_state.evaluator.evaluate(x);}},
                    g_state.plotXMin,g_state.plotXMax,500);
                auto deriv=[&](double x){g_state.evaluator.setExpression(g_state.expr);return Differentiator::derivative([&](double t){g_state.evaluator.setExpression(g_state.expr);return g_state.evaluator.evaluate(t);},x).value;};
                g_state.plots.push_back(PlotGenerator::generate(deriv,g_state.plotXMin,g_state.plotXMax,500,"f'(x)"));
            }
            if(ImGui::IsItemHovered())ImGui::SetTooltip("%s",T("First derivative","一阶导数","一階導関数"));
            ImGui::SameLine();
            if(btn("f''(x)",ac)) {
                g_state.plotMode=CalcState::PLOT_DERIV;
                g_state.plots=PlotGenerator::generateMultiple(
                    {[&](double x){g_state.evaluator.setExpression(g_state.expr);return g_state.evaluator.evaluate(x);}},
                    g_state.plotXMin,g_state.plotXMax,500);
                auto d2=[&](double x){g_state.evaluator.setExpression(g_state.expr);return Differentiator::secondDerivative([&](double t){g_state.evaluator.setExpression(g_state.expr);return g_state.evaluator.evaluate(t);},x).value;};
                g_state.plots.push_back(PlotGenerator::generate(d2,g_state.plotXMin,g_state.plotXMax,500,"f''(x)"));
            }
            if(ImGui::IsItemHovered())ImGui::SetTooltip("%s",T("Second derivative","二阶导数","二階導関数"));

            // Inverse function
            ImGui::SameLine();
            if(btn("f⁻¹(y)",ac)){
                double y=g_state.evalX;
                // Simple numeric inverse via binary search (monotonic assumed)
                auto f=[&](double x){g_state.evaluator.setExpression(g_state.expr);return g_state.evaluator.evaluate(x);};
                double lo=-100,hi=100;
                for(int it=0;it<100;it++){
                    double mid=(lo+hi)/2;
                    if(f(mid)<y)lo=mid;else hi=mid;
                }
                g_state.result="f⁻¹("+fmt(y)+") ≈ "+fmt((lo+hi)/2);
                g_state.plotMode=CalcState::PLOT_NONE;
            }
            if(ImGui::IsItemHovered())ImGui::SetTooltip("%s",T("Inverse function (numeric)","反函数（数值）","逆関数（数値）"));

            // Integration result
            if(!g_state.intResult.empty())
                ImGui::TextColored(ImVec4(0.6f,0.4f,0.1f,1),"%s",g_state.intResult.c_str());

            ImGui::Separator();

            // ---- Complex tools ----
            ImGui::TextColored(ImVec4(0.6f,0.6f,0.8f,1),"%s",T("Complex Analysis","复分析","複素解析"));

            if(btn("Cauchy ∮",ac)) {
                Complex a(g_state.complexRe,g_state.complexIm);
                auto f=[&](Complex z)->Complex{g_state.evaluator.setExpression(g_state.expr);return Complex(g_state.evaluator.evaluate(z.real()),0);};
                auto cr=ComplexAnalysis::cauchyIntegral(f,a,g_state.cauchyRadius);
                g_state.complexResult="Cauchy ∮ = "+to_string(cr,4); g_state.result=to_string(cr,4);
            }
            ImGui::SameLine(); if(ImGui::IsItemHovered())ImGui::SetTooltip("%s",T("Cauchy integral formula","柯西积分公式","コーシー積分公式"));

            if(btn("Residue",ac)) {
                Complex a(g_state.complexRe,g_state.complexIm);
                auto f=[&](Complex z)->Complex{g_state.evaluator.setExpression(g_state.expr);return Complex(g_state.evaluator.evaluate(z.real()),0);};
                auto res=ComplexAnalysis::residue(f,a);
                g_state.complexResult="Res(f,"+to_string(a,4)+") = "+to_string(res,4);
            }
            ImGui::SameLine(); if(ImGui::IsItemHovered())ImGui::SetTooltip("%s",T("Residue at pole","留数","留数"));

            if(btn("Γ(z)",ac)){g_state.result="Γ(0.5) = "+fmt(SpecialFunctions::gamma(0.5),8);}
            ImGui::SameLine(); if(ImGui::IsItemHovered())ImGui::SetTooltip("%s",T("Gamma function","伽马函数","ガンマ関数"));

            // Complex params
            ImGui::SetNextItemWidth(56); ImGui::InputDouble("Re(a)",&g_state.complexRe,0,0,"%.2f"); ImGui::SameLine();
            ImGui::SetNextItemWidth(56); ImGui::InputDouble("Im(a)",&g_state.complexIm,0,0,"%.2f"); ImGui::SameLine();
            ImGui::SetNextItemWidth(56); ImGui::InputDouble("R=",&g_state.cauchyRadius,0,0,"%.2f");
            if(!g_state.complexResult.empty())
                ImGui::TextColored(ImVec4(0.4f,0.6f,0.1f,1),"%s",g_state.complexResult.c_str());

            ImGui::EndChild(); // left panel

            // ---- RIGHT PANEL: Graph ----
            ImGui::SameLine();
            ImGui::BeginChild("##right",ImVec2(rightW,0),false);
            ImVec2 gs(ImGui::GetContentRegionAvail().x,ah-20);
            if(ImGui::BeginChild("##plot",gs,true,ImGuiWindowFlags_NoScrollbar)){
                auto* dl=ImGui::GetWindowDrawList();
                auto o=ImGui::GetCursorScreenPos(); auto sz=ImGui::GetContentRegionAvail();
                float mg=50; ImVec2 po(o.x+mg,o.y), ps(sz.x-mg,sz.y-mg);
                if(ps.x>0&&ps.y>0){
                    double xMin=g_state.plotXMin,xMax=g_state.plotXMax,yMin=-5,yMax=5;

                    if(g_state.plotMode==CalcState::PLOT_DERIV&&g_state.plots.size()>=2){
                        // Auto-scale: find bounds from both curves
                        yMin=1e30;yMax=-1e30;
                        for(auto&p:g_state.plots){if(!p.points.empty()){if(p.y_min<yMin)yMin=p.y_min;if(p.y_max>yMax)yMax=p.y_max;}}
                        double pad=(yMax-yMin)*0.1; yMin-=pad;yMax+=pad;
                    } else if(g_state.plotMode==CalcState::PLOT_INTEG&&!g_state.plots.empty()){
                        auto& p=g_state.plots[0]; yMin=p.y_min;yMax=p.y_max;
                        double pad=(yMax-yMin)*0.1; yMin-=pad;yMax+=pad;
                    }

                    drawGraphBg(dl,po,ps);
                    drawGrid(dl,po,ps,xMin,xMax,yMin,yMax);

                    if(g_state.plotMode==CalcState::PLOT_INTEG&&!g_state.plots.empty()){
                        // Shade area under curve
                        auto ts=makeToScreen(po,ps,xMin,xMax,yMin,yMax);
                        auto&p=g_state.plots[0];
                        for(size_t i=1;i<p.points.size();i++){
                            double x0=p.points[i-1].x,x1=p.points[i].x;
                            if(x1>=g_state.intA&&x0<=g_state.intB){
                                double clipX0=std::max(x0,g_state.intA),clipX1=std::min(x1,g_state.intB);
                                if(clipX1>clipX0){
                                    double y0=p.points[i-1].y,y1=p.points[i].y;
                                    auto a=ts(clipX0,y0),b=ts(clipX1,y1);
                                    auto c=ts(clipX1,0),d=ts(clipX0,0);
                                    dl->AddQuadFilled(a,b,c,d,IM_COL32(80,160,255,60));
                                }
                            }
                        }
                    }

                    drawPlot(dl,po,ps,xMin,xMax,yMin,yMax,g_state.plots,{IM_COL32(80,200,255,255),IM_COL32(255,180,80,255)});
                    drawAxesLabels(dl,po,ps,xMin,xMax,yMin,yMax);

                    // Legend
                    if(!g_state.plots.empty()){
                        if(g_state.plots.size()>=1){
                            dl->AddText(ImVec2(po.x+8,po.y+8),IM_COL32(80,200,255,255)," f(x)");
                        }
                        if(g_state.plots.size()>=2){
                            dl->AddText(ImVec2(po.x+8,po.y+28),IM_COL32(255,180,80,255)," f'(x)");
                        }
                        if(g_state.plots.size()>=3){
                            dl->AddText(ImVec2(po.x+8,po.y+48),IM_COL32(180,255,80,255)," f''(x)");
                        }
                    }
                }
            }
            ImGui::EndChild();
            ImGui::EndChild(); // right panel
        }

        // ============================================================
        // TAB: FOURIER
        // ============================================================
        if(g_state.mode==CalcState::TAB_FOURIER){
            ImGui::RadioButton(T("Square","方波","方形波"),&g_state.fourierType,0);ImGui::SameLine();
            ImGui::RadioButton(T("Sawtooth","锯齿波","鋸波"),&g_state.fourierType,1);ImGui::SameLine();
            ImGui::RadioButton(T("Triangle","三角波","三角波"),&g_state.fourierType,2);
            ImGui::SetNextItemWidth(200); if(ImGui::SliderInt(T("N Terms","项数 N","項数 N"),&g_state.fourierTerms,1,50)) generateFourierPlot();
            ImGui::SameLine(); if(ImGui::Button(T("Redraw","重绘","再描画"))) generateFourierPlot();
            ImVec2 gs(ImGui::GetContentRegionAvail().x,500);
            if(ImGui::BeginChild("##fp",gs,true,ImGuiWindowFlags_NoScrollbar)){
                auto*dl=ImGui::GetWindowDrawList(); auto o=ImGui::GetCursorScreenPos(); auto sz=ImGui::GetContentRegionAvail();
                double xM=-10,xX=10,yM=-2,yX=2;
                if(!g_state.fourierPlots.empty()){yM=g_state.fourierPlots[0].y_min;yX=g_state.fourierPlots[0].y_max;}
                float mg=50; ImVec2 po(o.x+mg,o.y), ps(sz.x-mg,sz.y-mg);
                if(ps.x>0&&ps.y>0){
                    drawGraphBg(dl,po,ps); drawGrid(dl,po,ps,xM,xX,yM,yX);
                    for(auto&p:g_state.fourierPlots){
                        bool ex=p.label.find("Exact")!=std::string::npos;
                        auto col=ex?IM_COL32(255,160,80,255):IM_COL32(80,200,255,255);
                        auto ts=makeToScreen(po,ps,xM,xX,yM,yX);
                        for(size_t i=1;i<p.points.size();i++)dl->AddLine(ts(p.points[i-1].x,p.points[i-1].y),ts(p.points[i].x,p.points[i].y),col,ex?1:2);
                    }
                    drawAxesLabels(dl,po,ps,xM,xX,yM,yX);
                }
            } ImGui::EndChild();
            ImGui::TextColored(ImVec4(0.3f,0.8f,1,1),"█ %s",T("Fourier approx.","傅里叶逼近","フーリエ近似"));
            ImGui::SameLine(); ImGui::TextColored(ImVec4(1,0.6f,0.3f,1),"█ %s",T("Exact","精确","正確"));
        }

        // ============================================================
        // TAB: COMPLEX DOMAIN
        // ============================================================
        if(g_state.mode==CalcState::TAB_COMPLEX){
            ImGui::TextUnformatted("f(z) ="); ImGui::SameLine();
            char buf[128]; strncpy(buf,g_state.complexExpr.c_str(),sizeof(buf)-1); buf[sizeof(buf)-1]=0;
            if(ImGui::InputText("##ce",buf,sizeof(buf))){g_state.complexExpr=buf;g_state.domainDirty=true;}
            ImGui::SameLine(); if(ImGui::Button(T("Render","渲染","レンダリング"))) generateDomainColoring();
            auto pr=[&](const char* n,const char* e){ImGui::SameLine();if(ImGui::SmallButton(n)){g_state.complexExpr=e;generateDomainColoring();}};
            pr("z","z"); pr("z²","z^2"); pr("z³","z^3"); pr("1/z","1/z"); pr("sin(z)","sin(z)"); pr("cos(z)","cos(z)"); pr("exp(z)","exp(z)"); pr("Γ(z)","Gamma(z)");
            ImGui::SetNextItemWidth(60);ImGui::InputDouble("x min",&g_state.cxMin);ImGui::SameLine();
            ImGui::SetNextItemWidth(60);ImGui::InputDouble("x max",&g_state.cxMax);ImGui::SameLine();
            ImGui::SetNextItemWidth(60);ImGui::InputDouble("y min",&g_state.cyMin);ImGui::SameLine();
            ImGui::SetNextItemWidth(60);ImGui::InputDouble("y max",&g_state.cyMax);ImGui::SameLine();
            if(ImGui::Button(T("Update","更新","更新"))) generateDomainColoring();
            ImVec2 gs(ImGui::GetContentRegionAvail().x,500);
            if(ImGui::BeginChild("##dp",gs,true,ImGuiWindowFlags_NoScrollbar)){
                auto*dl=ImGui::GetWindowDrawList(); auto o=ImGui::GetCursorScreenPos(); auto sz=ImGui::GetContentRegionAvail();
                dl->AddRectFilled(o,ImVec2(o.x+sz.x,o.y+sz.y),IM_COL32(10,10,14,255));
                if(!g_state.domainPts.empty()){
                    float dx=sz.x/(float)(g_state.cxMax-g_state.cxMin), dy=sz.y/(float)(g_state.cyMax-g_state.cyMin);
                    for(auto&pt:g_state.domainPts){
                        float sx=o.x+(float)((pt.x-g_state.cxMin)/(g_state.cxMax-g_state.cxMin))*sz.x;
                        float sy=o.y+(float)(1-(pt.y-g_state.cyMin)/(g_state.cyMax-g_state.cyMin))*sz.y;
                        dl->AddRectFilled(ImVec2(sx,sy),ImVec2(sx+dx,sy+dy),IM_COL32((int)(pt.r*255),(int)(pt.g*255),(int)(pt.b*255),255));
                    }
                } else {ImGui::SetCursorPos(ImVec2(gs.x/2-80,gs.y/2-10));ImGui::TextUnformatted(T("Press Render","点击渲染","レンダリング"));}
            } ImGui::EndChild();
            ImGui::TextColored(ImVec4(0.6f,0.6f,0.8f,1),"%s",T("Hue=arg(f(z)) Bright=|f(z)|","色相=辐角 亮度=模长","色相=偏角 輝度=絶対値"));
        }

        ImGui::End(); // main window

        // ---- Vulkan render ----
        ImGui::Render();
        VkCommandBuffer cmd=renderer.m_commandBuffer; vkResetCommandBuffer(cmd,0);
        VkCommandBufferBeginInfo bi{}; bi.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO; bi.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd,&bi);
        VkClearValue cl{}; cl.color.float32[0]=0.06f; cl.color.float32[1]=0.06f; cl.color.float32[2]=0.10f; cl.color.float32[3]=1.0f;
        VkRenderPassBeginInfo rp{}; rp.sType=VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO; rp.renderPass=renderer.m_renderPass;
        rp.framebuffer=renderer.m_framebuffers[ii]; rp.renderArea.offset={0,0}; rp.renderArea.extent=renderer.m_swapchainExtent;
        rp.clearValueCount=1; rp.pClearValues=&cl;
        vkCmdBeginRenderPass(cmd,&rp,VK_SUBPASS_CONTENTS_INLINE); ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),cmd); vkCmdEndRenderPass(cmd);
        vkEndCommandBuffer(cmd);
        VkSubmitInfo si{}; si.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO; VkSemaphore ws[]={renderer.m_imageSemaphore}; VkPipelineStageFlags wst[]={VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        si.waitSemaphoreCount=1; si.pWaitSemaphores=ws; si.pWaitDstStageMask=wst; si.commandBufferCount=1; si.pCommandBuffers=&cmd;
        VkSemaphore ss[]={renderer.m_renderSemaphore}; si.signalSemaphoreCount=1; si.pSignalSemaphores=ss;
        vkQueueSubmit(renderer.m_graphicsQueue,1,&si,renderer.m_fence);
        VkPresentInfoKHR pi{}; pi.sType=VK_STRUCTURE_TYPE_PRESENT_INFO_KHR; pi.waitSemaphoreCount=1; pi.pWaitSemaphores=ss;
        pi.swapchainCount=1; pi.pSwapchains=&renderer.m_swapchain; pi.pImageIndices=&ii;
        vkQueuePresentKHR(renderer.m_presentQueue,&pi);
    }
    vkDeviceWaitIdle(renderer.m_device); ImGui_ImplVulkan_Shutdown(); ImGui_ImplGlfw_Shutdown(); ImGui::DestroyContext();
    return 0;
}
