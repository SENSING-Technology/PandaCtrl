
TARGET := PandaCtrl

SRC_DIR := src
INC_DIR := include

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(SRCS:.c=.o)

CROSS_COMPILE ?= aarch64-linux-gnu-
SYSROOT ?= /home/ubuntu2204/qualcomm/sysroot

CC := $(CROSS_COMPILE)gcc
AR := $(CROSS_COMPILE)ar
STRIP := $(CROSS_COMPILE)strip

CFLAGS := -I$(INC_DIR) -Wall -Wextra
CFLAGS += --sysroot=$(SYSROOT)

LDFLAGS := --sysroot=$(SYSROOT)
LDFLAGS += -L$(SYSROOT)/usr/lib
LDFLAGS += -L$(SYSROOT)/lib
LDFLAGS += -Wl,-rpath-link,$(SYSROOT)/usr/lib

LIBS := -lusb-1.0 -ludev -lpthread -ldl

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)
	$(STRIP) $@ || true

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean