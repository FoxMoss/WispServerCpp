COMPILER = clang++
EXE = game
LIBS = -ljsoncpp
FlAGS =-Wall -Wno-sign-compare -lstdc++ -std=c++17 
DEBUG = -g -ggdb
SOURCES = $(wildcard ./*.cpp) 
OBJ_DIR = obj
OBJECTS = $(patsubst ./%.cpp,$(OBJ_DIR)/%.o,$(SOURCES))


all: $(EXE)	

$(EXE): $(OBJECTS) 
	$(COMPILER) -o $(EXE) $(DEBUG) $(OBJECTS) $(FlAGS) $(LIBS) 
$(OBJ_DIR)/%.o: ./%.cpp $(wildcard ./*.hpp) Makefile
	$(COMPILER) -c -o $@ $< $(FlAGS) $(DEBUG)
