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
                return;

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
        const int THRESHOLD = 1000;

        void oddEvenMergeParallel(vector<int> &v, int start, int n, int step)
        {
            int m = step * 2;
            if (m < n)
            {
                // Csak akkor hozunk létre taskokat, ha elég nagy az adathalmaz
                if (n > THRESHOLD)
                {
#pragma omp task
                    oddEvenMergeParallel(v, start, n, m);

#pragma omp task
                    oddEvenMergeParallel(v, start + step, n, m);

#pragma omp taskwait // Megvárjuk mindkét ág végét a Compare-Exchange előtt
                }
                else
                {
                    oddEvenMergeParallel(v, start, n, m);
                    oddEvenMergeParallel(v, start + step, n, m);
                }

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

        void oddEvenMergeSortParallel(vector<int> &v, int start, int n)
        {
            if (n > 1)
            {
                int m = n / 2;

                if (n > THRESHOLD)
                {
#pragma omp task
                    oddEvenMergeSortParallel(v, start, m);

#pragma omp task
                    oddEvenMergeSortParallel(v, start + m, m);

#pragma omp taskwait // Barrier: a rendezésnek kész kell lennie az összefésülés előtt
                }
                else
                {
                    oddEvenMergeSortParallel(v, start, m);
                    oddEvenMergeSortParallel(v, start + m, m);
                }

                oddEvenMergeParallel(v, start, n, 1);
            }
        }

        void parallelOddEvenMergeSort(vector<int> &values)
        {
            // 1. Padding (ha szükséges) - a korábbi kódod alapján
            size_t originalSize = values.size();
            if (!isPowerOfTwo(originalSize))
            {
                values.resize(nextPowerOfTwo(originalSize), INT_MAX);
            }

// 2. Párhuzamos régió indítása
#pragma omp parallel
            {
// A rekurzív task-indítást egyetlen szálnak kell elkezdenie!
#pragma omp single
                {
                    oddEvenMergeSortParallel(values, 0, values.size());
                }
            }

            // 3. Visszavágás
            if (values.size() > originalSize)
            {
                values.resize(originalSize);
            }
        }
    }

    void test()
    {
        vector<int> values{3, 9, 5, 2, 1, 10, 4, 8, 6, 7};
        // auto res = oddEvenBubbleSort(values);
        oddEvenBubbleSort::paralellOddEvenBubbleSort(values);
        for (int val : values)
        {
            cout << val << ' ';
        }
        cout << '\n';
    }

    void testMinSearchAlgorithms(int vector_size, int execution_count, int timeout_sec)
    {
        map<string, function<void(vector<int> &)>> functions = {
            {"Serial odd-even bubble", oddEvenBubbleSort::oddEvenBubbleSort},
            {"Parallel odd-even bubble", oddEvenBubbleSort::paralellOddEvenBubbleSort},
            {"Serial odd-even merge", oddEvenMergeSort::oddEvenMergeSort},
            {"Parallel odd-even merge", oddEvenMergeSort::parallelOddEvenMergeSort}};

        srand(time(nullptr));
        vector<int> startValues(vector_size);
        generate(startValues.begin(), startValues.end(), rand);

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

            for (int i = 0; i < execution_count; ++i)
            {
                // Minden iterációnál friss másolatot készítünk az eredeti adatokból
                auto values = startValues;
                auto start = chrono::high_resolution_clock::now();

                // JAVÍTÁS 1: future<void>, mivel a függvény nem ad vissza semmit
                future<void> fut = async(launch::async, func, ref(values));

                if (fut.wait_for(chrono::seconds(timeout_sec)) == future_status::timeout)
                {
                    timed_out = true;
                    break;
                }

                // JAVÍTÁS 2: Nincs visszatérési érték, csak megvárjuk a végét
                fut.get();

                auto end = chrono::high_resolution_clock::now();
                double duration = chrono::duration<double>(end - start).count();

                // JAVÍTÁS 3: Ellenőrizzük, hogy a vektor tényleg rendezett-e
                // std::is_sorted a leghatékonyabb módja a validálásnak
                if (!std::is_sorted(values.begin(), values.end()))
                {
                    failed = true;
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