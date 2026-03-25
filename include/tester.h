#ifndef TESTER
#define TESTER

#include <chrono>
#include <iostream>

/**
 * @brief Measures the execution time of a function that returns void.
 * 
 * This function wraps the execution of a provided function and measures the time
 * it takes to complete using std::chrono::high_resolution_clock. The function must
 * have a return type of void. The execution time is calculated as the difference
 * between the end and start time and returned as seconds.
 *
 * @tparam Func The type of the function to be executed.
 * @tparam Args The types of the arguments to be passed to the function.
 *
 * @param func The function object to measure execution time for.
 * @param args The arguments to pass to the function.
 *
 * @return A pair containing:
 *         - first: The return value of the executed function
 *         - second: The execution duration in seconds as a double
 *
 *
 * @example
 * void myFunction(int x, int y) {  }
 * double elapsed = measure_time_void(myFunction, 5, 10);
 * std::cout \<< "Time taken: " \<< elapsed \<< " seconds" \<< std::endl;
 */
template<typename Func, typename... Args>
auto measure_time(Func func, Args... args)
{
    auto start = std::chrono::high_resolution_clock::now();
    auto result = func(args...);
    auto end = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double> duration = end - start;
    // std::cout << "Execution time: " << duration.count() << " seconds" << std::endl;
    return std::make_pair(result, duration.count());
}

/**
 * @brief Measures the execution time of a function that returns void.
 * 
 * This function wraps the execution of a provided function and measures the time
 * it takes to complete using std::chrono::high_resolution_clock. The function must
 * have a return type of void. The execution time is calculated as the difference
 * between the end and start time and returned as seconds.
 *
 * @tparam Func The type of the function to be executed.
 * @tparam Args The types of the arguments to be passed to the function.
 *
 * @param func The function object to measure execution time for.
 * @param args The arguments to pass to the function.
 *
 * @return double The execution time in seconds as a double precision floating-point number.
 *
 *
 * @example
 * void myFunction(int x, int y) {  }
 * double elapsed = measure_time_void(myFunction, 5, 10);
 * std::cout \<< "Time taken: " \<< elapsed \<< " seconds" \<< std::endl;
 */
template<typename Func, typename... Args>
auto measure_time_void(Func func, Args... args)
{
    auto start = std::chrono::high_resolution_clock::now();
    func(args...);
    auto end = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double> duration = end - start;
    // std::cout << "Execution time: " << duration.count() << " seconds" << std::endl;
    return duration.count();
}

#endif