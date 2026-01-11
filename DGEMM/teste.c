#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <immintrin.h> // Obrigatório para AVX
#include <stdint.h>

// --- CONFIGURAÇÕES ---
#define BLOCK_SIZE 32   // Otimizado para L1 Cache
#define NUM_RUNS 5      // Execuções para média estatística
#define WARMUP_RUNS 1   // Aquecimento de cache

// --- UTILITÁRIOS DE TEMPO (Alta Precisão) ---
double get_time_sec() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

// Alocação alinhada em 32 bytes para AVX
double* alloc_matrix(int n) {
    double* ptr = (double*)_mm_malloc(n * n * sizeof(double), 32);
    if (!ptr) exit(1);
    for (int i = 0; i < n * n; i++) {
        ptr[i] = (double)(rand() % 100) / 10.0;
    }
    return ptr;
}

void clean_matrix(double* C, int n) {
    memset(C, 0, n * n * sizeof(double));
}

// --- 1. NAIVE (IKJ Optimization) ---
void dgemm_naive(int n, double* A, double* B, double* C) {
    for (int i = 0; i < n; i++) {
        for (int k = 0; k < n; k++) {
            double r = A[i * n + k];
            for (int j = 0; j < n; j++) {
                C[i * n + j] += r * B[k * n + j];
            }
        }
    }
}

// --- 2. AVX (Vectorized) ---
void dgemm_avx(int n, double* A, double* B, double* C) {
    for (int i = 0; i < n; i++) {
        for (int k = 0; k < n; k++) {
            __m256d a_vec = _mm256_set1_pd(A[i * n + k]);
            int j = 0;
            for (; j <= n - 4; j += 4) {
                __m256d c_vec = _mm256_load_pd(&C[i * n + j]);
                __m256d b_vec = _mm256_load_pd(&B[k * n + j]);
                c_vec = _mm256_add_pd(c_vec, _mm256_mul_pd(a_vec, b_vec));
                _mm256_store_pd(&C[i * n + j], c_vec);
            }
            // Cleanup para N não múltiplo de 4
            for (; j < n; j++) {
                C[i * n + j] += A[i * n + k] * B[k * n + j];
            }
        }
    }
}

// --- 3. AVX + BLOCKING + PREFETCH (Extensivo) ---
void dgemm_avx_block_prefetch(int n, double* A, double* B, double* C) {
    for (int i_blk = 0; i_blk < n; i_blk += BLOCK_SIZE) {
        for (int k_blk = 0; k_blk < n; k_blk += BLOCK_SIZE) {
            for (int j_blk = 0; j_blk < n; j_blk += BLOCK_SIZE) {
                
                int i_max = (i_blk + BLOCK_SIZE > n) ? n : i_blk + BLOCK_SIZE;
                int k_max = (k_blk + BLOCK_SIZE > n) ? n : k_blk + BLOCK_SIZE;
                int j_max = (j_blk + BLOCK_SIZE > n) ? n : j_blk + BLOCK_SIZE;

                for (int i = i_blk; i < i_max; i++) {
                    for (int k = k_blk; k < k_max; k++) {
                        __m256d a_vec = _mm256_set1_pd(A[i * n + k]);
                        int j = j_blk;
                        for (; j <= j_max - 4; j += 4) {
                            // Prefetching
                            _mm_prefetch((const char*)&B[k * n + j + 16], _MM_HINT_T0);
                            
                            __m256d c_vec = _mm256_load_pd(&C[i * n + j]);
                            __m256d b_vec = _mm256_load_pd(&B[k * n + j]);
                            c_vec = _mm256_add_pd(c_vec, _mm256_mul_pd(a_vec, b_vec));
                            _mm256_store_pd(&C[i * n + j], c_vec);
                        }
                        for (; j < j_max; j++) {
                            C[i * n + j] += A[i * n + k] * B[k * n + j];
                        }
                    }
                }
            }
        }
    }
}

double run_benchmark(void (*func)(int, double*, double*, double*), int n, double* A, double* B, double* C, const char* name) {
    // Warm-up
    for(int w = 0; w < WARMUP_RUNS; w++) {
        clean_matrix(C, n);
        func(n, A, B, C);
    }
    
    // Média
    double total_time = 0.0;
    for (int r = 0; r < NUM_RUNS; r++) {
        clean_matrix(C, n);
        double start = get_time_sec();
        func(n, A, B, C);
        double end = get_time_sec();
        total_time += (end - start);
    }
    double avg_time = total_time / NUM_RUNS;
    double operations = 2.0 * (double)n * (double)n * (double)n;
    double gflops = (operations / avg_time) * 1e-9;
    
    printf("%-20s | N=%-4d | Time: %.4fs | GFLOPS: %.2f\n", name, n, avg_time, gflops);
    return gflops;
}

int main() {
    // Inicializar gerador de números aleatórios
    srand(time(NULL));
    
    int sizes[] = {32, 512, 1024};
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);

    printf("=== BENCHMARK DGEMM: NAIVE vs AVX vs AVX+BLOCKING ===\n");
    printf("==========================================================\n");
    
    for (int s = 0; s < num_sizes; s++) {
        int n = sizes[s];
        printf("\nTamanho da matriz: %d x %d\n", n, n);
        printf("==========================================================\n");
        
        double* A = alloc_matrix(n);
        double* B = alloc_matrix(n);
        double* C = alloc_matrix(n);

        run_benchmark(dgemm_naive, n, A, B, C, "Naive (IKJ)");
        run_benchmark(dgemm_avx, n, A, B, C, "AVX (Pure)");
        run_benchmark(dgemm_avx_block_prefetch, n, A, B, C, "AVX+Block+Prefetch");
        
        _mm_free(A); 
        _mm_free(B); 
        _mm_free(C);
    }
    
    printf("\n==========================================================\n");
    printf("Benchmark concluído!\n");
    
    return 0;
}