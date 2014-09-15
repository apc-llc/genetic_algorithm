/**

Genetic algorithm for finding function aproximation.

Given data points {x, f(x)+noise} generated by noisy polynomial function
f(x) = c4*x^3 + c3*x^2 + c2*x + c1,
find unknown parameters c1, c2, c3 and c4.


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

#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <time.h>
#include <algorithm>

#include "config.h"

using namespace std;

// Reads input file with noisy points. Points will be approximated by 
// polynomial function using GA.
float *readData(const char *name, const int POINTS_CNT);

// Generages random no. with normal distribution
float nrand(float mu, float sigma);

// Box–Muller transform
float stdrand();

// Returns random number from interval <0.0, 1.0>
float frand();

/**
    An individual fitness function is the difference between measured f(x) and
    approximated polynomial gi(x), built using individual's coeficients,
    evaluated on input data N.

    Smaller value means bigger fitness
*/
float *fitness(float *individuals, float *points, float *current_fitnesses)
{

    //for every individual in population
	for(int i=0; i < POPULATION_SIZE; i++)
	{
        float sumError = 0;

        //for every given data point
		for(int pt=0; pt<N_POINTS; pt++)
		{
			float f_approx = 0.;
			
            //for every polynomial parameter: Ci * x^(order)
			for (int order=0; order < INDIVIDUAL_LEN; order++)
			{
				f_approx += individuals[i*INDIVIDUAL_LEN + order] * pow(points[pt], order);
			}

			sumError += pow(f_approx - points[N_POINTS+pt], 2);
		}
		
        //The lower value of fitness is, the better individual fits the model
		current_fitnesses[i] = sumError;
	}

	return current_fitnesses;
}


/**
    Individual is set of coeficients c1-c4. 

    For example:
    parent1 == [0 0 0 0]
    parent2 == [1 1 1 1]
    crosspoint(random between 1 and 3) = 2
      then
    child1  = [0 0 1 1]
    child2  = [1 1 0 0]
*/

void crossover(float *oldPopulation, float *newPopulation)
{
    
    //copy fittest first half of population
    for(int i = 0; i < POPULATION_SIZE/2*INDIVIDUAL_LEN; i++)
    {
        newPopulation[i] = oldPopulation[i];    
    }

    //create children from first half of the fittest population
	for(int i = POPULATION_SIZE/2*INDIVIDUAL_LEN;
            i < POPULATION_SIZE*INDIVIDUAL_LEN;
            i += 2*INDIVIDUAL_LEN) 
	{
        //randomly select two fit parrents for mating from the fittest half of the population
		int parent1_i = (random() % (POPULATION_SIZE/2)) * INDIVIDUAL_LEN;
		int parent2_i = (random() % (POPULATION_SIZE/2)) * INDIVIDUAL_LEN;


        //select crosspoint, do not select beginning and end of individual as crosspoint
		int crosspoint = random() % (INDIVIDUAL_LEN - 2) + 1;
        for(int j=0; j<INDIVIDUAL_LEN; j++){
            if(j<crosspoint)
            {
                newPopulation[i+j] = oldPopulation[parent1_i*INDIVIDUAL_LEN + j];
                newPopulation[i+j+INDIVIDUAL_LEN] = oldPopulation[parent2_i*INDIVIDUAL_LEN + j];  
            } else
            {
                newPopulation[i+j] = oldPopulation[parent2_i*INDIVIDUAL_LEN + j];
                newPopulation[i+j+INDIVIDUAL_LEN] = oldPopulation[parent1_i*INDIVIDUAL_LEN + j];
            }
        }

	}
}

/**
    Mutation is addition of noise to genes, given is mean and stddev of noise.

    For example(binary representation of genes):
    individual == [1 1 1 1]
    mutNumber = 2
    loop 2 times:
       1st: num_of_bit_to_mutate = 2
            inverse individuals[2]   ->   [1 1 0 1] 
       2nd: num_of_bit_to_mutate = 0
            inverse individuals[0]   ->   [0 1 0 1]
    return mutated individual         [0 1 0 1]
*/
float *mutation(float *individuals)
{
	//first individual is left without changes to keep the best individual  		
    for(int i=1; i<POPULATION_SIZE; i++)
    {
        //probability of mutating individual
        int mutNumber = nrand(mu_individuals, sigma_individuals);

        for(int j=0; j<INDIVIDUAL_LEN; j++)
        {
            int idx = i*INDIVIDUAL_LEN + j;
            //probability of mutating gene 
            if(nrand(mu_genes, sigma_genes) < mutNumber)
                individuals[idx] += 0.01*(2*frand()-1);        
        }
    }
	
	return individuals;
}


// pair<fitness,index> - take fitness as sorting key
bool myCmpPair(pair<float,int> p1, pair<float,int> p2)
{
    return p1.first < p2.first;
}

/*
    Sort individuals according to their fitness

	individuals with small (good) fitness value to the beginning 
	individuals with large (bad) fitness value to the end;
    return sorted population of individuals;
*/
float *selection(float *population, float *fitnesses, float *newPopulation)
{
    //array of fitness-indexes pairs for sorting algorithm, AoS
    pair<float,int> *pairs = new pair<float,int>[POPULATION_SIZE];

    for (int i=0; i<POPULATION_SIZE; i++) 
    {
        //pair(fitness, index)
        pairs[i] = make_pair(fitnesses[i],i);
    }        
    sort(pairs, pairs+POPULATION_SIZE, myCmpPair);

    //reorder population so that fittest individuals are first
    for (int i=0; i<POPULATION_SIZE; i++){
        for (int j=0; j<INDIVIDUAL_LEN; j++)
        {
            newPopulation[i*INDIVIDUAL_LEN + j]
                = population[pairs[i].second*INDIVIDUAL_LEN + j];
        }
    }
    
    delete [] pairs;

    return newPopulation;
}

/*
    Main body of the GA
*/
int main(int argc, char **argv)
{
    if(argc != 2){
        cerr << "Usage: $./cpu inputFile" << endl;
        return -1;
    }

    //read input data
    //points are the data to approximate by a polynomial
    float *points = readData(argv[1], N_POINTS);
    if(points == NULL)
        return -1;

    //arrays to hold old and new population    
    float *population = new float[POPULATION_SIZE * INDIVIDUAL_LEN * sizeof(float)];
    float *newPopulation = new float[POPULATION_SIZE * INDIVIDUAL_LEN * sizeof(float)];

    //arrays that keeps fitness of individuals withing current population
    float *current_fitnesses = new float[POPULATION_SIZE];

    //Initialize first population ( with zeros or some random values )
	for(int i=0; i<POPULATION_SIZE * INDIVIDUAL_LEN; i++){
        population[i] = ((float)rand()/RAND_MAX)*10 - 5; //<-5.0; 5.0>
    }


    /**
        Main GA loop
    */
    int t1 = clock(); //start timer

    int generationNumber = 0;
    int noChangeIter = 0;

    float bestFitness = INFINITY;
    float previousBestFitness = INFINITY;

	while ( (generationNumber < maxGenerationNumber)
            && (bestFitness > targetErr)
            && (noChangeIter < maxConstIter) )
	{
		generationNumber++;
	
        /** crossover first half of the population and create new population */
		crossover(population, newPopulation);
        float *tmp = population;//put new individuals into $population
        population = newPopulation;
        newPopulation = tmp;

		/** mutate population and childrens in the whole population*/
		population = mutation(population);
		
        /** evaluate fitness of individuals in population */
		fitness(population, points, current_fitnesses);
        bestFitness = current_fitnesses[0];

        //check if the fitness is decreasing or if we are stuck at local minima
        if(fabs(bestFitness - previousBestFitness) < 0.01)
            noChangeIter++;
        else
            noChangeIter = 0;
        previousBestFitness = bestFitness;

        /** select individuals for mating for next generation,
            i.e. sort population according to its fitness and keep
            fittest individuals first in population  */
        tmp = population; //put sorted individuals into $population
        population = selection(population, current_fitnesses, newPopulation);
        newPopulation = tmp;

        //log message
        #if defined(DEBUG)
        cout << "#" << generationNumber<< " Fitness: " << bestFitness << \
        " Iterations without change: " << noChangeIter << endl;
        #endif
	}

    int t2 = clock(); //stop timer

    cout << "------------------------------------------------------------" << endl;    
    cout << "Finished! Found Solution:" << endl;
    
    //solution is first individual of population with the best params of a polynomial    
    cout << "\tc0 = " << population[0] << endl \
    << "\tc1 = " << population[1] << endl \
    << "\tc2 = " << population[2] << endl \
    << "\tc3 = " << population[3] << endl \
    << "Best fitness: " << bestFitness << endl \
    << "Generations: " << generationNumber << endl;

    cout << "Time for CPU calculation equals \033[35m" \
        << (t2-t1)/(double)CLOCKS_PER_SEC << " seconds\033[0m" << endl;

    delete [] current_fitnesses;
    delete [] population;
    delete [] newPopulation;
    delete [] points;
}

//------------------------------------------------------------------------------
float frand() 
{
	return float(rand())/RAND_MAX;
}

float stdrand() //Box–Muller transform
{
    float U1, U2,V1, V2;
    float S=2;
    while (S>=1){
        U1 = frand();
        U2 = frand();
        V1 = -1.0 + 2.0*U1;
        V2 = -1.0 + 2.0*U2;
        S = pow(V1,2) + pow(V2,2);
    };
    float X1 = V1 * sqrt((-2.0*log(S))/S);
    return X1;
}
float nrand(float mu, float sigma)
{
    float X2 = mu + sigma * stdrand();
    return X2;
}

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
