module forwarding_unit(
    input [4:0] ex_rs1,
    input [4:0] ex_rs2,
    input mem_reg_write,
    input [4:0] mem_rd,
    input wb_reg_write,
    input [4:0] wb_rd,
    output reg [1:0] forwardA,
    output reg [1:0] forwardB
);
    always @(*) begin
        forwardA = 2'b00;
        forwardB = 2'b00;
        
        // Forward A
        if (mem_reg_write && mem_rd != 0 && mem_rd == ex_rs1) begin
            forwardA = 2'b10;
        end else if (wb_reg_write && wb_rd != 0 && wb_rd == ex_rs1) begin
            forwardA = 2'b01;
        end
        
        // Forward B
        if (mem_reg_write && mem_rd != 0 && mem_rd == ex_rs2) begin
            forwardB = 2'b10;
        end else if (wb_reg_write && wb_rd != 0 && wb_rd == ex_rs2) begin
            forwardB = 2'b01;
        end
    end
endmodule