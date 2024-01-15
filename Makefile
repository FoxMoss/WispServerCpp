COMPILER = g++
EXE = anonfox
LIBS = /usr/lib/x86_64-linux-gnu/libjsoncpp.so
FlAGS =-Wall -Wno-sign-compare -lstdc++ -std=c++17 
# DEBUG = -g -ggdb
SOURCES = $(wildcard ./*.cpp) 
OBJ_DIR = obj
OBJECTS = $(patsubst ./%.cpp,$(OBJ_DIR)/%.o,$(SOURCES))
LIBRARY_PATH = /usr/local/lib/


all: $(EXE)	

$(EXE): $(OBJECTS) 
	$(COMPILER) -o $(EXE) $(DEBUG) $(OBJECTS) $(FlAGS) $(LIBS)
$(OBJ_DIR)/%.o: ./%.cpp $(wildcard ./*.hpp)
	$(COMPILER) -c -o $@ $< $(FlAGS) $(DEBUG)

dev: debug $(EXE)

debug:
	set DEBUG = -g -ggdb

