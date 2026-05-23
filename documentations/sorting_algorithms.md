# Sorting Algorithms Documentation

## 1. Serial Sorting Algorithms
Classic algorithms designed to run on a single processor core.

* **Bubble Sort**: 
    * Time: $O(n^2)$
    * Memory: $O(1)$
* **Selection Sort**: 
    * Time: $O(n^2)$
    * Memory: $O(1)$
* **Merge Sort**: 
    * Time: $O(n \log n)$
    * Memory: $O(n)$
* **Quick Sort**:
    * Time (Best/Average): $O(n \log n)$
    * Time (Worst): $O(n^2)$
    * Memory: $O(\log n)$ (due to the recursion stack)

---

## 2. Parallel Sorting Algorithms
Algorithms optimized for multiple processors or specialized hardware (sorting networks) to reduce total execution time.

### Odd-Even Transposition Sort (Parallel Bubble Sort)
A parallel version of bubble sort that compares and swaps adjacent elements in two alternating phases. The process terminates in at most $n$ steps.



1.  **Odd Phase**: Pairs $(1, 2), (3, 4), (5, 6) \dots$ are compared and swapped in parallel.
2.  **Even Phase**: Pairs $(2, 3), (4, 5), (6, 7) \dots$ are compared and swapped in parallel.

* **Parallel Time**: $O(n)$.
* **Total Work**: $O(n^2)$.
* **Memory**: $O(1)$ (in-place).
* **Processor Count**: $n/2$ processors perform $n/2$ comparisons per phase.

### Batcher's Odd-Even Merge Sort
A sorting network based on the divide-and-conquer principle, specifically optimized for parallel merging.



* **Parallel Time (Depth)**: $O(\log^2 n)$.
* **Total Work**: $O(n \log^2 n)$.
* **Memory**: $O(n)$ for storage, plus $O(\log^2 n)$ for the recursion stack in software.

**Algorithm Steps:**
1.  **Divide**: Recursively split the list into halves $A$ and $B$, then sort each half.
2.  **Sub-sequences**: Separate sorted lists $A$ and $B$ into odd-indexed and even-indexed elements ($odd(A), odd(B), even(A), even(B)$).
3.  **Merge**: 
    * $Odd = \text{merge}(odd(A), odd(B))$.
    * $Even = \text{merge}(even(A), even(B))$.
4.  **Interleave**: Combine the results of the $Odd$ and $Even$ merges by interleaving them.
5.  **Final Compare-Exchange**: A final parallel pass compares and swaps adjacent elements ($even_i$ with $odd_{i+1}$) to ensure the entire sequence is ordered.

---

### Bitonic Sort
A parallel sorting algorithm that builds a "bitonic" sequence and then sorts it through a series of "Bitonic Merge" operations.



* **Parallel Time**: $O(\log^2 n)$.
* **Total Work**: $O(n \log^2 n)$.
* **Memory**: $O(n)$.

**Algorithm Steps:**
1.  **Bitonic Preparation**: Create a bitonic sequence (a sequence that increases then decreases) by sorting one half ascending and the other descending.
2.  **Bitonic Split**: Compare each element in the first half with its corresponding element in the second half ($i$ with $i + n/2$).
    
3.  **Shuffle/Partition**: Swapping creates two smaller bitonic sequences where every element in the first is smaller than every element in the second.
4.  **Recursive Merge**: Apply the split and shuffle recursively to each half until the entire sequence is sorted.

---

## 3. Comparison Summary

| Algorithm | Parallel Time | Total Work | Memory | Scaling Efficiency |
| :--- | :--- | :--- | :--- | :--- |
| **Odd-Even Transposition** | $O(n)$ | $O(n^2)$ | $O(1)$ | Low |
| **Odd-Even Merge** | $O(\log^2 n)$ | $O(n \log^2 n)$ | $O(n)$ | High |
| **Bitonic Sort** | $O(\log^2 n)$ | $O(n \log^2 n)$ | $O(n)$ | Excellent for GPU/FPGA |
| **Optimal PRAM Sort** | $O(\log n)$ | $O(n \log n)$ | $O(n)$ | Theoretical Maximum |

---
**Reference Links:**
* [Odd-even Visualization](https://youtu.be/1UEWb_dgkx8?si=xB3hhyZjR5uOGNXV)
* [Sorting Networks in Depth](https://hwlang.de/algorithmen/sortieren/networks/indexen.htm)