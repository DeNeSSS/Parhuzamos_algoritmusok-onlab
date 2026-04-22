#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <climits>

namespace minSpanningTree
{
    using namespace std;

    // Közös Disjoint Set Union segédosztály
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
}