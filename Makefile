CXX := g++
TARGET := spotlight
TMPDIR := .tmp

CXXFLAGS := -std=c++23 -O3
NAME := spotlight

SOURCES := $(wildcard src/*.cpp)
OBJECTS := $(patsubst %.cpp,$(TMPDIR)/%.o,$(SOURCES))
DEPENDS := $(patsubst %.cpp,$(TMPDIR)/%.d,$(SOURCES))

all: $(TARGET)
clean:
	rm -rf $(TMPDIR) *.o
debug: CXXFLAGS += -g -Wall
debug: all

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $(NAME)

$(TMPDIR)/%.o: %.cpp Makefile | $(TMPDIR)
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $@

-include $(DEPENDS)

$(TMPDIR):
	mkdir "$(TMPDIR)" "$(TMPDIR)/src"