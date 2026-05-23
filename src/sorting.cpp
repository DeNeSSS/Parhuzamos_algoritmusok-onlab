#include "sorting.h"

#include <climits>
#include <cmath>
#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include <set>
#include <iomanip>
#include <fstream>
#include <future>
#include <algorithm>
#include <chrono>
#include <time.h>
#include <omp.h>

namespace sorting
{
    using namespace std;

    // --- ABSZTRAKT ALAPOSZTÁLY (Strategy Pattern) ---
    class SortingAlgorithm
    {
    public:
        int PARALLEL_THRESHOLD = 10000;
        virtual ~SortingAlgorithm() = default;

        // A fő függvény, amit minden algoritmusnak meg kell valósítania
        virtual void sort(vector<int> &values) const = 0;

        // Név lekérése a benchmarkhoz
        virtual string getName() const = 0;
    };

    namespace
    {

        double get_cpu_time_seconds()
        {
            struct timespec ts;
            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
            return ts.tv_sec + ts.tv_nsec / 1e9;
        }

        inline void compareExchange(vector<int> &values, size_t idx, size_t jdx)
        {
            if (values[idx] > values[jdx])
            {
                swap(values[idx], values[jdx]); // std::swap beépített és gyors
            }
        }

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
    }

    // --- 1. ODD-EVEN BUBBLE SORT ALGORITMUSOK ---

    class SerialOddEvenBubbleSort : public SortingAlgorithm
    {
    public:
        void sort(vector<int> &values) const override
        {
            size_t size = values.size();
            for (size_t i = 0; i < size; i++)
            {
                if (i % 2 == 0)
                {
                    for (size_t j = 0; j < size - 1; j += 2)
                        compareExchange(values, j, j + 1);
                }
                else
                {
                    for (size_t j = 1; j < size - 1; j += 2)
                        compareExchange(values, j, j + 1);
                }
            }
        }
        string getName() const override { return "Serial Odd-Even Bubble"; }
    };

    class ParallelOddEvenBubbleSort : public SortingAlgorithm
    {
    public:
        void sort(vector<int> &values) const override
        {
            size_t size = values.size();
#pragma omp parallel
            for (size_t i = 0; i < size; i++)
            {
                if (i % 2 == 0)
                {
#pragma omp for
                    for (size_t j = 0; j < size - 1; j += 2)
                        compareExchange(values, j, j + 1);
                }
                else
                {
#pragma omp for
                    for (size_t j = 1; j < size - 1; j += 2)
                        compareExchange(values, j, j + 1);
                }
            }
        }
        string getName() const override { return "Parallel Odd-Even Bubble"; }
    };

    // --- 2. QUICKSORT ALGORITMUSOK ---

    class SerialQuickSort : public SortingAlgorithm
    {
    private:
        int partition(vector<int> &arr, int low, int high) const
        {
            int pivot = arr[high];
            int i = low - 1;

            for (int j = low; j <= high - 1; j++)
            {
                if (arr[j] < pivot)
                {
                    i++;
                    swap(arr[i], arr[j]);
                }
            }
            swap(arr[i + 1], arr[high]);
            return i + 1;
        }

        void quickSortRec(vector<int> &arr, int low, int high) const
        {
            if (low < high)
            {
                int pi = partition(arr, low, high);
                quickSortRec(arr, low, pi - 1);
                quickSortRec(arr, pi + 1, high);
            }
        }

    public:
        void sort(vector<int> &arr) const override
        {
            if (!arr.empty())
            {
                // JAVÍTVA: arr.size() - 1, hogy ne címezzünk túl
                quickSortRec(arr, 0, arr.size() - 1);
            }
        }
        string getName() const override { return "Serial QuickSort"; }
    };

    class ParallelQuickSort : public SortingAlgorithm
    {
    private:
        int partition(vector<int> &arr, int low, int high) const
        {
            int pivot = arr[high];
            int i = low - 1;

            for (int j = low; j <= high - 1; j++)
            {
                if (arr[j] < pivot)
                {
                    i++;
                    swap(arr[i], arr[j]);
                }
            }
            swap(arr[i + 1], arr[high]);
            return i + 1;
        }

