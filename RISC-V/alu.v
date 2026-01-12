module alu(
    input [31:0] a,
    input [31:0] b,
    input [5:0] alu_ctrl,  // [5:4]=modo, [3:0]=operação
    output reg [31:0] result,
    output zero
);
    wire [1:0] vec_mode = alu_ctrl[5:4];
    wire [3:0] op = alu_ctrl[3:0];
    reg [7:0] b0,b1,b2,b3;
    reg [15:0] h0,h1;
    
    always @(*) begin
        case (vec_mode)
            2'b00: begin // escalar
                case (op)
                    4'b0000: result = a + b;
                    4'b0001: result = a - b;
                    4'b0010: result = a & b;
                    4'b0011: result = a | b;
                    4'b0100: result = a ^ b;
                    4'b0101: result = a << b[4:0];
                    4'b0110: result = a >> b[4:0];
                    4'b0111: result = $signed(a) >>> b[4:0];
                    4'b1000: result = b; // LUI
                    default: result = 0;
                endcase
            end
            
            2'b01: begin // 4x8 bits
                case (op)
                    4'b0000: begin // vadd.8
                        b0 = a[7:0] + b[7:0];
                        b1 = a[15:8] + b[15:8];
                        b2 = a[23:16] + b[23:16];
                        b3 = a[31:24] + b[31:24];
                        result = {b3, b2, b1, b0};
                    end
                    4'b0001: begin // vsub.8
                        b0 = a[7:0] - b[7:0];
                        b1 = a[15:8] - b[15:8];
                        b2 = a[23:16] - b[23:16];
                        b3 = a[31:24] - b[31:24];
                        result = {b3, b2, b1, b0};
                    end
                    4'b0010: result = a & b; // vand.8
                    4'b0011: result = a | b; // vor.8
                    default: result = 0;
                endcase
            end
            
            2'b10: begin // 2x16 bits
                case (op)
                    4'b0000: begin // vadd.16
                        h0 = a[15:0] + b[15:0];
                        h1 = a[31:16] + b[31:16];
                        result = {h1, h0};
                    end
                    4'b0001: begin // vsub.16
                        h0 = a[15:0] - b[15:0];
                        h1 = a[31:16] - b[31:16];
                        result = {h1, h0};
                    end
                    4'b0010: result = a & b; // vand.16
                    4'b0011: result = a | b; // vor.16
                    default: result = 0;
                endcase
            end
            
            default: result = 0;
        endcase
    end
    
    assign zero = (result == 0);
endmodule