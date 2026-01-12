PROJETO CPU RISC-V PIPELINE COM EXTENSÃO VETORIAL SIMPLIFICADA

RESUMO DOS COMPONENTES E DECISÕES ARBITRÁRIAS

1. ARQUITETURA GERAL
- Pipeline de 5 estágios: IF, ID, EX, MEM, WB
- Suporte ao conjunto básico RV32I (add, sub, lw, sw, beq, etc.)
- Extensão vetorial personalizada (não compatível com RISC-V V-extension oficial)

2. COMPONENTES IMPLEMENTADOS

2.1 pc.v (Contador de Programa)
- Atualiza na borda de subida do clock
- Reset assíncrono para inicialização independente
- Sinal 'pc_write' habilita/desabilita atualização (para stalls)

2.2 regfile.v (Banco de Registradores)
- 32 registradores de 32 bits
- Leitura combinacional, escrita síncrona
- Registrador x0 sempre zero
- Usado tanto para operações escalares quanto vetoriais

2.3 alu.v (Unidade Lógico-Aritmética)
- Suporte a operações escalares: ADD, SUB, AND, OR, XOR, SLL, SRL, SRA
- Suporte a operações vetoriais: 
  * Modo 4x8 bits: vadd.8, vsub.8, vand.8, vor.8
  * Modo 2x16 bits: vadd.16, vsub.16, vand.16, vor.16
- Sinal de controle de 6 bits: [5:4] = modo, [3:0] = operação

2.4 control.v (Unidade de Controle)
- Decodifica opcode de 7 bits
- Gera sinais: reg_write, mem_to_reg, mem_read, mem_write, alu_src, alu_op, branch
- Reconhece opcode vetorial personalizado: 7'b0100111
- Sinal 'is_vector' identifica instruções vetoriais

2.5 imm_gen.v (Gerador de Imediatos)
- Extrai e estende imediatos para todos formatos RISC-V
- I-type, S-type, B-type, U-type, J-type
- Extensão de sinal para imediatos com sinal

2.6 alu_control.v (Controle da ALU)
- Recebe alu_op[1:0] da control unit
- Para modo vetorial: funct3 define operação específica
- Para modo escalar: funct3 + funct7 definem operação
- Saída de 6 bits para controle da ALU

2.7 hazard_unit.v (Detecção de Hazards)
- Detecta apenas hazards do tipo load-use
- Stall quando instrução em EX é lw e ID precisa do resultado
- Ignora registrador x0

2.8 forwarding_unit.v (Encaminhamento)
- Forwarding de MEM e WB para EX
- Prioridade: MEM > WB (dado mais recente)
- Não há forwarding de EX (dado não está pronto)

2.9 pipeline_ctrl.v (Controle do Pipeline)
- Gera sinais de enable/flush para registradores
- Stall: congela PC e IF/ID, flusha ID/EX
- Branch taken: flusha IF/ID e ID/EX

2.10 Registradores de Pipeline (if_id_reg.v, id_ex_reg.v, ex_mem_reg.v, mem_wb_reg.v)
- Todos com reset assíncrono
- Suporte a enable (if_id_write) e flush
- Transportam sinais necessários entre estágios
- id_ex_reg.v transporta sinal 'is_vector'

2.11 cpu_top.v (Integração - esqueleto)
- Conecta todos componentes
- Pontos de conexão para memórias externas

3. DECISÕES ARBITRÁRIAS PRINCIPAIS

3.1 Extensão Vetorial Personalizada
- Opcode: 7'b0100111 (reservado no RISC-V, diferente do oficial 1010111)
- Motivo: Não interferir com padrão RISC-V, liberdade de design

3.2 Modelo de Computação Vetorial
- SIMD usando registradores inteiros existentes
- Não implementa registradores vetoriais separados (32 novos registradores)
- Motivo: Simplificação extrema vs especificação oficial complexa

3.3 Tamanhos Vetoriais Suportados
- Apenas 4 elementos de 8 bits e 2 elementos de 16 bits
- Não suporta vetores de tamanho configurável
- Motivo: Implementação simples, foco no conceito

3.4 Operações Vetoriais Implementadas
- Apenas add, sub, and, or
- Não implementa: multiplicação, shifts, comparações, saturação
- Motivo: Demonstrar conceito com mínimo de complexidade

3.5 Arquitetura de Controle
- Dois níveis: control.v → alu_op, alu_control.v → alu_ctrl
- alu_ctrl[5:4] = modo (00=escalar, 01=4x8b, 10=2x16b)
- Motivo: Organização clara, extensível

