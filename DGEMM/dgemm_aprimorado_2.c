#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <immintrin.h>
#include <stdint.h>
#include <unistd.h>

// --- CONFIGURAÇÕES ---
#define BLOCK_SIZE 32   // Otimizado para L1 Cache
#define NUM_RUNS 5      // Execuções para média estatística
#define WARMUP_RUNS 1   // Aquecimento de cache
#define MAX_METHODS 3   // Número de métodos implementados
#define MAX_SIZES 7     // Número máximo de tamanhos de matriz

// --- ESTRUTURAS DE DADOS ---
typedef struct {
    char vendor[13];
    char brand[49];
    int family;
    int model;
    int stepping;
    int cores;
    int threads;
    int avx_support;
    int avx2_support;
    int fma_support;
    float base_freq;    // GHz
    float max_freq;     // GHz
    size_t l1_cache;    // KB
    size_t l2_cache;    // KB
    size_t l3_cache;    // KB
} CPUInfo;

typedef struct {
    char name[50];
    double gflops[MAX_SIZES];
    double time[MAX_SIZES];
    double efficiency[MAX_SIZES];
} MethodResult;

// --- UTILITÁRIOS DE TEMPO (Alta Precisão) ---
double get_time_sec() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

// --- DETECÇÃO SIMPLIFICADA DE CAPACIDADES DA CPU ---
void detect_cpu_features(CPUInfo* cpu) {
    // Inicializar com valores padrão
    strcpy(cpu->vendor, "Desconhecido");
    strcpy(cpu->brand, "Processador Desconhecido");
    cpu->cores = sysconf(_SC_NPROCESSORS_ONLN);
    cpu->threads = cpu->cores;
    
    // Usar macros do compilador para detectar suporte AVX
    #ifdef __AVX__
    cpu->avx_support = 1;
    #else
    cpu->avx_support = 0;
    #endif
    
    #ifdef __AVX2__
    cpu->avx2_support = 1;
    #else
    cpu->avx2_support = 0;
    #endif
    
    #ifdef __FMA__
    cpu->fma_support = 1;
    #else
    cpu->fma_support = 0;
    #endif
    
    // Valores padrão razoáveis
    cpu->family = 0;
    cpu->model = 0;
    cpu->stepping = 0;
}

// --- MEDIÇÃO DE FREQUÊNCIA (Linux) ---
float get_cpu_freq() {
    FILE* fp = fopen("/proc/cpuinfo", "r");
    if (!fp) {
        return 2.5; // 2.5 GHz como padrão
    }
    
    char line[256];
    float freq = 0.0;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "cpu MHz")) {
            sscanf(line, "cpu MHz : %f", &freq);
            break;
        } else if (strstr(line, "model name")) {
            char* ghz = strstr(line, "GHz");
            if (ghz) {
                char freq_str[20];
                int i = 0;
                for (char* p = ghz - 1; p >= line && i < 10; p--) {
                    if ((*p >= '0' && *p <= '9') || *p == '.') {
                        freq_str[i++] = *p;
                    } else if (*p == '@') {
                        break;
                    }
                }
                freq_str[i] = '\0';
                for (int j = 0; j < i/2; j++) {
                    char temp = freq_str[j];
                    freq_str[j] = freq_str[i-1-j];
                    freq_str[i-1-j] = temp;
                }
                freq = atof(freq_str);
                break;
            }
        }
    }
    fclose(fp);
    
    if (freq < 100) {
        freq = freq * 1000;
    }
    
    return freq / 1000.0;
}

// --- VERIFICAÇÃO DE ALINHAMENTO ---
void check_alignment(double* ptr, int required, const char* name) {
    uintptr_t addr = (uintptr_t)ptr;
    if (addr % required != 0) {
        printf("[WARNING] %s não está alinhado em %d bytes! (endereço: %p)\n", 
               name, required, ptr);
    }
}

// --- ALOCAÇÃO E INICIALIZAÇÃO ---
double* alloc_matrix(int n, const char* name) {
    double* ptr = (double*)_mm_malloc(n * n * sizeof(double), 64);
    if (!ptr) {
        printf("[ERRO] Falha ao alocar %s\n", name);
        exit(1);
    }
    
    for (int i = 0; i < n * n; i++) {
        ptr[i] = (double)((i % 100) + 1) * 0.01;
    }
    
    check_alignment(ptr, 64, name);
    return ptr;
}

void clean_matrix(double* C, int n) {
    memset(C, 0, n * n * sizeof(double));
}

// --- KERNELS DGEMM ---

// 1. NAIVE (IKJ Optimization) - Baseline
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

// 2. AVX (Vectorized) - Otimização por vetorização
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
            for (; j < n; j++) {
                C[i * n + j] += A[i * n + k] * B[k * n + j];
            }
        }
    }
}

