#include <chrono>
#include <iostream>
#include <omp.h>

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
int sum_parallel(int n)
{
    int sum = 0;
#pragma omp parallel for reduction(+ : sum)
    for (int i = 0; i <= n; ++i) {
        sum += i;
    }
    return sum;
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

    int result_parallel = sum_parallel(n);
    end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> parallel_duration
        = end_time - start_time;

    std::cout << "Serial result: " << result_serial
              << std::endl;
    std::cout << "Parallel result: " << result_parallel
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

    std::cout << "\nExample 2: Pi Calculation" << std::endl;
    Example_2();

    return 0;
}
