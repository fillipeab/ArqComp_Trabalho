module alu_control(
    input [1:0] alu_op,
    input [2:0] funct3,
    input [6:0] funct7,
    input is_vector,
    output reg [5:0] alu_ctrl  // 6 bits agora
);
    always @(*) begin
        if (is_vector) begin
            // Modo vetorial: [5:4]=modo, [3:0]=operação
            case (funct3)
                3'b000: alu_ctrl = 6'b01_0000; // vadd.8
                3'b001: alu_ctrl = 6'b10_0000; // vadd.16
                3'b010: alu_ctrl = 6'b01_0001; // vsub.8
                3'b011: alu_ctrl = 6'b10_0001; // vsub.16
                3'b100: alu_ctrl = 6'b01_0010; // vand.8
                3'b101: alu_ctrl = 6'b10_0010; // vand.16
                3'b110: alu_ctrl = 6'b01_0011; // vor.8
                3'b111: alu_ctrl = 6'b10_0011; // vor.16
                default: alu_ctrl = 6'b00_0000;
            endcase
        end else begin
            // Modo escalar
            alu_ctrl[5:4] = 2'b00; // modo escalar
            case (alu_op)
                2'b00: alu_ctrl[3:0] = 4'b0000; // ADD
                2'b01: alu_ctrl[3:0] = 4'b0001; // SUB
                2'b10: begin
                    case (funct3)
                        3'b000: alu_ctrl[3:0] = (funct7[5]) ? 4'b0001 : 4'b0000;
                        3'b001: alu_ctrl[3:0] = 4'b0101;
                        3'b010: alu_ctrl[3:0] = 4'b0001;
                        3'b011: alu_ctrl[3:0] = 4'b0001;
                        3'b100: alu_ctrl[3:0] = 4'b0100;
                        3'b101: alu_ctrl[3:0] = (funct7[5]) ? 4'b0111 : 4'b0110;
                        3'b110: alu_ctrl[3:0] = 4'b0011;
                        3'b111: alu_ctrl[3:0] = 4'b0010;
                        default: alu_ctrl[3:0] = 4'b0000;
                    endcase
                end
                2'b11: alu_ctrl[3:0] = 4'b1000; // LUI
                default: alu_ctrl[3:0] = 4'b0000;
            endcase
        end
    end
endmodule