3.6 Tratamento de Hazards
- Stall apenas para load-use hazard
- Forwarding apenas de MEM e WB
- Motivo: Cobertura dos casos mais comuns, simplicidade

3.7 Formato de Instrução Vetorial
- Usa formato R-type (mesmo que add, sub)
- funct3 seleciona operação vetorial
- Motivo: Reutilizar decodificação existente

3.8 Ausência de Recursos Avançados
- Sem máscaras (masking)
- Sem load/store vetoriais  
- Sem CSRs de configuração
- Motivo: Foco no pipeline básico, não em vetores complexos

4. PONTOS DE INTEGRAÇÃO COM O DIGITAL

4.1 Memória de Instruções (ROM)
- Endereço: pc_out[31:0]
- Instrução lida: instruction[31:0]
- Converter endereço byte para palavra: pc_out[11:2] para ROM de 1KB

4.2 Memória de Dados (RAM)
- Endereço: alu_result_out[31:0] (do estágio MEM)
- Dados de escrita: write_data_out[31:0]
- Dados de leitura: mem_data_in[31:0]
- Sinais de controle: mem_read_out, mem_write_out

4.3 Sinais de Clock e Reset
- Clock global: clk
- Reset global: reset (ativo alto)

5. INSTRUÇÕES SUPORTADAS

5.1 Instruções Escalares (RV32I básico)
- R-type: add, sub, and, or, xor, sll, srl, sra
- I-type: addi, andi, ori, xori, slli, srli, srai
- Load/Store: lw, sw
- Branch: beq, bne
- Outras: lui

5.2 Instruções Vetoriais Personalizadas
- vadd.8  rd, rs1, rs2  (funct3=000)
- vadd.16 rd, rs1, rs2  (funct3=001)
- vsub.8  rd, rs1, rs2  (funct3=010)
- vsub.16 rd, rs1, rs2  (funct3=011)
- vand.8  rd, rs1, rs2  (funct3=100)
- vand.16 rd, rs1, rs2  (funct3=101)
- vor.8   rd, rs1, rs2  (funct3=110)
- vor.16  rd, rs1, rs2  (funct3=111)

6. LIMITAÇÕES CONHECIDAS

6.1 Comparação com RISC-V V-extension Oficial
- Não compatível - é uma extensão personalizada
- Muito mais simples (aprox. 1% da complexidade)
- Usa registradores inteiros, não vetoriais dedicados

6.2 Ausência de Recursos
- Não suporta instruções de sistema (ecall, ebreak)
- Não suporta multiplicação/divisão
- Não suporta instruções de ponto flutuante
- Branch prediction simples (sempre not taken)

6.3 Performance
- Stalls para todos hazards load-use
- Penalidade de 1 ciclo para branches taken
- Forwarding limitado a MEM e WB

7. PRÓXIMOS PASSOS PARA INTEGRAÇÃO COMPLETA

7.1 Conexão das Memórias
- Criar wrapper para ROM do Digital
- Criar wrapper para RAM do Digital
- Configurar endereçamento correto (byte → word)

7.2 Lógica de Controle de Fluxo
- Implementar cálculo de next_pc
- Suporte a jumps (jal, jalr)
- Detecção de branch taken

7.3 Multiplexadores de Forwarding
- Conectar MUXes na entrada da ALU
- Selecionar entre: banco de registradores, MEM, WB

7.4 Testes
- Criar programa de teste em .hex
- Verificar pipeline básico
- Testar operações vetoriais
- Validar hazard handling

8. RESUMO DE ARQUITETURA

8.1 Pipeline
IF:  Busca instrução da ROM
ID:  Decodifica + lê registradores + gera imediato
EX:  Execução na ALU + forwarding
MEM: Acesso à RAM (apenas lw/sw)
WB:  Escrita no banco de registradores

8.2 Extensão Vetorial
- Modo transparente ao pipeline
- Mesmos estágios, mesma latência
- ALU expandida para operações SIMD
- Reutiliza toda infraestrutura existente

9. JUSTIFICATIVAS DE PROJETO

9.1 Simplicidade vs Completude
- Escolhida simplicidade para foco educacional
- Pipeline funcional > conjunto de instruções completo
- Conceitos de hazards/forwarding > performance máxima

9.2 Extensão Vetorial Educacional
- Demonstra extensibilidade do RISC-V
- Mostra conceitos SIMD sem complexidade da V-extension
- Permite foco no pipeline, não em vetores complexos

9.3 Decisões de Implementação
- Reset assíncrono: facilita depuração
- Leitura combinacional de registradores: menor latency
- Stall apenas para load-use: cobre caso mais crítico

