module regfile(
    input clk,
    input reset,
    input we,
    input [4:0] rs1,
    input [4:0] rs2,
    input [31:0] wd,
    input [4:0] rd,
    output [31:0] rd1,
    output [31:0] rd2
);
    reg [31:0] regs [0:31];
    integer i;
    always @(posedge clk or posedge reset) begin
        if (reset) for (i=0; i<32; i=i+1) regs[i] <= 0;
        else if (we && rd != 0) regs[rd] <= wd;
    end
    assign rd1 = (rs1 != 0) ? regs[rs1] : 0;
    assign rd2 = (rs2 != 0) ? regs[rs2] : 0;
endmodule