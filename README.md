# ArqComp_Trabalho
Codigos_e_Etc

dgemm_test --- código que está no trabalho atual
Dgemm aprimorado --- É bom, mas não tem mt coisa, e é confuso de obter os resultados
Dgeem aprimorado_2 --- Faz uma tabela no final com tudo

to compile dgemm_tes: 
gcc -march=native -O3 -mavx teste.c -o dgemm_test


to dgemm aprimorado:
gcc -O3 -mavx2 -mfma -march=native -funroll-loops -fopt-info-vec -o dgemm_aprimorado dgemm_aprimorado.c