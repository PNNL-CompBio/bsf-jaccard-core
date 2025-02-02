#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <x86intrin.h>
#ifdef _OPENMP
#include <omp.h>
#endif
#include "BSFCoreDll.h"

#ifdef BSF_XOR
#define sim(a,b) ((a)^(b))
#elif BSF_AND
#define sim(a,b) ((a)&(b))
#else
#define sim(a,b) ((a)^(b))
#endif

#define NUM_THREADS 8

namespace BSF
{
    // to compute the similarity between columns of lib matrix and query matrix
    unsigned BSFCore::query(const uint64_t** lib, const uint64_t** q, unsigned** c, const unsigned nlib, const unsigned nq, const unsigned nrow)
    {
        unsigned size = (nrow / 64);
        unsigned k, i, j;
        uint64_t _xor;
        unsigned count = 0;

        #ifdef _OPENMP
        printf("==================OPENMP====================\n");
        #pragma omp parallel num_threads(NUM_THREADS)
        #pragma omp for private(_xor, i, j) schedule(dynamic) reduction(+:count)
        #endif
        for( k = 0; k < nlib; k++) {
            for ( i = 0; i < nq; i++) {
                count = 0;
                for ( j = 0; j < size; j++) {
                    _xor = sim(q[i][j], lib[k][j]);
                    count += __builtin_popcountll(_xor);
                }
                c[k][i] = count;
            }
        }
        return 0;
    }

    // to compute the similarity between columns of lib matrix and query matrix
    unsigned BSFCore::queryXOR(const uint64_t** lib, const uint64_t** q, unsigned** c, const unsigned nlib, const unsigned nq, const unsigned nrow)
    {
        unsigned size = (nrow / 64);
        unsigned k, i, j;
        uint64_t _xor;

        #ifdef _OPENMP
        printf("==================OPENMP====================\n");
        #pragma omp parallel num_threads(NUM_THREADS)
        #pragma omp for private(_xor, i, j) schedule(dynamic)
        #endif
        for( k = 0; k < nlib; k++) {
            for ( i = 0; i < nq; i++) {
                for ( j = 0; j < size; j++) {
                    _xor = (q[i][j]) ^ (lib[k][j]);
                    c[k][i] = __builtin_popcountll(_xor);
                }
            }
        }
        return 0;
    }

    // to analyse the similarity between columns of lib matrix
    unsigned BSFCore::analysis(const uint64_t** lib, unsigned** c, const unsigned nlib, const unsigned nrow)
    {
        unsigned size = (nrow / 64);
        unsigned k, i, j;
        uint64_t _xor;
        unsigned count = 0;

        #ifdef _OPENMP
        printf("==================OPENMP====================\n");
        #pragma omp parallel num_threads(NUM_THREADS)
        #pragma omp for private(_xor, i, j) schedule(dynamic) reduction(+:count)
        #endif
        for( k = 0; k < nlib - 1; k++) {
            for ( i = k+1; i < nlib; i++) {
                count = 0;
                for ( j = 0; j < size; j++) {
                    _xor = sim(lib[i][j], lib[k][j]);
                    count += __builtin_popcountll(_xor);
                }
                c[k][i] = count;
            }
        }
        return 0;
    }