10. CÓDIGOS HEXADECIMAIS EXEMPLO

10.1 Instrução Vetorial vadd.8 x3, x1, x2
Formato R-type: | funct7 | rs2 | rs1 | funct3 | rd | opcode |
Valores: funct7=0000000, rs2=00010, rs1=00001, funct3=000, rd=00011, opcode=0100111
Código binário: 0000000_00010_00001_000_00011_0100111
Hexadecimal: 0x00208127

10.2 Programa de Teste Simples
# Instruções em hex para ROM
00500093  # addi x1, x0, 5
00C00113  # addi x2, x0, 12
00208127  # vadd.8 x3, x1, x2  (nossa instrução)
00000013  # nop
00000013  # nop


### PARTE 2 ###

RESUMO DAS DECISÕES ARBITRÁRIAS E STATUS FINAL

DECISÕES ARBITRÁRIAS PRINCIPAIS:

1. EXTENSÃO VETORIAL PERSONALIZADA
   - Opcode escolhido: 7'b0100111
   - Por que: É um opcode reservado na especificação RISC-V, permitindo criar uma extensão 
     personalizada sem conflitar com instruções padrão. Diferente do 1010111 usado pela 
     extensão vetorial oficial, evitando confusão.

2. MODELO DE COMPUTAÇÃO SIMPLIFICADO
   - Usa os mesmos 32 registradores inteiros para dados escalares e vetoriais
   - Por que: A extensão vetorial oficial exige 32 novos registradores vetoriais e lógica 
     complexa. Nosso modelo mantém a simplicidade do banco de registradores existente.

3. FORMATOS VETORIAIS FIXOS
   - Apenas 4 elementos de 8 bits (4x8b) e 2 elementos de 16 bits (2x16b)
   - Por que: A V-extension oficial permite vetores de tamanho configurável via CSRs. 
     Formatos fixos simplificam drasticamente a ALU e o controle.

4. CONJUNTO LIMITADO DE OPERAÇÕES
   - Apenas add, sub, and, or para cada formato
   - Por que: A V-extension oficial tem dezenas de operações. Este subconjunto mínimo 
     demonstra o conceito de SIMD sem complexidade excessiva.

5. ARQUITETURA DE CONTROLE DE DOIS NÍVEIS
   - control.v → alu_op[1:0] → alu_control.v → alu_ctrl[5:0]
   - alu_ctrl[5:4] = modo vetorial (00=escalar, 01=4x8b, 10=2x16b)
   - Por que: Separa decisões de alto nível (tipo de instrução) das decisões de baixo 
     nível (operação exata da ALU). Organizado e extensível.

6. TRATAMENTO DE HAZARDS SIMPLIFICADO
   - Stall apenas para hazards do tipo load-use
   - Forwarding apenas dos estágios MEM e WB (não de EX)
   - Por que: Cobre os casos mais comuns sem a complexidade de detecção completa de 
     todos os hazards possíveis.

7. AUSÊNCIA DE RECURSOS AVANÇADOS
   - Sem máscaras (masking), sem saturação, sem arredondamento
   - Sem instruções de load/store vetoriais
   - Por que: Foco principal é o pipeline de 5 estágios e conceitos de hazard/forwarding, 
     não operações vetoriais completas.

STATUS FINAL DO PROJETO:

✅ COMPONENTES IMPLEMENTADOS E PRONTOS:
1. pc.v - Contador de programa com enable para stall
2. regfile.v - Banco de 32 registradores (leitura combinacional, escrita no clock)
3. alu.v - ALU com suporte a operações escalares + vetoriais (4x8b, 2x16b)
4. control.v - Unidade de controle que reconhece opcode vetorial 0100111
5. imm_gen.v - Gerador de imediatos para todos formatos RISC-V
6. alu_control.v - Decodificador que gera sinais de 6 bits para a ALU
7. hazard_unit.v - Detecta apenas hazards load-use
8. forwarding_unit.v - Encaminha dados de MEM e WB para EX
9. pipeline_ctrl.v - Gera sinais de enable/flush para o pipeline
10. if_id_reg.v - Registrador IF/ID com enable e flush
11. id_ex_reg.v - Registrador ID/EX com flush e transporte de sinal is_vector
12. ex_mem_reg.v - Registrador EX/MEM
13. mem_wb_reg.v - Registrador MEM/WB
14. cpu_top.v - Esqueleto de integração (conexões principais)

