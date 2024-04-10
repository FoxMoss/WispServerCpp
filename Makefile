COMPILER = g++
EXE = wispserver
LIBS = -lpthread -lusockets -lz
HASH = $(shell git rev-parse HEAD)
FlAGS =-Wall -Wno-comments -lstdc++ -std=c++17 -DHASH="\"$(HASH)\""
SOURCES = $(wildcard ./*.cpp) 
OBJ_DIR = obj
OBJECTS = $(patsubst ./%.cpp,$(OBJ_DIR)/%.o,$(SOURCES))
LIBRARY_PATH = /usr/local/lib/

all: $(EXE)

$(EXE): $(OBJECTS) 
	$(COMPILER) -o $(EXE) $(DEBUG) $(OBJECTS) $(FlAGS) $(LIBS)
$(OBJ_DIR)/%.o: ./%.cpp
	$(COMPILER) -c -o $@ $< $(FlAGS) $(DEBUG)
