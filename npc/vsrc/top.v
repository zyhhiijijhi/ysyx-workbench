module top(
    input clk,
    input rst,
    input [4:0] btn,
    input [7:0] sw,
    output [15:0] ledr
);


light my_light(
	.a(sw[0]),
	.b(sw[1]),
	.f(ledr[0])
);
endmodule


module vmem(
    input [9:0] h_addr,
    input [8:0] v_addr,
    output [23:0] vga_data
);

reg [23:0] vga_mem [524287:0];

initial begin
    $readmemh("resource/picture.hex", vga_mem);
end

assign vga_data = vga_mem[{h_addr, v_addr}];

endmodule

