CXX := g++
TARGET := spotlight
TMPDIR := .tmp

CXXFLAGS := -O3 -g
NAME := spotlight

SOURCES := $(wildcard src/*.cpp)
OBJECTS := $(patsubst %.cpp,$(TMPDIR)/%.o,$(SOURCES))
DEPENDS := $(patsubst %.cpp,$(TMPDIR)/%.d,$(SOURCES))

all: $(TARGET)
clean:
	rm -rf $(TMPDIR) *.o

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $(NAME)

$(TMPDIR)/%.o: %.cpp Makefile | $(TMPDIR)
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $@

-include $(DEPENDS)

$(TMPDIR):
	mkdir "$(TMPDIR)" "$(TMPDIR)/src"