.SUFFIXES:

# ==================
# = PROJECT CONFIG =
# ==================

TARGET := lyn

SOURCES := \
  3rdparty/fmt/src/format.cc \
  natelf/natelf.cc \
  natelf/arm32.cc \
  lynelf.cc \
  layout.cc \
  symtab.cc \
  relocation.cc \
  event_helpers.cc \
  output.cc \
  main.cc

BUILD_DIR := build

CXXFLAGS += -g -Wall -Wextra -Wno-unused -std=c++20 -Og \
  -I 3rdparty/fmt/include -iquote .

CXX ?= g++

# ===================
# = RULES & RECIPES =
# ===================

OBJECTS := $(SOURCES:%.cc=$(BUILD_DIR)/%.o)
DEPENDS := $(SOURCES:%.cc=$(BUILD_DIR)/%.d)

all: $(TARGET)

clean:
	rm -rf $(BUILD_DIR)/
	rm -f $(TARGET)

.PHONY: all clean

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS)

$(OBJECTS): $(BUILD_DIR)/%.o: %.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ $< -c

$(DEPENDS): $(BUILD_DIR)/%.d: %.cc
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -o $@ $< -MM -MG -MT $@ -MT $(BUILD_DIR)/$*.o

-include $(DEPENDS)
