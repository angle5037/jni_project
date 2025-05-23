cmake_minimum_required(VERSION 3.16)
project(JniAlgorithm LANGUAGES CXX)

# ---------- 目录布局 ----------
set(SRC_DIR      "${CMAKE_SOURCE_DIR}/src")
set(INCLUDE_DIR  "${CMAKE_SOURCE_DIR}/include")
set(BIN_DIR      "${CMAKE_SOURCE_DIR}/bin")     # 最终输出在这里

# ---------- 找 JNI ----------
# Raspberry Pi 装有 OpenJDK，FindJNI 会自动探测 /usr/lib/jvm/*
find_package(JNI REQUIRED)

# ---------- 编译选项 ----------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Release 默认 -O3；开发阶段可指定 -DCMAKE_BUILD_TYPE=Debug
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ---------- 目标 1：共享库 libJniAlgorithm.so ----------
file(GLOB LIB_SRC "${SRC_DIR}/*.cpp")

add_library(JniAlgorithm SHARED ${LIB_SRC})
target_include_directories(JniAlgorithm
    PRIVATE
        ${INCLUDE_DIR}
        ${JNI_INCLUDE_DIRS}
)

# 如需链接其他系统库或用户库，追加到 target_link_libraries
# 例：target_link_libraries(JniAlgorithm PRIVATE G2Encryption)
# 当前编译阶段无外部依赖，留空即可

# 自定义输出目录（bin）
set_target_properties(JniAlgorithm PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${BIN_DIR}
)

# ---------- 目标 2：测试可执行文件 ----------
add_executable(test_jni_algorithm test_jni_algorithm.cpp)
target_include_directories(test_jni_algorithm
    PRIVATE
        ${JNI_INCLUDE_DIRS}
)
target_link_libraries(test_jni_algorithm PRIVATE dl)

# 测试程序输出也放在 bin，保持整洁
set_target_properties(test_jni_algorithm PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR}
)

# ---------- 安装规则（可选） ----------
# install(TARGETS JniAlgorithm DESTINATION lib)
# install(TARGETS test_jni_algorithm DESTINATION bin)
