#include <algorithm>
#include <array>
#include <filesystem>
#include <set>
#include <atomic>
#include <chrono>
#include <climits>
#include <fstream>
#include <functional>
#include <future>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <omp.h>
#include <queue>
#include <thread>
#include <tuple>
#include <vector>
#include <random>

#include <ext/pb_ds/priority_queue.hpp>

#include "minSpanningTree.h"

namespace minSpanningTree
{
    using namespace std;
    namespace pb = __gnu_pbds;

    // =========================================================================
    // DSU SEGÉDOSZTÁLYOK
    // =========================================================================

    class DSU
    {
        vector<int> parent, rank;

    public:
        DSU(int n)
        {
            parent.resize(n);
            rank.resize(n, 1);
            for (int i = 0; i < n; i++)
                parent[i] = i;
        }
        int find(int i)
        {
            return (parent[i] == i) ? i : (parent[i] = find(parent[i]));
        }
        void unite(int x, int y)
        {
            int s1 = find(x), s2 = find(y);
            if (s1 != s2)
            {
                if (rank[s1] < rank[s2])
                    parent[s1] = s2;
                else if (rank[s1] > rank[s2])
                    parent[s2] = s1;
                else
                {
                    parent[s2] = s1;
                    rank[s1]++;
                }
            }
        }
    };

    class ParallelDSU
    {
        unique_ptr<atomic<int>[]> parent;
        vector<int> rank;
        vector<unique_ptr<mutex>> locks;

    public:
        ParallelDSU(int n)
        {
            parent = make_unique<atomic<int>[]>(n);
            rank.resize(n, 1);
            locks.reserve(n);
            for (int i = 0; i < n; i++)
            {
                parent[i].store(i, memory_order_relaxed);
                locks.emplace_back(make_unique<mutex>());
            }
        }

        int find(int i)
        {
            int curr = i;
            while (true)
            {
                int p = parent[curr].load(memory_order_relaxed);
                if (p == curr)
                    return curr;
                int gp = parent[p].load(memory_order_relaxed);
                parent[curr].store(gp, memory_order_relaxed);
                curr = p;
            }
        }

        bool unite(int x, int y)
        {
            while (true)
            {
                int s1 = find(x);
                int s2 = find(y);

                if (s1 == s2)
                    return false;

                scoped_lock lock(*locks[s1], *locks[s2]);

                int new_s1 = parent[s1].load(memory_order_relaxed);
                int new_s2 = parent[s2].load(memory_order_relaxed);
                if (new_s1 != s1 || new_s2 != s2)
                {
                    continue;
                }

                if (rank[s1] < rank[s2])
                {
                    parent[s1].store(s2, memory_order_relaxed);
                }
                else
                {
                    parent[s2].store(s1, memory_order_relaxed);
                    if (rank[s1] == rank[s2])
                        rank[s1]++;
                }
                return true;
            }
        }
    };

    // =========================================================================
    // 1. KRUSKAL SOLVER
    // =========================================================================

    class KruskalSolver
    {
    private:
        int V;
        const vector<vector<int>> &edges;

    public:
        KruskalSolver(int V, const vector<vector<int>> &edges) : V(V), edges(edges) {}

        long long solve()
        {
            auto cmp = [](const vector<int> &a, const vector<int> &b)
            { return a[2] > b[2]; };
            priority_queue<vector<int>, vector<vector<int>>, decltype(cmp)> pq(cmp);

            for (const auto &e : edges)
                pq.push(e);

            DSU dsu(V);
            long long cost = 0;
            int edgeCount = 0;

            while (!pq.empty() && edgeCount < V - 1)
            {
                vector<int> e = pq.top();
                pq.pop();
                if (dsu.find(e[0]) != dsu.find(e[1]))
                {
                    dsu.unite(e[0], e[1]);
                    cost += e[2];
                    edgeCount++;
                }
            }
            return cost;
        }
    };

    // =========================================================================
    // 2. MÁTRIX ÉS LISTA PRIM SOLVER
    // =========================================================================

