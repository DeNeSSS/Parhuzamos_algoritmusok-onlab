#include "minSearch.h"
#include "threadPool.h"
#include "tester.h"
#include <thread>
#include <omp.h>
#include <iostream>
#include <vector>
#include <map>
#include <queue>
#include <algorithm>
#include <execution>
#include <atomic>
#include <future>
#include <climits>
#include <iomanip>
#include <functional>
#include <chrono>
#include <set>
#include <fstream>
#include <ctime>
#include <memory>
#include <condition_variable>
#include <mutex>

namespace minSearch
{
    using namespace std;

    // --- ABSZTRAKT ALAPOSZTÁLY (Strategy Pattern) ---
    class MinSearchAlgorithm
    {
    public:
        size_t TREASHOLD = 10000;
        virtual ~MinSearchAlgorithm() = default;
        virtual int findMin(const vector<int> &vals) const = 0;
        virtual string getName() const = 0;
    };

    // --- 1. ALGORITMUS IMPLEMENTÁCIÓK OSZTÁLYOKKÉNT ---
    class SerialMin : public MinSearchAlgorithm
    {
    public:
        int findMin(const vector<int> &vals) const override
        {
            if (vals.empty())
                return INT_MAX;
            int min_val = vals[0];
            for (int val : vals)
            {
                if (min_val > val)
                    min_val = val;
            }
            return min_val;
        }
        string getName() const override { return "Serial"; }
    };

    class OmpReduction : public MinSearchAlgorithm
    {
    public:
        int findMin(const vector<int> &values) const override
        {
            if (values.empty())
                return INT_MAX;
            int min_val = values[0];
#pragma omp parallel for reduction(min : min_val)
            for (size_t i = 0; i < values.size(); ++i)
            {
                if (values[i] < min_val)
                    min_val = values[i];
            }
            return min_val;
        }
        string getName() const override { return "OMP Reduction"; }
    };

    class ThreadPoolReduction : public MinSearchAlgorithm
    {
    public:
        int findMin(const vector<int> &values) const override
        {
            if (values.empty())
                return INT_MAX;
            size_t num_workers = thread::hardware_concurrency();
            ThreadPool pool(num_workers);

            vector<int> partial_mins(num_workers, INT_MAX);

            // JAVÍTÁS: Spin-lock helyett tiszta Condition Variable várakozás!
            mutex mtx;
            condition_variable cv;
            size_t completed_tasks = 0;
            size_t chunk_size = (values.size() + num_workers - 1) / num_workers;

            for (size_t i = 0; i < num_workers; ++i)
            {
                pool.enqueue([&values, &partial_mins, i, num_workers, &mtx, &cv, &completed_tasks, chunk_size]
                             {
                    size_t start = i * chunk_size;
                    size_t end = min(start + chunk_size, values.size());

                    if (start < end) {
                        int local_min = values[start];
                        for (size_t j = start + 1; j < end; ++j) {
                            if (values[j] < local_min) local_min = values[j];
                        }
                        partial_mins[i] = local_min;
                    }

                    {
                        lock_guard<mutex> lock(mtx);
                        completed_tasks++;
                        if (completed_tasks == num_workers) {
                            cv.notify_one();
                        }
                    } });
            }

            {
                unique_lock<mutex> lock(mtx);
                cv.wait(lock, [&]
                        { return completed_tasks == num_workers; });
            }

            // Soros minimumkeresés a részeredményeken
            int final_min = partial_mins[0];
            for (int val : partial_mins)
            {
                if (val < final_min)
                    final_min = val;
            }
            return final_min;
        }
        string getName() const override { return "ThreadPool Reduction"; }
    };

    class OmpSimd : public MinSearchAlgorithm
    {
    public:
        int findMin(const vector<int> &values) const override
        {
            if (values.empty())
                return INT_MAX;

            int min_val = values[0];
#pragma omp simd reduction(min : min_val)
            for (size_t i = 1; i < values.size(); ++i)
            {
                min_val = std::min(min_val, values[i]);
            }
            return min_val;
        }

        string getName() const override { return "OMP SIMD"; }
    };

    class OmpCompression : public MinSearchAlgorithm
    {
    private:
        // A küszöbérték, ami alatt már nem éri meg szálakat indítani
        const size_t THRESHOLD = 5000;