🔄 COMPONENTES QUE PRECISAM SER CONECTADOS/COMPLETADOS:
1. WRAPPER PARA ROM DO DIGITAL - Interface para memória de instruções
2. WRAPPER PARA RAM DO DIGITAL - Interface para memória de dados  
3. LÓGICA DE CÁLCULO DE next_pc - Para branches e jumps
4. MULTIPLEXADORES DE FORWARDING - Na entrada da ALU
5. TESTBENCH/VERIFICAÇÃO - Programas de teste em .hex

📋 PRÓXIMOS PASSOS PARA CPU FUNCIONAL:
1. Criar wrappers para as memórias do Digital
2. Conectar todos os MUXes de seleção de dados
3. Implementar lógica de desvio (branch/jump)
4. Carregar programa de teste na ROM
5. Simular e depurar o pipeline completo



### Parte 3 ###
DECISÕES ARBITRÁRIAS - CÓDIGOS E TAMANHOS

1. CODIFICAÇÃO DA ALU (alu_ctrl[5:0])
   Bits [5:4]: Modo de operação
     00 = Escalar (32-bit)
     01 = 4 elementos de 8-bit
     10 = 2 elementos de 16-bit
     11 = Reservado
   Bits [3:0]: Operação específica
     0000 = ADD / vadd
     0001 = SUB / vsub
     0010 = AND / vand
     0011 = OR / vor
     0100 = XOR
     0101 = SLL
     0110 = SRL
     0111 = SRA
     1000 = LUI (passa imediato)
   Por que arbitrário: Sequência lógica para facilitar depuração.

2. OPCODE VETORIAL PERSONALIZADO
   Opcode escolhido: 7'b0100111
   Por que arbitrário: É um opcode reservado no RISC-V, não conflita com o 
   padrão (1010111), permite liberdade de design.

3. FORMATO DA INSTRUÇÃO VETORIAL
   Usa formato R-type (igual add, sub)
   funct3 define operação (000=vadd.8, 001=vadd.16, etc.)
   Por que arbitrário: Reutiliza decodificação existente, poderia ser outro formato.

4. TAMANHOS DE MEMÓRIA
   ROM e RAM: Address bits = 10 (1KB, 256 palavras)
   Endereçamento: addr[9:2] para converter byte→word
   Por que arbitrário: Balance entre simplicidade e espaço para testes.

5. CONTROLE DE DOIS NÍVEIS
   control.v → alu_op[1:0] → alu_control.v → alu_ctrl[5:0]
   Por que arbitrário: Separa decisões de alto e baixo nível, mas não é obrigatório.

6. HAZARD HANDLING
   Stall apenas para Load-Use hazard
   Forwarding apenas de MEM e WB (não de EX)
   Prioridade: MEM > WB
   Por que arbitrário: Cobre casos mais comuns, simplifica implementação.

7. OPERAÇÕES VETORIAIS IMPLEMENTADAS
   Apenas: add, sub, and, or
   Não: mul, shifts, comparações, saturação, máscaras
   Por que arbitrário: Foco no conceito SIMD básico, não complexidade.

8. FORMATOS VETORIAIS
   Apenas: 4x8-bit e 2x16-bit
   Não: 8x4-bit, 1x32-bit, tamanhos variáveis
   Por que arbitrário: Casos comuns sem necessidade de CSRs de configuração.

9. PIPELINE CONTROL SEPARADO
   pipeline_ctrl.v separado de control.v
   Por que arbitrário: Separa responsabilidades (decodificação vs gerenciamento).

10. RESET ASSÍNCRONO
    always @(posedge clk or posedge reset)
    Por que arbitrário: Facilita depuração, comum em FPGAs educacionais.

11. BRANCH PREDICTION
    Sempre "not taken", flush se branch taken
    Por que arbitrário: Simplicidade máxima, penalidade de 1 ciclo aceitável.

RESUMO DAS ESCOLHAS DIDÁTICAS:
1. Foco no pipeline → Instruções vetoriais simplificadas
2. Foco em hazards → Forwarding/stall básico mas funcional
3. Foco em conceitos → Não em performance ou completude
4. Foco em depuração → Sinais explícitos, reset assíncrono
5. Compatibilidade educacional → Não padrão, mas demonstra extensibilidade









OBSERVAÇÃO EX/MEM -> RAM:
Do ex_mem_reg:   alu_result_out[31:0]
Para RAM:        A[9:2]  ← IMPORTANTE: só bits 9:2!

POR QUÊ? 
- alu_result_out é endereço de BYTE (0, 4, 8...)
- RAM espera endereço de PALAVRA (0, 1, 2...)
- Dividir por 4 = pegar bits [9:2]



Referencias adicionais:
https://five-embeddev.com/riscv-v-spec/v1.0/v-spec.html