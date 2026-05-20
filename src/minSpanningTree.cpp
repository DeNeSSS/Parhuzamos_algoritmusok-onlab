#include <algorithm>
#include <array>
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

namespace minSpanningTree
{
    using namespace std;

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

    // Közös Disjoint Set Union segédosztály
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
            int root = i;
            while (parent[root].load(memory_order_relaxed) != root)
            {
                root = parent[root].load(memory_order_relaxed);
            }

            int curr = i;
            while (parent[curr].load(memory_order_relaxed) != root)
            {
                int next = parent[curr].load(memory_order_relaxed);
                parent[curr].store(root, memory_order_relaxed);
                curr = next;
            }
            return root;
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
                    continue; // Vissza a while(true) elejére
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

    class Graph
    {
    private:
        int V;
        // 1. Éllista (u, v, w) - Kruskal és Boruvka számára
        vector<vector<int>> edges;
        // 2. Szomszédsági lista (szomszéd, súly) - Optimális Prim számára
        vector<vector<pair<int, int>>> adj;
        // 3. Szomszédsági mátrix - Klasszikus Prim számára
        vector<vector<int>> matrix;

    public:
        Graph(int vertices) : V(vertices)
        {
            adj.resize(V);
            // Csak akkor foglaljunk mátrixot, ha kicsi a gráf!
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

        // --- KRUSKAL ALGORITMUS ---
        long long kruskalMST()
        {
            // Prioritási sor az élek súly szerinti rendezéséhez
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

        // --- KLASSZIKUS PRIM (Mátrixszal, O(V^2)) ---
        long long primMSTMatrix()
        {
            if (V > 10000)
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
                    break; // Gráf nem összefüggő

                mstSet[u] = true;
                totalCost += key[u];

                for (int v = 0; v < V; v++)
                    if (matrix[u][v] && !mstSet[v] && matrix[u][v] < key[v])
                        key[v] = matrix[u][v];
            }
            return totalCost;
        }

        // --- OPTIMÁLIS PRIM (Szomszédsági listával és kupaccal, O(E log V)) ---
        long long primMSTOptimal()
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

        // --- BORUVKA ALGORITMUS ---
        long long boruvkaMST()
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
                    break; // Nem összefüggő gráf
            }
            return totalWeight;
        }

        long long naivParallelBoruvkaMST()
        {
            ParallelDSU dsu(V);
            int numTrees = V;
            long long totalWeight = 0;

            while (numTrees > 1)
            {
                vector<vector<int>> cheapest(V, vector<int>(3, -1));

#pragma omp parallel for
                for (size_t i = 0; i < edges.size(); ++i) // size_t jobb az OMP-hez
                {
                    const auto &e = edges[i];
                    int set1 = dsu.find(e[0]);
                    int set2 = dsu.find(e[1]);

                    if (set1 != set2)
                    {
#pragma omp critical
                        {
                            if (cheapest[set1][2] == -1 || cheapest[set1][2] > e[2])
                                cheapest[set1] = e;
                            if (cheapest[set2][2] == -1 || cheapest[set2][2] > e[2])
                                cheapest[set2] = e;
                        }
                    }
                }

                bool added = false;
                int weight_delta = 0;
                int trees_merged = 0;

// A reduction(|| : added) automatikusan megoldja a boolean flag problémát!
#pragma omp parallel for reduction(+ : weight_delta, trees_merged) reduction(|| : added)
                for (int i = 0; i < V; i++)
                {
                    if (cheapest[i][2] != -1)
                    {
                        // lock-free keresés
                        int set1 = dsu.find(cheapest[i][0]);
                        int set2 = dsu.find(cheapest[i][1]);

                        if (set1 != set2)
                        {
                            // FIGYELEM: A unite-nak bool-lal kell visszatérnie!
                            // Csak akkor vonjuk be a statisztikába, ha EZ a szál csinálta az egyesítést.
                            if (dsu.unite(set1, set2))
                            {
                                weight_delta += cheapest[i][2];
                                trees_merged++;
                                added = true; // A reduction(||) miatt ez teljesen biztonságos
                            }
                        }
                    }
                }

                // A ciklus után frissítjük a globális változókat
                totalWeight += weight_delta;
                numTrees -= trees_merged; // Kivonjuk az egyesített fák számát
                if (!added)
                    break; // Nem összefüggő gráf
            }
            return totalWeight;
        }

        long long bufferedParallelBoruvkaMST()
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

