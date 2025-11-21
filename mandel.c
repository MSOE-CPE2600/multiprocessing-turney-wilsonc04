/* Name: Caleb Wilson
*  Assignment: Lab 11
*  CPE 2600 111
*/

/// 
//  mandel.c
//  Based on example code found here:
//  https://users.cs.fiu.edu/~cpoellab/teaching/cop4610_fall22/project3.html
//
//  Converted to use jpg instead of BMP and other minor changes
//  
///
///
/// mandel.c
/// Single-image Mandelbrot renderer using libjpeg.
/// Supports:
///   -x, -y  : center coordinates
///   -s      : scale (span in x direction)
///   -W, -H  : image width/height in pixels
///   -m      : max iterations
///   -o      : output JPEG filename
///   -c      : color scheme (0..3)
///   -t      : number of threas (1-20) for multithreading Lab12
///   -h      : help
///

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <pthread.h>

#include "jpegrw.h"

#define MAX_THREADS 20

// Struct to hold per-thread arguments
typedef struct {
    imgRawImage *img;
    double xmin;
    double xmax;
    double ymin;
    double ymax;
    int max_iter;
    int scheme;
    int y_start;        // inclusive
    int y_end;          // exclusive
} ThreadArgs;

// Prototypes
static unsigned int iteration_to_color(int iters, int max, int scheme);
static int iterations_at_point(double x, double y, int max);
static void compute_image(imgRawImage *img,
                          double xmin, double xmax,
                          double ymin, double ymax,
                          int max, int scheme, int threads);
static void *compute_image_stripe(void *arg);
static void show_help(const char *prog);

int main(int argc, char *argv[])
{
    int opt;

    // Defaults
    const char *outfile = "mandel.jpg";
    double xcenter = 0.0;
    double ycenter = 0.0;
    double xscale  = 4.0;
    double yscale  = 0.0; // computed from aspect ratio
    int    image_width  = 1000;
    int    image_height = 1000;
    int    max = 1000;
    int    scheme = 0;    // color scheme
    int    threads = 1;   // number of threads (default=1)

    while ((opt = getopt(argc, argv, "x:y:s:W:H:m:o:c:t:h")) != -1) {
        switch (opt) {
            case 'x': xcenter       = atof(optarg); break;
            case 'y': ycenter       = atof(optarg); break;
            case 's': xscale        = atof(optarg); break;
            case 'W': image_width   = atoi(optarg); break;
            case 'H': image_height  = atoi(optarg); break;
            case 'm': max           = atoi(optarg); break;
            case 'o': outfile       = optarg; break;
            case 'c': scheme        = atoi(optarg); break;
            case 't': threads       = atoi(optarg); break;
            case 'h':
                show_help(argv[0]);
                return 0;
            default:
                show_help(argv[0]);
                return 1;
        }
    }

    if (image_width <= 0 || image_height <= 0) {
        fprintf(stderr, "error: width and height must be positive\n");
        return 2;
    }
    if (xscale <= 0.0) {
        fprintf(stderr, "error: scale (-s) must be positive\n");
        return 2;
    }
    if (max <= 0) {
        fprintf(stderr, "error: max iterations (-m) must be positive\n");
        return 2;
    }

    if (threads < 1) {
        threads = 1;
    } else if (threads > MAX_THREADS) {
        threads = MAX_THREADS;
    }

    // Calculate yscale from xscale and aspect ratio
    yscale = xscale * ((double)image_height / (double)image_width);

    printf("mandel: x=%f y=%f xscale=%f yscale=%f max=%d W=%d H=%d outfile=%s scheme=%d threads=%d\n",
           xcenter, ycenter, xscale, yscale, max,
           image_width, image_height, outfile, scheme, threads);

    // Create image
    imgRawImage *img = initRawImage((unsigned int)image_width,
                                    (unsigned int)image_height);
    if (!img) {
        fprintf(stderr, "error: failed to allocate image\n");
        return 3;
    }

    // Fill with black
    setImageCOLOR(img, 0x000000);

    // Compute Mandelbrot set
    double xmin = xcenter - xscale / 2.0;
    double xmax = xcenter + xscale / 2.0;
    double ymin = ycenter - yscale / 2.0;
    double ymax = ycenter + yscale / 2.0;

    compute_image(img, xmin, xmax, ymin, ymax, max, scheme, threads);

    // Save JPEG
    if (storeJpegImageFile(img, outfile) != 0) {
        fprintf(stderr, "error: could not write %s\n", outfile);
        freeRawImage(img);
        return 4;
    }

    freeRawImage(img);
    return 0;
}

// ----------------------------------------------------------
// Iterations at a point
// ----------------------------------------------------------
static int iterations_at_point(double x, double y, int max)
{
    double x0 = x;
    double y0 = y;
    int iter = 0;

    while ((x*x + y*y <= 4.0) && iter < max) {
        double xt = x*x - y*y + x0;
        double yt = 2.0*x*y + y0;
        x = xt;
        y = yt;
        iter++;
    }

    return iter;
}

