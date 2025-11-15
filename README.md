## System Programming Lab 11 Multiprocessing

### (a) Overview of Implementation

This lab builds a multiprocessing system to render multiple Mandelbrot frames in parallel.  
The project contains two programs:

**1. `mandel`** – a single-frame Mandelbrot renderer  
- Takes parameters such as center (`-x`, `-y`), scale (`-s`), resolution (`-W`, `-H`), max iterations (`-m`),  
  and color scheme (`-c`).  
- Computes the fractal and outputs a JPEG image (e.g., `mandel17.jpg`).  
- Uses libjpeg and helper code from `jpegrw.c`.

**2. `movie`** (mandelmovie.c) – a multiprocessing frame generator  
- Uses `fork()` and `execv()` to launch multiple `./mandel` child processes simultaneously.  
- The `-p` option controls how many child processes can run at the same time.  
- The `-f` option controls how many frames are generated (`mandel0.jpg`, `mandel1.jpg`, …).  
- Each frame uses a zoom factor (`-z`) to zoom deeper into the Mandelbrot set.  
- The parent process manages concurrency, collects finished children using `waitpid()`,  
  and prevents overspawning.

This setup allows us to measure how increasing the number of processes affects total runtime.

---

### (b) Runtime Results Graph

The following graph shows the runtime (in seconds) required to generate **10 frames**  
at **800×800 resolution**, **1000 iterations**, using 1, 2, 5, 10, and 20 processes.

![Runtime vs Number of Processes](img/runtime.png)

---

### (c) Discussion of Results

The runtime decreases significantly as the number of processes increases from 1 → 2 → 5.  
This is expected because multiple frames can be computed in parallel, and the workload is  
independent per frame.

The improvement begins to level off around 10–20 processes.  
This is due to:

- The limited number of physical CPU cores  
- Process creation overhead  
- Context switching overhead  
- I/O contention from writing many JPEG files simultaneously

Overall, the results show strong speedup with moderate parallelism (5–10 processes), followed  
by diminishing returns at higher process counts. This behavior matches typical multiprocessing  
performance on CPU-bound workloads.