    class PrimSolver
    {
    private:
        int V;
        const vector<vector<pair<int, int>>> &adj;
        const vector<vector<int>> &matrix;

    public:
        PrimSolver(int V, const vector<vector<pair<int, int>>> &adj, const vector<vector<int>> &matrix)
            : V(V), adj(adj), matrix(matrix) {}

        long long solveMatrix()
        {
            if (V >= 10000)
            {
                cout << "Túl nagy gráf - nem fut a prim";
                return 0;
            }

            vector<int> key(V, INT_MAX);
            vector<bool> mstSet(V, false);
            key[0] = 0;
            long long totalCost = 0;

            for (int count = 0; count < V; count++)
            {
                int min = INT_MAX, u;
                for (int v = 0; v < V; v++)
                    if (!mstSet[v] && key[v] < min)
                        min = key[v], u = v;

                if (min == INT_MAX)
                    break;

                mstSet[u] = true;
                totalCost += key[u];

                for (int v = 0; v < V; v++)
                    if (matrix[u][v] && !mstSet[v] && matrix[u][v] < key[v])
                        key[v] = matrix[u][v];
            }
            return totalCost;
        }

        long long solveList()
        {
            priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq;
            vector<bool> visited(V, false);
            long long totalCost = 0;

            pq.push({0, 0});
            while (!pq.empty())
            {
                auto [wt, u] = pq.top();
                pq.pop();

                if (visited[u])
                    continue;
                visited[u] = true;
                totalCost += wt;

                for (auto &[v, weight] : adj[u])
                {
                    if (!visited[v])
                        pq.push({weight, v});
                }
            }
            return totalCost;
        }
    };

    // =========================================================================
    // 3. BORUVKA SOLVER (SZEKVENCIÁLIS ÉS PÁRHUZAMOS)
    // =========================================================================

    class BoruvkaSolver
    {
    private:
        int V;
        const vector<vector<int>> &edges;

    public:
        BoruvkaSolver(int V, const vector<vector<int>> &edges) : V(V), edges(edges) {}

        long long solveStandard()
        {
            DSU dsu(V);
            int numTrees = V;
            long long totalWeight = 0;

            while (numTrees > 1)
            {
                vector<vector<int>> cheapest(V, vector<int>(3, -1));

                for (const auto &e : edges)
                {
                    int set1 = dsu.find(e[0]);
                    int set2 = dsu.find(e[1]);
                    if (set1 != set2)
                    {
                        if (cheapest[set1][2] == -1 || cheapest[set1][2] > e[2])
                            cheapest[set1] = e;
                        if (cheapest[set2][2] == -1 || cheapest[set2][2] > e[2])
                            cheapest[set2] = e;
                    }
                }

                bool added = false;
                for (int i = 0; i < V; i++)
                {
                    if (cheapest[i][2] != -1)
                    {
                        int set1 = dsu.find(cheapest[i][0]);
                        int set2 = dsu.find(cheapest[i][1]);
                        if (set1 != set2)
                        {
                            totalWeight += cheapest[i][2];
                            dsu.unite(set1, set2);
                            numTrees--;
                            added = true;
                        }
                    }
                }
                if (!added)
                    break;
            }
            return totalWeight;
        }

        long long solveNaivParallel()
        {
            ParallelDSU dsu(V);
            int numTrees = V;
            long long totalWeight = 0;

            while (numTrees > 1)
            {
                vector<vector<int>> cheapest(V, vector<int>(3, INT_MAX));

#pragma omp parallel for
                for (size_t i = 0; i < edges.size(); ++i)
                {
                    const auto &e = edges[i];
                    int set1 = dsu.find(e[0]);
                    int set2 = dsu.find(e[1]);

                    if (set1 != set2)
                    {
#pragma omp critical
                        {
                            if (cheapest[set1][2] == INT_MAX || cheapest[set1][2] > e[2])
                                cheapest[set1] = e;
                            if (cheapest[set2][2] == INT_MAX || cheapest[set2][2] > e[2])
                                cheapest[set2] = e;
                        }
                    }
                }

                bool added = false;
                long long weight_delta = 0;
                int trees_merged = 0;

#pragma omp parallel for reduction(+ : weight_delta, trees_merged) reduction(|| : added)
                for (int i = 0; i < V; i++)
                {
                    if (cheapest[i][2] != INT_MAX)
                    {
                        int set1 = dsu.find(cheapest[i][0]);
                        int set2 = dsu.find(cheapest[i][1]);

                        if (set1 != set2)
                        {
                            if (dsu.unite(set1, set2))
                            {
                                weight_delta += cheapest[i][2];
                                trees_merged++;
                                added = true;
                            }
                        }
                    }
                }

                totalWeight += weight_delta;
                numTrees -= trees_merged;
                if (!added)
                    break;
            }
            return totalWeight;
        }

