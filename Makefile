# ----------- Raspberry Pi 3B (ARM64) JNI Makefile -----------

# 1. Java 环境
JAVA_HOME ?= /usr/lib/jvm/java-17-openjdk-arm64

# 2. 目录
SRC_DIR  := src
OBJ_DIR  := obj
BIN_DIR  := bin
INC_DIR  := include

# 3. 源文件 / 目标文件
SRC      := $(wildcard $(SRC_DIR)/*.cpp)
OBJ      := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC))
TARGET   := $(BIN_DIR)/libJniAlgorithm.so

# 4. 编译 / 链接参数
CXX      := g++
CXXFLAGS := -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux \
            -I$(INC_DIR) -fPIC -Wall -O2
LDFLAGS  := -shared            # 无其它库依赖

# ------------------- 规则 -------------------

all: $(TARGET)

$(TARGET): $(OBJ) | $(BIN_DIR)
	$(CXX) $(LDFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all clean
# ------------------------------------------------------------
