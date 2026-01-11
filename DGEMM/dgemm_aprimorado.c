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

// --- ESTRUTURA DE INFORMAÇÕES DA CPU ---
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
        // Valor padrão se não conseguir ler
        return 2.5; // 2.5 GHz como padrão
    }
    
    char line[256];
    float freq = 0.0;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "cpu MHz")) {
            sscanf(line, "cpu MHz : %f", &freq);
            break;
        } else if (strstr(line, "model name")) {
            // Tentar extrair frequência do nome do modelo
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
                // Inverter a string
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
    
    // Se não encontrou MHz, converter GHz para MHz
    if (freq < 100) { // Provavelmente está em GHz
        freq = freq * 1000;
    }
    
    return freq / 1000.0; // MHz para GHz
}

// --- VERIFICAÇÃO DE ALINHAMENTO ---
void check_alignment(double* ptr, int required, const char* name) {
    uintptr_t addr = (uintptr_t)ptr;
    if (addr % required != 0) {
        printf("[WARNING] %s não está alinhado em %d bytes! (endereço: %p)\n", 
               name, required, ptr);
    } else {
        printf("[OK] %s alinhado em %d bytes\n", name, required);
    }
}

// --- ALOCAÇÃO E INICIALIZAÇÃO ---
double* alloc_matrix(int n, const char* name) {
    double* ptr = (double*)_mm_malloc(n * n * sizeof(double), 64);
    if (!ptr) {
        printf("[ERRO] Falha ao alocar %s\n", name);
        exit(1);
    }
    
    // Inicializar com valores que facilitam debug
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
            // Cleanup para N não múltiplo de 4
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
                        // Loop desenrolado 2x (se FMA disponível)
                        #ifdef __FMA__
                        for (; j <= j_max - 8; j += 8) {
                            // Bloco 1
                            __m256d c_vec1 = _mm256_load_pd(&C[i * n + j]);
                            __m256d b_vec1 = _mm256_load_pd(&B[k * n + j]);
                            c_vec1 = _mm256_fmadd_pd(a_vec, b_vec1, c_vec1);
                            _mm256_store_pd(&C[i * n + j], c_vec1);
                            
                            // Bloco 2
                            __m256d c_vec2 = _mm256_load_pd(&C[i * n + j + 4]);
                            __m256d b_vec2 = _mm256_load_pd(&B[k * n + j + 4]);
                            c_vec2 = _mm256_fmadd_pd(a_vec, b_vec2, c_vec2);
                            _mm256_store_pd(&C[i * n + j + 4], c_vec2);
                        }
                        #endif
                        
                        // Loop regular para o restante
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
                        
                        // Scalar cleanup
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
                     const char* name, double peak_gflops) {
    
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
    // Estimativa simplificada:
    // Frequência (GHz) × Núcleos × Operações por ciclo
    // AVX pode fazer 8 ops FMA por ciclo (4 multiplicações + 4 adições)
    return freq * cores * 8 * 2; // ×2 para FMA
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
    
    // Obter número real de núcleos
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
    
    // Tamanhos das matrizes para teste (adaptados ao cache)
    int sizes[] = {64, 128, 256, 512, 1024};
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);
    
    printf("\n=== CONFIGURAÇÃO DO TESTE ===\n");
    printf("Block size:       %d (otimizado para cache L1)\n", BLOCK_SIZE);
    printf("Execuções:        %d por benchmark\n", NUM_RUNS);
    printf("Warm-up:          %d execução\n", WARMUP_RUNS);
    printf("\n");

    // Cabeçalho da tabela de resultados
    printf("=== RESULTADOS DO BENCHMARK ===\n");
    printf("+--------+------------+------------+------------+------------+\n");
    printf("| Tamanho| Naive      | AVX        | AVX+Block  | Efic.(%%)   |\n");
    printf("+--------+------------+------------+------------+------------+\n");
    
    // Executar benchmarks para cada tamanho
    for (int s = 0; s < num_sizes; s++) {
        int n = sizes[s];
        
        // Verificar uso de memória aproximado
        size_t mem_usage = 3 * n * n * sizeof(double) / (1024*1024);
        if (mem_usage > 512) { // Limitar a 512MB
            printf("\n[INFO] Pulando tamanho %dx%d (requer ~%zu MB)\n", 
                   n, n, mem_usage);
            continue;
        }
        
        printf("\nProcessando matriz %dx%d (~%zu MB)...\n", n, n, mem_usage);
        
        // Alocar matrizes
        double* A = alloc_matrix(n, "Matriz A");
        double* B = alloc_matrix(n, "Matriz B");
        double* C = alloc_matrix(n, "Matriz C");
        
        // Executar cada versão
        double gflops_naive = run_benchmark(dgemm_naive, n, A, B, C, "Naive (IKJ)", 0);
        double gflops_avx = run_benchmark(dgemm_avx, n, A, B, C, "AVX (Pure)", peak_gflops);
        double gflops_avx_block = run_benchmark(dgemm_avx_block, n, A, B, C, 
                                               "AVX+Blocking+Unroll", peak_gflops);
        
        // Calcular speedup
        double speedup_avx = (gflops_naive > 0) ? gflops_avx / gflops_naive : 0;
        double speedup_block = (gflops_naive > 0) ? gflops_avx_block / gflops_naive : 0;
        double efficiency = (peak_gflops > 0) ? (gflops_avx_block / peak_gflops) * 100.0 : 0;
        
        // Imprimir linha da tabela
        printf("| %6d | %10.2f | %10.2f | %10.2f | %10.1f |\n", 
               n, gflops_naive, gflops_avx, gflops_avx_block, efficiency);
        
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
    
    printf("+--------+------------+------------+------------+------------+\n");
    
    // Resumo final e recomendações
    printf("\n=== ANÁLISE E CONCLUSÕES ===\n");
    printf("\n1. IMPACTO DAS OTIMIZAÇÕES:\n");
    printf("   - Vetorização AVX: Aceleração de 4-8x sobre código naive\n");
    printf("   - Blocking: Melhora localidade de cache, crucial para >512\n");
    printf("   - Loop unrolling: Reduz overhead de controle\n");
    
    printf("\n2. FATORES QUE AFETAM DESEMPENHO:\n");
    printf("   • Alinhamento de memória (crítico para AVX)\n");
    printf("   • Tamanho do cache (L1/L2/L3)\n");
    printf("   • Frequência da CPU e thermal throttling\n");
    printf("   • Overhead de chamadas de função\n");
    
    printf("\n3. PRÓXIMOS PASSOS PARA OTIMIZAÇÃO:\n");
    printf("   • Paralelização com OpenMP (multi-core)\n");
    printf("   • Otimização de prefetch manual\n");
    printf("   • Ajuste fino do block size para cache específico\n");
    printf("   • Uso de AVX-512 (se disponível)\n");
    
    printf("\n=== COMPILAÇÃO RECOMENDADA ===\n");
    printf("# Compilar com todas as otimizações:\n");
    printf("gcc -O3 -mavx2 -mfma -march=native -funroll-loops \\\n");
    printf("    -ftree-vectorize -o dgemm_benchmark dgemm_final.c\n");
    
    printf("\n# Para debug de vetorização:\n");
    printf("gcc -O3 -mavx2 -mfma -march=native -fopt-info-vec \\\n");
    printf("    -o dgemm_debug dgemm_final.c 2> vectorization.log\n");
    
    printf("\n==========================================================\n");
    time_t now = time(NULL);
    printf("Benchmark concluído em: %s", ctime(&now));
    printf("Tempo total de execução: %.1f segundos\n", get_time_sec());
    printf("==========================================================\n");
    
    return 0;
}