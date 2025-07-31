# 项目名称
TARGET = PandaCtrl

# 目录结构
SRC_DIR = src
INC_DIR = include
LIB_DIR = lib

# 编译器设置
CC = gcc
CFLAGS = -I$(INC_DIR) -Wall -Wextra
LDFLAGS = -L$(LIB_DIR) -lusb-1.0 -lpthread -ludev -ldl

# 源文件和对象文件
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:.c=.o)

# 默认目标
all: $(TARGET)

# 主目标链接
$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

# 编译规则
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 清理
clean:
	rm -f $(SRC_DIR)/*.o $(TARGET)

# 安装依赖（可选）
#install_deps:
#	sudo apt-get install libusb-1.0-0-dev

.PHONY: all clean install_deps
