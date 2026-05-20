#ifndef MIN_SEARCH
#define MIN_SEARCH

#include <vector>
#include <functional>
#include <string>

namespace minSearch
{

    using namespace std;

    void measureMinSearchAlgorithms(int min_vector_size, int max_vector_size, function<int(int)> size_multiplier, string folder_name = "", int execution_count = 2, int timeout_sec = 5);

} // namespace minSearch

#endif