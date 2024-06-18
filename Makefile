CXX := g++
TARGET := spotlight
TMPDIR := .tmp

CXXFLAGS := -O3
NAME := spotlight

SOURCES := $(wildcard src/*.cpp)
OBJECTS := $(patsubst %.cpp,$(TMPDIR)/%.o,$(SOURCES))

all: $(TARGET)
clean:
	rm -rf $(TMPDIR) *.o

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $(NAME)

$(TMPDIR)/%.o: %.cpp | $(TMPDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TMPDIR):
	mkdir "$(TMPDIR)" "$(TMPDIR)/src"