/**

Genetic algorithm for finding function aproximation. GPU accelerated version

Given data points {x, f(x)+noise} generated by noisy polynomial function
f(x) = c3*x^3 + c2*x^2 + c1*x + c0,
find unknown parameters c1, c2, c3 and c0.


Inputs:
• The set of points on a surface (500–1000);
• The size of population P (1000–2000);
• E_m , D_m – mean and variance for Mutation to generate the random number of mutated genes;
• maxIter - the maximum number of generations, 
  maxConstIter - the maximum number of generations with constant value of the best fitness.

Outputs:
• The time of processing on GPU;
• The set of coefficients of the polynomial that approximates the given set of points;
• The best fitness value;
• The last generation number (number of evaluated iterations).
*/

#include <mpi.h>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <time.h>
#include <algorithm>

#include "mpi_version.h"

using namespace std;

// Error handling macros
#define MPI_CHECK(call) \
    if((call) != MPI_SUCCESS) { \
        cerr << "MPI error calling \""#call"\"\n"; \
        my_abort(-1); }

/*
    ------------------------
    |  MPI communication   |
    ------------------------
*/
int main(int argc, char **argv)
{
    if(argc != 2) {
        cerr << "Usage: $mpirun -np N ./gpu inputFile" << endl;    
        return -1;
    }

    // Initialize MPI state
    MPI_CHECK(MPI_Init(&argc, &argv));

    // Get our MPI node number and node count
    int commSize, commRank;
    MPI_CHECK(MPI_Comm_size(MPI_COMM_WORLD, &commSize));
    MPI_CHECK(MPI_Comm_rank(MPI_COMM_WORLD, &commRank));

    if(commSize > 4)
    {
        cerr << "Cannot run with more than 4 processes!" << endl;
        return -1;        
    }

    //read input data
    //points are the data to approximate by a polynomial
    float *points = readData(argv[1], N_POINTS);
    if(points == NULL)
        return -1;

    //variables for results
    float *solution_o = new float[INDIVIDUAL_LEN];
    float bestFitness_o;
    double time_o;
    int genNumber_o;

    //compute solution at each process on different GPUs
    computeGA(points, commRank, solution_o, &bestFitness_o, &genNumber_o, &time_o);


    /**
       Process results on master process.
       Other processes send its results to master that will find best solution.
    */
    int masterProcess = 0;
    
    //storage for results from all processes at master process
    float *solution_all;
    float *bestFitness_all;
    double *time_all;
    int *genNumber_all;
    if(commRank == masterProcess){
        solution_all = new float[INDIVIDUAL_LEN*commSize];
        bestFitness_all = new float[commSize];
        time_all = new double[commSize];
        genNumber_all = new int[commSize];
    }


        cout << "Rank: " << commRank << endl;
        cout << "Best fitness: " << bestFitness_o << endl \
        << "Generations: " << genNumber_o << endl;

        cout << "Time for GPU calculation equals \033[35m" \
            << time_o << " seconds\033[0m" << endl;


    if(commRank == masterProcess)
    {
        //recv solutions from all slave processes and copy your own results
        for(int i=1; i<commSize; i++)
        {
            //receive solution
            MPI_CHECK(MPI_Recv(&solution_all[i*INDIVIDUAL_LEN], INDIVIDUAL_LEN, MPI_FLOAT,
                      i, 11, MPI_COMM_WORLD, MPI_STATUS_IGNORE));

            //receive fitness, time and generations count
            MPI_CHECK(MPI_Recv(&bestFitness_all[i], 1, MPI_FLOAT,
                      i, 22, MPI_COMM_WORLD, MPI_STATUS_IGNORE));
            MPI_CHECK(MPI_Recv(&time_all[i], 1, MPI_DOUBLE,
                      i, 33, MPI_COMM_WORLD, MPI_STATUS_IGNORE));
            MPI_CHECK(MPI_Recv(&genNumber_all[i], 1, MPI_INT,
                      i, 44, MPI_COMM_WORLD, MPI_STATUS_IGNORE));
        }

        //copy your own results
        for(int i=0; i<INDIVIDUAL_LEN; i++)
            solution_all[masterProcess+i] = solution_o[i]; 
        bestFitness_all[masterProcess] = bestFitness_o;
        time_all[masterProcess] = time_o;
        genNumber_all[masterProcess] =  genNumber_o;
        
    }else //send results to master process with rank 0
    {
        //send solution
        MPI_CHECK(MPI_Send(solution_o, INDIVIDUAL_LEN, MPI_FLOAT, 0, 11, MPI_COMM_WORLD));
        
        //send fitness, time and generations count
        MPI_CHECK(MPI_Send(&bestFitness_o, 1, MPI_FLOAT, 0, 22, MPI_COMM_WORLD));
        MPI_CHECK(MPI_Send(&time_o, 1, MPI_DOUBLE, 0, 33, MPI_COMM_WORLD));
        MPI_CHECK(MPI_Send(&genNumber_o, 1, MPI_INT, 0, 44, MPI_COMM_WORLD));
    }

    //find best solution amongst obtained results and print it
    if(commRank == 0) {
        //select best result
        int bestSolution = findMinimum(bestFitness_all, commSize); 

        cout << "------------------------------------------------------------" << endl;    
        cout << "Finished! Found Solution at process " << bestSolution << ": " << endl;       

        //solution with the best params of a polynomial 
        for(int i=0; i<INDIVIDUAL_LEN; i++){   
            cout << "\tc" << i << " = " << \
            solution_all[bestSolution*INDIVIDUAL_LEN + i] << endl;
        }

        cout << "Best fitness: " << bestFitness_all[bestSolution] << endl \
        << "Generations: " << genNumber_all[bestSolution] << endl;

        cout << "Time for GPU calculation equals \033[35m" \
            << time_all[bestSolution] << " seconds\033[0m" << endl;

    }

    delete [] points;
    delete [] solution_o;

    if(commRank == masterProcess){
        delete [] solution_all;
        delete [] bestFitness_all;
        delete [] time_all;
        delete [] genNumber_all;
    }


    MPI_CHECK(MPI_Finalize());
}

//------------------------------------------------------------------------------

float *readData(const char *name, const int POINTS_CNT)
{
    FILE *file = fopen(name,"r");
 
	float *points = new float[2*POINTS_CNT]; 
    if (file != NULL){

        int k=0;
        //x, f(x)
        while(fscanf(file,"%f %f",&points[k],&points[POINTS_CNT+k])!= EOF){
            k++;
        }
        fclose(file);
        cout << "Reading file - success!" << endl;
    }else{
        cerr << "Error while opening the file " << name << "!!!" << endl;
        delete [] points;
        return NULL;
    }

    return points;
}

// returns index of minimal value in the input array
int findMinimum(float *array, int arrayLen){

    float min = array[0];
    int minIdx = 0;

    for(int i=1; i<arrayLen; i++)
    {
        if(array[i] < min){
            min = array[i];         
            minIdx = i;
        }
    }

    return minIdx;
}

// Shut down MPI cleanly if something goes wrong
void my_abort(int err)
{
    cout << "Test FAILED\n";
    MPI_Abort(MPI_COMM_WORLD, err);
}