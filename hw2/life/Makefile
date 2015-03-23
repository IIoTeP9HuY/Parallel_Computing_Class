MPICXX = mpic++
CC = gcc
MPICXXFLAGS= --std=c++0x -Wall
CCFLAGS= -Wall
SOURCES = life_MPI.cpp data-gen.c
OBJECTS = $(SOURCES:.cpp = .o)
EXECUTABLES = life_MPI data-gen
FIELD_SIZE = 1000

build: $(SOURCES) $(EXECUTABLES)

life_MPI:
	$(MPICXX) $(MPICXXFLAGS) life_MPI.cpp -o life_MPI

data-gen:
	$(CC) $(CCFLAGS) data-gen.c -o data-gen

run: build
	./data-gen 5 $(POINTS_NUMBER) 50 data.txt

report: build
	./generate_report.sh

clean:
	rm -rf *.o $(EXECUTABLES)
