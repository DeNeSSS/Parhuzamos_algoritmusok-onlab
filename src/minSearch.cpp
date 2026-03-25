#include <thread>
#include <omp.h>
#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <execution>
#include <atomic>
#include <future>
#include <map>
#include <climits>
#include <iomanip>
#include <functional>

#include "threadPool.h"
#include "minSearch.h"
#include "tester.h" // Feltételezve, hogy itt van a measure_time(func, args...)

namespace minSearch {

using namespace std;

// --- 1. ALAP ÉS RÉGI ALGORITMUSOK ---

auto serial(const vector<int>& vals) -> int {
    if (vals.empty()) return INT_MAX;
    int min_val = vals[0];
    for (int val : vals) {
        if (min_val > val) min_val = val;
    }
    return min_val;
}

auto OMP_Compression(const vector<int>& values) -> int {
    if (values.size() <= 1) return values.empty() ? INT_MAX : values[0];

    size_t new_size = (values.size() + 1) / 2;
    vector<int> values2(new_size);

    #pragma omp parallel for
    for (size_t i = 0; i < values.size() / 2; ++i) {
        values2[i] = min(values[2 * i], values[2 * i + 1]);
    }
    if (values.size() % 2 != 0) {
        values2[new_size - 1] = values.back();
    }
    return OMP_Compression(values2);
}

auto threadPool_Pairwise(const vector<int>& values) -> int {
    if (values.empty()) return INT_MAX;
    if (values.size() == 1) return values[0];

    ThreadPool pool(thread::hardware_concurrency());
    deque<int> current_values(values.begin(), values.end());

    while (current_values.size() > 1) {
        vector<future<int>> futures;
        while (current_values.size() >= 2) {
            int v1 = current_values.front(); current_values.pop_front();
            int v2 = current_values.front(); current_values.pop_front();

            auto promise = make_shared<std::promise<int>>();
            auto fut = promise->get_future();
            pool.enqueue([v1, v2, promise]() {
                promise->set_value(min(v1, v2));
            });
            futures.push_back(move(fut));
        }

        int leftover = -1;
        bool has_leftover = false;
        if (!current_values.empty()) {
            leftover = current_values.front();
            current_values.pop_front();
            has_leftover = true;
        }

        for (auto& f : futures) current_values.push_back(f.get());
        if (has_leftover) current_values.push_back(leftover);
    }
    return current_values.front();
}

int threadPool_Chunked(const vector<int>& values) {
    if (values.empty()) return INT_MAX;
    size_t num_workers = thread::hardware_concurrency();
    ThreadPool pool(num_workers);

    vector<int> partial_mins(num_workers, INT_MAX);
    atomic<size_t> completed_tasks{0};
    size_t chunk_size = (values.size() + num_workers - 1) / num_workers;

    for (size_t i = 0; i < num_workers; ++i) {
        pool.enqueue([&values, &partial_mins, i, num_workers, &completed_tasks, chunk_size] {
            size_t start = i * chunk_size;
            size_t end = min(start + chunk_size, values.size());

            if (start < end) {
                int local_min = values[start];
                for (size_t j = start + 1; j < end; ++j) {
                    if (values[j] < local_min) local_min = values[j];
                }
                partial_mins[i] = local_min;
            }
            completed_tasks.fetch_add(1);
        });
    }

    while (completed_tasks.load() < num_workers) {
        this_thread::yield();
    }
    return serial(partial_mins);
}

auto OMP_Reduction(const vector<int>& values) -> int {
    if (values.empty()) return INT_MAX;
    int min_val = values[0];
    #pragma omp parallel for reduction(min : min_val)
    for (size_t i = 0; i < values.size(); ++i) {
        if (values[i] < min_val) min_val = values[i];
    }
    return min_val;
}

// Rekurzív Async küszöbértékkel (Threshold), hogy ne fogyjon el a memória
int recursiveAsyncWorker(const int* data, size_t n) {
    if (n <= 5000) return *min_element(data, data + n);
    size_t mid = n / 2;
    auto handle = async(launch::async, recursiveAsyncWorker, data, mid);
    int right_min = recursiveAsyncWorker(data + mid, n - mid);
    return min(handle.get(), right_min);
}

auto recursiveAsyncMin(const vector<int>& values) -> int {
    if (values.empty()) return INT_MAX;
    return recursiveAsyncWorker(values.data(), values.size());
}

auto stdParallelMin(const vector<int>& values) -> int {
    if (values.empty()) return INT_MAX;
    auto it = min_element(execution::par, values.begin(), values.end());
    return *it;
}

auto OMP_simdMin(const vector<int>& values) -> int {
    if (values.empty()) return INT_MAX;
    int min_val = values[0];
    #pragma omp parallel for simd reduction(min : min_val)
    for (size_t i = 0; i < values.size(); ++i) {
        if (values[i] < min_val) min_val = values[i];
    }
    return min_val;
}

// --- TESZTELŐ KERETRENDSZER TIMEOUT-TAL ---

void testMinSearchAlgorithms(int vector_size, int execution_count, int timeout_sec) {
    map<string, function<int(const vector<int>&)>> functions = {
        {"Serial", serial},
        {"OMP Compression", OMP_Compression},
        {"OMP Reduction", OMP_Reduction},
        {"OMP SIMD", OMP_simdMin},
        {"STD Parallel (C++17)", stdParallelMin},
        {"Recursive Async", recursiveAsyncMin},
        {"ThreadPool Chunked", threadPool_Chunked},
        {"ThreadPool Pairwise", threadPool_Pairwise}
    };

    srand(time(nullptr));
    vector<int> values(vector_size);
    generate(values.begin(), values.end(), rand);

    int expected = serial(values);

    // Szép fejléc kiírása
    cout << "\n" << setfill('=') << setw(100) << "" << endl;
    cout << " MIN SEARCH BENCHMARK | Size: " << vector_size << " | Iterations: " << execution_count << endl;
    cout << setfill('=') << setw(100) << "" << setfill(' ') << endl;
    cout << left << setw(25) << "Algorithm" 
         << setw(15) << "Avg Time (s)" 
         << setw(15) << "Fastest (s)" 
         << setw(15) << "Slowest (s)" 
         << "Status" << endl;
    cout << string(100, '-') << endl;

    for (auto const& [name, func] : functions) {
        double total_time = 0, fastest = 1e9, slowest = 0;
        bool failed = false, timed_out = false;

        for (int i = 0; i < execution_count; ++i) {
            auto start = chrono::high_resolution_clock::now();
            
            // Timeout kezelés std::async-kel
            future<int> fut = async(launch::async, func, ref(values));
            
            if (fut.wait_for(chrono::seconds(timeout_sec)) == future_status::timeout) {
                timed_out = true;
                break;
            }

            int result = fut.get();
            auto end = chrono::high_resolution_clock::now();
            double duration = chrono::duration<double>(end - start).count();

            if (result != expected) { failed = true; break; }

            total_time += duration;
            fastest = min(fastest, duration);
            slowest = max(slowest, duration);
        }

        cout << left << setw(25) << name;
        if (timed_out) {
            cout << "\033[33mTIMEOUT (> " << timeout_sec << "s)\033[0m" << endl;
        } else if (failed) {
            cout << "\033[31mFAILED (Wrong Result)\033[0m" << endl;
        } else {
            cout << fixed << setprecision(6) 
                 << setw(15) << (total_time / execution_count)
                 << setw(15) << fastest
                 << setw(15) << slowest
                 << "\033[32mSUCCESS\033[0m" << endl;
        }
    }
    cout << setfill('=') << setw(100) << "" << setfill(' ') << endl;
}

} // namespace minSearch