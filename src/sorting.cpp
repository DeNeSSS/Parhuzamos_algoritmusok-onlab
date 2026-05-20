#include "sorting.h"

#include <climits>
#include <cmath>
#include <iostream>
#include <vector>
#include <map>
#include <functional>
#include <iomanip>
#include <future>
#include <algorithm>

namespace sorting
{
    using namespace std;

    const int PARALLEL_THRESHOLD = 100000;

    // ----- General functions -----

    void compareExchange(vector<int> &values, size_t idx, size_t jdx)
    {
        if (values[idx] > values[jdx])
        {
            int temp = values[idx];
            values[idx] = values[jdx];
            values[jdx] = temp;
        }
    }

    // ----- Odd-even bubble sort -----
    namespace oddEvenBubbleSort
    {
        // ----- Serial -----
        void oddEvenBubbleSort(vector<int> &values)
        {
            size_t size = values.size();
            for (size_t i = 0; i < size; i++)
            {
                if (i % 2 == 0)
                {
                    for (size_t j = 0; j < size - 1; j += 2)
                    {
                        compareExchange(values, j, j + 1);
                    }
                }
                else
                {
                    for (size_t j = 1; j < size - 1; j += 2)
                    {
                        compareExchange(values, j, j + 1);
                    }
                }
            }
        }

        // ----- Paralell -----
        void paralellOddEvenBubbleSort(vector<int> &values)
        {
            size_t size = values.size();
#pragma omp parallel
            for (size_t i = 0; i < size; i++)
            {
                if (i % 2 == 0)
                {
#pragma omp for
                    for (size_t j = 0; j < size - 1; j += 2)
                    {
                        compareExchange(values, j, j + 1);
                    }
                }
                else
                {
#pragma omp for
                    for (size_t j = 1; j < size - 1; j += 2)
                    {
                        compareExchange(values, j, j + 1);
                    }
                }
            }
        }

    }

    // ----- Odd-even merge sort -----
    namespace oddEvenMergeSort
    {

        bool isPowerOfTwo(size_t n) { return (n > 0) && ((n & (n - 1)) == 0); }

        size_t nextPowerOfTwo(size_t n)
        {
            if (isPowerOfTwo(n))
                return n;
            size_t power = 1;
            while (power < n)
                power <<= 1;
            return power;
        }

        // ---- Without references -----
        vector<int> slowOddEvenMerge(vector<int> A, vector<int> B)
        {
            int n = A.size() + B.size();
            if (n == 2)
            {
                vector<int> res = {A[0], B[0]};
                compareExchange(res, 0, 1);
                return res;
            }

            // Unshuffle: különválasztjuk az odd és even indexeket
            vector<int> A_even, A_odd, B_even, B_odd;
            for (int i = 0; i < A.size(); i++)
            {
                if (i % 2 == 0)
                    A_even.push_back(A[i]);
                else
                    A_odd.push_back(A[i]);
            }
            for (int i = 0; i < B.size(); i++)
            {
                if (i % 2 == 0)
                    B_even.push_back(B[i]);
                else
                    B_odd.push_back(B[i]);
            }

            // Rekurzív merge az ágakon
            vector<int> even_merge = slowOddEvenMerge(A_even, B_even);
            vector<int> odd_merge = slowOddEvenMerge(A_odd, B_odd);

            // Interleave: összefésülés
            vector<int> res(n);
            for (int i = 0; i < n / 2; i++)
            {
                res[2 * i] = even_merge[i];
                res[2 * i + 1] = odd_merge[i];
            }

            // Záró Compare-Exchange fázis
            for (int i = 1; i < n - 1; i += 2)
            {
                compareExchange(res, i, i + 1);
            }

            return res;
        }

        vector<int> slowOddEvenMergeSortRecursive(vector<int> values)
        {
            int n = values.size();
            if (n <= 1)
                return values;

            int half = n / 2;
            vector<int> left(values.begin(), values.begin() + half);
            vector<int> right(values.begin() + half, values.end());

            left = slowOddEvenMergeSortRecursive(left);
            right = slowOddEvenMergeSortRecursive(right);

            return slowOddEvenMerge(left, right);
        }

        vector<int> slowOddEvenMergeSort(vector<int> values)
        {
            if (values.empty())
                return values;

            size_t originalSize = values.size();

            // Ha nem kettő-hatvány, kiegészítjük
            if (!isPowerOfTwo(originalSize))
            {
                size_t paddedSize = nextPowerOfTwo(originalSize);
                // INT_MAX-szal töltjük fel, hogy a végére rendeződjenek
                values.resize(paddedSize, INT_MAX);
            }

            // Elvégezzük a rendezést a kiegészített tömbön
            vector<int> result = slowOddEvenMergeSortRecursive(values);

            // A végén levágjuk a kiegészítést
            result.resize(originalSize);
            return result;
        }

