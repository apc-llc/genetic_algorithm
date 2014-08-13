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
    | Main body of the GA  |
    ------------------------
*/
int main(int argc, char **argv)
{
    if(argc != 2) {
        cerr << "Usage: $./gpu inputFile" << endl;    
        return -1;
    }

    // Initialize MPI state
    MPI_CHECK(MPI_Init(&argc, &argv));

    // Get our MPI node number and node count
    int commSize, commRank;
    MPI_CHECK(MPI_Comm_size(MPI_COMM_WORLD, &commSize));
    MPI_CHECK(MPI_Comm_rank(MPI_COMM_WORLD, &commRank));

    cout << "My rank: " << commRank << endl;

    //read input data
    //points are the data to approximate by a polynomial
    float *points = readData(argv[1], N_POINTS);
    if(points == NULL)
        return -1;

    //variables for results
    float *solution = new float[INDIVIDUAL_LEN];
    float bestFitness_o;
    double time_o;
    int genNumber_o;

    computeGA(points, solution, &bestFitness_o, &genNumber_o, &time_o);


    /**
        Results
    */
    

    cout << "------------------------------------------------------------" << endl;    
    cout << "Finished! Found Solution:" << endl;
    
    //solution is first individual of population with the best params of a polynomial 
    for(int i=0; i<INDIVIDUAL_LEN; i++){   
        cout << "\tc" << i << " = " << solution[i] << endl;
    }

    cout << "Best fitness: " << bestFitness_o << endl \
    << "Generations: " << genNumber_o << endl;

    cout << "Time for GPU calculation equals \033[35m" \
        << time_o << " seconds\033[0m" << endl;

    //delete [] points;
    //delete [] solution;


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

// Shut down MPI cleanly if something goes wrong
void my_abort(int err)
{
    cout << "Test FAILED\n";
    MPI_Abort(MPI_COMM_WORLD, err);
}