        long long casParallelBoruvkaMST()
        {
            ParallelDSU dsu(V);
            long long totalWeight = 0;
            int numTrees = V;

            while (numTrees > 1)
            {
                // Atomi élek: felső 32 bit a súly, alsó 32 bit az él indexe
                // LLONG_MAX-szal inicializálunk (ez jelenti a -1-et)
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

                        // Atomi "update if smaller" logika (CAS) mindkét komponensre
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

    int test()
    {
        minSpanningTree::Graph g(4);
        g.addEdge(0, 1, 10);
        g.addEdge(0, 2, 6);
        g.addEdge(0, 3, 5);
        g.addEdge(1, 3, 15);
        g.addEdge(2, 3, 4);

        std::cout << "Kruskal cost: " << g.kruskalMST() << std::endl;
        std::cout << "Prim Matrix cost: " << g.primMSTMatrix() << std::endl;
        std::cout << "Prim Optimal cost: " << g.primMSTOptimal() << std::endl;
        std::cout << "Boruvka cost: " << g.boruvkaMST() << std::endl;
        std::cout << "Parallel Boruvka cost: " << g.naivParallelBoruvkaMST() << std::endl;

        return 0;
    }

    pair<int, vector<array<int, 3>>> read_MST_test(const string &file_name)
    {
        ifstream file("../test_input/spanning_tree_test/" + file_name);
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

    void testMSTAlgorithms(int execution_count, int timeout_sec)
    {
        // 1. Gráf beolvasása és felépítése
        auto graph_info = read_MST_test("custom_test.in");
        int V = graph_info.first;
        const auto &edges_data = graph_info.second;

        // Létrehozunk egy alap gráfot, amit referenciaként használunk
        minSpanningTree::Graph g(V);
        for (const auto &edge : edges_data)
        {
            g.addEdge(edge[0], edge[1], edge[2]);
        }

        // 2. Függvények regisztrálása (lambda-k segítségével, hogy a 'g' példányon fussanak)
        // Mivel az MST függvények int-et adnak vissza, a map-et is ehhez igazítjuk
        map<string, function<long long()>> functions = {
            {"Kruskal", [&]()
             { return g.kruskalMST(); }},
            {"Prim (matrix)", [&]()
             { return g.primMSTMatrix(); }},
            {"Prim (list)", [&]()
             { return g.primMSTOptimal(); }},
            {"Boruvka", [&]()
             { return g.boruvkaMST(); }},
            {"Naiv parallel Boruvka", [&]()
             { return g.naivParallelBoruvkaMST(); }},
            {"Buffered parallel Boruvka", [&]()
             { return g.bufferedParallelBoruvkaMST(); }},
            {"CAS parallel Boruvka", [&]()
             { return g.casParallelBoruvkaMST(); }},
        };

        cout << "\n"
             << setfill('=') << setw(100) << "" << endl;
        cout << " MST ALGORITHM BENCHMARK | Vertices: " << V << " | Iterations: " << execution_count << endl;
        cout << setfill('=') << setw(100) << "" << setfill(' ') << endl;
        cout << left << setw(25) << "Algorithm"
             << setw(15) << "Result (Cost)"
             << setw(15) << "Avg Time (s)"
             << setw(15) << "Fastest (s)"
             << "Status" << endl;
        cout << string(100, '-') << endl;

        // Tároljuk az első sikeres futás eredményét, hogy ellenőrizzük a többiek helyességét
        long long reference_cost = -1;

        for (auto const &[name, func] : functions)
        {
            double total_time = 0, fastest = 1e9, slowest = 0;
            bool failed = false, timed_out = false;
            int current_result = 0;

            for (int i = 0; i < execution_count; ++i)
            {
                auto start = chrono::high_resolution_clock::now();

                // Async hívás a timeout kezeléséhez
                future<long long> fut = async(launch::async, func);

                if (fut.wait_for(chrono::seconds(timeout_sec)) == future_status::timeout)
                {
                    timed_out = true;
                    break;
                }

                current_result = fut.get();

                auto end = chrono::high_resolution_clock::now();
                double duration = chrono::duration<double>(end - start).count();

                // Eredmény validálása: minden algoritmusnak ugyanazt a költséget kell visszaadnia
                if (reference_cost == -1)
                    reference_cost = current_result;
                if (current_result != reference_cost)
                {
                    failed = true;
                    break;
                }

                total_time += duration;
                fastest = min(fastest, duration);
                slowest = max(slowest, duration);
            }

            cout << left << setw(25) << name;

            if (timed_out)
            {
                cout << setw(15) << "-" << "\033[33mTIMEOUT\033[0m" << endl;
            }
            else if (failed)
            {
                cout << setw(15) << current_result << "\033[31mFAILED (Wrong Cost)\033[0m" << endl;
            }
            else
            {
                cout << setw(15) << current_result
                     << fixed << setprecision(6)
                     << setw(15) << (total_time / execution_count)
                     << setw(15) << fastest
                     << "\033[32mSUCCESS\033[0m" << endl;
            }
        }
        cout << setfill('=') << setw(100) << "" << setfill(' ') << endl;
    }
} // namespace minSpanningTree