// ----------------------------------------------------------
// Compute a stripe of the image
// ----------------------------------------------------------
static void *compute_image_stripe(void *arg)
{
    ThreadArgs *a = (ThreadArgs *)arg;
    int width  = (int)a->img->width;
    int height = (int)a->img->height;

    for (int j = a->y_start; j < a->y_end; j++) {
        double y = a->ymin + (a->ymax - a->ymin) * ((double)j / (double)height);
        for (int i = 0; i < width; i++) {
            double x = a->xmin + (a->xmax - a->xmin) * ((double)i / (double)width);
            int iters = iterations_at_point(x, y, a->max_iter);
            unsigned int rgb = iteration_to_color(iters, a->max_iter, a->scheme);
            setPixelCOLOR(a->img, (unsigned int)i, (unsigned int)j, rgb);
        }
    }
    return NULL;
}

// ----------------------------------------------------------
// Compute entire image
// ----------------------------------------------------------
static void compute_image(imgRawImage *img,
                          double xmin, double xmax,
                          double ymin, double ymax,
                          int max, int scheme, int threads)
{
    int width  = (int)img->width;
    int height = (int)img->height;

    if (threads <=1) {
        for (int j = 0; j < height; j++) {
            double y = ymin + (ymax - ymin) * ((double)j / (double)height);
            for (int i = 0; i < width; i++) {
                double x = xmin + (xmax - xmin) * ((double)i / (double)width);
                int iters = iterations_at_point(x, y, max);
                unsigned int rgb = iteration_to_color(iters, max, scheme);
                setPixelCOLOR(img, (unsigned int)i, (unsigned int)j, rgb);
            }
        }
        return;
    }
    if (threads > MAX_THREADS) {
        threads = MAX_THREADS;
    }

    pthread_t   tids[MAX_THREADS];
    ThreadArgs  args[MAX_THREADS];

    int rows_per_thread = height / threads;
    int leftover        = height % threads; 

    int current_row = 0;
    for (int t = 0; t < threads; t++) {
        int rows = rows_per_thread + (t < leftover ? 1 : 0);
        int start = current_row;
        int end = start + rows;

        args[t].img         = img;
        args[t].xmin        = xmin;
        args[t].xmax        = xmax;
        args[t].ymin        = ymin;
        args[t].ymax        = ymax;
        args[t].max_iter    = max;
        args[t].scheme      = scheme;
        args[t].y_start     = start;
        args[t].y_end       = end;

        current_row = end;

        pthread_create(&tids[t], NULL, compute_image_stripe, &args[t]);
    }
    for (int t = 0; t < threads; t++) {
        pthread_join(tids[t], NULL);
    }
}

// ----------------------------------------------------------
// Color mapping
// ----------------------------------------------------------
static unsigned int iteration_to_color(int iters, int max, int scheme)
{
    if (iters >= max) {
        return 0x000000; // inside the set = black
    }

    double t = (double)iters / (double)max;
    unsigned char r = 0, g = 0, b = 0;

    switch (scheme) {
        case 0: { // grayscale
            unsigned char v = (unsigned char)(255.0 * t);
            r = g = b = v;
            break;
        }
        case 1: { // blue -> cyan -> white
            r = (unsigned char)(128.0 * t);
            g = (unsigned char)(200.0 * t);
            b = (unsigned char)(255.0 * t);
            break;
        }
        case 2: { // rainbow via sines
            double rf = 0.5 + 0.5 * sin(6.28318 * (t + 0.00));
            double gf = 0.5 + 0.5 * sin(6.28318 * (t + 0.33));
            double bf = 0.5 + 0.5 * sin(6.28318 * (t + 0.66));
            r = (unsigned char)(255.0 * rf);
            g = (unsigned char)(255.0 * gf);
            b = (unsigned char)(255.0 * bf);
            break;
        }
        case 3: { // fire
            double u = pow(t, 0.4);
            r = (unsigned char)(255.0 * u);
            g = (unsigned char)(200.0 * u);
            b = (unsigned char)(50.0  * u);
            break;
        }
        default: { // fallback grayscale
            unsigned char v = (unsigned char)(255.0 * t);
            r = g = b = v;
            break;
        }
    }

    return ((unsigned int)r << 16) | ((unsigned int)g << 8) | (unsigned int)b;
}

// ----------------------------------------------------------
// Help
// ----------------------------------------------------------
static void show_help(const char *prog)
{
    printf("Use: %s [options]\n", prog);
    printf("Options:\n");
    printf("  -m <max>     Maximum iterations per point (default=1000)\n");
    printf("  -x <coord>   X coordinate of image center (default=0)\n");
    printf("  -y <coord>   Y coordinate of image center (default=0)\n");
    printf("  -s <scale>   X-axis span in Mandelbrot coords (default=4)\n");
    printf("  -W <pixels>  Image width in pixels (default=1000)\n");
    printf("  -H <pixels>  Image height in pixels (default=1000)\n");
    printf("  -o <file>    Output JPEG filename (default=mandel.jpg)\n");
    printf("  -c <scheme>  Color scheme (0=gray,1=cyan,2=rainbow,3=fire; default=0)\n");
    printf("  -t <threads> Number of threads (1-20, default=1)\n");
    printf("  -h           Show this help text\n");
}
