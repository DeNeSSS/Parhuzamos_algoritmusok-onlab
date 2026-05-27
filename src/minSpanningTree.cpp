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

#include "minSpanningTree.h"

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

        const int TREASHOLD = 100;

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

        // --- SETIA ET AL. PARALLEL PRIM ALGORITMUS ---
        long long parallelPrimSetiaMST()
        {
            int num_threads = omp_get_max_threads();

            struct ThreadData
            {
                mutex mtx;
                bool aborted = false;
                long long mst_weight = 0;
                priority_queue<pair<int, pair<int, int>>,
                               vector<pair<int, pair<int, int>>>,
                               greater<>>
                    pq;
            };
            vector<unique_ptr<ThreadData>> threads(num_threads);
            for (int i = 0; i < num_threads; ++i)
                threads[i] = make_unique<ThreadData>();

            struct NodeData
            {
                mutex mtx;
                int color = -1;
            };
            vector<unique_ptr<NodeData>> nodes(V);
            for (int i = 0; i < V; ++i)
                nodes[i] = make_unique<NodeData>();

            ParallelDSU dsu(V);
            atomic<int> uncolored_count(V);

            auto merge_pqs = [](auto &dest, auto &src)
            {
                while (!src.empty())
                {
                    dest.push(src.top());
                    src.pop();
                }
            };

#pragma omp parallel
            {
                int tid = omp_get_thread_num();
                auto &my_thread = *threads[tid];

                // Thread-local véletlenszám-generátor: minden szálnak saját, lock-mentes példánya van
                thread_local std::mt19937 gen(std::random_device{}() + tid);
                std::uniform_int_distribution<int> dist(0, V - 1);

                while (uncolored_count.load(memory_order_relaxed) > 0)
                {
                    int root = -1;

                    // 1. Heurisztika: Véletlenszerű uncolored gyökér keresése (Wrap-around find-Min helyettesítője)
                    for (int attempt = 0; attempt < 50; ++attempt)
                    {
                        int r = dist(gen);
                        int r_set = dsu.find(r);
                        if (nodes[r_set]->color == -1)
                        {
                            root = r;
                            break;
                        }
                    }
                    // Ha a véletlen nem talált, szekvenciális keresés
                    if (root == -1)
                    {
                        for (int i = 0; i < V; ++i)
                        {
                            int r_set = dsu.find(i);
                            if (nodes[r_set]->color == -1)
                            {
                                root = i;
                                break;
                            }
                        }
                    }
                    if (root == -1)
                    {
                        break; // Minden csúcs ki van színezve
                    }

                    // 2. Gyökér lefoglalása és színezése
                    int root_set = dsu.find(root);
                    bool root_acquired = false;
                    {
                        lock_guard<mutex> lk(nodes[root_set]->mtx);
                        if (nodes[root_set]->color == -1)
                        {
                            nodes[root_set]->color = tid;
                            uncolored_count--;
                            root_acquired = true;
                        }
                    }
                    if (!root_acquired)
                    {
                        continue;
                    }

                    // 3. Szál inicializálása (új MST építésének kezdete)
                    {
                        lock_guard<mutex> lk(my_thread.mtx);
                        my_thread.aborted = false;
                        my_thread.mst_weight = 0;
                        while (!my_thread.pq.empty())
                            my_thread.pq.pop();

                        // Szomszédok bedobása a prioritási sorba
                        for (auto &edge : adj[root])
                        {
                            my_thread.pq.push({edge.second, {root, edge.first}});
                        }
                    }

                    // 4. Belső Prim iteráció (MinNode keresés és fa növelés)
                    // 4. Belső Prim iteráció (MinNode keresés és fa növelés)
                    while (true)
                    {
                        pair<int, pair<int, int>> min_edge;
                        {
                            lock_guard<mutex> lk(my_thread.mtx);
                            // 1. LÉPÉS: Csak megnézzük az élt (PEEK), de NEM vesszük ki!
                            if (my_thread.aborted || my_thread.pq.empty())
                                break;
                            min_edge = my_thread.pq.top();
                        }

                        int w = min_edge.first;
                        int u = min_edge.second.first;
                        int v = min_edge.second.second;

                        int root_u = dsu.find(u);
                        int root_v = dsu.find(v);

                        // Ha az él két végpontja már egy fában van (belső él)
                        if (root_u == root_v)
                        {
                            lock_guard<mutex> lk(my_thread.mtx);
                            if (!my_thread.aborted)
                                my_thread.pq.pop(); // Eldobjuk
                            continue;
                        }

                        int c = -2;
                        {
                            // Egyszerre lockoljuk a szálat és a célpont gyökerét
                            scoped_lock lk(my_thread.mtx, nodes[root_v]->mtx);

                            if (my_thread.aborted)
                                break;

                            // Leellenőrizzük, hogy root_v még mindig gyökér-e
                            if (dsu.find(v) != root_v)
                                continue; // Megváltozott! Hagyjuk a sorban az élt és újrapróbáljuk.

                            c = nodes[root_v]->color;
                            if (c == -1)
                            {
                                // Sikeresen elfoglaltunk egy új csúcsot
                                nodes[root_v]->color = tid;
                                uncolored_count--;
                                my_thread.mst_weight += w;

                                // FELHASZNÁLTUK AZ ÉLT, MOST VESSZÜK KI A SORBÓL!
                                my_thread.pq.pop();

                                for (auto &edge : adj[v])
                                {
                                    my_thread.pq.push({edge.second, {v, edge.first}});
                                }
                            }
                        }

                        if (c == -1)
                        {
                            // Lockok nélkül egyesítjük a két fát
                            dsu.unite(root_u, root_v);
                            continue;
                        }

                        if (c == tid)
                        {
                            // Közben a mi fánk része lett egy másik ágon
                            lock_guard<mutex> lk(my_thread.mtx);
                            if (!my_thread.aborted)
                                my_thread.pq.pop(); // Eldobjuk
                            continue;
                        }

                        // B) ÜTKÖZÉS EGY MÁSIK SZÁLLAL!
                        int j = c;
                        int m1 = min(tid, j);
                        int m2 = max(tid, j);

                        int rm1 = min(root_u, root_v);
                        int rm2 = max(root_u, root_v);

                        {
                            // 4 Mutex egyidejű, holtpontmentes lockolása
                            scoped_lock lk(threads[m1]->mtx, threads[m2]->mtx, nodes[rm1]->mtx, nodes[rm2]->mtx);

                            if (my_thread.aborted)
                                break;

                            // Ha még mindig fennállnak a feltételek
                            if (dsu.find(u) == root_u && dsu.find(v) == root_v && nodes[root_v]->color == j && !threads[j]->aborted)
                            {
                                if (tid < j)
                                {
                                    // 1. Eset: Mi nyerünk, 'j' a vesztes
                                    my_thread.mst_weight += w + threads[j]->mst_weight;
                                    my_thread.pq.pop(); // Sikeres beolvasztás, az élt kivesszük
                                    merge_pqs(my_thread.pq, threads[j]->pq);
                                    threads[j]->aborted = true;

                                    nodes[root_v]->color = tid;
                                    nodes[root_u]->color = tid;
                                    dsu.unite(root_u, root_v);
                                }
                                else
                                {
                                    // 2. Eset: 'j' a nyertes, mi vagyunk a vesztes
                                    threads[j]->mst_weight += w + my_thread.mst_weight;
                                    my_thread.pq.pop(); // Átadjuk az élt, kivesszük a saját sorunkból
                                    merge_pqs(threads[j]->pq, my_thread.pq);
                                    my_thread.aborted = true;

                                    nodes[root_u]->color = j;
                                    nodes[root_v]->color = j;
                                    dsu.unite(root_u, root_v);
                                    break;
                                }
                            }
                            // Ha a feltételek elromlottak (pl. 'j' közben meghalt),
                            // a szál békésen továbblép. Mivel NEM hívtunk pop()-ot,
                            // az él biztonságban vár a pq tetején a következő ciklusra!
                        }
                    }
                }
            }

            long long final_weight = 0;
            for (int i = 0; i < num_threads; ++i)
            {
                if (!threads[i]->aborted && threads[i]->mst_weight > 0)
                {
                    final_weight = threads[i]->mst_weight;
                    break;
                }
            }
            return final_weight;
        }

        long long parallelPrimHeuristicSetiaMST()
        {
            int num_threads = omp_get_max_threads();

            struct ThreadData
            {
                mutex mtx;
                bool aborted = false;
                long long mst_weight = 0;
                priority_queue<pair<int, pair<int, int>>,
                               vector<pair<int, pair<int, int>>>,
                               greater<>>
                    pq;
                int tree_size = 0;
                int small_tree_count = 0;
                int root_search_count = 0;
            };
            vector<unique_ptr<ThreadData>> threads(num_threads);
            for (int i = 0; i < num_threads; ++i)
                threads[i] = make_unique<ThreadData>();

            struct NodeData
            {
                mutex mtx;
                int color = -1;
            };
            vector<unique_ptr<NodeData>> nodes(V);
            for (int i = 0; i < V; ++i)
                nodes[i] = make_unique<NodeData>();

            ParallelDSU dsu(V);

            auto merge_pqs = [](auto &dest, auto &src)
            {
                while (!src.empty())
                {
                    dest.push(src.top());
                    src.pop();
                }
            };

#pragma omp parallel
            {
                int tid = omp_get_thread_num();
                auto &my_thread = *threads[tid];

                // Thread-local véletlenszám-generátor: minden szálnak saját, lock-mentes példánya van
                thread_local std::mt19937 gen(std::random_device{}() + tid);
                std::uniform_int_distribution<int> dist(0, V - 1);

                while (my_thread.small_tree_count < this->TREASHOLD &&
                       my_thread.root_search_count < this->TREASHOLD)
                {
                    // 1. Gyökér keresése
                    int root = -1;
                    int r = dist(gen);
                    int r_set = dsu.find(r);
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

                    // 2. Gyökér lefoglalása és színezése
                    int root_set = dsu.find(root);
                    bool root_acquired = false;
                    {
                        lock_guard<mutex> lk(nodes[root_set]->mtx);
                        if (nodes[root_set]->color == -1)
                        {
                            nodes[root_set]->color = tid;
                            root_acquired = true;
                        }
                    }
                    if (!root_acquired)
                    {
                        continue;
                    }

                    // 3. Szál inicializálása (új MST építésének kezdete)
                    {
                        lock_guard<mutex> lk(my_thread.mtx);
                        my_thread.aborted = false;
                        my_thread.mst_weight = 0;
                        while (!my_thread.pq.empty())
                            my_thread.pq.pop();

                        // Szomszédok bedobása a prioritási sorba
                        for (auto &edge : adj[root])
                        {
                            my_thread.pq.push({edge.second, {root, edge.first}});
                        }
                    }

                    // 4. Belső Prim iteráció (MinNode keresés és fa növelés)
                    while (true)
                    {
                        pair<int, pair<int, int>> min_edge;
                        {
                            lock_guard<mutex> lk(my_thread.mtx);
                            // 1. LÉPÉS: Csak megnézzük az élt (PEEK), de NEM vesszük ki!
                            if (my_thread.aborted || my_thread.pq.empty())
                                break;
                            min_edge = my_thread.pq.top();
                        }

                        int w = min_edge.first;
                        int u = min_edge.second.first;
                        int v = min_edge.second.second;

                        int root_u = dsu.find(u);
                        int root_v = dsu.find(v);

                        // Ha az él két végpontja már egy fában van (belső él)
                        if (root_u == root_v)
                        {
                            lock_guard<mutex> lk(my_thread.mtx);
                            if (!my_thread.aborted)
                                my_thread.pq.pop(); // Eldobjuk
                            continue;
                        }

                        int c = -2;
                        {
                            // Egyszerre lockoljuk a szálat és a célpont gyökerét
                            scoped_lock lk(my_thread.mtx, nodes[root_v]->mtx);

                            if (my_thread.aborted)
                                break;

                            // Leellenőrizzük, hogy root_v még mindig gyökér-e
                            if (dsu.find(v) != root_v)
                                continue; // Megváltozott! Hagyjuk a sorban az élt és újrapróbáljuk.

                            c = nodes[root_v]->color;
                            if (c == -1)
                            {
                                // Sikeresen elfoglaltunk egy új csúcsot
                                nodes[root_v]->color = tid;
                                my_thread.mst_weight += w;

                                // FELHASZNÁLTUK AZ ÉLT, MOST VESSZÜK KI A SORBÓL!
                                my_thread.pq.pop();

                                for (auto &edge : adj[v])
                                {
                                    my_thread.pq.push({edge.second, {v, edge.first}});
                                }

                                my_thread.tree_size++;
                            }
                        }

                        if (c == -1)
                        {
                            // Lockok nélkül egyesítjük a két fát
                            dsu.unite(root_u, root_v);
                            continue;
                        }

                        if (c == tid)
                        {
                            // Közben a mi fánk része lett egy másik ágon
                            lock_guard<mutex> lk(my_thread.mtx);
                            if (!my_thread.aborted)
                                my_thread.pq.pop(); // Eldobjuk
                            continue;
                        }

                        // B) ÜTKÖZÉS EGY MÁSIK SZÁLLAL!
                        int j = c;
                        int m1 = min(tid, j);
                        int m2 = max(tid, j);

                        int rm1 = min(root_u, root_v);
                        int rm2 = max(root_u, root_v);

                        {
                            // 4 Mutex egyidejű, holtpontmentes lockolása
                            scoped_lock lk(threads[m1]->mtx, threads[m2]->mtx, nodes[rm1]->mtx, nodes[rm2]->mtx);

                            if (my_thread.aborted)
                                break;

                            // Fák méretének ellenőrzése
                            if (my_thread.tree_size < this->TREASHOLD)
                            {
                                my_thread.small_tree_count++;
                            }
                            else
                            {
                                my_thread.small_tree_count = 0;
                            }

                            if (threads[j]->small_tree_count < this->TREASHOLD)
                            {
                                threads[j]->small_tree_count++;
                            }
                            else
                            {
                                threads[j]->small_tree_count = 0;
                            }

                            // Ha még mindig fennállnak a feltételek
                            if (dsu.find(u) == root_u && dsu.find(v) == root_v && nodes[root_v]->color == j && !threads[j]->aborted)
                            {
                                if (tid < j)
                                {
                                    // 1. Eset: Mi nyerünk, 'j' a vesztes
                                    my_thread.mst_weight += w + threads[j]->mst_weight;
                                    my_thread.pq.pop(); // Sikeres beolvasztás, az élt kivesszük
                                    my_thread.tree_size += threads[j]->tree_size;
                                    threads[j]->tree_size = 0;
                                    merge_pqs(my_thread.pq, threads[j]->pq);
                                    threads[j]->aborted = true;

                                    nodes[root_v]->color = tid;
                                    nodes[root_u]->color = tid;
                                    dsu.unite(root_u, root_v);
                                }
                                else
                                {
                                    // 2. Eset: 'j' a nyertes, mi vagyunk a vesztes
                                    threads[j]->mst_weight += w + my_thread.mst_weight;
                                    my_thread.pq.pop(); // Átadjuk az élt, kivesszük a saját sorunkból
                                    threads[j]->tree_size += my_thread.tree_size;
                                    my_thread.tree_size = 0;
                                    merge_pqs(threads[j]->pq, my_thread.pq);
                                    my_thread.aborted = true;

                                    nodes[root_u]->color = j;
                                    nodes[root_v]->color = j;
                                    dsu.unite(root_u, root_v);
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            long long final_weight = 0;
            for (int i = 0; i < num_threads; ++i)
            {
                if (!threads[i]->aborted && threads[i]->mst_weight > 0)
                {
                    final_weight = threads[i]->mst_weight;
                    break;
                }
            }
            return final_weight;
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
                // JAVÍTVA: -1 helyett INT_MAX, nehogy a 0-ás vagy negatív élsúlyok beakadjanak
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

    namespace fs = std::filesystem;

    double get_cpu_time_seconds()
    {
        struct timespec ts;
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
        return ts.tv_sec + ts.tv_nsec / 1e9;
    }

    pair<int, vector<array<int, 3>>> read_MST_test(const string &file_name)
    {
        // vagy megtartjuk az eredeti relatív útvonalat.
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

    struct TestCase
    {
        string filename;
        int V;
        int E;
        vector<array<int, 3>> edges;
    };

    void measureAllMSTAlgorithms(string folder_name, int execution_count, int timeout_sec)
    {
        string input_dir = "../test_input/spanning_tree_test/";

        // 1. CSAK A FÁJLNEVEK beolvasása (Memóriatakarékos!)
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

        // Fájlnevek sorbarendezése.
        // A "01.in", "02.in", "10.in" formátum miatt a sima ABC sorrend (lexikografikus) tökéletesen működik.
        sort(test_files.begin(), test_files.end());

        // 2. Algoritmusok regisztrálása
        struct AlgDef
        {
            string name;
            function<long long(Graph &)> func;
        };

        vector<AlgDef> algorithms = {
            // {"Kruskal", [](Graph &g)
            //  { return g.kruskalMST(); }},
            // {"Prim Matrix", [](Graph &g)
            //  { return g.primMSTMatrix(); }},
            // {"Prim List", [](Graph &g)
            //  { return g.primMSTOptimal(); }},
            {"Parallel Prim", [](Graph &g)
             { return g.parallelPrimSetiaMST(); }},
             {"Parallel Prim Heuristic", [](Graph &g)
             { return g.parallelPrimHeuristicSetiaMST(); }},
            // {"Boruvka", [](Graph &g)
            //  { return g.boruvkaMST(); }},
            // {"Naiv Parallel Boruvka", [](Graph &g)
            //  { return g.naivParallelBoruvkaMST(); }},
            // {"Buffered Parallel Boruvka", [](Graph &g)
            //  { return g.bufferedParallelBoruvkaMST(); }},
            // {"CAS Parallel Boruvka", [](Graph &g)
            //  { return g.casParallelBoruvkaMST(); }}
        };

        // 3. Adminisztrációs és CSV változók előkészítése
        map<string, ofstream> alg_files;
        map<string, double> total_wall_time;
        map<string, double> total_cpu_time;
        map<string, int> files_processed_count;
        map<string, string> max_graph_reached;
        map<string, string> final_status;
        set<string> timed_out_algorithms;

        string output_dir = "../test_data/mst/";
        if (folder_name != "")
            output_dir += folder_name + "/";
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

        // 4. Fő tesztelési ciklus (LAZY LOADING - Csak azt a fájlt töltjük be, amelyiket épp teszteljük)
        for (const string &current_filename : test_files)
        {
            // --- GRÁF BEOLVASÁSA A MEMÓRIÁBA ---
            auto graph_info = read_MST_test(current_filename);
            int current_V = graph_info.first;
            const auto &current_edges = graph_info.second;
            int current_E = current_edges.size();

            if (current_V == 0)
            {
                cerr << "[Hiba] Nem sikerult beolvasni vagy ures a fajl: " << current_filename << endl;
                continue; // Hibás fájl ugrása
            }

            cout << "\n--- Teszteles: " << current_filename << " (V: " << current_V << ", E: " << current_E << ") ---" << endl;

            // Alap referencia gráf a helyesség (Cost) ellenőrzéséhez
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

                // Explicit védelem a Prim Mátrix ellen óriási gráfoknál
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
                    // Új gráf építése a memóriában már ott lévő él-listából
                    Graph g(current_V);
                    for (const auto &edge : current_edges)
                    {
                        g.addEdge(edge[0], edge[1], edge[2]);
                    }

                    auto wall_start = chrono::high_resolution_clock::now();
                    double cpu_start = get_cpu_time_seconds();

                    // KÖZVETLEN (Szinkron) futtatás
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

            // Itt ér véget a 'current_filename' ciklusa.
            // A graph_info, current_edges, base_graph megsemmisülnek, és a lefoglalt RAM felszabadul!
        }

        // --- TERMINÁL ÖSSZEFOGLALÓ JELENTÉS ---
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