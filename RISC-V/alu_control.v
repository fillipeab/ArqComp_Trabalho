module alu_control(
    input [1:0] alu_op,      // Vem da Unidade de Controle
    input [2:0] funct3,      // funct3 da instrução
    input [6:0] funct7,      // funct7 da instrução (para SUB/SRA)
    output reg [3:0] alu_ctrl  // Vai para a ALU
);

    always @(*) begin
        case (alu_op)
            2'b00: alu_ctrl = 4'b0000; // LW/SW: ADD
            2'b01: alu_ctrl = 4'b0001; // Branch: SUB
            
            2'b10: begin // R-type/I-type
                case (funct3)
                    3'b000: alu_ctrl = (funct7[5]) ? 4'b0001 : 4'b0000; // SUB : ADD
                    3'b001: alu_ctrl = 4'b0101; // SLL
                    3'b010: alu_ctrl = 4'b0001; // SLT (usar SUB por enquanto)
                    3'b011: alu_ctrl = 4'b0001; // SLTU
                    3'b100: alu_ctrl = 4'b0100; // XOR
                    3'b101: alu_ctrl = (funct7[5]) ? 4'b0111 : 4'b0110; // SRA : SRL
                    3'b110: alu_ctrl = 4'b0011; // OR
                    3'b111: alu_ctrl = 4'b0010; // AND
                    default: alu_ctrl = 4'b0000;
                endcase
            end
            
            2'b11: alu_ctrl = 4'b1000; // LUI: passa o imediato
            
            default: alu_ctrl = 4'b0000;
        endcase
    end
endmodule