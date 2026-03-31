#include <cstdlib>
#include <iostream>

#include "OpenMP_examples_FOR.h"
#include "minSearch.h"
#include "sorting.h"
#include "tester.h"
#include "threadPool.h"

using namespace std;

int main()
{
    // minSearch::testMinSearchAlgorithms(1000000, 10);
    sorting::testMinSearchAlgorithms(10000000, 1, 20);
    // sorting::test();

    return 0;
}