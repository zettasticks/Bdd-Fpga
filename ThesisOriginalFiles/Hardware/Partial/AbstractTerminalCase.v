`timescale 10 ns / 10 ns

`include "Constants.v"

module AbstractTerminalCase(
		input wire `INDEX_DEF f,
		input wire `INDEX_DEF g,

		output wire hit,
		output wire `INDEX_DEF result
	);

wire equal_f_zero,equal_g_zero,equal_negate,equal_f_one,equal_g_one;

assign equal_f_zero = (f == `BDD_ZERO);
assign equal_g_zero = (g == `BDD_ZERO);
assign equal_negate = (`EQUAL_NEGATE(f,g));

assign equal_f_one = (f == `BDD_ONE);
assign equal_g_one = (g == `BDD_ONE);

wire return_zero,return_one;

assign return_zero = (equal_f_zero | equal_g_zero | equal_negate);
assign return_one = (equal_f_one && equal_g_one);

assign hit = (return_zero | return_one);
assign result = return_one ? `BDD_ONE : `BDD_ZERO;

endmodule