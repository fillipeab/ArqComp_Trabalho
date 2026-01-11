module control(
    input [6:0] opcode,
    output reg reg_write,
    output reg mem_read,
    output reg mem_write,
    output reg mem_to_reg,
    output reg alu_src
);

    always @(*) begin
        reg_write = 0;
        mem_read = 0;
        mem_write = 0;
        mem_to_reg = 0;
        alu_src = 0;

        case (opcode)
            7'b0110011: reg_write = 1; // R-type
            7'b0010011: begin reg_write = 1; alu_src = 1; end // I-type
            7'b0000011: begin reg_write = 1; mem_read = 1; mem_to_reg = 1; alu_src = 1; end // LW
            7'b0100011: begin mem_write = 1; alu_src = 1; end // SW
            7'b0110111: reg_write = 1; // LUI
        endcase
    end
endmodule