        // ----- With references -----
        void oddEvenMergeRecursive(vector<int> &v, int start, int n, int step)
        {
            int m = step * 2;
            if (m < n)
            {
                // Rekurzív hívások a páros és páratlan ágakra
                oddEvenMergeRecursive(v, start, n, m);        // Even
                oddEvenMergeRecursive(v, start + step, n, m); // Odd

                // Összehasonlítás és csere (Compare-Exchange fázis)
                for (int i = start + step; i + step < start + n; i += m)
                {
                    compareExchange(v, i, i + step);
                }
            }
            else
            {
                compareExchange(v, start, start + step);
            }
        }

        void oddEvenMergeSortRecursive(vector<int> &v, int start, int n)
        {
            if (n > 1)
            {
                int m = n / 2;
                // Oszd meg: két fél rendezése helyben
                oddEvenMergeSortRecursive(v, start, m);
                oddEvenMergeSortRecursive(v, start + m, m);

                // Uralkodj: összefésülés (szintén helyben, indexekkel)
                oddEvenMergeRecursive(v, start, n, 1);
            }
        }

        void oddEvenMergeSort(vector<int> &values)
        {
            if (values.empty())
            {
                return;
            }

            size_t originalSize = values.size();
            if (!isPowerOfTwo(originalSize))
            {
                size_t paddedSize = nextPowerOfTwo(originalSize);
                values.resize(paddedSize, INT_MAX);
            }

            // Indítjuk a helyben rendezést a teljes tartományra
            oddEvenMergeSortRecursive(values, 0, values.size());

            if (values.size() > originalSize)
            {
                values.resize(originalSize);
            }
        }

        // ----- Paralell -----

        void parallelOddEvenMergeSortRecursive(vector<int> &v, int start, int n)
        {
            if (n > 1)
            {
                int m = n / 2;

                if (n > PARALLEL_THRESHOLD)
                {
#pragma omp task shared(v) firstprivate(start, m)
                    parallelOddEvenMergeSortRecursive(v, start, m);
#pragma omp task shared(v) firstprivate(start, m)
                    parallelOddEvenMergeSortRecursive(v, start + m, m);
#pragma omp taskwait
                }
                else
                {
                    oddEvenMergeSortRecursive(v, start, m);
                    oddEvenMergeSortRecursive(v, start + m, m);
                }

                // Uralkodj: összefésülés (szintén helyben, indexekkel)
                oddEvenMergeRecursive(v, start, n, 1);
            }
        }

        void parallelOddEvenMergeSort(vector<int> &values)
        {
            if (values.empty())
            {
                return;
            }

            size_t originalSize = values.size();
            if (!isPowerOfTwo(originalSize))
            {
                size_t paddedSize = nextPowerOfTwo(originalSize);
                values.resize(paddedSize, INT_MAX);
            }

#pragma omp parallel
            {
#pragma omp single
                {
                    parallelOddEvenMergeSortRecursive(values, 0, values.size());
                }
            }

            if (values.size() > originalSize)
            {
                values.resize(originalSize);
            }
        }
    }

    namespace quickSort
    {
        // ----- Serial -----
        // partition function
        int partition(vector<int> &arr, int low, int high)
        {

            // choose the pivot
            int pivot = arr[high];

            // undex of smaller element and indicates
            // the right position of pivot found so far
            int i = low - 1;

            // Traverse arr[low..high] and move all smaller
            // elements on left side. Elements from low to
            // i are smaller after every iteration
            for (int j = low; j <= high - 1; j++)
            {
                if (arr[j] < pivot)
                {
                    i++;
                    swap(arr[i], arr[j]);
                }
            }

            // move pivot after smaller elements and
            // return its position
            swap(arr[i + 1], arr[high]);
            return i + 1;
        }

        // the QuickSort function implementation
        void quickSort(vector<int> &arr, int low, int high)
        {

            if (low < high)
            {

                // pi is the partition return index of pivot
                int pi = partition(arr, low, high);

                // recursion calls for smaller elements
                // and greater or equals elements
                quickSort(arr, low, pi - 1);
                quickSort(arr, pi + 1, high);
            }
        }

        void quickSort(vector<int> &arr)
        {
            quickSort(arr, 0, arr.size());
        }

