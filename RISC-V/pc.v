module pc(
    input clk,
    input reset,           // reset assincrono
    input pc_write,        // enable para stalls
    input [31:0] next_pc,
    output reg [31:0] pc
);
    always @(posedge clk or posedge reset) begin
        if (reset) pc <= 0;
        else if (pc_write) pc <= next_pc;
    end
endmodule