        void parallelQuickSortRec(vector<int> &arr, int low, int high) const
        {
            if (low < high)
            {
                if ((high - low) > this->PARALLEL_THRESHOLD)
                {
                    int pi = partition(arr, low, high);
#pragma omp task shared(arr) firstprivate(low, pi)
                    parallelQuickSortRec(arr, low, pi - 1);
#pragma omp task shared(arr) firstprivate(pi, high)
                    parallelQuickSortRec(arr, pi + 1, high);
#pragma omp taskwait
                }
                else
                {
                    int pi = partition(arr, low, high);

                    // Fallback to serial logic directly here
                    SerialQuickSort serialQS;
                    // JAVÍTÁS: Nem a teljes tömböt, hanem csak az al-részt rendezzük sorosan
                    // Mivel a SerialQuickSort csak a fő interfészt publikálja,
                    // írunk egy pici beépített soros fall-back hívást:
                    auto serial_fallback = [&](auto &self, int l, int h) -> void
                    {
                        if (l < h)
                        {
                            int p = partition(arr, l, h);
                            self(self, l, p - 1);
                            self(self, p + 1, h);
                        }
                    };
                    serial_fallback(serial_fallback, low, pi - 1);
                    serial_fallback(serial_fallback, pi + 1, high);
                }
            }
        }

    public:
        void sort(vector<int> &arr) const override
        {
            if (arr.empty())
                return;

#pragma omp parallel
            {
#pragma omp single
                {
                    parallelQuickSortRec(arr, 0, arr.size() - 1);
                }
            }
        }
        string getName() const override { return "Parallel QuickSort"; }
    };

    // --- 3. k SORT ALGORITMUSOK ---
    class SerialMergeSort : public SortingAlgorithm
    {
    private:
        void merge(vector<int> &arr, vector<int> &temp, int left, int mid, int right) const
        {
            int i = left;
            int j = mid + 1;
            int k = left;

            // Két rendezett rész összefésülése a temp tömbbe
            while (i <= mid && j <= right)
            {
                if (arr[i] <= arr[j])
                {
                    temp[k++] = arr[i++];
                }
                else
                {
                    temp[k++] = arr[j++];
                }
            }

            while (i <= mid)
            {
                temp[k++] = arr[i++];
            }

            while (j <= right)
            {
                temp[k++] = arr[j++];
            }

            for (int p = left; p <= right; p++)
            {
                arr[p] = temp[p];
            }
        }

        void mergeSort(vector<int> &arr, vector<int> &temp, int left, int right) const
        {
            if (left >= right)
                return;

            int mid = left + (right - left) / 2;
            mergeSort(arr, temp, left, mid);
            mergeSort(arr, temp, mid + 1, right);
            merge(arr, temp, left, mid, right);
        }

    public:
        void sort(vector<int> &values) const override
        {
            if (values.empty())
                return;

            vector<int> temp(values.size());

            mergeSort(values, temp, 0, values.size() - 1);
        }

        string getName() const override { return "Serial Merge Sort"; }
    };

    class ParallelMergeSort : public SortingAlgorithm
    {
    private:
        void merge(vector<int> &arr, vector<int> &temp, int left, int mid, int right) const
        {
            int i = left;
            int j = mid + 1;
            int k = left;

            // Két rendezett rész összefésülése a temp tömbbe
            while (i <= mid && j <= right)
            {
                if (arr[i] <= arr[j])
                {
                    temp[k++] = arr[i++];
                }
                else
                {
                    temp[k++] = arr[j++];
                }
            }

            while (i <= mid)
            {
                temp[k++] = arr[i++];
            }

            while (j <= right)
            {
                temp[k++] = arr[j++];
            }

            for (int p = left; p <= right; p++)
            {
                arr[p] = temp[p];
            }
        }

