module hazard_unit(
    input [4:0] id_rs1,
    input [4:0] id_rs2,
    input ex_mem_read,
    input [4:0] ex_rd,
    output reg stall
);
    always @(*) begin
        stall = 0;
        if (ex_mem_read && ex_rd != 0) begin
            if (ex_rd == id_rs1 || ex_rd == id_rs2) begin
                stall = 1;
            end
        end
    end
endmodule