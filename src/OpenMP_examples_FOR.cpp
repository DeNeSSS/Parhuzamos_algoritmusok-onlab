#include <chrono>
#include <iostream>
#include <omp.h>
#include <thread>
#include <atomic>

#include "OpenMP_examples_FOR.h"
#include <vector>

// ===== Example 1 =====

int sum_serial(int n)
{
    int sum = 0;
    for (int i = 0; i <= n; ++i) {
        sum += i;
    }
    return sum;
}

// Parallel programming function
int openMP_sum_parallel(int n)
{
    int sum = 0;
#pragma omp parallel for reduction(+ : sum)
    for (int i = 0; i <= n; ++i) {
        sum += i;
    }
    return sum;
}

void partial_sum(int start, int end, std::atomic<int>& global_sum) {
    long long local_sum = 0;
    for (int i = start; i <= end; ++i) {
        local_sum += i;
    }

    global_sum.fetch_add(local_sum);
}

int thread_sum_parallel(int n) {
    std::atomic<int> sum(0);
    std::vector<std::thread> threads;
    int num_threads = std::thread::hardware_concurrency();
    
    int grain_size = n / num_threads;

    for (int i = 0; i < num_threads; i++) {
        int start = i * grain_size + 1;
        int end = (i == num_threads - 1) ? n : (i + 1) * grain_size;

        threads.emplace_back(partial_sum, start, end, std::ref(sum));
    }

    for (auto& t : threads) {
        t.join();
    }

    return sum.load();
}

// Driver Function
int Example_1()
{
    const int n = 100000000;

    auto start_time = std::chrono::high_resolution_clock::now();
    int result_serial = sum_serial(n);
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> serial_duration = end_time - start_time;
    
    start_time = std::chrono::high_resolution_clock::now();
    int openMP_result_parallel = openMP_sum_parallel(n);
    end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> openMP_parallel_duration = end_time - start_time;

    start_time = std::chrono::high_resolution_clock::now();
    int thread_result_parallel = thread_sum_parallel(n);
    end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> thread_parallel_duration = end_time - start_time;

    std::cout << "Serial result: " << result_serial << std::endl;
    std::cout << "OpenMP parallel result: " << openMP_result_parallel << std::endl;
    std::cout << "Thread parallel result: " << thread_result_parallel << std::endl;

    std::cout << "Serial duration: "
              << serial_duration.count() << " seconds"
              << std::endl;
    std::cout << "OpenMP parallel duration: "
              << openMP_parallel_duration.count() << " seconds"
              << std::endl;
    std::cout << "Thread parallel duration: "
              << thread_parallel_duration.count() << " seconds"
              << std::endl;
              
    std::cout << "OpenMP speedup: "
              << serial_duration.count()
                     / openMP_parallel_duration.count()
              << std::endl;
    std::cout << "Thread speedup: "
              << serial_duration.count()
                     / thread_parallel_duration.count()
              << std::endl;

    return 0;
}

// ===== Example 2 =====


// Computes the value of pi using a serial computation.
double compute_pi_serial(long num_steps)
{
    double step = 1.0 / num_steps;
    double sum = 0.0;
    for (long i = 0; i < num_steps; i++) {
        double x = (i + 0.5) * step;
        sum += 4.0 / (1.0 + x * x);
    }
    return sum * step;
}

// Computes the value of pi using a parallel computation.
double compute_pi_parallel(long num_steps)
{
    double step = 1.0 / num_steps;
    double sum = 0.0;
    // parallelize loop and reduce sum variable
#pragma omp parallel for reduction(+ : sum)
    for (long i = 0; i < num_steps; i++) {
        double x = (i + 0.5) * step;
        sum += 4.0 / (1.0 + x * x);
    }
    return sum * step;
}

// Driver function
int Example_2()
{
    const long num_steps = 1000000000L;

    // Compute pi using serial computation and time it.
    auto start_time
        = std::chrono::high_resolution_clock::now();
    double pi_serial = compute_pi_serial(num_steps);
    auto end_time
        = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> serial_duration
        = end_time - start_time;

    // Compute pi using parallel computation and time it.
    start_time = std::chrono::high_resolution_clock::now();
    double pi_parallel = compute_pi_parallel(num_steps);
    end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> parallel_duration
        = end_time - start_time;

    std::cout << "Serial result: " << pi_serial
              << std::endl;
    std::cout << "Parallel result: " << pi_parallel
              << std::endl;
    std::cout << "Serial duration: "
              << serial_duration.count() << " seconds"
              << std::endl;
    std::cout << "Parallel duration: "
              << parallel_duration.count() << " seconds"
              << std::endl;
    std::cout << "Speedup: "
              << serial_duration.count()
                     / parallel_duration.count()
              << std::endl;

    return 0;
}


int run_FOR_examples()
{
    std::cout << "Example 1: Summation" << std::endl;
    Example_1();

    // std::cout << "\nExample 2: Pi Calculation" << std::endl;
    // Example_2();

    return 0;
}
