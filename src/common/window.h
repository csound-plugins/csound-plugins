#ifndef WINDOW_H
#define WINDOW_H

#include <stddef.h>
#include <math.h>


/**
 * Supported window function types.
 */
typedef enum {
    WINDOW_HAMMING,          /* Hamming window          */
    WINDOW_HANN,             /* Hann (Hanning) window   */
    WINDOW_KAISER_BESSEL,    /* Kaiser-Bessel window    */
    WINDOW_BLACKMAN_HARRIS,  /* Blackman-Harris window  */
} WindowType;

/**
 * Compute a window function and write the coefficients into `out`.
 *
 * @param out   Pointer to a caller-allocated array of at least `size` doubles.
 * @param size  Number of samples in the window (must be >= 2).
 * @param type  Which window function to compute.
 * @param beta  Shape parameter used only by WINDOW_KAISER_BESSEL (typical
 *              values: 4–9; ignored for all other types).
 * @return      0 on success, -1 if out is NULL or size < 2.
 */

/* ------------------------------------------------------------------ */
/* Internal helpers                                                     */
/* ------------------------------------------------------------------ */

#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif

/**
 * Modified zeroth-order Bessel function of the first kind I₀(x).
 * Computed via the standard power-series expansion — converges
 * rapidly for the beta values used in Kaiser-Bessel windows (< ~15).
 */
static double bessel_i0(double x)
{
    double sum   = 1.0;
    double term  = 1.0;
    double half_x = x / 2.0;

    for (int k = 1; k <= 30; ++k) {
        term *= (half_x / k);
        term *= (half_x / k);   /* term = (x/2)^(2k) / (k!)^2 */
        sum  += term;
        if (term < sum * 1e-12) /* converged */
            break;
    }
    return sum;
}

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

int window_compute(double *out, size_t size, WindowType type, double beta)
{
    if (!out || size < 2)
        return -1;

    const double N   = (double)(size - 1);   /* symmetric: endpoints included */
    const double i0b = bessel_i0(beta);      /* pre-compute denominator once  */

    for (size_t n = 0; n < size; ++n) {
        double w;
        double ratio = (double)n / N;        /* 0 … 1                         */

        switch (type) {

        /* ----------------------------------------------------------
         * Hamming window
         *   w[n] = 0.54 - 0.46 · cos(2π·n/N)
         * ---------------------------------------------------------- */
        case WINDOW_HAMMING:
            w = 0.54 - 0.46 * cos(2.0 * M_PI * ratio);
            break;

        /* ----------------------------------------------------------
         * Hann (Hanning) window
         *   w[n] = 0.5 · (1 - cos(2π·n/N))
         * ---------------------------------------------------------- */
        case WINDOW_HANN:
            w = 0.5 * (1.0 - cos(2.0 * M_PI * ratio));
            break;

        /* ----------------------------------------------------------
         * Kaiser-Bessel window
         *   w[n] = I₀( β · √(1 - (2n/N - 1)²) ) / I₀(β)
         * ---------------------------------------------------------- */
        case WINDOW_KAISER_BESSEL: {
            double t = 2.0 * ratio - 1.0;        /* maps n to [-1, 1] */
            double arg = beta * sqrt(1.0 - t * t);
            w = bessel_i0(arg) / i0b;
            break;
        }

        /* ----------------------------------------------------------
         * Blackman-Harris window (4-term, minimum 92 dB sidelobe)
         *   w[n] = a₀ - a₁·cos(2πn/N) + a₂·cos(4πn/N) - a₃·cos(6πn/N)
         *   a₀=0.35875  a₁=0.48829  a₂=0.14128  a₃=0.01168
         * ---------------------------------------------------------- */
        case WINDOW_BLACKMAN_HARRIS:
            w =  0.35875
               - 0.48829 * cos(2.0 * M_PI * ratio)
               + 0.14128 * cos(4.0 * M_PI * ratio)
               - 0.01168 * cos(6.0 * M_PI * ratio);
            break;

        default:
            return -1;
        }

        out[n] = w;
    }

    return 0;
}

#endif /* WINDOW_H */
