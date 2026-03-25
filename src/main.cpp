#include <iostream>
#include <cstdlib>

#include "OpenMP_examples_FOR.h"
#include "threadPool.h"
#include "minSearch.h"
#include "tester.h"

using namespace std;


int main()
{

    minSearch::testMinSearchAlgorithms(1000000, 10);


    return 0;
}