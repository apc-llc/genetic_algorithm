CPUCC=g++
CPUCFLAGS=
CPUSOURCES=cpu_version.cpp
CPUEXECUTABLE=cpu

GPUCC=nvcc
GPUCFLAGS=-lcurand
GPUSOURCES=gpu_version.cu
GPUEXECUTABLE=gpu

all:
	g++ $(CPUSOURCES) -g -o $(CPUEXECUTABLE)
	nvcc $(GPUSOURCES) -arch=sm_35 -g -o $(GPUEXECUTABLE) -lcurand
	gcc -std=c99 generator.c -o generator

mpi: mpi_version.cpp mpi_version.cu mpi_version.h
	nvcc -c mpi_version.cu -g -o mpi_gpu.o
	mpic++ -c mpi_version.cpp -g -o mpi_cpu.o
	mpic++ -L/opt/cuda/lib64 -stdlib=libstdc++ mpi_gpu.o mpi_cpu.o -o $@ -lcurand -lcudart

run: 
	./generator 100    #generate input file
	./$(CPUEXECUTABLE) input.txt
	./$(GPUEXECUTABLE) input.txt
	gnuplot plot.gnu   #need to set found polynomial parameters manually
    mpirun -np 3 ./mpi input5.txt

test:
	cuda-memcheck --leak-check full --report-api-errors yes ./gpu input.txt

clean:
	rm -rf $(GPUEXECUTABLE)
	rm -rf $(CPUEXECUTABLE)