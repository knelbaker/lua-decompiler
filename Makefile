CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
SOURCES = $(wildcard src/*.cpp)
TARGET = build/lua_decompiler

all: $(TARGET)

$(TARGET): $(SOURCES)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean
