#include <curand.h>
#include <curand_kernel.h>

#include <thrust/device_ptr.h>

// Forward declarations
extern "C" {
    // Reads input file with noisy points. Points will be approximated by 
    // polynomial function using GA.
    float *readData(const char *name, const int POINTS_CNT);

    // Gets last error and prints message when error is present
    void check_cuda_error(const char *message);

    // Finished MPI and aborts
    void my_abort(int err);

    void doInitPopulation(float *population_dev, curandState *state_random);

    void doCrossover(float *population_dev, curandState* state_random);

    void doMutation(float *population_dev, curandState *state_random,
                    float *mutIndivid_d, float *mutGene_d, int size);

    void doFitness_evaluate(float *population_dev, float *points_dev, float *fitness_dev,
                            int size);

    void doSelection(thrust::device_ptr<float>fitnesses_thrust,
                     thrust::device_ptr<int>indexes_thrust, int *indexes_dev,
                     float *population_dev, float* newPopulation_dev);
}

void check_cuda_error(const char *message);
