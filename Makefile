EXEC = ims
SRC = $(wildcard *.cpp)
OBJ = $(patsubst %.cpp,%.o,$(SRC))

CPP = g++
CPPFLAGS = -std=c++20 #-Wall -Wextra -pedantic -O2 -Werror
LIBS = 

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
	./ims

pack: clean
	zip -rv	04_xeffen00_xvalik05.zip Makefile *.cpp *.hpp doc.pdf
