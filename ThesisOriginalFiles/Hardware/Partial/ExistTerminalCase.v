`timescale 10 ns / 10 ns

`include "Constants.v"

module ExistTerminalCase(
		input wire `INDEX_DEF f,
		input wire `VAR_DEF top,
		input wire `VAR_DEF cubeLastVar,

		output wire hit,
		output wire `INDEX_DEF result
	);

wire topBigger,equal_f_zero,equal_f_one;

assign topBigger = (top > cubeLastVar);
assign equal_f_zero = (f == `BDD_ZERO);
assign equal_f_one = (f == `BDD_ONE);

assign hit = (topBigger | equal_f_zero | equal_f_one);
assign result = f;

endmodule

module ExistTerminalCaseNoVar(
		input wire `INDEX_DEF f,

		output wire hit,
		output wire `INDEX_DEF result
	);

wire equal_f_zero,equal_f_one;

assign equal_f_zero = (f == `BDD_ZERO);
assign equal_f_one = (f == `BDD_ONE);

assign hit = (equal_f_zero | equal_f_one);
assign result = f;

endmodule
