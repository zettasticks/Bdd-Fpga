`timescale 10 ns / 10 ns

module LatencyTester #(parameter 
		ADDRESS_WIDTH = 32,
		DATA_WIDTH = 16
	)(
		input wire avs_write,
		input wire [31:0] avs_writedata,
		input wire avs_read,
		output wire [31:0] avs_readdata,
		output wire avs_waitrequest,

		input wire avs_data_read,
		output wire [DATA_WIDTH-1:00] avs_data_readdata,
		output wire avs_data_waitrequest,

		output reg [ADDRESS_WIDTH-1:0] avm_address,
		output reg avm_read,
		input wire [DATA_WIDTH-1:0] avm_readdata,
		input wire avm_waitrequest,

		input wire clk,
		input wire reset

	);

reg [31:0] counter;
reg working;

reg [31:0] address;

reg [DATA_WIDTH-1:0] readData;

assign avs_waitrequest = working;
assign avs_data_waitrequest = working;

assign avs_readdata = counter;
assign avs_data_readdata = readData;

always @*
begin
	avm_address = 0;
	avm_read = 1'b0;

	if(working)
	begin
		avm_address = address;
		avm_read = 1'b1;
	end
end

always @(posedge clk)
begin
	if(avs_write)
	begin
		counter <= 0;
		address <= avs_writedata;
		working <= 1'b1;
	end

	if(working)
	begin
		counter <= counter + 1;

		if(!avm_waitrequest)
		begin
			working <= 1'b0;
			readData <= avm_readdata;
		end
	end

	if(reset)
	begin
		address <= 0;
		counter <= 0;
		working <= 1'b0;
	end
end

endmodule