        long long solveBufferedParallel()
        {
            ParallelDSU dsu(V);
            long long totalWeight = 0;
            int numTrees = V;
            int num_threads = omp_get_max_threads();

            while (numTrees > 1)
            {
                vector<vector<vector<int>>> local_cheapest(num_threads, vector<vector<int>>(V, {-1, -1, -1}));

#pragma omp parallel
                {
                    int tid = omp_get_thread_num();
#pragma omp for
                    for (size_t i = 0; i < edges.size(); ++i)
                    {
                        int u = edges[i][0], v = edges[i][1], w = edges[i][2];
                        int set1 = dsu.find(u), set2 = dsu.find(v);

                        if (set1 != set2)
                        {
                            if (local_cheapest[tid][set1][2] == -1 || local_cheapest[tid][set1][2] > w)
                                local_cheapest[tid][set1] = edges[i];
                            if (local_cheapest[tid][set2][2] == -1 || local_cheapest[tid][set2][2] > w)
                                local_cheapest[tid][set2] = edges[i];
                        }
                    }
                }

                vector<vector<int>> global_cheapest(V, {-1, -1, -1});
#pragma omp parallel for
                for (int i = 0; i < V; ++i)
                {
                    for (int t = 0; t < num_threads; ++t)
                    {
                        if (local_cheapest[t][i][2] != -1)
                        {
                            if (global_cheapest[i][2] == -1 || global_cheapest[i][2] > local_cheapest[t][i][2])
                                global_cheapest[i] = local_cheapest[t][i];
                        }
                    }
                }

                bool added = false;
                long long weight_delta = 0;
                int trees_merged = 0;
#pragma omp parallel for reduction(+ : weight_delta, trees_merged) reduction(|| : added)
                for (int i = 0; i < V; i++)
                {
                    if (global_cheapest[i][2] != -1)
                    {
                        if (dsu.unite(global_cheapest[i][0], global_cheapest[i][1]))
                        {
                            weight_delta += global_cheapest[i][2];
                            trees_merged++;
                            added = true;
                        }
                    }
                }

                totalWeight += weight_delta;
                numTrees -= trees_merged;
                if (!added)
                    break;
            }
            return totalWeight;
        }

        long long solveCasParallel()
        {
            ParallelDSU dsu(V);
            long long totalWeight = 0;
            int numTrees = V;

            while (numTrees > 1)
            {
                vector<atomic<long long>> cheapest(V);
                for (int i = 0; i < V; ++i)
                    cheapest[i].store(LLONG_MAX);

#pragma omp parallel for
                for (size_t i = 0; i < edges.size(); ++i)
                {
                    int u = edges[i][0], v = edges[i][1], w = edges[i][2];
                    int set1 = dsu.find(u), set2 = dsu.find(v);

                    if (set1 != set2)
                    {
                        long long new_val = ((long long)w << 32) | (i & 0xFFFFFFFFLL);

                        for (int set : {set1, set2})
                        {
                            long long current = cheapest[set].load(memory_order_relaxed);
                            while (true)
                            {
                                if (current != LLONG_MAX && (current >> 32) <= w)
                                    break;
                                if (cheapest[set].compare_exchange_weak(current, new_val))
                                    break;
                            }
                        }
                    }
                }

                bool added = false;
                long long weight_delta = 0;
                int trees_merged = 0;

#pragma omp parallel for reduction(+ : weight_delta, trees_merged) reduction(|| : added)
                for (int i = 0; i < V; i++)
                {
                    long long val = cheapest[i].load();
                    if (val != LLONG_MAX)
                    {
                        int edge_idx = (int)(val & 0xFFFFFFFFLL);
                        const auto &e = edges[edge_idx];
                        if (dsu.unite(e[0], e[1]))
                        {
                            weight_delta += e[2];
                            trees_merged++;
                            added = true;
                        }
                    }
                }

                totalWeight += weight_delta;
                numTrees -= trees_merged;
                if (!added)
                    break;
            }
            return totalWeight;
        }
    };

