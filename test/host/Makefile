# Host-side tests for the hardware-independent firmware logic.
# No Teensy, no toolchain, no network required:
#
#   make -C test/host        build and run
#   make -C test/host clean

CXX      ?= g++
CXXFLAGS ?= -std=gnu++17 -Wall -Wextra -Werror -g -O1
SRC_DIR   = ../../firmware/TeensyIntercom/src
STUB_DIR  = arduino_stubs
BUILD_DIR = build

INCLUDES = -I$(STUB_DIR) -I$(SRC_DIR)

SOURCES = test_intercom.cpp \
          $(STUB_DIR)/Arduino.cpp \
          $(STUB_DIR)/EEPROM.cpp \
          $(SRC_DIR)/IntercomConfig.cpp \
          $(SRC_DIR)/ButtonPanel.cpp \
          $(SRC_DIR)/StatusLed.cpp

TARGET = $(BUILD_DIR)/test_intercom

.PHONY: all test clean
all: test

test: $(TARGET)
	@$(TARGET)

$(TARGET): $(SOURCES) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SOURCES) -o $@

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

clean:
	@rm -rf $(BUILD_DIR)
