## Lab 12 – Multithreading Mandelbrot

### A. Overview of Implementation

For Lab 12, I extended the Lab 11 Mandelbrot renderer by adding multithreading inside each `mandel` process. The program now accepts a `-t <threads>` command-line argument (clamped between 1 and 20), which controls how many pthreads are used per image.

In `mandel.c`, the image height is divided into `threads` horizontal stripes. For each stripe, the program initializes a `ThreadArgs` structure containing the shared `imgRawImage` pointer, the Mandelbrot coordinate bounds, the maximum iteration count, the color scheme, and the start/end row indices for that stripe. A pthread is created for each stripe and runs a thread function that:

1. Loops over its assigned rows.
2. Maps each pixel `(i, j)` to a point in the complex plane.
3. Computes the iteration count at that point.
4. Converts the iteration count to an RGB color.
5. Writes the color into the shared image buffer.

Each thread writes to a unique, non-overlapping range of rows, so no explicit locking or synchronization is needed beyond `pthread_join`. After all threads finish, the main thread in `mandel` writes the final JPEG using `storeJpegImageFile`. The `movie` program from Lab 11 is unchanged in structure; it still uses `-p <procs>` to control how many `mandel` processes run concurrently, but now it also forwards `-t <threads>` to each child process so that multiprocessing and multithreading are combined.

---

### B. Runtime Results

The table below shows the measured runtimes (in seconds) for rendering 50 frames with various combinations of processes and threads:

+-----------------------------------------------------------------------------------------------+
| Processes \ Threads | 1 Thread | 2 Threads | 4 Threads | 8 Threads | 16 Threads | Max Threads |
+-----------------------------------------------------------------------------------------------+
| 1 Process           | 244 s    | 115 s     | 80 s      | 46 s      | 30 s       | 28 s        |
| 2 Processes         | 116 s    | 64 s      | 42 s      | 29 s      | 25 s       | 26 s        |
| 4 Processes         | 64 s     | 33 s      | 27 s      | 26 s      | 25 s       | 23 s        |
| 8 Processes         | 36 s     | 24 s      | 27 s      | 27 s      | 26 s       | 26 s        |
+-----------------------------------------------------------------------------------------------+

---

### C. Discussion of Results

The runtime data shows that both multiprocessing (`-p`) and multithreading (`-t`) can significantly reduce the total rendering time compared to the single-process, single-thread baseline. Increasing the number of processes tends to give strong speedups at first, because multiple `mandel` processes can run in parallel on different CPU cores. Adding threads inside each process also improves performance, especially when going from 1 thread to a small number of threads (e.g., 2 or 4), since each process can better utilize its share of the CPU.

However, the results also show diminishing returns when the total amount of parallelism (processes × threads) becomes larger than the number of physical cores. Beyond that point, increasing processes or threads can lead to smaller gains or even slight slowdowns due to scheduling overhead, context switching, and contention for shared resources. The table and graph indicate a “sweet spot” where the product `procs × threads` is roughly aligned with the core count of the machine, giving the best overall runtimes. This behavior matches expectations for hybrid multiprocessing/multithreading programs: moderate parallelism efficiently uses the hardware, while excessive parallelism adds overhead without providing proportional speedup.