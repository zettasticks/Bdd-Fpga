`timescale 10 ns / 10 ns

`include "Constants.v"

// Counter using OneHot encoding. Starts at 01 then 10 then goes back to 01 for the case NUMBER = 2
// Start starts the counter
// Running indicates when its working and its asserted in the same cycle as start
// Last indicates the last cycle. Running is asserted when last is. The immediatly next cycle running is deasserted
module OneHotCounter #(parameter 
		NUMBER = 2
	)(
		input wire start, // Asserted once, afterwards the counter runs on its own

		output reg [NUMBER-1:0] counter, // The one hot enconding counter
		output wire running, // Asserted at the start and when the counter is progressing.
		output wire last, // Asserted the last cycle before running is deasserted

		input clk,
		input reset
	);

assign running = !counter[0] | start;
assign last = counter[NUMBER-1];

always @(posedge clk)
begin
	if(running)
	begin
		counter <= counter << 1;
		if(last)
		begin
			counter <= 1;
		end
	end

	if(reset)
	begin
		counter <= 1;
	end
end

endmodule

// A type of OneHotCounter that doesn't 
module OneHotStepCounter #(parameter 
		NUMBER = 2
	)(
		input wire step, // Asserted once, afterwards the counter runs on its own

		output reg [NUMBER-1:0] counter, // A one-hot encoding counter that starts from 01
		output wire last, // Indicates the counter is in the last position
		output wire lastStep, // Counter is last position and step is asserted. Next cycle counter will reset back to the start

		input clk,
		input reset
	);

assign last = counter[NUMBER-1];
assign lastStep = step & counter[NUMBER-1];

always @(posedge clk)
begin
	if(step)
	begin
		counter[NUMBER-1:1] <= counter[NUMBER-2:0];
		counter[0] <= counter[NUMBER-1];
	end

	if(reset)
	begin
		counter <= 1;
	end
end

endmodule