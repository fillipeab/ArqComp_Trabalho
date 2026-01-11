// if_id_reg.v - Registrador entre IF e ID
module if_id_reg(
    input clk,
    input reset,
    input [31:0] pc_in,      // PC+4 do estágio IF
    input [31:0] instr_in,   // Instrução da memória
    output reg [31:0] pc_out,
    output reg [31:0] instr_out
);
    
    always @(posedge clk or posedge reset) begin
        if (reset) begin
            pc_out <= 32'b0;
            instr_out <= 32'b0;
        end else begin
            pc_out <= pc_in;
            instr_out <= instr_in;
        end
    end
    
endmodule


// id_ex_reg.v - Registrador entre ID e EX
module id_ex_reg(
    input clk,
    input reset,
    
    // Sinais de controle da ID
    input reg_write_in,
    input mem_to_reg_in,
    input mem_read_in,
    input mem_write_in,
    input [2:0] alu_ctrl_in,
    
    // Dados da ID
    input [31:0] pc_in,
    input [31:0] read_data1_in,
    input [31:0] read_data2_in,
    input [31:0] imm_in,
    input [4:0] rd_in,
    
    // Saídas para EX
    output reg reg_write_out,
    output reg mem_to_reg_out,
    output reg mem_read_out,
    output reg mem_write_out,
    output reg [2:0] alu_ctrl_out,
    output reg [31:0] pc_out,
    output reg [31:0] read_data1_out,
    output reg [31:0] read_data2_out,
    output reg [31:0] imm_out,
    output reg [4:0] rd_out
);
    
    always @(posedge clk or posedge reset) begin
        if (reset) begin
            reg_write_out <= 1'b0;
            mem_to_reg_out <= 1'b0;
            mem_read_out <= 1'b0;
            mem_write_out <= 1'b0;
            alu_ctrl_out <= 3'b000;
            pc_out <= 32'b0;
            read_data1_out <= 32'b0;
            read_data2_out <= 32'b0;
            imm_out <= 32'b0;
            rd_out <= 5'b00000;
        end else begin
            reg_write_out <= reg_write_in;
            mem_to_reg_out <= mem_to_reg_in;
            mem_read_out <= mem_read_in;
            mem_write_out <= mem_write_in;
            alu_ctrl_out <= alu_ctrl_in;
            pc_out <= pc_in;
            read_data1_out <= read_data1_in;
            read_data2_out <= read_data2_in;
            imm_out <= imm_in;
            rd_out <= rd_in;
        end
    end
    
endmodule

// ex_mem_reg.v - Registrador entre EX e MEM
module ex_mem_reg(
    input clk,
    input reset,
    
    // Sinais de controle
    input reg_write_in,
    input mem_to_reg_in,
    input mem_read_in,
    input mem_write_in,
    
    // Resultados da EX
    input [31:0] alu_result_in,
    input [31:0] write_data_in,  // Dado para store
    input [4:0] rd_in,
    
    // Saídas para MEM
    output reg reg_write_out,
    output reg mem_to_reg_out,
    output reg mem_read_out,
    output reg mem_write_out,
    output reg [31:0] alu_result_out,
    output reg [31:0] write_data_out,
    output reg [4:0] rd_out
);
    
    always @(posedge clk or posedge reset) begin
        if (reset) begin
            reg_write_out <= 1'b0;
            mem_to_reg_out <= 1'b0;
            mem_read_out <= 1'b0;
            mem_write_out <= 1'b0;
            alu_result_out <= 32'b0;
            write_data_out <= 32'b0;
            rd_out <= 5'b00000;
        end else begin
            reg_write_out <= reg_write_in;
            mem_to_reg_out <= mem_to_reg_in;
            mem_read_out <= mem_read_in;
            mem_write_out <= mem_write_in;
            alu_result_out <= alu_result_in;
            write_data_out <= write_data_in;
            rd_out <= rd_in;
        end
    end
    
endmodule

// mem_wb_reg.v - Registrador entre MEM e WB
module mem_wb_reg(
    input clk,
    input reset,
    
    // Sinais de controle
    input reg_write_in,
    input mem_to_reg_in,
    
    // Dados da MEM
    input [31:0] alu_result_in,
    input [31:0] mem_data_in,  // Dado lido da memória
    input [4:0] rd_in,
    
    // Saídas para WB
    output reg reg_write_out,
    output reg mem_to_reg_out,
    output reg [31:0] alu_result_out,
    output reg [31:0] mem_data_out,
    output reg [4:0] rd_out
);
    
    always @(posedge clk or posedge reset) begin
        if (reset) begin
            reg_write_out <= 1'b0;
            mem_to_reg_out <= 1'b0;
            alu_result_out <= 32'b0;
            mem_data_out <= 32'b0;
            rd_out <= 5'b00000;
        end else begin
            reg_write_out <= reg_write_in;
            mem_to_reg_out <= mem_to_reg_in;
            alu_result_out <= alu_result_in;
            mem_data_out <= mem_data_in;
            rd_out <= rd_in;
        end
    end
    
endmodule