这是一个完全由AI编写的工具

# VulkanCalc 2.0 — Vulkan 加速的高级科学计算器

![Version](https://img.shields.io/badge/version-2.0.0-blue)
![Language](https://img.shields.io/badge/C%2B%2B-20-blue)
![Vulkan](https://img.shields.io/badge/Vulkan-1.3-red)
![License](https://img.shields.io/badge/license-MIT-green)

一个基于 **Vulkan** 硬件加速渲染和 **Dear ImGui** 图形界面的高级科学计算器，支持实时函数绘图、数值分析、概率统计、3D曲面可视化、复变函数可视化。

## ✨ 功能 (v2.0)

### 🔢 表达式计算
- 输入任意表达式并实时求值（如 `sin(x)^2 + cos(x)^2`）
- 支持常用数学函数：`sin`, `cos`, `tan`, `log`, `ln`, `exp`, `sqrt`, `abs` 等
- 支持复变函数求值

### 📈 函数图像绘制
- 单函数/多函数叠加绘制，可调节坐标范围
- 自动坐标轴缩放，支持网格线
- 支持导数曲线/积分曲线的**双曲线对比**显示
- 支持**反函数**图像绘制
- **导数图像模式**：直接绘制一阶/二阶导数曲线

### 🔬 数值分析
- **数值积分**：自适应 Simpson、Romberg、Gauss-Legendre、Monte Carlo 四种方法
- **数值微分**：一阶导数、二阶导数、偏导数
- **复变导数**：Cauchy-Riemann 验证 + 复导数计算
- **体积分/曲面积分**：三维标量/向量场体积分与曲面积分计算

### 📊 概率与统计
- **概率分布**：正态分布、二项分布、泊松分布、均匀分布的概率密度/CDF 计算与绘图
- **描述统计**：均值、方差、标准差、偏度、峰度
- **柱状图**：数据分组柱状图可视化

### 🌈 复变函数可视化
- **复变函数彩色相位图** (Domain Coloring)
  - 色相 = arg(f(z))
  - 亮度 = |f(z)|
- **傅里叶级数可视化**
- **Cauchy 积分公式**数值计算
- **留数计算**和 Laurent 级数展开

### 🏗️ 3D 可视化
- **3D 曲面图**：`z = f(x, y)` 三维曲面实时绘制
- 可旋转/缩放视角（鼠标拖拽 + 滚轮）

### 📐 特殊函数
- Gamma 函数（实数 + 复数）
- 误差函数 erf
- Bessel 函数 Jₙ(x), Yₙ(x)

### 🌍 多语言界面
- 中文 / English / 日本語 三语言切换

### 🧩 扩展 API
- 脚本化函数扩展接口，支持用户自定义数学函数

## 🖼️ 截图

| 主界面 | 函数绘图 | 3D曲面 |
|:-----:|:-------:|:------:|
| *(待补充)* | *(待补充)* | *(待补充)* |

| 复变相位图 | 概率统计 | 数值积分 |
|:---------:|:-------:|:-------:|
| *(待补充)* | *(待补充)* | *(待补充)* |

## 🛠️ 构建

### 依赖

| 依赖 | 版本要求 |
|------|---------|
| CMake | ≥ 3.22 |
| C++ 编译器 | GCC 11+ / Clang 14+ / MSVC 2022 |
| Vulkan SDK | 1.3+ |
| GLFW | 3.x |
| GLM | 0.9.9+ |

### Linux (含 WSL)

```bash
# 安装依赖 (Ubuntu/Debian)
sudo apt install cmake g++ libglfw3-dev libglm-dev \
    libvulkan-dev vulkan-validationlayers \
    glslang-tools

# 构建
cd vulkan-calc
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# 运行
./build/vulkan_calc
```

> **WSL 注意**：默认使用软件渲染器。如需硬件加速，确保安装并更新了 Mesa 的 Vulkan 驱动：
> ```bash
> sudo apt install mesa-vulkan-drivers
> ```

### Windows

```bat
# 需要 Visual Studio 2022 + Vulkan SDK + vcpkg
# 从 "Developer Command Prompt for VS 2022" 运行：
build-windows.bat
```

或者手动：

```bat
# 安装 vcpkg 依赖（首次）
vcpkg install glfw3:x64-windows glm:x64-windows

# 配置并构建
cmake -B build -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"
cmake --build build --config Release

# 运行
build\Release\vulkan_calc.exe
```

> **注意**：构建脚本 `build-windows.bat` 会自动检查 Vulkan SDK、vcpkg 环境，
> 使用 CMake 配置 Visual Studio 17 2022 生成器，构建 Release 版，
> 并将输出文件（exe、图标、Vulkan DLL）复制到 `F:\vulkan-calc\` 目录。

### 安装包

Windows 安装包使用 Inno Setup 构建：

```bat
# 安装 Inno Setup 6 (https://jrsoftware.org/isinfo.php)
# 在项目根目录运行：
iscc installer.iss
```

安装包输出到 `F:\vulkan-calc\installer\`，包含：
- 桌面快捷方式 & 开始菜单
- 自动安装后运行
- 完整卸载支持
- 中文界面

## 🚀 快速开始

1. 启动程序后，顶部工具栏选择语言
2. 在计算器面板输入表达式（如 `x^2 + 3*x - 1`）
3. 点击「绘制」查看函数图像
4. 使用左侧面板切换不同功能模式

### 快捷键

| 快捷键 | 功能 |
|--------|------|
| `Esc` | 关闭/退出 |
| `F11` | 全屏切换 |
| 鼠标滚轮 | 缩放函数图像 |
| 鼠标拖拽 | 平移函数图像 |

## 📁 项目结构

```
vulkan-calc/
├── CMakeLists.txt          # CMake 构建配置
├── build-windows.bat       # Windows 一键构建脚本
├── installer.iss           # Inno Setup 安装包脚本（Windows）
├── resources.rc            # Windows 资源文件（图标）
├── src/
│   ├── main.cpp            # 主程序 + ImGui 界面
│   ├── math_engine.hpp     # 数学引擎头文件
│   ├── math_engine.cpp     # 数学引擎实现
│   ├── vulkan_renderer.hpp # Vulkan 渲染器头文件
│   └── vulkan_renderer.cpp # Vulkan 渲染器实现
├── shaders/
│   ├── calc.vert/.frag     # 计算器着色器
│   ├── graph.vert/.frag    # 图形绘制着色器
│   └── text.vert/.frag     # 文本渲染着色器
├── imgui/                  # Dear ImGui (bundled)
├── fonts/                  # 字体文件
└── icon/                   # 应用图标
```

## 📜 许可

MIT License — 详见 [LICENSE](LICENSE)

---

*用 Vulkan 加速数学之美*