        void mergeSort(vector<int> &arr, vector<int> &temp, int left, int right) const
        {
            if (left >= right)
                return;

            int mid = left + (right - left) / 2;

            if (this->PARALLEL_THRESHOLD < (right - left))
            {
#pragma omp task shared(arr, temp) firstprivate(left, mid)
                mergeSort(arr, temp, left, mid);
#pragma omp task shared(arr, temp) firstprivate(mid, right)
                mergeSort(arr, temp, mid + 1, right);
#pragma omp taskwait
            }
            else
            {
                mergeSort(arr, temp, left, mid);
                mergeSort(arr, temp, mid + 1, right);
            }

            merge(arr, temp, left, mid, right);
        }

    public:
        void sort(vector<int> &values) const override
        {
            if (values.empty())
                return;

            vector<int> temp(values.size());
#pragma omp parallel
            {
#pragma omp single
                {
                    mergeSort(values, temp, 0, values.size() - 1);
                }
            }
        }

        string getName() const override { return "Parallel Merge Sort"; }
    };

    class ParallelMergeSort2 : public SortingAlgorithm
    {
    private:
        int THREASHOLD_SCALE = 10;

        void
        serialMerge(vector<int> &arr, vector<int> &temp, int left, int mid, int right) const
        {
            int i = left;
            int j = mid + 1;
            int k = left;

            while (i <= mid && j <= right)
            {
                if (arr[i] <= arr[j])
                    temp[k++] = arr[i++];
                else
                    temp[k++] = arr[j++];
            }
            while (i <= mid)
                temp[k++] = arr[i++];
            while (j <= right)
                temp[k++] = arr[j++];

            for (int p = left; p <= right; p++)
            {
                arr[p] = temp[p];
            }
        }

        void parallelMerge(vector<int> &arr, vector<int> &temp, int left, int mid, int right) const
        {
            int n1 = mid - left + 1;
            int n2 = right - mid;
            int total_len = right - left + 1;

            if (total_len < this->PARALLEL_THRESHOLD * this->THREASHOLD_SCALE)
            {
                serialMerge(arr, temp, left, mid, right);
                return;
            }

            int k = omp_get_max_threads();
            if (k > n1)
                k = n1;

            vector<int> splitA(k + 1);
            vector<int> splitB(k + 1);
            vector<int> outStart(k + 1);

            for (int i = 0; i <= k; ++i)
            {
                splitA[i] = left + i * n1 / k;
            }

            splitB[0] = mid + 1;
            splitB[k] = right + 1;
            for (int i = 1; i < k; ++i)
            {
                auto it = std::lower_bound(arr.begin() + mid + 1, arr.begin() + right + 1, arr[splitA[i]]);
                splitB[i] = std::distance(arr.begin(), it);
            }

            for (int i = 0; i <= k; ++i)
            {
                outStart[i] = left + (splitA[i] - left) + (splitB[i] - (mid + 1));
            }

#pragma omp parallel for schedule(static)
            for (int i = 0; i < k; ++i)
            {
                int startA = splitA[i];
                int endA = splitA[i + 1] - 1;
                int startB = splitB[i];
                int endB = splitB[i + 1] - 1;
                int outIdx = outStart[i];

                int i_idx = startA;
                int j_idx = startB;

                while (i_idx <= endA && j_idx <= endB)
                {
                    if (arr[i_idx] <= arr[j_idx])
                        temp[outIdx++] = arr[i_idx++];
                    else
                        temp[outIdx++] = arr[j_idx++];
                }
                while (i_idx <= endA)
                    temp[outIdx++] = arr[i_idx++];
                while (j_idx <= endB)
                    temp[outIdx++] = arr[j_idx++];
            }

#pragma omp parallel for schedule(static)
            for (int p = left; p <= right; ++p)
            {
                arr[p] = temp[p];
            }
        }

        void mergeSort(vector<int> &arr, vector<int> &temp, int left, int right) const
        {
            if (left >= right)
                return;

            int mid = left + (right - left) / 2;

            if ((right - left) > this->PARALLEL_THRESHOLD)
            {
#pragma omp task shared(arr, temp) firstprivate(left, mid)
                mergeSort(arr, temp, left, mid);
#pragma omp task shared(arr, temp) firstprivate(mid, right)
                mergeSort(arr, temp, mid + 1, right);
#pragma omp taskwait
            }
            else
            {
                mergeSort(arr, temp, left, mid);
                mergeSort(arr, temp, mid + 1, right);
            }

            // Meghívjuk a blokk-alapú párhuzamos merge-öt
            parallelMerge(arr, temp, left, mid, right);
        }

