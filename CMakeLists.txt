cmake_minimum_required(VERSION 3.10)
project(SoilHMsInversion)
# 设置可执行文件的输出目录为项目根目录下的 DLLs 文件夹
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/DLLs)

set(CMAKE_CXX_STANDARD 17)

# 1. 头文件包含路径
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/gdal
    ${CMAKE_CURRENT_SOURCE_DIR}/include/json
)

# 2. 生成可执行文件
add_executable(HMs_Predict main.cpp)

# 3. 链接动态库
target_link_libraries(HMs_Predict 
    ${CMAKE_CURRENT_SOURCE_DIR}/DLLs/xgboost.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/DLLs/gdal.dll   
)