// 3. AVX + BLOCKING + LOOP UNROLLING (versão otimizada)
void dgemm_avx_block(int n, double* A, double* B, double* C) {
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
                        #ifdef __FMA__
                        for (; j <= j_max - 8; j += 8) {
                            __m256d c_vec1 = _mm256_load_pd(&C[i * n + j]);
                            __m256d b_vec1 = _mm256_load_pd(&B[k * n + j]);
                            c_vec1 = _mm256_fmadd_pd(a_vec, b_vec1, c_vec1);
                            _mm256_store_pd(&C[i * n + j], c_vec1);
                            
                            __m256d c_vec2 = _mm256_load_pd(&C[i * n + j + 4]);
                            __m256d b_vec2 = _mm256_load_pd(&B[k * n + j + 4]);
                            c_vec2 = _mm256_fmadd_pd(a_vec, b_vec2, c_vec2);
                            _mm256_store_pd(&C[i * n + j + 4], c_vec2);
                        }
                        #endif
                        
                        for (; j <= j_max - 4; j += 4) {
                            __m256d c_vec = _mm256_load_pd(&C[i * n + j]);
                            __m256d b_vec = _mm256_load_pd(&B[k * n + j]);
                            #ifdef __FMA__
                            c_vec = _mm256_fmadd_pd(a_vec, b_vec, c_vec);
                            #else
                            c_vec = _mm256_add_pd(c_vec, _mm256_mul_pd(a_vec, b_vec));
                            #endif
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

// --- BENCHMARK COMPLETO ---
double run_benchmark(void (*func)(int, double*, double*, double*), 
                     int n, double* A, double* B, double* C, 
                     const char* name, double peak_gflops, 
                     MethodResult* result, int method_idx, int size_idx) {
    
    printf("\n--- Executando: %s ---\n", name);
    
    // Warm-up
    for(int w = 0; w < WARMUP_RUNS; w++) {
        clean_matrix(C, n);
        func(n, A, B, C);
    }
    
    // Benchmark principal
    double min_time = 1e9;
    double max_time = 0;
    double total_time = 0.0;
    double total_gflops = 0.0;
    
    for (int r = 0; r < NUM_RUNS; r++) {
        clean_matrix(C, n);
        
        double start = get_time_sec();
        func(n, A, B, C);
        double end = get_time_sec();
        
        double elapsed = end - start;
        double operations = 2.0 * (double)n * (double)n * (double)n;
        double gflops = (operations / elapsed) * 1e-9;
        
        total_time += elapsed;
        total_gflops += gflops;
        
        if (elapsed < min_time) min_time = elapsed;
        if (elapsed > max_time) max_time = elapsed;
        
        printf("  Execução %d: %.4fs (%.2f GFLOPS)\n", r+1, elapsed, gflops);
    }
    
    double avg_time = total_time / NUM_RUNS;
    double avg_gflops = total_gflops / NUM_RUNS;
    double efficiency = (peak_gflops > 0) ? (avg_gflops / peak_gflops * 100.0) : 0.0;
    
    // Armazenar resultados
    strcpy(result[method_idx].name, name);
    result[method_idx].gflops[size_idx] = avg_gflops;
    result[method_idx].time[size_idx] = avg_time;
    result[method_idx].efficiency[size_idx] = efficiency;
    
    printf("\n  RESULTADO FINAL:\n");
    printf("  Tempo médio:    %.4fs\n", avg_time);
    printf("  GFLOPS médio:   %.2f\n", avg_gflops);
    if (max_time > min_time) {
        printf("  Variação:       ±%.1f%%\n", ((max_time - min_time) / avg_time * 50.0));
    }
    if (peak_gflops > 0) {
        printf("  Eficiência:     %.1f%% do pico teórico\n", efficiency);
    }
    printf("  Operações:      %.0f FLOPS\n", 2.0 * (double)n * (double)n * (double)n);
    
    return avg_gflops;
}

// --- ESTIMATIVA DE DESEMPENHO PICO ---
double estimate_peak_gflops(int cores, float freq) {
    return freq * cores * 8 * 2;
}

// --- IMPRIMIR MATRIZ DE RESULTADOS ---
void print_results_matrix(MethodResult* results, int num_methods, 
                          int* sizes, int num_sizes, double peak_gflops) {
    
    printf("\n╔════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                                                  MATRIZ DE RESULTADOS - DGEMM BENCHMARK                                                  ║\n");
    printf("╠═══════════╦═════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n");
    printf("║ Tamanho   ║");
    
    // Cabeçalho dos métodos
    for (int m = 0; m < num_methods; m++) {
        if (m < num_methods - 1) {
            printf(" %-28s ║", results[m].name);
        } else {
            printf(" %-28s ║", results[m].name);
        }
    }
    printf("\n");
    printf("╠═══════════╬═════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n");
    
    // Dados para cada tamanho
    for (int s = 0; s < num_sizes; s++) {
        int n = sizes[s];
        printf("║ %4d x %-4d ║", n, n);
        
        for (int m = 0; m < num_methods; m++) {
            if (results[m].gflops[s] > 0) {
                printf(" %6.2f GFLOPS (%5.1f%%) ║", 
                       results[m].gflops[s], 
                       results[m].efficiency[s]);
            } else {
                printf(" %-28s ║", "N/A");
            }
        }
        printf("\n");
        
        // Linha separadora entre tamanhos
        if (s < num_sizes - 1) {
            printf("║           ╠═════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n");
        }
    }
    
    printf("╚═══════════╩═════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╝\n");
    
    // Tabela de tempos
    printf("\n╔════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                                                TEMPOS DE EXECUÇÃO (segundos)                                                           ║\n");
    printf("╠═══════════╦═════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n");
    printf("║ Tamanho   ║");
    
    for (int m = 0; m < num_methods; m++) {
        printf(" %-28s ║", results[m].name);
    }
    printf("\n");
    printf("╠═══════════╬═════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n");
    
    for (int s = 0; s < num_sizes; s++) {
        int n = sizes[s];
        printf("║ %4d x %-4d ║", n, n);
        
        for (int m = 0; m < num_methods; m++) {
            if (results[m].time[s] > 0) {
                printf(" %10.4f s              ║", results[m].time[s]);
            } else {
                printf(" %-28s ║", "N/A");
            }
        }
        printf("\n");
        
        if (s < num_sizes - 1) {
            printf("║           ╠═════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n");
        }
    }
    printf("╚═══════════╩═════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╝\n");
    
    // Tabela de speedup relativo
    printf("\n╔════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                                               SPEEDUP RELATIVO (vs Naive)                                                             ║\n");
    printf("╠═══════════╦═════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n");
    printf("║ Tamanho   ║");
    
    for (int m = 1; m < num_methods; m++) {
        printf(" %-28s ║", results[m].name);
    }
    printf("\n");
    printf("╠═══════════╬═════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n");
    
    for (int s = 0; s < num_sizes; s++) {
        int n = sizes[s];
        printf("║ %4d x %-4d ║", n, n);
        
        for (int m = 1; m < num_methods; m++) {
            if (results[0].gflops[s] > 0 && results[m].gflops[s] > 0) {
                double speedup = results[m].gflops[s] / results[0].gflops[s];
                printf(" %6.2fx mais rápido       ║", speedup);
            } else {
                printf(" %-28s ║", "N/A");
            }
        }
        printf("\n");
        
        if (s < num_sizes - 1) {
            printf("║           ╠═════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n");
        }
    }
    printf("╚═══════════╩═════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╝\n");
    
    // Resumo estatístico
    printf("\n══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n");
    printf("                                         RESUMO ESTATÍSTICO\n");
    printf("══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n");
    
    for (int m = 0; m < num_methods; m++) {
        printf("\n%s:\n", results[m].name);
        
        double max_gflops = 0;
        double min_gflops = 1e9;
        double avg_gflops = 0;
        int valid_sizes = 0;
        
        for (int s = 0; s < num_sizes; s++) {
            if (results[m].gflops[s] > 0) {
                if (results[m].gflops[s] > max_gflops) max_gflops = results[m].gflops[s];
                if (results[m].gflops[s] < min_gflops) min_gflops = results[m].gflops[s];
                avg_gflops += results[m].gflops[s];
                valid_sizes++;
            }
        }
        
        if (valid_sizes > 0) {
            avg_gflops /= valid_sizes;
            printf("  • Máximo: %.2f GFLOPS\n", max_gflops);
            printf("  • Mínimo: %.2f GFLOPS\n", min_gflops);
            printf("  • Média:  %.2f GFLOPS\n", avg_gflops);
            printf("  • Eficiência média: %.1f%% do pico teórico\n", avg_gflops / peak_gflops * 100.0);
        }
    }
}

// --- FUNÇÃO PRINCIPAL ---
int main() {
    printf("==========================================================\n");
    printf("           BENCHMARK DGEMM - OTIMIZAÇÃO AVX\n");
    printf("==========================================================\n");
    
    // Configurar semente aleatória
    srand(time(NULL));
    
    // Informações do sistema
    CPUInfo cpu = {0};
    detect_cpu_features(&cpu);
    float current_freq = get_cpu_freq();
    int actual_cores = sysconf(_SC_NPROCESSORS_ONLN);
    
    printf("\n=== INFORMAÇÕES DO SISTEMA ===\n");
    printf("Processador:      %s\n", cpu.brand);
    printf("Vendor:           %s\n", cpu.vendor);
    printf("Núcleos lógicos:  %d\n", actual_cores);
    printf("Frequência atual: %.2f GHz\n", current_freq);
    printf("\nCapacidades SIMD detectadas pelo compilador:\n");
    printf("  - AVX:          %s\n", cpu.avx_support ? "SIM" : "NÃO");
    printf("  - AVX2:         %s\n", cpu.avx2_support ? "SIM" : "NÃO");
    printf("  - FMA:          %s\n", cpu.fma_support ? "SIM" : "NÃO");
    
    // Calcular desempenho pico teórico
    double peak_gflops = estimate_peak_gflops(actual_cores, current_freq);
    printf("\nDesempenho pico estimado: %.0f GFLOPS\n", peak_gflops);
    printf("(Baseado em %.2f GHz × %d núcleos × 16 FLOPS/ciclo)\n", 
           current_freq, actual_cores);
    
    // Tamanhos das matrizes para teste
    int sizes[] = {64, 128, 256, 512, 1024, 2048};
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);
    
    // Estrutura para armazenar resultados
    MethodResult results[MAX_METHODS];
    for (int i = 0; i < MAX_METHODS; i++) {
        for (int j = 0; j < MAX_SIZES; j++) {
            results[i].gflops[j] = 0;
            results[i].time[j] = 0;
            results[i].efficiency[j] = 0;
        }
    }
    
    printf("\n=== CONFIGURAÇÃO DO TESTE ===\n");
    printf("Block size:       %d (otimizado para cache L1)\n", BLOCK_SIZE);
    printf("Execuções:        %d por benchmark\n", NUM_RUNS);
    printf("Warm-up:          %d execução\n", WARMUP_RUNS);
    printf("\n");

    // Executar benchmarks para cada tamanho
    printf("=== EXECUTANDO BENCHMARKS ===\n");
    for (int s = 0; s < num_sizes; s++) {
        int n = sizes[s];
        
        // Verificar uso de memória aproximado
        size_t mem_usage = 3 * n * n * sizeof(double) / (1024*1024);
        if (mem_usage > 4096) { // Limitar a 4GB
            printf("\n[INFO] Pulando tamanho %dx%d (requer ~%zu MB - muito grande)\n", 
                   n, n, mem_usage);
            continue;
        }
        
        printf("\n══════════════════════════════════════════════════════════════\n");
        printf("Processando matriz %dx%d (~%zu MB)...\n", n, n, mem_usage);
        printf("══════════════════════════════════════════════════════════════\n");
        
        // Alocar matrizes
        double* A = alloc_matrix(n, "Matriz A");
        double* B = alloc_matrix(n, "Matriz B");
        double* C = alloc_matrix(n, "Matriz C");
        
        // Executar cada versão e armazenar resultados
        run_benchmark(dgemm_naive, n, A, B, C, "Naive (IKJ)", peak_gflops, results, 0, s);
        run_benchmark(dgemm_avx, n, A, B, C, "AVX (Pure)", peak_gflops, results, 1, s);
        run_benchmark(dgemm_avx_block, n, A, B, C, "AVX+Blocking+Unroll", peak_gflops, results, 2, s);
        
        // Liberar memória
        _mm_free(A); 
        _mm_free(B); 
        _mm_free(C);
        
        // Pequena pausa entre testes grandes
        if (n >= 512) {
            printf("\n[Aguardando 1s para estabilização térmica...]\n");
            sleep(1);
        }
    }
    
    // Imprimir matriz de resultados
    print_results_matrix(results, MAX_METHODS, sizes, num_sizes, peak_gflops);
    
    // Informações finais
    printf("\n══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n");
    printf("                                            INFORMAÇÕES DO SISTEMA\n");
    printf("══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n");
    printf("Data e hora da execução: %s", ctime(&(time_t){time(NULL)}));
    printf("Tempo total de benchmark: %.1f segundos\n", get_time_sec());
    printf("Pico teórico da CPU: %.0f GFLOPS\n", peak_gflops);
    printf("Flags de compilação usadas:\n");
    #ifdef __AVX2__
    printf("  - AVX2: SIM\n");
    #endif
    #ifdef __FMA__
    printf("  - FMA: SIM\n");
    #endif
    #ifdef __AVX__
    printf("  - AVX: SIM\n");
    #endif
    
    printf("\n══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n");
    printf("                                         INTERPRETAÇÃO DOS RESULTADOS\n");
    printf("══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n");
    printf("1. Matriz pequena (<256): Cache L1/L2 dominante, vetorização mais eficiente\n");
    printf("2. Matriz média (256-1024): Cache L3 importante, blocking crucial\n");
    printf("3. Matriz grande (>1024): Memória principal limitante, otimizações de acesso essenciais\n");
    printf("\n");
    printf("OBS: Eficiência calculada como (GFLOPS alcançados / GFLOPS teóricos) × 100%%\n");
    
    return 0;
}