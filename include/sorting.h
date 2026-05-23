#ifndef SORTING
#define SORTING

#include <vector>
#include <functional>
#include <string>

namespace sorting
{

    void measureSortingAlgorithms(int vector_size, int execution_count, int timeout_sec = 5);

    void measureSortingAlgorithms(int min_vector_size, int max_vector_size, std::function<int(int)> size_multiplier, std::string folder_name = "", int execution_count = 3, int timeout_sec = 5);

}

#endif