    public:
        ParallelMergeSort2(int threashold_scale = 10)
        {
            this->THREASHOLD_SCALE = threashold_scale;
        }

        void sort(vector<int> &values) const override
        {
            if (values.empty())
                return;

            vector<int> temp(values.size());
#pragma omp parallel
            {
#pragma omp single
                {
                    mergeSort(values, temp, 0, values.size() - 1);
                }
            }
        }

        string getName() const override { return "Parallel Merge Sort 2 - " + to_string(this->THREASHOLD_SCALE); }
    };

    class ParallelMergeSort3 : public SortingAlgorithm
    {
    private:
        void
        serialMerge(vector<int> &arr, vector<int> &temp, int left, int mid, int right) const
        {
            int i = left;
            int j = mid + 1;
            int k = left;

            while (i <= mid && j <= right)
            {
                if (arr[i] <= arr[j])
                    temp[k++] = arr[i++];
                else
                    temp[k++] = arr[j++];
            }
            while (i <= mid)
                temp[k++] = arr[i++];
            while (j <= right)
                temp[k++] = arr[j++];

            for (int p = left; p <= right; p++)
            {
                arr[p] = temp[p];
            }
        }

        void parallelMerge(vector<int> &arr, vector<int> &temp, int left, int mid, int right) const
        {
            int n1 = mid - left + 1;
            int n2 = right - mid;

            if (!(left == 0 and right == arr.size() - 1))
            {
                serialMerge(arr, temp, left, mid, right);
                return;
            }

            int k = omp_get_max_threads();
            if (k > n1)
                k = n1;

            vector<int> splitA(k + 1);
            vector<int> splitB(k + 1);
            vector<int> outStart(k + 1);

            for (int i = 0; i <= k; ++i)
            {
                splitA[i] = left + i * n1 / k;
            }

            splitB[0] = mid + 1;
            splitB[k] = right + 1;
            for (int i = 1; i < k; ++i)
            {
                auto it = std::lower_bound(arr.begin() + mid + 1, arr.begin() + right + 1, arr[splitA[i]]);
                splitB[i] = std::distance(arr.begin(), it);
            }

            for (int i = 0; i <= k; ++i)
            {
                outStart[i] = left + (splitA[i] - left) + (splitB[i] - (mid + 1));
            }

#pragma omp parallel for schedule(static)
            for (int i = 0; i < k; ++i)
            {
                int startA = splitA[i];
                int endA = splitA[i + 1] - 1;
                int startB = splitB[i];
                int endB = splitB[i + 1] - 1;
                int outIdx = outStart[i];

                int i_idx = startA;
                int j_idx = startB;

                while (i_idx <= endA && j_idx <= endB)
                {
                    if (arr[i_idx] <= arr[j_idx])
                        temp[outIdx++] = arr[i_idx++];
                    else
                        temp[outIdx++] = arr[j_idx++];
                }
                while (i_idx <= endA)
                    temp[outIdx++] = arr[i_idx++];
                while (j_idx <= endB)
                    temp[outIdx++] = arr[j_idx++];
            }

#pragma omp parallel for schedule(static)
            for (int p = left; p <= right; ++p)
            {
                arr[p] = temp[p];
            }
        }

        void mergeSort(vector<int> &arr, vector<int> &temp, int left, int right) const
        {
            if (left >= right)
                return;

            int mid = left + (right - left) / 2;

            if ((right - left) > this->PARALLEL_THRESHOLD)
            {
#pragma omp task shared(arr, temp) firstprivate(left, mid)
                mergeSort(arr, temp, left, mid);
#pragma omp task shared(arr, temp) firstprivate(mid, right)
                mergeSort(arr, temp, mid + 1, right);
#pragma omp taskwait
            }
            else
            {
                mergeSort(arr, temp, left, mid);
                mergeSort(arr, temp, mid + 1, right);
            }

            // Meghívjuk a blokk-alapú párhuzamos merge-öt
            parallelMerge(arr, temp, left, mid, right);
        }

