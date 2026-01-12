// pipeline_ctrl.v
module pipeline_ctrl(
    input stall,           // Do hazard_unit
    input branch_taken,    // Do estágio EX (branch foi tomado?)
    input jal_taken,       // Do estágio EX (jal foi executado?)
    input jalr_taken,      // Do estágio EX (jalr foi executado?)
    
    // Sinais de controle para o pipeline
    output reg pc_write,     // Para PC: 1=atualiza, 0=congela
    output reg if_id_write,  // Para IF/ID: 1=atualiza, 0=congela
    output reg if_id_flush,  // Para IF/ID: 1=insere NOP (flush)
    output reg id_ex_flush   // Para ID/EX: 1=insere NOP (flush)
);

    // Sinal combinado para jumps/branches
    wire jump_taken;
    assign jump_taken = branch_taken | jal_taken | jalr_taken;
    
    always @(*) begin
        // Valores padrão: tudo normal, pipeline fluindo
        pc_write = 1'b1;
        if_id_write = 1'b1;
        if_id_flush = 1'b0;
        id_ex_flush = 1'b0;
        
        // CASO 1: Stall (Load-Use Hazard)
        if (stall) begin
            // Congela o pipeline
            pc_write = 1'b0;      // PC não atualiza
            if_id_write = 1'b0;   // IF/ID não atualiza
            id_ex_flush = 1'b1;   // Insere NOP em ID/EX (bolha)
        end
        
        // CASO 2: Jump/Branch taken
        else if (jump_taken) begin
            // Descartar instruções erradas no pipeline
            if_id_flush = 1'b1;   // Descarta instrução em IF/ID
            id_ex_flush = 1'b1;   // Descarta instrução em ID/EX
        end
        
        // Se não stall nem jump, mantém valores padrão
    end
    
endmodule