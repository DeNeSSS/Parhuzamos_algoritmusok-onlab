# Minimum Spanning Trees (MST) – Serial Algorithms

This document summarizes the classic Minimum Spanning Tree algorithms designed to run sequentially on a single processor core. An MST is a subset of edges in a connected, undirected graph that connects all vertices without any cycles and with the minimum possible total edge weight.
Sources: [link](https://www.geeksforgeeks.org/dsa/spanning-tree/)

---
## Notations
- **V**: Number of vertices
- **E**: Number of edges

---
## 1. Kruskal's Algorithm
Kruskal's algorithm is an edge-based greedy approach.
Source: [link](https://www.geeksforgeeks.org/dsa/kruskals-minimum-spanning-tree-algorithm-greedy-algo-2/)
### Description
1. **Sort** all edges in the graph by weight in non-decreasing order.
2. Initialize a forest where each vertex is a separate component.
3. Iterate through the sorted edges and add an edge to the MST if it connects two different components (i.e., it does not form a cycle).
### Implementation Details
- **Graph Representation:** Edge List.
- **Helper Structure:** **Disjoint Set Union (DSU)** with path compression and union by rank for efficient cycle detection.
- **Optimization:** Instead of a full sort ($O(E \log E)$), a **Min-Heap** can be used to extract the smallest edges one by one ($O(E + k \log E)$), which is efficient if the MST is found before processing all edges.
### Complexity
- **Time:**
    - Using Sort: $O(E \log E)$ or $O(E \log V)$.
    - Using Heap: $O(E)$ for heap construction and $O(\log E)$ for each extraction.
- **Space:** $O(E + V)$ to store the edges and DSU structure.

---
## 2. Prim's Algorithm
Prim's algorithm is a vertex-based greedy approach.
Source: [link](https://www.geeksforgeeks.org/dsa/prims-minimum-spanning-tree-mst-greedy-algo-5/)
### Description
1. Start with an arbitrary vertex and include it in the MST.
2. In each step, find the minimum weight edge that connects a vertex in the MST to a vertex outside the MST.    
3. Add this edge and the new vertex to the MST.   
4. Repeat until all vertices are included.
### Variants and Representation
- **Adjacency Matrix (Classical):**
    - Best for dense graphs ($E \approx V^2$).
    - **Time:** $O(V^2)$.
    - **Space:** $O(V^2)$ for the matrix.
- **Adjacency List and Priority Queue (Optimal):**
    - Best for sparse graphs.        
    - **Time:** $O((E + V) \log V)$.       
    - **Space:** $O(E + V)$.       

---
## 3. Borůvka's Algorithm
Borůvka's algorithm is a component-based approach.
Source: [link](https://www.geeksforgeeks.org/dsa/boruvkas-algorithm-greedy-algo-9/)
### Description
1. Begin with each vertex representing a separate component (tree).
   2. In each iteration, for every component, find the cheapest edge that connects it to a different component.    
3. Add these edges to the MST and merge the components.
4. Repeat until only one component remains.
### Implementation Details
- **Graph Representation:** Adjacency list or edge list.
- **Progress:** The number of components is at least halved in every iteration.
- **Time:** $O(E \log V)$.

---
|**Algorithm**|**Data Structure**|**Time Complexity**|**Best Use Case**|
|---|---|---|---|
|**Kruskal**|Edge List + DSU|$O(E \log V)$|Sparse graphs, simple edge management.|
|**Prim (Matrix)**|Adjacency Matrix|$O(V^2)$|Very dense graphs.|
|**Prim (Heap)**|Adj List + PQ|$O(E \log V)$|Sparse graphs, general-purpose.|
|**Borůvka**|Adj List + DSU|$O(E \log V)$|Frameworks preparing for parallelization.|