    // =========================================================================
    // 4. SETIA Prim SOLVER
    // =========================================================================

    using EdgeType = pair<int, pair<int, int>>;
    using PairingHeap = pb::priority_queue<EdgeType, greater<EdgeType>, pb::pairing_heap_tag>;

    struct ThreadData
    {
        mutex mtx;
        bool aborted = false;
        long long mst_weight = 0;
        PairingHeap pq;
        int tree_size = 0;
        int small_tree_count = 0;
        int root_search_count = 0;
    };

    struct NodeData
    {
        mutex mtx;
        int color = -1;
    };

    class SetiaMSTSolver
    {
    private:
        int V;
        const vector<vector<pair<int, int>>> &adj;
        int THRESHOLD;

        vector<unique_ptr<ThreadData>> threads;
        vector<unique_ptr<NodeData>> nodes;
        unique_ptr<ParallelDSU> dsu;
        atomic<int> uncolored_count;

        void reset_state()
        {
            int num_threads = omp_get_max_threads();
            threads.clear();
            threads.resize(num_threads);
            for (int i = 0; i < num_threads; ++i)
                threads[i] = make_unique<ThreadData>();

            nodes.clear();
            nodes.resize(V);
            for (int i = 0; i < V; ++i)
                nodes[i] = make_unique<NodeData>();

            dsu = make_unique<ParallelDSU>(V);
            uncolored_count.store(V, memory_order_relaxed);
        }

        bool try_acquire_root(int root, int tid, bool is_heuristic)
        {
            int root_set = dsu->find(root);
            lock_guard<mutex> lk(nodes[root_set]->mtx);
            if (nodes[root_set]->color == -1)
            {
                nodes[root_set]->color = tid;
                if (!is_heuristic)
                    uncolored_count--;
                return true;
            }
            return false;
        }

        void init_thread(int tid, int root)
        {
            auto &my_thread = *threads[tid];
            lock_guard<mutex> lk(my_thread.mtx);
            my_thread.aborted = false;
            my_thread.mst_weight = 0;
            my_thread.tree_size = 1;

            my_thread.pq.clear();
            for (auto &edge : adj[root])
            {
                my_thread.pq.push({edge.second, {root, edge.first}});
            }
        }

        bool resolve_collision(int tid, int j, int u, int v, int w, int root_u, int root_v, bool is_heuristic)
        {
            auto &my_thread = *threads[tid];
            int m1 = min(tid, j);
            int m2 = max(tid, j);
            int rm1 = min(root_u, root_v);
            int rm2 = max(root_u, root_v);

            scoped_lock lk(threads[m1]->mtx, threads[m2]->mtx, nodes[rm1]->mtx, nodes[rm2]->mtx);

            if (my_thread.aborted)
                return true;

            if (is_heuristic)
            {
                my_thread.small_tree_count = (my_thread.tree_size < THRESHOLD) ? (my_thread.small_tree_count + 1) : 0;
                threads[j]->small_tree_count = (threads[j]->tree_size < THRESHOLD) ? (threads[j]->small_tree_count + 1) : 0;
            }

            if (dsu->find(u) == root_u && dsu->find(v) == root_v && nodes[root_v]->color == j && !threads[j]->aborted)
            {
                if (tid < j)
                {
                    my_thread.mst_weight += w + threads[j]->mst_weight;
                    my_thread.pq.pop();
                    if (is_heuristic)
                    {
                        my_thread.tree_size += threads[j]->tree_size;
                        threads[j]->tree_size = 0;
                    }
                    my_thread.pq.join(threads[j]->pq);
                    threads[j]->aborted = true;

                    nodes[root_v]->color = tid;
                    nodes[root_u]->color = tid;
                    dsu->unite(root_u, root_v);
                    return false;
                }
                else
                {
                    threads[j]->mst_weight += w + my_thread.mst_weight;
                    my_thread.pq.pop();
                    if (is_heuristic)
                    {
                        threads[j]->tree_size += my_thread.tree_size;
                        my_thread.tree_size = 0;
                    }
                    threads[j]->pq.join(my_thread.pq);
                    my_thread.aborted = true;

                    nodes[root_u]->color = j;
                    nodes[root_v]->color = j;
                    dsu->unite(root_u, root_v);
                    return true;
                }
            }
            return false;
        }

