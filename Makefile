COMPILER = x86_64-linux-gnu-g++
EXE = anonfox
LIBS = -ljsoncpp
FlAGS =-Wall -Wno-sign-compare -lstdc++ -std=c++17 -O3
# DEBUG = -g -ggdb
SOURCES = $(wildcard ./*.cpp) 
OBJ_DIR = obj
OBJECTS = $(patsubst ./%.cpp,$(OBJ_DIR)/%.o,$(SOURCES))
LIBRARY_PATH = /usr/local/lib/


all: $(EXE)	

$(EXE): $(OBJECTS) 
	$(COMPILER) -o $(EXE) $(DEBUG) $(OBJECTS) $(FlAGS) $(LIBS) -Wl,-rpath=$(LIBRARY_PATH)
$(OBJ_DIR)/%.o: ./%.cpp $(wildcard ./*.hpp) Makefile
	$(COMPILER) -c -o $@ $< $(FlAGS) $(DEBUG)
