module regfile(
    input clk,
    input we,
    input [4:0] rs1,
    input [4:0] rs2,
    input [4:0] rd,
    input [31:0] wd,
    output [31:0] rd1,
    output [31:0] rd2
);

    reg [31:0] regs[31:0];

    assign rd1 = (rs1 != 0) ? regs[rs1] : 32'b0;
    assign rd2 = (rs2 != 0) ? regs[rs2] : 32'b0;

    always @(posedge clk) begin
        if (we && rd != 0)
            regs[rd] <= wd;
    end
endmodule