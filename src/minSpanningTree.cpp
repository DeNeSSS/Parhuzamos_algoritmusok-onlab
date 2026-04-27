#include <algorithm>
#include <climits>
#include <iostream>
#include <queue>
#include <vector>
#include <atomic>
#include <mutex>
#include <thread>

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
        vector<atomic<int>> parent;
        vector<int> rank;
        vector<mutex> locks;

    public:
        ParallelDSU(int n)
        {
            parent.resize(n);
            rank.resize(n, 1);
            for (int i = 0; i < n; i++)
            {
                parent[i].store(i, memory_order_relaxed);
            }
            locks.resize(n)
        }

        int find(int i)
        {
            int p = parent[i].load(memory_order_relaxed) return (parent[i] == i) ? i : (parent[i] = find(parent[i]));
            if (p == i)
            {
                return i;
            }

            int root = find(p);
            parent[i].store(root, std::memory_order_relaxed);
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

                // Zároljuk a két GYÖKERET (nem x-et és y-t!)
                // A scoped_lock garantálja, hogy nem lesz deadlock, akárhogy érkeznek a szálak
                scoped_lock lock(locks[s1], locks[s2]);

                // 2. Lépés: DUPLA ELLENŐRZÉS (Double-checked locking)
                // Mivel a lockolás alatt egy másik szál már átírhatta s1 vagy s2 gyökerét,
                // újra le kell ellenőriznünk, hogy Még MINDIG ezek-e a gyökerek!
                int new_s1 = parent[s1].load(memory_order_relaxed);
                int new_s2 = parent[s2].load(memory_order_relaxed);

                // Ha bármelyik gyökér megváltozott a lockra várás közben,
                // elengedjük a lockokat (a scoped_lock destruktora megteszi), és újrapróbáljuk
                if (new_s1 != s1 || new_s2 != s2)
                {
                    continue; // Vissza a while(true) elejére
                }

                // --- INNENTŐL BIZTONSÁGBAN VAGYUNK ---
                // Biztosan s1 és s2 az aktuális gyökerek, és csak mi férünk hozzájuk

                if (rank[s1] < rank[s2])
                {
                    parent[s1].store(s2, memory_order_relaxed);
                }
                else if (rank[s1] > rank[s2])
                {
                    parent[s2].store(s1, memory_order_relaxed);
                }
                else
                {
                    parent[s2].store(s1, memory_order_relaxed);
                    rank[s1]++; // Mivel a lock alatt vagyunk, ezt nyugodtan növelhetjük
                }

                // Sikeres egyesítés, kilépünk a ciklusból
                return true
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
            matrix.resize(V, vector<int>(V, 0));
        }

        void addEdge(int u, int v, int w)
        {
            edges.push_back({u, v, w});
            adj[u].push_back({v, w});
            adj[v].push_back({u, w}); // Irányítatlan gráf
            matrix[u][v] = w;
            matrix[v][u] = w;
        }

        // --- KRUSKAL ALGORITMUS ---
        int kruskalMST()
        {
            // Prioritási sor az élek súly szerinti rendezéséhez
            auto cmp = [](const vector<int> &a, const vector<int> &b)
            { return a[2] > b[2]; };
            priority_queue<vector<int>, vector<vector<int>>, decltype(cmp)> pq(cmp);

            for (const auto &e : edges)
                pq.push(e);

            DSU dsu(V);
            int cost = 0, edgeCount = 0;

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
        int primMSTMatrix()
        {
            vector<int> key(V, INT_MAX);
            vector<bool> mstSet(V, false);
            key[0] = 0;
            int totalCost = 0;

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
        int primMSTOptimal()
        {
            priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq;
            vector<bool> visited(V, false);
            int totalCost = 0;

            pq.push({0, 0}); // {súly, csúcs}
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
        int boruvkaMST()
        {
            DSU dsu(V);
            int numTrees = V;
            int totalWeight = 0;

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

        int naivParallelBoruvkaMST()
        {
            ParallelDSU dsu(V);
            int numTrees = V;
            int totalWeight = 0;

            while (numTrees > 1)
            {
                vector<vector<int>> cheapest(V, vector<int>(3, -1));

                // Serarch for the cheapest edge in ervery sub tree
#pragma omp parallel for
                for (size_t i = 0; i < edges.size(); ++i) // size_t jobb az OMP-hez
                {
                    const auto &e = edges[i];
                    int set1 = dsu.find(e[0]);
                    int set2 = dsu.find(e[1]);

                    if (set1 != set2)
                    {
// Csak egy szál léphet be egyszerre ebbe a blokkba a set1 miatt
#pragma omp critical
                        {
                            if (cheapest[set1][2] == -1 || cheapest[set1][2] > e[2])
                                cheapest[set1] = e;
                            if (cheapest[set2][2] == -1 || cheapest[set2][2] > e[2])
                                cheapest[set2] = e;
                        }
                    }
                }

                // Feltételezzük, hogy az 'added' a cikluson kívül van deklarálva
                bool added = false;

                // Külön változók a változások (delták) követésére
                int weight_delta = 0;
                int trees_merged = 0; // Azt számoljuk, hány fát kötöttünk össze

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

        return 0;
    }
} // namespace minSpanningTree