        /**
         * @brief Ping-pong pufferes rekurzív tömörítő
         * * @param read_buf  A tömb, amiből a szálak OLVASNAK (forrás)
         * @param write_buf A tömb, amibe a szálak ÍRNAK (cél)
         * @param size      A jelenlegi aktív méret
         */
        int compress_impl(const int *read_buf, int *write_buf, size_t size) const
        {
            // Bázisfeltételek
            if (size == 0)
                return INT_MAX;
            if (size == 1)
                return read_buf[0];

            // 1. SOROS VÉGREHAJTÁS (Threshold alatt)
            if (size <= THRESHOLD)
            {
                // Mivel C-stílusú pointereink vannak, a std::min_element is tökéletesen működik velük
                return *std::min_element(read_buf, read_buf + size);
            }

            // 2. PÁRHUZAMOS TÖMÖRÍTÉS KÜLÖN PUFFERBE
            size_t half_size = size / 2;
            size_t new_size = (size + 1) / 2; // Felfelé kerekítés a páratlan elemszám miatt

#pragma omp parallel for
            for (size_t i = 0; i < half_size; ++i)
            {
                // Tiszta szálbiztonság: Olvasás a read_buf-ból, írás a write_buf-ba
                write_buf[i] = std::min(read_buf[2 * i], read_buf[2 * i + 1]);
            }

            // Páratlan elemszám esetén az utolsó "pár nélküli" elem átmentése
            if (size % 2 != 0)
            {
                write_buf[new_size - 1] = read_buf[size - 1];
            }

            // REKURZIÓ: Szerepcsere! A write_buf lesz az új forrás, a read_buf az új cél.
            return compress_impl(write_buf, const_cast<int *>(read_buf), new_size);
        }

    public:
        int findMin(const vector<int> &vals) const override
        {
            if (vals.empty())
                return INT_MAX;
            if (vals.size() == 1)
                return vals[0];

            // PING-PONG PUFFEREK ELŐKÉSZÍTÉSE (Csak itt foglalunk memóriát, egyszer!)

            // bufferA megkapja a kezdőértékeket (ebből fogunk először olvasni)
            vector<int> bufferA = vals;

            // bufferB-nek elég fele akkora méret, hiszen a legelső íráskor is már megfeleződik az adat
            vector<int> bufferB((vals.size() + 1) / 2);

            // C-stílusú pointereket adunk át a gyorsaság érdekében
            return compress_impl(bufferA.data(), bufferB.data(), vals.size());
        }

        string getName() const override { return "OMP Compression"; }
    };

    class StridedCompression : public MinSearchAlgorithm
    {
    public:
        int findMin(const vector<int> &vals) const override
        {
            if (vals.empty())
                return INT_MAX;

            // Mivel in-place dolgozunk, készítünk egy másolatot
            vector<int> values = vals;
            int n = values.size();

            // A lépéshossz (stride) minden körben duplázódik: 1, 2, 4, 8, 16...
            for (int stride = 1; stride < n; stride *= 2)
            {
                // A belső ciklus ugrik: 0, 2*stride, 4*stride...
#pragma omp parallel for
                for (int i = 0; i < n - stride; i += 2 * stride)
                {
                    // A kisebbiket mindig az alacsonyabb (i) indexre írjuk vissza
                    values[i] = std::min(values[i], values[i + stride]);
                }
            }

            // A végeredmény mindig a legelső (0.) elemen gyűlik össze
            return values[0];
        }

        string getName() const override { return "Strided Compression"; }
    };

    class ThreadPoolPairwise : public MinSearchAlgorithm
    {
    public:
        int findMin(const vector<int> &values) const override
        {
            if (values.empty())
                return INT_MAX;
            if (values.size() == 1)
                return values[0];

            ThreadPool pool(thread::hardware_concurrency());
            deque<int> current_values(values.begin(), values.end());

            while (current_values.size() > 1)
            {
                vector<future<int>> futures;
                while (current_values.size() >= 2)
                {
                    int v1 = current_values.front();
                    current_values.pop_front();
                    int v2 = current_values.front();
                    current_values.pop_front();

                    auto promise = make_shared<std::promise<int>>();
                    auto fut = promise->get_future();
                    pool.enqueue([v1, v2, promise]()
                                 { promise->set_value(std::min(v1, v2)); });
                    futures.push_back(move(fut));
                }

                int leftover = -1;
                bool has_leftover = false;
                if (!current_values.empty())
                {
                    leftover = current_values.front();
                    current_values.pop_front();
                    has_leftover = true;
                }

                for (auto &f : futures)
                    current_values.push_back(f.get());
                if (has_leftover)
                    current_values.push_back(leftover);
            }
            return current_values.front();
        }
        string getName() const override { return "ThreadPool Pairwise"; }
    };

    class AsyncCompression : public MinSearchAlgorithm
    {
    private:
        int worker(const int *data, size_t n) const
        {
            if (n <= this->TREASHOLD)
                return *min_element(data, data + n);
            size_t mid = n / 2;
            auto handle = async(launch::async, &AsyncCompression::worker, this, data, mid);
            int right_min = worker(data + mid, n - mid);
            return min(handle.get(), right_min);
        }