    // to analyse the similarity between columns of lib matrix (with chunks)
    // compare between [x1:x2] and [x1:x2]
    // Modified to return Jaccard similarity
    unsigned BSFCore::analysis_with_chunks(const uint64_t** lib, unsigned** c, const unsigned x1, const unsigned x2, const unsigned nrow)
    {
        unsigned size = (nrow / 64);
        unsigned k, i, j;
        uint64_t _sim;
        uint64_t _lor;
        unsigned count = 0;
        unsigned lcount = 0;
        float jacc = 0;

        #ifdef _OPENMP
        printf("==================OPENMP========analysis_with_chunks-Jaccard============\n");
        #pragma omp parallel num_threads(NUM_THREADS)
        #pragma omp for private(_sim, _lor, i, j) schedule(dynamic) reduction(+:count), reduction(+:lcount)
        #endif
        for( k = x1; k < x2 - 1; k++) {
            for ( i = k+1; i < x2; i++) {
                count = 0;
                lcount = 0;
                for ( j = 0; j < size; j++) {
                    _sim = sim(lib[i][j], lib[k][j]);
                    _lor = (lib[i][j]|lib[k][j]);
                    count += __builtin_popcountll(_sim);
                    lcount += __builtin_popcountll(_lor);
                }

                // this should return an integer between 0 and 100
                // that represents a Jaccard similarity * 100
                jacc = ((float)count/(float)lcount)*100.0;

                c[k-x1][i-x1] = (int)jacc;
            }
        }
        return 0;
    }


    // to analyse the similarity between columns of lib matrix (with chunks)
    // compare between [x1:x2] and [y1:y2]
    unsigned BSFCore::analysis_with_chunks(const uint64_t** lib, unsigned** c, const unsigned x1, const unsigned x2, const unsigned y1, const unsigned y2, const unsigned nrow)
    {
        unsigned size = (nrow / 64);
        unsigned k, i, j;
        uint64_t _xor;
        unsigned count = 0;

        #ifdef _OPENMP
        printf("==================OPENMP=======analysis_with_chunks2=============\n");
        #pragma omp parallel num_threads(NUM_THREADS)
        #pragma omp for private(_xor, i, j) schedule(dynamic) reduction(+:count)
        #endif
        for( k = x1; k < x2; k++) {
            for ( i = y1; i < y2; i++) {
                count = 0;
                for ( j = 0; j < size; j++) {
                    _xor = sim(lib[i][j], lib[k][j]);
                    count += __builtin_popcountll(_xor);
                }
                c[k-x1][i-y1] = count;
            }
        }
        return 0;
    }

    // to analyse the similarity between columns of lib and q matrix
    // compare between lib[x1:x2] and q[y1:y2]
    unsigned BSFCore::analysis_with_query(const uint64_t** lib, const uint64_t** q, unsigned** c, const unsigned x1, const unsigned x2, const unsigned y1, const unsigned y2, const unsigned nrow)
    {
        unsigned size = (nrow / 64);
        unsigned k, i, j;
        uint64_t _xor;
        unsigned count = 0;

        #ifdef _OPENMP
        printf("==================OPENMP=======analysis_with_query=============\n");
        #pragma omp parallel num_threads(NUM_THREADS)
        #pragma omp for private(_xor, i, j) schedule(dynamic) reduction(+:count)
        #endif
        for( k = x1; k < x2; k++) {
            for ( i = y1; i < y2; i++) {
                count = 0;
                for ( j = 0; j < size; j++) {
                    _xor = sim(q[i][j], lib[k][j]);
                    count += __builtin_popcountll(_xor);
                }
                c[k-x1][i-y1] = count;
            }
        }
        return 0;
    }

    // to analyse the similarity between columns of lib matrix
    unsigned BSFCore::benchmark(const uint64_t** lib, const unsigned nlib, const unsigned nrow)
    {
        unsigned size = (nrow / 64);
        unsigned k, i, j;
        uint64_t _xor;
        unsigned count = 0;

        #ifdef _OPENMP
        printf("==================OPENMP====================\n");
        #pragma omp parallel num_threads(NUM_THREADS)
        #pragma omp for private(_xor, i, j) schedule(dynamic) reduction(+:count)
        #endif
        for( k = 0; k < nlib - 1; k++) {
            for ( i = k+1; i < nlib; i++) {
                for ( j = 0; j < size; j++) {
                    _xor = sim(lib[i][j], lib[k][j]);
                    count += __builtin_popcountll(_xor);
                }
            }
        }

        printf("count: %d", count);
        return count;
    }
}
