# 土壤重金属反演 C++ 项目
## 一、项目目录结构
创建一个名为 `HMs` 的文件夹，按以下结构组织：
```Plaintext
HMs/
├── input.tif              # 输入卫星影像
├── CMakeLists.txt         # 编译配置文件
├── main.cpp               # 核心 C++ 推理源码
├── data_test.csv          # 测试数据集
├── gdal-data/             # GDAL 坐标转换支持数据
├── include/               # 第三方头文件
│   ├── xgboost/           # XGBoost API 头文件
│   ├── gdal/              # GDAL C/C++ 头文件
│   └── json/              
│       └── json.hpp       # JSON 解析库
├── lib/                   
│   └── gdal_i.lib         # GDAL 导入库 (针对 MSVC)
├── DLLs/                  # 运行时依赖文件夹 (运行环境)
│   ├── xgboost.dll        # XGBoost 动态链接库
│   ├── gdal.dll           # GDAL 核心库
│   ├── (其他 GDAL DLLs)   # proj.dll, geos.dll 等
│   └── HMs_Predict.exe    # build后生成的执行文件
└── model/            
    ├── v5_xgb_model.json  # 训练好的 XGBoost 模型
    └── scaler_params.json # 标准化均值与缩放系数
```
## 二、 环境准备与依赖下载
1. **安装编译器**: 下载并安装 [MinGW-w64](https://github.com/niXman/mingw-builds-binaries/releases)，确保将 `bin` 文件夹路径添加到系统的**环境变量 (Path)** 中。
2. **安装 VS Code 插件**:
- **C/C++** (Microsoft)
- **CMake Tools** (Microsoft)
3. **获取依赖库**:
- **XGBoost**: 在 Python 环境的 `Lib\site-packages\xgboost` 文件夹中找到 `xgboost.dll`。
- **nlohmann/json**: 下载 [json.hpp](https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp) 并放入 `include/json/` 目录下。
- **c_api.h**：访问 XGBoost 的 GitHub 发布页：[Releases · dmlc/xgboost](https://github.com/dmlc/xgboost/releases)，找到最新版本（或者与你 pip 安装版本相近的版本），在 Assets 区域，下载 Source code (zip)，解压这个压缩包。
- **GDAL**: 推荐从 [GISInternals](https://www.gisinternals.com/) 下载与编译器匹配的编译包。
    - 将 `bin` 文件夹下的所有 `.dll` 放入项目 `DLLs/`。
    - 将 `include` 文件夹下的头文件放入项目 `include/gdal/`。
    - 将 `gdal-data` 文件夹放入项目根目录。
    - 将 `gdal_i.lib` 放入 `lib`。

## 三、编写 `CMakeLists.txt`
## 四、编写 `main.cpp` (核心代码)
## 五、在 VS Code 中编译运行
1. **打开项目**: 在 VS Code 中“打开文件夹”选择 `HMs`。
2. **配置 CMake**:
- 点击底部状态栏的 **CMake: [Release]: Ready** 按钮（或按 `Ctrl+Shift+P` 输入 `CMake: Configure`）。
- 选择你的编译器（例如 `GCC 13.x.x x86_64-w64-mingw32`）。
3. **编译**:
- 点击底部状态栏的 **Build**。
## 第六步：执行
### 第一步：找到生成的可执行文件
在 VS Code 的左侧文件资源管理器中，展开 `DLLs` 文件夹。通常情况下，路径是这样的：
- `HMs/DLLs/HMs_Predict.exe`
### 第二步：执行预测
推荐使用 **VS Code 的终端 (Terminal)** 来运行，这样能清晰地看到输出结果。
1. **打开终端**：在 VS Code 中按 Ctrl + ` (反引号键) 打开终端。
2. **进入目录**：使用 `cd` 命令进入包含 exe 的目录。
3. **运行程序**： 输入`.\HMs_Predict.exe`命令并回车。