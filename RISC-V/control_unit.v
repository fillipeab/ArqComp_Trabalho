module control(
    input [6:0] opcode,
    output reg reg_write,
    output reg mem_to_reg,
    output reg mem_read,
    output reg mem_write,
    output reg alu_src,
    output reg [1:0] alu_op,
    output reg branch,
    output reg is_vector
);
    always @(*) begin
        reg_write = 0; mem_to_reg = 0; mem_read = 0;
        mem_write = 0; alu_src = 0; alu_op = 0; 
        branch = 0; is_vector = 0;
        
        case (opcode)
            7'b0110011: begin reg_write = 1; alu_op = 2'b10; end // R-type
            7'b0010011: begin reg_write = 1; alu_src = 1; alu_op = 2'b10; end // I-type
            7'b0000011: begin reg_write = 1; mem_read = 1; mem_to_reg = 1; alu_src = 1; end // LW
            7'b0100011: begin mem_write = 1; alu_src = 1; end // SW
            7'b1100011: begin branch = 1; alu_op = 2'b01; end // Branch
            7'b0110111: begin reg_write = 1; alu_src = 1; alu_op = 2'b11; end // LUI
            7'b0100111: begin reg_write = 1; is_vector = 1; alu_op = 2'b11; end // Vetorial
            default: ; // NOP
        endcase
    end
endmodule