        void run_inner_prim(int tid, bool is_heuristic)
        {
            auto &my_thread = *threads[tid];

            while (true)
            {
                EdgeType min_edge;
                {
                    lock_guard<mutex> lk(my_thread.mtx);
                    if (my_thread.aborted || my_thread.pq.empty())
                        break;
                    min_edge = my_thread.pq.top();
                }

                int w = min_edge.first;
                int u = min_edge.second.first;
                int v = min_edge.second.second;

                int root_u = dsu->find(u);
                int root_v = dsu->find(v);

                if (root_u == root_v)
                {
                    lock_guard<mutex> lk(my_thread.mtx);
                    if (!my_thread.aborted)
                        my_thread.pq.pop();
                    continue;
                }

                int c = -2;
                {
                    scoped_lock lk(my_thread.mtx, nodes[root_v]->mtx);
                    if (my_thread.aborted)
                        break;
                    if (dsu->find(v) != root_v)
                        continue;

                    c = nodes[root_v]->color;
                    if (c == -1)
                    {
                        nodes[root_v]->color = tid;
                        if (!is_heuristic)
                            uncolored_count--;
                        my_thread.mst_weight += w;
                        my_thread.pq.pop();

                        for (auto &edge : adj[v])
                        {
                            my_thread.pq.push({edge.second, {v, edge.first}});
                        }
                        if (is_heuristic)
                            my_thread.tree_size++;
                    }
                }

                if (c == -1)
                {
                    dsu->unite(root_u, root_v);
                    continue;
                }

                if (c == tid)
                {
                    lock_guard<mutex> lk(my_thread.mtx);
                    if (!my_thread.aborted)
                        my_thread.pq.pop();
                    continue;
                }

                bool aborted = resolve_collision(tid, c, u, v, w, root_u, root_v, is_heuristic);
                if (aborted)
                    break;
            }
        }

        long long get_final_weight()
        {
            for (size_t i = 0; i < threads.size(); ++i)
            {
                if (!threads[i]->aborted && threads[i]->mst_weight > 0)
                {
                    return threads[i]->mst_weight;
                }
            }
            return 0;
        }

    public:
        SetiaMSTSolver(int V, const vector<vector<pair<int, int>>> &adj, int threshold)
            : V(V), adj(adj), THRESHOLD(threshold) {}

        long long solveStandard()
        {
            reset_state();

#pragma omp parallel
            {
                int tid = omp_get_thread_num();
                thread_local mt19937 gen(random_device{}() + tid);
                uniform_int_distribution<int> dist(0, V - 1);

                while (uncolored_count.load(memory_order_relaxed) > 0)
                {
                    int root = -1;
                    for (int attempt = 0; attempt < 50; ++attempt)
                    {
                        int r = dist(gen);
                        if (nodes[dsu->find(r)]->color == -1)
                        {
                            root = r;
                            break;
                        }
                    }
                    if (root == -1)
                    {
                        for (int i = 0; i < V; ++i)
                        {
                            if (nodes[dsu->find(i)]->color == -1)
                            {
                                root = i;
                                break;
                            }
                        }
                    }
                    if (root == -1)
                        break;

                    if (!try_acquire_root(root, tid, false))
                        continue;

                    init_thread(tid, root);
                    run_inner_prim(tid, false);
                }
            }
            return get_final_weight();
        }