        void parallelQuickSort(vector<int> &arr, int low, int high)
        {

            if (low < high)
            {
                if (high - low > PARALLEL_THRESHOLD)
                {
                    int pi = partition(arr, low, high);
#pragma omp task shared(arr) firstprivate(low, pi)
                    parallelQuickSort(arr, low, pi - 1);
#pragma omp task shared(arr) firstprivate(pi, high)
                    parallelQuickSort(arr, pi + 1, high);
#pragma omp taskwait
                }
                else
                {
                    int pi = partition(arr, low, high);
                    quickSort(arr, low, pi - 1);
                    quickSort(arr, pi + 1, high);
                }
            }
        }

        void parallelQuickSort(vector<int> &arr)
        {
#pragma omp parallel
            {
#pragma omp single
                {
                    parallelQuickSort(arr, 0, arr.size() - 1);
                }
            }
        }
    }

    void test()
    {
        const int TEST_SIZE = 100;
        srand(time(nullptr));
        vector<int> values(TEST_SIZE);
        generate(values.begin(), values.end(), []()
                 { return rand() % TEST_SIZE; });
        // oddEvenBubbleSort::paralellOddEvenBubbleSort(values);
        quickSort::parallelQuickSort(values);
        for (int val : values)
        {
            cout << val << ' ';
        }
        cout << '\n';
    }

    void testMinSearchAlgorithms(int vector_size, int execution_count, int timeout_sec)
    {
        map<string, function<void(vector<int> &)>> functions = {
            // {"Serial odd-even bubble", oddEvenBubbleSort::oddEvenBubbleSort},
            // {"Parallel odd-even bubble", oddEvenBubbleSort::paralellOddEvenBubbleSort},
            {"Serial quickSort", [](vector<int> &v)
             { quickSort::quickSort(v); }},
            {"Parallel quickSort", [](vector<int> &v)
             { quickSort::parallelQuickSort(v); }},
            {"Serial odd-even merge", oddEvenMergeSort::oddEvenMergeSort},
            {"Parallel odd-even merge", oddEvenMergeSort::parallelOddEvenMergeSort}};

        srand(time(nullptr));
        vector<int> startValues(vector_size);
        generate(startValues.begin(), startValues.end(), [vector_size]()
                 { return rand() % vector_size; });

        cout << "\n"
             << setfill('=') << setw(100) << "" << endl;
        cout << " SORTING BENCHMARK | Size: " << vector_size << " | Iterations: " << execution_count << endl;
        cout << setfill('=') << setw(100) << "" << setfill(' ') << endl;
        cout << left << setw(30) << "Algorithm"
             << setw(15) << "Avg Time (s)"
             << setw(15) << "Fastest (s)"
             << setw(15) << "Slowest (s)"
             << "Status" << endl;
        cout << string(100, '-') << endl;

        for (auto const &[name, func] : functions)
        {
            double total_time = 0, fastest = 1e9, slowest = 0;
            bool failed = false, timed_out = false;
            vector<int> failed_sort;

            for (int i = 0; i < execution_count; ++i)
            {
                auto values = startValues;
                auto start = chrono::high_resolution_clock::now();

                future<void> fut = async(launch::async, func, ref(values));

                if (fut.wait_for(chrono::seconds(timeout_sec)) == future_status::timeout)
                {
                    timed_out = true;
                    break;
                }

                fut.get();

                auto end = chrono::high_resolution_clock::now();
                double duration = chrono::duration<double>(end - start).count();

                if (!std::is_sorted(values.begin(), values.end()))
                {
                    failed = true;
                    failed_sort = values;
                    break;
                }

                total_time += duration;
                fastest = min(fastest, duration);
                slowest = max(slowest, duration);
            }

            cout << left << setw(30) << name;
            if (timed_out)
            {
                cout << "\033[33mTIMEOUT (> " << timeout_sec << "s)\033[0m" << endl;
            }
            else if (failed)
            {
                cout << "\033[31mFAILED (Not Sorted)\033[0m" << endl;
                cout << "\n\n";
                for (size_t i = 0; i < failed_sort.size() - 1; ++i)
                {
                    if (failed_sort[i] > failed_sort[i + 1])
                    {
                        cout << "Error at index " << i << ": " << failed_sort[i] << " > " << failed_sort[i + 1] << endl;
                        break; // Csak az első hibát írjuk ki
                    }
                }
                break;
            }
            else
            {
                cout << fixed << setprecision(6)
                     << setw(15) << (total_time / execution_count)
                     << setw(15) << fastest
                     << setw(15) << slowest
                     << "\033[32mSUCCESS\033[0m" << endl;
            }
        }
        cout << setfill('=') << setw(100) << "" << setfill(' ') << endl;
    }

} // namespace sorting