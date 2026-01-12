module if_id_reg(
    input clk, input reset, input if_id_write, input flush,
    input [31:0] pc_in, input [31:0] instr_in,
    output reg [31:0] pc_out, output reg [31:0] instr_out
);
    always @(posedge clk or posedge reset) begin
        if (reset) begin pc_out <= 0; instr_out <= 0; end
        else if (flush) begin pc_out <= 0; instr_out <= 32'h00000013; end
        else if (if_id_write) begin pc_out <= pc_in; instr_out <= instr_in; end
    end
endmodule


module id_ex_reg(
    input clk, input reset, input flush,
    input reg_write_in, input mem_to_reg_in, input mem_read_in,
    input mem_write_in, input alu_src_in, input [1:0] alu_op_in,
    input branch_in, input is_vector_in,  // novo
    input [31:0] pc_in, input [31:0] read_data1_in, input [31:0] read_data2_in,
    input [31:0] imm_in, input [4:0] rs1_in, input [4:0] rs2_in,
    input [4:0] rd_in, input [2:0] funct3_in, input [6:0] funct7_in,
    output reg reg_write_out, output reg mem_to_reg_out, output reg mem_read_out,
    output reg mem_write_out, output reg alu_src_out, output reg [1:0] alu_op_out,
    output reg branch_out, output reg is_vector_out,  // novo
    output reg [31:0] pc_out, output reg [31:0] read_data1_out,
    output reg [31:0] read_data2_out, output reg [31:0] imm_out,
    output reg [4:0] rs1_out, output reg [4:0] rs2_out, output reg [4:0] rd_out,
    output reg [2:0] funct3_out, output reg [6:0] funct7_out
);
    always @(posedge clk or posedge reset) begin
        if (reset) begin
            reg_write_out <= 0; mem_to_reg_out <= 0; mem_read_out <= 0;
            mem_write_out <= 0; alu_src_out <= 0; alu_op_out <= 0;
            branch_out <= 0; is_vector_out <= 0;
            pc_out <= 0; read_data1_out <= 0; read_data2_out <= 0;
            imm_out <= 0; rs1_out <= 0; rs2_out <= 0; rd_out <= 0;
            funct3_out <= 0; funct7_out <= 0;
        end else if (flush) begin
            reg_write_out <= 0; mem_to_reg_out <= 0; mem_read_out <= 0;
            mem_write_out <= 0; alu_src_out <= 0; alu_op_out <= 0;
            branch_out <= 0; is_vector_out <= 0;
            pc_out <= pc_in; read_data1_out <= 0; read_data2_out <= 0;
            imm_out <= 0; rs1_out <= 0; rs2_out <= 0; rd_out <= 0;
            funct3_out <= 0; funct7_out <= 0;
        end else begin
            reg_write_out <= reg_write_in; mem_to_reg_out <= mem_to_reg_in;
            mem_read_out <= mem_read_in; mem_write_out <= mem_write_in;
            alu_src_out <= alu_src_in; alu_op_out <= alu_op_in;
            branch_out <= branch_in; is_vector_out <= is_vector_in;
            pc_out <= pc_in; read_data1_out <= read_data1_in;
            read_data2_out <= read_data2_in; imm_out <= imm_in;
            rs1_out <= rs1_in; rs2_out <= rs2_in; rd_out <= rd_in;
            funct3_out <= funct3_in; funct7_out <= funct7_in;
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
    input branch_in,
    input [31:0] alu_result_in, 
    input [31:0] write_data_in, 
    input [4:0] rd_in,
    input [4:0] rs1_in,
    input [4:0] rs2_in,
    
    output reg reg_write_out, 
    output reg mem_to_reg_out, 
    output reg mem_read_out, 
    output reg mem_write_out,
    output reg branch_out,
    output reg [31:0] alu_result_out,
    output reg [31:0] write_data_out, 
    output reg [4:0] rd_out,
    output reg [4:0] rs1_out,
    output reg [4:0] rs2_out
);
    always @(posedge clk or posedge reset) begin
        if (reset) begin
            reg_write_out <= 0; mem_to_reg_out <= 0; mem_read_out <= 0;
            mem_write_out <= 0; branch_out <= 0;                     
            alu_result_out <= 0; write_data_out <= 0; 
            rd_out <= 0; rs1_out <= 0; rs2_out <= 0;                 
        end else begin
            reg_write_out <= reg_write_in; 
            mem_to_reg_out <= mem_to_reg_in;
            mem_read_out <= mem_read_in; 
            mem_write_out <= mem_write_in;
            branch_out <= branch_in;                                 
            alu_result_out <= alu_result_in; 
            write_data_out <= write_data_in;
            rd_out <= rd_in;
            rs1_out <= rs1_in; rs2_out <= rs2_in;                    
        end
    end
endmodule

// mem_wb_reg.v - Registrador entre MEM e WB
module mem_wb_reg(
    input clk, input reset,
    input reg_write_in, input mem_to_reg_in,
    input [31:0] alu_result_in, input [31:0] mem_data_in, input [4:0] rd_in,
    output reg reg_write_out, output reg mem_to_reg_out,
    output reg [31:0] alu_result_out, output reg [31:0] mem_data_out,
    output reg [4:0] rd_out
);
    always @(posedge clk or posedge reset) begin
        if (reset) begin
            reg_write_out <= 0; mem_to_reg_out <= 0;
            alu_result_out <= 0; mem_data_out <= 0; rd_out <= 0;
        end else begin
            reg_write_out <= reg_write_in; mem_to_reg_out <= mem_to_reg_in;
            alu_result_out <= alu_result_in; mem_data_out <= mem_data_in;
            rd_out <= rd_in;
        end
    end
endmodule