        long long solveHeuristic()
        {
            reset_state();

#pragma omp parallel
            {
                int tid = omp_get_thread_num();
                auto &my_thread = *threads[tid];
                thread_local mt19937 gen(random_device{}() + tid);
                uniform_int_distribution<int> dist(0, V - 1);

                while (my_thread.small_tree_count < THRESHOLD && my_thread.root_search_count < THRESHOLD)
                {
                    int root = -1;
                    int r = dist(gen);
                    int r_set = dsu->find(r);

                    if (nodes[r_set]->color == -1)
                    {
                        root = r;
                        my_thread.root_search_count = 0;
                    }
                    else
                    {
                        my_thread.root_search_count++;
                        continue;
                    }

                    if (!try_acquire_root(root, tid, true))
                        continue;

                    init_thread(tid, root);
                    run_inner_prim(tid, true);
                }
            }
            return get_final_weight();
        }
    };

    // =========================================================================
    // GRAPH OSZTÁLY
    // =========================================================================

    class Graph
    {
    private:
        int V;
        vector<vector<int>> edges;
        vector<vector<pair<int, int>>> adj;
        vector<vector<int>> matrix;

        const int TREASHOLD = 100;

    public:
        Graph(int vertices) : V(vertices)
        {
            adj.resize(V);
            if (V < 10000)
            {
                matrix.resize(V, vector<int>(V, 0));
            }
        }

        void addEdge(int u, int v, int w)
        {
            edges.push_back({u, v, w});
            adj[u].push_back({v, w});
            adj[v].push_back({u, w});
            if (V < 10000)
            {
                matrix[u][v] = w;
                matrix[v][u] = w;
            }
        }

        // Tiszta API delegálások a Solver osztályok felé
        long long kruskalMST() { return KruskalSolver(V, edges).solve(); }

        long long primMSTMatrix() { return PrimSolver(V, adj, matrix).solveMatrix(); }
        long long primMSTOptimal() { return PrimSolver(V, adj, matrix).solveList(); }

        long long parallelPrimSetiaMST() { return SetiaMSTSolver(V, adj, TREASHOLD).solveStandard(); }
        long long parallelPrimHeuristicSetiaMST() { return SetiaMSTSolver(V, adj, TREASHOLD).solveHeuristic(); }

        long long boruvkaMST() { return BoruvkaSolver(V, edges).solveStandard(); }
        long long naivParallelBoruvkaMST() { return BoruvkaSolver(V, edges).solveNaivParallel(); }
        long long bufferedParallelBoruvkaMST() { return BoruvkaSolver(V, edges).solveBufferedParallel(); }
        long long casParallelBoruvkaMST() { return BoruvkaSolver(V, edges).solveCasParallel(); }
    };

    // =========================================================================
    // TESZTELÉS ÉS ADMINISZTRÁCIÓ
    // =========================================================================

    namespace fs = std::filesystem;

    double get_cpu_time_seconds()
    {
        struct timespec ts;
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
        return ts.tv_sec + ts.tv_nsec / 1e9;
    }

    pair<int, vector<array<int, 3>>> read_MST_test(const string &file_name, const string &input_dir)
    {
        ifstream file(input_dir + file_name);
        if (!file.is_open())
        {
            cerr << "Error: Could not open file " << file_name << endl;
            return {0, {}};
        }

        vector<array<int, 3>> result;
        int n = 0, m = 0;

        if (!(file >> n >> m))
            return {0, {}};

        int u, v, w;
        int real_max_v = 0;
        while (file >> u >> v >> w)
        {
            int u_zero = u - 1;
            int v_zero = v - 1;

            result.push_back({u_zero, v_zero, w});
            if (u_zero > real_max_v)
                real_max_v = u_zero;
            if (v_zero > real_max_v)
                real_max_v = v_zero;
        }

        n = max(n, real_max_v + 1);

        file.close();
        return {n, result};
    }

