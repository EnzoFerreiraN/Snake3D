# =============================================================================
#  Makefile — Snake 3D (SnakeJogo.cpp)
#  Suporta macOS (Darwin) e Linux (freeglut).
#
#  WINDOWS: use o comando documentado no topo de SnakeJogo.cpp:
#    g++ SnakeJogo.cpp -o ProjetoFinal.exe -std=c++0x -DGLUT_STATIC -static ^
#        -lglut32 -lglu32 -lopengl32 -lwinmm -lgdi32
#
#  macOS  : brew install freeglut  (opcional — usa o GLUT.framework nativo)
#           make
#  Linux  : sudo apt install freeglut3-dev   (ou equivalente)
#           make
# =============================================================================

TARGET  := ProjetoFinal
SRC     := SnakeJogo.cpp

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
  # macOS — usa os frameworks nativos; suprime avisos de depreciação do OpenGL
  CXX      ?= clang++
  CXXFLAGS := -std=c++11 -Wno-deprecated-declarations
  LIBS     := -framework OpenGL -framework GLUT
else
  # Linux — freeglut
  CXX      ?= g++
  CXXFLAGS := -std=c++11
  LIBS     := -lglut -lGLU -lGL
endif

# -----------------------------------------------------------------------------

$(TARGET): $(SRC) stb_image.h
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: run clean
