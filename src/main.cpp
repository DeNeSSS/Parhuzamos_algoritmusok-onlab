#include <cstdlib>
#include <iostream>
#include <math.h>

#include "minSearch.h"
#include "sorting.h"
#include "threadPool.h"
#include "minSpanningTree.h"

using namespace std;

int main()
{
    auto linear = [](int current)
    { return current + 100000; };
    auto exponental = [](int current)
    { return current * cbrt(10); };
    auto powerOfTwo = [](int current)
    { return current * 2; };

    minSearch::measureMinSearchAlgorithms(100000, 10000000, linear, "change2", 3, 5);
    // minSearch::measureMinSearchAlgorithms(100000, 100000000, exponental, "full", 5, 5);
    // minSearch::measureMinSearchAlgorithms(100000, 100000, exponental, "test", 3, 5);

    // sorting::measureSortingAlgorithms(100000, 100000000, exponental, "mergeSort");
    // sorting::measureSortingAlgorithms(pow(2, 17), pow(2, 24), powerOfTwo, "powerOfTwo");

    // sorting::measureSortingAlgorithms(1000000, 3, 5);

    // minSpanningTree::measureAllMSTAlgorithms("sparse");
    // minSpanningTree::measureAllMSTAlgorithms("dense");
}