    public:
        void sort(vector<int> &values) const override
        {
            if (values.empty())
                return;

            vector<int> temp(values.size());
#pragma omp parallel
            {
#pragma omp single
                {
                    mergeSort(values, temp, 0, values.size() - 1);
                }
            }
        }

        string getName() const override { return "Parallel Merge Sort 3"; }
    };

    // --- 4. ODD-EVEN MERGE SORT ALGORITMUSOK ---

    class SerialOddEvenMergeSort : public SortingAlgorithm
    {
    private:
        void oddEvenMergeRecursive(vector<int> &v, int start, int n, int step) const
        {
            int m = step * 2;
            if (m < n)
            {
                oddEvenMergeRecursive(v, start, n, m);        // Even
                oddEvenMergeRecursive(v, start + step, n, m); // Odd

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

        void oddEvenMergeSortRecursive(vector<int> &v, int start, int n) const
        {
            if (n > 1)
            {
                int m = n / 2;
                oddEvenMergeSortRecursive(v, start, m);
                oddEvenMergeSortRecursive(v, start + m, m);
                oddEvenMergeRecursive(v, start, n, 1);
            }
        }

    public:
        void sort(vector<int> &values) const override
        {
            if (values.empty())
                return;

            size_t originalSize = values.size();
            if (!isPowerOfTwo(originalSize))
            {
                size_t paddedSize = nextPowerOfTwo(originalSize);
                values.resize(paddedSize, INT_MAX);
            }

            oddEvenMergeSortRecursive(values, 0, values.size());

            if (values.size() > originalSize)
            {
                values.resize(originalSize);
            }
        }
        string getName() const override { return "Serial Odd-Even Merge"; }
    };

    class ParallelOddEvenMergeSort : public SortingAlgorithm
    {
    private:
        void oddEvenMergeRecursive(vector<int> &v, int start, int n, int step) const
        {
            int m = step * 2;
            if (m < n)
            {
                oddEvenMergeRecursive(v, start, n, m);
                oddEvenMergeRecursive(v, start + step, n, m);

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

        void oddEvenMergeSortRecursiveSerial(vector<int> &v, int start, int n) const
        {
            if (n > 1)
            {
                int m = n / 2;
                oddEvenMergeSortRecursiveSerial(v, start, m);
                oddEvenMergeSortRecursiveSerial(v, start + m, m);
                oddEvenMergeRecursive(v, start, n, 1);
            }
        }

        void parallelOddEvenMergeSortRecursive(vector<int> &v, int start, int n) const
        {
            if (n > 1)
            {
                int m = n / 2;

                if (n > this->PARALLEL_THRESHOLD)
                {
#pragma omp task shared(v) firstprivate(start, m)
                    parallelOddEvenMergeSortRecursive(v, start, m);
#pragma omp task shared(v) firstprivate(start, m)
                    parallelOddEvenMergeSortRecursive(v, start + m, m);
#pragma omp taskwait
                }
                else
                {
                    oddEvenMergeSortRecursiveSerial(v, start, m);
                    oddEvenMergeSortRecursiveSerial(v, start + m, m);
                }

                oddEvenMergeRecursive(v, start, n, 1);
            }
        }

    public:
        void sort(vector<int> &values) const override
        {
            if (values.empty())
                return;

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
        string getName() const override { return "Parallel Odd-Even Merge"; }
    };

    // --- 4. TESZTELŐ KERETRENDSZER ---

    void measureSortingAlgorithms(int vector_size, int execution_count, int timeout_sec)
    {
        // Algoritmusok regisztrálása a polimorf vektorba
        vector<unique_ptr<SortingAlgorithm>> algorithms;

        // algorithms.push_back(make_unique<SerialOddEvenBubbleSort>());
        // algorithms.push_back(make_unique<ParallelOddEvenBubbleSort>());
        // algorithms.push_back(make_unique<SerialQuickSort>());
        // algorithms.push_back(make_unique<ParallelQuickSort>());
        // algorithms.push_back(make_unique<SerialOddEvenMergeSort>());
        // algorithms.push_back(make_unique<ParallelOddEvenMergeSort>());
        // algorithms.push_back(make_unique<SerialMergeSort>());
        algorithms.push_back(make_unique<ParallelMergeSort>());
        algorithms.push_back(make_unique<ParallelMergeSort2>());

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

        for (const auto &alg : algorithms)
        {
            double total_time = 0, fastest = 1e9, slowest = 0;
            bool failed = false, timed_out = false;
            vector<int> failed_sort;

            for (int i = 0; i < execution_count; ++i)
            {
                auto values = startValues;
                auto start = chrono::high_resolution_clock::now();

                // KÖZVETLEN HÍVÁS (Nincs std::async)
                alg->sort(values);

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

                // UTÓLAGOS TIMEOUT VIZSGÁLAT
                if (duration > timeout_sec)
                {
                    timed_out = true;
                    break;
                }
            }

            cout << left << setw(30) << alg->getName();
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
                        break;
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

    void measureSortingAlgorithms(int min_vector_size, int max_vector_size, function<int(int)> size_multiplier, string folder_name, int execution_count, int timeout_sec)
    {
        // Algoritmusok regisztrálása a polimorf vektorba
        vector<unique_ptr<SortingAlgorithm>> algorithms;

        // algorithms.push_back(make_unique<SerialOddEvenBubbleSort>());
        // algorithms.push_back(make_unique<ParallelOddEvenBubbleSort>());
        // algorithms.push_back(make_unique<SerialQuickSort>());
        // algorithms.push_back(make_unique<ParallelQuickSort>());
        // algorithms.push_back(make_unique<SerialOddEvenMergeSort>());
        // algorithms.push_back(make_unique<ParallelOddEvenMergeSort>());
        algorithms.push_back(make_unique<SerialMergeSort>());
        algorithms.push_back(make_unique<ParallelMergeSort>());
        algorithms.push_back(make_unique<ParallelMergeSort3>());

        srand(time(nullptr));

        map<string, ofstream> alg_files;
        map<string, double> total_wall_time;
        map<string, double> total_cpu_time;
        map<string, int> sizes_processed_count;
        map<string, int> max_size_reached;
        map<string, string> final_status;
        set<string> timed_out_algorithms;

        for (const auto &alg : algorithms)
        {
            string name = alg->getName();
            string clean_name = name;
            replace(clean_name.begin(), clean_name.end(), ' ', '_');
            replace(clean_name.begin(), clean_name.end(), '(', '_');
            replace(clean_name.begin(), clean_name.end(), ')', '_');

            string filename = "../test_data/sorting/";
            if (folder_name != "")
            {
                filename += folder_name + "/";
            }
            filename += clean_name + "_results.csv";

            cout << filename << endl;

            alg_files[name].open(filename);
            if (alg_files[name].is_open())
            {
                alg_files[name] << "Size,Wall_Avg_s,CPU_Work_s,Wall_Fast_s,Wall_Slow_s,Status\n";
            }

            final_status[name] = "SUCCESS";
            max_size_reached[name] = 0;
            total_wall_time[name] = 0.0;
            total_cpu_time[name] = 0.0;
            sizes_processed_count[name] = 0;
        }

        auto benchmark_start = chrono::high_resolution_clock::now();
        cout << "\n[INFO] Sorting Benchmarking started (Measuring Wall Time and CPU Process Time)..." << endl;

        int current_size = min_vector_size;

        while (current_size <= max_vector_size)
        {
            vector<int> startValues(current_size);
            generate(startValues.begin(), startValues.end(), [current_size]()
                     { return rand() % current_size; });

            for (const auto &alg : algorithms)
            {
                string name = alg->getName();

                if (timed_out_algorithms.count(name))
                {
                    if (alg_files[name].is_open())
                    {
                        alg_files[name] << current_size << ",,,,,SKIPPED\n";
                    }
                    continue;
                }

                double sum_wall_time = 0, sum_cpu_time = 0;
                double fastest_wall = 1e9, slowest_wall = 0;
                bool failed = false, timed_out = false;
                int actual_iterations = 0;

                for (int i = 0; i < execution_count; ++i)
                {
                    auto values = startValues;

                    auto wall_start = chrono::high_resolution_clock::now();
                    double cpu_start = get_cpu_time_seconds();

                    // KÖZVETLEN HÍVÁS (Nincs std::async)
                    alg->sort(values);

                    double cpu_end = get_cpu_time_seconds();
                    auto wall_end = chrono::high_resolution_clock::now();

                    double wall_duration = chrono::duration<double>(wall_end - wall_start).count();
                    double cpu_duration = cpu_end - cpu_start;

                    if (!std::is_sorted(values.begin(), values.end()))
                    {
                        failed = true;
                        final_status[name] = "FAILED";
                        break;
                    }

                    sum_wall_time += wall_duration;
                    sum_cpu_time += cpu_duration;
                    fastest_wall = min(fastest_wall, wall_duration);
                    slowest_wall = max(slowest_wall, wall_duration);
                    actual_iterations++;

                    // UTÓLAGOS TIMEOUT VIZSGÁLAT
                    if (wall_duration > timeout_sec)
                    {
                        timed_out = true;
                        timed_out_algorithms.insert(name);
                        final_status[name] = "TIMEOUT";
                        break;
                    }
                }

                if (alg_files[name].is_open())
                {
                    alg_files[name] << current_size << ",";
                    if (timed_out)
                    {
                        alg_files[name] << ",,,,TIMEOUT\n";
                    }
                    else if (failed)
                    {
                        alg_files[name] << ",,,,FAILED\n";
                    }
                    else
                    {
                        alg_files[name] << fixed << setprecision(6)
                                        << (sum_wall_time / actual_iterations) << ","
                                        << (sum_cpu_time / actual_iterations) << ","
                                        << fastest_wall << ","
                                        << slowest_wall << ","
                                        << "SUCCESS\n";
                    }
                }

                if (!failed && !timed_out)
                {
                    max_size_reached[name] = current_size;
                    total_wall_time[name] += (sum_wall_time / actual_iterations);
                    total_cpu_time[name] += (sum_cpu_time / actual_iterations);
                    sizes_processed_count[name]++;
                }
            }

            int next_size = size_multiplier(current_size);
            if (next_size <= current_size)
            {
                cerr << "\n[Warning] size_multiplier did not increase size. Aborting." << endl;
                break;
            }
            current_size = next_size;
        }

        auto benchmark_end = chrono::high_resolution_clock::now();
        double benchmark_duration = chrono::duration<double>(benchmark_end - benchmark_start).count();

        cout << "\n"
             << setfill('=') << setw(110) << "" << endl;
        cout << " FINAL SORTING BENCHMARK SUMMARY REPORT - benchmark duration: " << benchmark_duration << " (s)" << endl;
        cout << setfill('=') << setw(110) << "" << setfill(' ') << endl;
        cout << left << setw(40) << "Algorithm"
             << setw(20) << "Max Size Reached"
             << setw(22) << "Avg Wall Time (s)"
             << setw(22) << "Avg CPU Work (s)"
             << "Final Status" << endl;
        cout << string(110, '-') << endl;

        for (const auto &alg : algorithms)
        {
            string name = alg->getName();
            cout << left << setw(40) << name;
            cout << setw(20) << max_size_reached[name];

            if (sizes_processed_count[name] > 0)
            {
                cout << fixed << setprecision(6)
                     << setw(22) << (total_wall_time[name] / sizes_processed_count[name])
                     << setw(22) << (total_cpu_time[name] / sizes_processed_count[name]);
            }
            else
            {
                cout << setw(22) << "-" << setw(22) << "-";
            }

            if (final_status[name] == "TIMEOUT")
                cout << "\033[33mTIMEOUT\033[0m" << endl;
            else if (final_status[name] == "FAILED")
                cout << "\033[31mFAILED\033[0m" << endl;
            else
                cout << "\033[32mSUCCESS\033[0m" << endl;

            if (alg_files[name].is_open())
                alg_files[name].close();
        }
        cout << setfill('=') << setw(110) << "" << setfill(' ') << endl;
    }

} // namespace sorting