#include <cstdlib>
#include <iostream>
#include <math.h>

#include "OpenMP_examples_FOR.h"
#include "minSearch.h"
#include "sorting.h"
#include "tester.h"
#include "threadPool.h"
#include "minSpanningTree.h"

using namespace std;

int main()
{
    // minSearch::testMinSearchAlgorithms(1000000, 10);
    // sorting::testMinSearchAlgorithms(1000000, 1, 20);
    // sorting::test();
    // minSpanningTree::test();
    // minSpanningTree::testMSTAlgorithms(3, 10);

    auto linear = [](int current)
    { return current + 100000; };
    minSearch::measureMinSearchAlgorithms(100000, 10000000, linear, "change", 5, 5);
    auto exponental = [](int current)
    { return current * cbrt(10); };
    // minSearch::measureMinSearchAlgorithms(100000, 100000000, exponental, "full", 5, 5);
    // minSearch::measureMinSearchAlgorithms(1000000, 10000000, exponental, 3, 5);
}