    public:
        int findMin(const vector<int> &values) const override
        {
            if (values.empty())
                return INT_MAX;
            return worker(values.data(), values.size());
        }
        string getName() const override { return "Recursive Async"; }
    };

    class StdParallel : public MinSearchAlgorithm
    {
    public:
        int findMin(const vector<int> &values) const override
        {
            if (values.empty())
                return INT_MAX;
            auto it = min_element(execution::par, values.begin(), values.end());
            return *it;
        }
        string getName() const override { return "STD Parallel (C++17)"; }
    };

    // --- 2. TESZTELŐ KERETRENDSZER ---

    double get_cpu_time_seconds()
    {
        struct timespec ts;
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
        return ts.tv_sec + ts.tv_nsec / 1e9;
    }

    void measureMinSearchAlgorithms(int min_vector_size, int max_vector_size, function<int(int)> size_multiplier, string folder_name, int execution_count, int timeout_sec)
    {
        // Algoritmusok regisztrálása polimorf módon
        vector<unique_ptr<MinSearchAlgorithm>> algorithms;
        algorithms.push_back(make_unique<SerialMin>());
        algorithms.push_back(make_unique<OmpCompression>());
        algorithms.push_back(make_unique<OmpReduction>());
        algorithms.push_back(make_unique<OmpSimd>());
        algorithms.push_back(make_unique<StdParallel>());
        algorithms.push_back(make_unique<AsyncCompression>());
        algorithms.push_back(make_unique<ThreadPoolReduction>());
        algorithms.push_back(make_unique<StridedCompression>());
        // algorithms.push_back(make_unique<ThreadPoolPairwise>());

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

            // Kiterjesztés módosítása .csv-re
            string filename = "../test_data/min_search/";
            if (folder_name != "")
            {
                filename += folder_name + "/";
            }
            filename += clean_name + "_results.csv";

            cout << filename << endl;

            alg_files[name].open(filename);
            if (alg_files[name].is_open())
            {
                // Szabványos CSV fejléc (nincsenek felesleges szóközök és vonalak)
                alg_files[name] << "Size,Wall_Avg_s,CPU_Work_s,Wall_Fast_s,Wall_Slow_s,Status\n";
            }

            final_status[name] = "SUCCESS";
            max_size_reached[name] = 0;
            total_wall_time[name] = 0.0;
            total_cpu_time[name] = 0.0;
            sizes_processed_count[name] = 0;
        }

        auto benchmark_start = chrono::high_resolution_clock::now();
        cout << "\n[INFO] Benchmarking started (Measuring Wall Time and CPU Process Time)..." << endl;

        int current_size = min_vector_size;
        SerialMin validator; // A helyesség ellenőrzéséhez a soros verziót használjuk bázisnak

        while (current_size <= max_vector_size)
        {
            vector<int> values(current_size);
            generate(values.begin(), values.end(), rand);

            int expected = validator.findMin(values);

            for (const auto &alg : algorithms)
            {
                string name = alg->getName();

                if (timed_out_algorithms.count(name))
                {
                    if (alg_files[name].is_open())
                    {
                        // Üres adatok (,,,,) a Pandas NaN konverzióhoz
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
                    auto wall_start = chrono::high_resolution_clock::now();
                    double cpu_start = get_cpu_time_seconds();

                    int result = alg->findMin(values);

                    double cpu_end = get_cpu_time_seconds();
                    auto wall_end = chrono::high_resolution_clock::now();

                    double wall_duration = chrono::duration<double>(wall_end - wall_start).count();
                    double cpu_duration = cpu_end - cpu_start;

                    if (result != expected)
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

        // --- TERMINÁL ÖSSZEFOGLALÓ JELENTÉS (Ez marad formázott!) ---
        auto benchmark_end = chrono::high_resolution_clock::now();
        double benchmark_duration = chrono::duration<double>(benchmark_end - benchmark_start).count();

        cout << "\n"
             << setfill('=') << setw(110) << "" << endl;
        cout << " FINAL BENCHMARK SUMMARY REPORT - benchmark duration: " << benchmark_duration << " (s)" << endl;
        cout << setfill('=') << setw(110) << "" << setfill(' ') << endl;
        cout << left << setw(25) << "Algorithm"
             << setw(20) << "Max Size Reached"
             << setw(22) << "Avg Wall Time (s)"
             << setw(22) << "Avg CPU Work (s)"
             << "Final Status" << endl;
        cout << string(110, '-') << endl;

        for (const auto &alg : algorithms)
        {
            string name = alg->getName();
            cout << left << setw(25) << name;
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

} // namespace minSearch