COMPILER = g++
EXE = out
LIBS = -lpthread
FlAGS =-Wall -Wno-sign-compare -lstdc++ -std=c++17
SOURCES = $(wildcard ./*.cpp) 
OBJ_DIR = obj
OBJECTS = $(patsubst ./%.cpp,$(OBJ_DIR)/%.o,$(SOURCES))
LIBRARY_PATH = /usr/local/lib/

DEBUG = -g -ggdb


all: $(EXE)	

$(EXE): $(OBJECTS) 
	$(COMPILER) -o $(EXE) $(DEBUG) $(OBJECTS) $(FlAGS) $(LIBS)
$(OBJ_DIR)/%.o: ./%.cpp
	$(COMPILER) -c -o $@ $< $(FlAGS) $(DEBUG)

dev: debug

debug: $(EXE)
	set DEBUG = -g -ggdb

