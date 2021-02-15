`timescale 10 ns / 10 ns

`include "Constants.v"

module Fifo #(parameter 
		DATA_SIZE_END = 63,
		DEPTH_BITS = 3
	)(
		input wire insertValue,
		input wire popValue,

		input wire [DATA_SIZE_END:0] inValue,
		output wire [DATA_SIZE_END:0] outValue,

		output wire empty,
		output wire full,

		input clk,
		input reset
	);

reg [DATA_SIZE_END:0] memory[(2**DEPTH_BITS)-1:0];

integer i;
initial
begin
	for(i = 0; i < (2**DEPTH_BITS); i = i + 1)
	begin
		memory[i] = 0;
	end
end

reg [DEPTH_BITS-1:0] read,write;

assign outValue = memory[read];

assign empty = (read == write);
assign full = ((write + 1) == read);

always @(posedge clk)
begin
	if(insertValue)
	begin
		memory[write] <= inValue;
		write <= write + 1;
	end

	if(popValue)
	begin
		read <= read + 1;
	end

	if(reset)
	begin
		read <= 0;
		write <= 0;
	end
end

endmodule