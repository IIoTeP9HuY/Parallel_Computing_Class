CXX = g++
CXXFLAGS= --std=c++0x -Wall -fopenmp
SOURCES = data-gen.cpp kmeans.cpp
OBJECTS = $(SOURCES:.cpp = .o)
EXECUTABLES = data-gen kmeans
POINTS_NUMBER = 50000

build: $(SOURCES) $(EXECUTABLES)

kmeans:
	$(CXX) $(CXXFLAGS) kmeans.cpp -o kmeans

data-gen:
	$(CXX) $(CXXFLAGS) data-gen.cpp -o data-gen

run: build
	./data-gen 5 $(POINTS_NUMBER) 50 data.txt
	OMP_NUM_THREADS=24 && time -p ./kmeans 50 data.txt clusters.txt

report: build
	./generate_report.sh

clean:
	rm -rf *.o $(EXECUTABLES)
	rm data.txt report.txt report.pdf clusters.txt