    struct TestCase
    {
        string filename;
        int V;
        int E;
        vector<array<int, 3>> edges;
    };

    void measureAllMSTAlgorithms(string folder_name, int execution_count, int timeout_sec)
    {
        string input_dir = "../test_input/" + folder_name + "/";
        string output_dir = "../test_data/mst/" + folder_name + "/";

        vector<string> test_files;
        if (!fs::exists(input_dir))
        {
            cerr << "Hiba: A teszt mappa nem talalhato: " << input_dir << endl;
            return;
        }

        cout << "[INFO] Tesztfájlok neveinek beolvasása a " << input_dir << " mappából..." << endl;
        for (const auto &entry : fs::directory_iterator(input_dir))
        {
            if (entry.path().extension() == ".in")
            {
                test_files.push_back(entry.path().filename().string());
            }
        }

        if (test_files.empty())
        {
            cout << "Nem talalhato ervenyes .in fajl a mappaban!" << endl;
            return;
        }

        sort(test_files.begin(), test_files.end());

        struct AlgDef
        {
            string name;
            function<long long(Graph &)> func;
        };

        vector<AlgDef> algorithms = {
            {"Kruskal", [](Graph &g)
             { return g.kruskalMST(); }},
            {"Prim Matrix", [](Graph &g)
             { return g.primMSTMatrix(); }},
            {"Prim List", [](Graph &g)
             { return g.primMSTOptimal(); }},
            {"Parallel Prim", [](Graph &g)
             { return g.parallelPrimSetiaMST(); }},
            {"Parallel Prim Heuristic", [](Graph &g)
             { return g.parallelPrimHeuristicSetiaMST(); }},
            {"Boruvka", [](Graph &g)
             { return g.boruvkaMST(); }},
            {"Naiv Parallel Boruvka", [](Graph &g)
             { return g.naivParallelBoruvkaMST(); }},
            {"Buffered Parallel Boruvka", [](Graph &g)
             { return g.bufferedParallelBoruvkaMST(); }},
            {"CAS Parallel Boruvka", [](Graph &g)
             { return g.casParallelBoruvkaMST(); }}};

        map<string, ofstream> alg_files;
        map<string, double> total_wall_time;
        map<string, double> total_cpu_time;
        map<string, int> files_processed_count;
        map<string, string> max_graph_reached;
        map<string, string> final_status;
        set<string> timed_out_algorithms;

        fs::create_directories(output_dir);

        for (const auto &alg : algorithms)
        {
            string clean_name = alg.name;
            replace(clean_name.begin(), clean_name.end(), ' ', '_');
            string filename = output_dir + clean_name + "_results.csv";

            alg_files[alg.name].open(filename);
            if (alg_files[alg.name].is_open())
            {
                alg_files[alg.name] << "File,Vertices,Edges,Wall_Avg_s,CPU_Work_s,Wall_Fast_s,Wall_Slow_s,Status\n";
            }

            final_status[alg.name] = "SUCCESS";
            max_graph_reached[alg.name] = "None";
            total_wall_time[alg.name] = 0.0;
            total_cpu_time[alg.name] = 0.0;
            files_processed_count[alg.name] = 0;
        }

        auto benchmark_start = chrono::high_resolution_clock::now();
        cout << "\n[INFO] MST Benchmarking started..." << endl;

        for (const string &current_filename : test_files)
        {
            auto graph_info = read_MST_test(current_filename, input_dir);
            int current_V = graph_info.first;
            const auto &current_edges = graph_info.second;
            int current_E = current_edges.size();

            if (current_V == 0)
            {
                cerr << "[Hiba] Nem sikerult beolvasni vagy ures a fajl: " << current_filename << endl;
                continue;
            }

            cout << "\n--- Teszteles: " << current_filename << " (V: " << current_V << ", E: " << current_E << ") ---" << endl;

            Graph base_graph(current_V);
            for (const auto &edge : current_edges)
            {
                base_graph.addEdge(edge[0], edge[1], edge[2]);
            }
            long long expected_cost = base_graph.kruskalMST();

            for (const auto &alg : algorithms)
            {
                string name = alg.name;

                if (timed_out_algorithms.count(name))
                {
                    if (alg_files[name].is_open())
                    {
                        alg_files[name] << current_filename << "," << current_V << "," << current_E << ",,,,,SKIPPED\n";
                    }
                    continue;
                }

                if (name == "Prim Matrix" && current_V > 10000)
                {
                    timed_out_algorithms.insert(name);
                    final_status[name] = "SKIPPED (>10k Vertices)";
                    if (alg_files[name].is_open())
                        alg_files[name] << current_filename << "," << current_V << "," << current_E << ",,,,,SKIPPED\n";
                    continue;
                }

                double sum_wall_time = 0, sum_cpu_time = 0;
                double fastest_wall = 1e9, slowest_wall = 0;
                bool failed = false, timed_out = false;
                int actual_iterations = 0;

                for (int i = 0; i < execution_count; ++i)
                {
                    Graph g(current_V);
                    for (const auto &edge : current_edges)
                    {
                        g.addEdge(edge[0], edge[1], edge[2]);
                    }

                    auto wall_start = chrono::high_resolution_clock::now();
                    double cpu_start = get_cpu_time_seconds();

                    long long result_cost = alg.func(g);

                    double cpu_end = get_cpu_time_seconds();
                    auto wall_end = chrono::high_resolution_clock::now();

                    double wall_duration = chrono::duration<double>(wall_end - wall_start).count();
                    double cpu_duration = cpu_end - cpu_start;

                    if (result_cost != expected_cost)
                    {
                        failed = true;
                        final_status[name] = "FAILED (Cost: " + to_string(result_cost) + " != " + to_string(expected_cost) + ")";
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
                    alg_files[name] << current_filename << "," << current_V << "," << current_E << ",";
                    if (timed_out)
                        alg_files[name] << ",,,,TIMEOUT\n";
                    else if (failed)
                        alg_files[name] << ",,,,FAILED\n";
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
                    max_graph_reached[name] = current_filename;
                    total_wall_time[name] += (sum_wall_time / actual_iterations);
                    total_cpu_time[name] += (sum_cpu_time / actual_iterations);
                    files_processed_count[name]++;
                }
            }
        }

        auto benchmark_end = chrono::high_resolution_clock::now();
        double benchmark_duration = chrono::duration<double>(benchmark_end - benchmark_start).count();

        cout << "\n"
             << setfill('=') << setw(120) << "" << endl;
        cout << " FINAL MST BENCHMARK SUMMARY REPORT - Total Time: " << benchmark_duration << " (s)" << endl;
        cout << setfill('=') << setw(120) << "" << setfill(' ') << endl;
        cout << left << setw(30) << "Algorithm"
             << setw(20) << "Max File Reached"
             << setw(20) << "Total Avg Wall(s)"
             << setw(20) << "Total Avg CPU(s)"
             << "Final Status" << endl;
        cout << string(120, '-') << endl;

        for (const auto &alg : algorithms)
        {
            string name = alg.name;
            cout << left << setw(30) << name;
            cout << setw(20) << max_graph_reached[name];

            if (files_processed_count[name] > 0)
            {
                cout << fixed << setprecision(6)
                     << setw(20) << (total_wall_time[name] / files_processed_count[name])
                     << setw(20) << (total_cpu_time[name] / files_processed_count[name]);
            }
            else
            {
                cout << setw(20) << "-" << setw(20) << "-";
            }

            if (final_status[name] == "TIMEOUT" || final_status[name].find("SKIPPED") != string::npos)
                cout << "\033[33m" << final_status[name] << "\033[0m" << endl;
            else if (final_status[name].find("FAILED") != string::npos)
                cout << "\033[31m" << final_status[name] << "\033[0m" << endl;
            else
                cout << "\033[32mSUCCESS\033[0m" << endl;

            if (alg_files[name].is_open())
                alg_files[name].close();
        }
        cout << setfill('=') << setw(120) << "" << setfill(' ') << endl;
    }

} // namespace minSpanningTree