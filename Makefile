EXEC = lab
SRC = $(wildcard *.cpp)
OBJ = $(patsubst %.cpp,%.o,$(SRC))

CPP = g++
CPPFLAGS = #-Wall -Wextra -pedantic
LIBS = -lsimlib -lm

.PHONY: all clean run pack

.DEFAULT_GOAL := all

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CPP) $(CPPFLAGS) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CPP) $(CPPFLAGS) -c $< -o $@

clean:
	rm -f *.o $(EXEC)

run: all
	./$(EXEC)

pack: clean
	zip -rv	04_xeffen00_xvalik05.zip Makefile *.cpp *.h doc.pdf -x ".DS_Store"
