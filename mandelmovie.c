/* Name: Caleb Wilson
*  Assignment: Lab 11
*  CPE 2600 111
*/

///
/// mandelmovie.c
/// Launches multiple ./mandel processes in parallel to render a
/// sequence of colored Mandelbrot frames (mandel0.jpg, mandel1.jpg, ...).
///
/// Requires:
///   - mandel (your single-image renderer with -c color support)
///   - jpegrw.c/.h linked into the project (for mandel)
///
/// Usage examples:
///   ./movie -p 1 -f 50
///   ./movie -p 5 -f 50 -W 1920 -H 1080 -x -0.75 -y 0 -s 2.0 -z 0.97 -m 2000 -c 3
///
/// Color is controlled by the -c flag passed through to ./mandel:
///   0 = grayscale
///   1 = blue→cyan→white
///   2 = rainbow
///   3 = fire
///
///
/// mandelmovie.c
/// Spawns multiple ./mandel processes in parallel to render a sequence
/// of JPEG frames (mandel0.jpg, mandel1.jpg, ...), with color support.
///

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <getopt.h>

/// Print help/usage information
static void usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s [options]\n"
        "Options:\n"
        "  -p <procs>   Number of concurrent processes (default=1)\n"
        "  -f <frames>  Number of frames to render (default=50)\n"
        "  -W <pixels>  Image width (default=1000)\n"
        "  -H <pixels>  Image height (default=1000)\n"
        "  -x <coord>   X center (default=-0.75)\n"
        "  -y <coord>   Y center (default=0.0)\n"
        "  -s <scale>   Initial X scale/span (default=2.0)\n"
        "  -z <zoom>    Per-frame zoom factor (default=0.97)\n"
        "  -m <max>     Max iterations (default=1000)\n"
        "  -c <scheme>  Color scheme (0–3)\n"
        "  -h           Help\n",
        prog);
}

int main(int argc, char *argv[])
{
    int opt;

    // -------- Default settings --------
    int procs  = 1;        // number of children allowed to run at once
    int frames = 50;       // number of frames to render
    int width  = 1000;     // image width
    int height = 1000;     // image height
    double xcenter = -0.75; // mandelbrot center X
    double ycenter = 0.0;   // mandelbrot center Y
    double start_scale = 2.0; // initial view width
    double zoom = 0.97;       // zoom applied per frame
    int max_iter = 1000;      // max iterations per pixel
    int scheme = 0;           // color scheme for mandel

    // -------- Parse command-line arguments --------
    while ((opt = getopt(argc, argv, "p:f:W:H:x:y:s:z:m:c:h")) != -1) {
        switch (opt) {
            case 'p': procs  = atoi(optarg); break;
            case 'f': frames = atoi(optarg); break;
            case 'W': width  = atoi(optarg); break;
            case 'H': height = atoi(optarg); break;
            case 'x': xcenter = atof(optarg); break;
            case 'y': ycenter = atof(optarg); break;
            case 's': start_scale = atof(optarg); break;
            case 'z': zoom   = atof(optarg); break;
            case 'm': max_iter = atoi(optarg); break;
            case 'c': scheme = atoi(optarg); break;
            case 'h':
                usage(argv[0]);
                return 0;
            default:
                usage(argv[0]);
                return 1;
        }
    }

    // -------- Sanity checks --------
    if (procs < 1)  procs = 1;
    if (frames < 1) frames = 1;
    if (width <= 0 || height <= 0) {
        fprintf(stderr, "error: width/height must be positive\n");
        return 2;
    }

    // Display configuration
    printf("movie: procs=%d frames=%d W=%d H=%d x=%f y=%f s=%f z=%f m=%d c=%d\n",
           procs, frames, width, height, xcenter, ycenter,
           start_scale, zoom, max_iter, scheme);

    int next = 0;         // next frame number to spawn
    int active = 0;       // how many children are currently running

    // -------- Main multiprocessing loop --------
    while (next < frames || active > 0) {

        // Spawn children until we reach the allowed limit
        while (active < procs && next < frames) {
            int frame = next++;  // assign frame number
            double scale = start_scale * pow(zoom, (double)frame);

            // Format strings for exec arguments
            char wbuf[32], hbuf[32], xbuf[64], ybuf[64], sbuf[64];
            char mbuf[32], cbuf[16], obuf[64];

            snprintf(wbuf, sizeof wbuf, "%d", width);
            snprintf(hbuf, sizeof hbuf, "%d", height);
            snprintf(xbuf, sizeof xbuf, "%.17g", xcenter);
            snprintf(ybuf, sizeof ybuf, "%.17g", ycenter);
            snprintf(sbuf, sizeof sbuf, "%.17g", scale);
            snprintf(mbuf, sizeof mbuf, "%d", max_iter);
            snprintf(cbuf, sizeof cbuf, "%d", scheme);
            snprintf(obuf, sizeof obuf, "mandel%d.jpg", frame);

            // ---- Fork a child to render one frame ----
            pid_t pid = fork();
            if (pid < 0) {
                fprintf(stderr, "fork failed: %s\n", strerror(errno));
                return 3;
            }

            if (pid == 0) {
                // ---- CHILD PROCESS ----
                // Build argument list for ./mandel
                char *const args[] = {
                    "./mandel",
                    "-W", wbuf,
                    "-H", hbuf,
                    "-x", xbuf,
                    "-y", ybuf,
                    "-s", sbuf,
                    "-m", mbuf,
                    "-c", cbuf,   // pass color scheme to mandel
                    "-o", obuf,   // output file mandelN.jpg
                    NULL
                };

                // Replace child process with mandel
                execv(args[0], args);

                // Exec only returns if there was an error
                fprintf(stderr, "execv('./mandel') failed: %s\n", strerror(errno));
                _exit(127);
            }
            else {
                // ---- PARENT PROCESS ----
                active++;   // count running child
            }
        }

        // ---- Wait for ANY child to finish ----
        int status = 0;
        pid_t done = waitpid(-1, &status, 0);

        if (done > 0) {
            active--;  // one child finished

            // Optional: print errors if a child crashed
            if (WIFSIGNALED(status)) {
                fprintf(stderr, "child %d killed by signal %d\n",
                        (int)done, WTERMSIG(status));
            }
        }
        else if (done < 0 && errno != ECHILD) {
            fprintf(stderr, "waitpid error: %s\n", strerror(errno));
            break;
        }
    }

    printf("movie: finished generating %d frame(s).\n", frames);
    return 0;
}
