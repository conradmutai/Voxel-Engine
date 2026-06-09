# Variables for compiler and flags
CXX = g++
CC = gcc
CXXFLAGS = -std=c++11 -I./include -I. 

# Standard Apple Silicon framework links
LDFLAGS = -L./lib -lglfw3 -framework Cocoa -framework OpenGL -framework IOKit -framework CoreFoundation

# Target executable name
TARGET = voxel_game

# Updated object list to include the new classes
OBJ = main.o glad.o stb_image.o texture.o window.o chunk.o blockmanager.o chunkmanager.o raycaster.o

# Default rule
all: $(TARGET)

# Link all object files into the final app
$(TARGET): $(OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)
	cp *.vs *.fs *.png *.jpg . 2>/dev/null || true

# Compile C++ source files from the src directory
main.o: src/main.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

texture.o: src/texture.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

window.o: src/window.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

chunk.o: src/chunk.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

chunkmanager.o: src/chunkmanager.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

blockmanager.o: src/blockmanager.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

raycaster.o: src/raycaster.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile C source files
stb_image.o: src/stb_image.c
	$(CC) -I./include -c $< -o $@

# If glad.c is in your root directory rather than src/, change this path to glad.c
glad.o: src/glad.c
	$(CC) -I./include -c $< -o $@

# Clean rule
clean:
	rm -f $(OBJ) $(TARGET)