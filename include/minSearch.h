#ifndef MIN_SEARCH
#define MIN_SEARCH

#include <vector>


namespace minSearch
{

using namespace std;

auto serial(const vector<int>& vals) -> int;

auto OMP_Compression(const vector<int>& values) -> int;

auto threadPool_Pairwise(const vector<int>& values) -> int;

int threadPool_Chunked(const vector<int>& values);

auto OMP_Reduction(const vector<int>& values) -> int;

auto recursiveAsyncMin(const vector<int>& values) -> int;

auto stdParallelMin(const vector<int>& values) -> int;

auto OMP_simdMin(const vector<int>& values) -> int;

void testMinSearchAlgorithms(int vector_size, int execution_count, int timeout_sec = 5);
} // namespace minSearch

#endif