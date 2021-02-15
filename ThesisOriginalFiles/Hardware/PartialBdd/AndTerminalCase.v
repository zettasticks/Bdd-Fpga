`timescale 10 ns / 10 ns

`include "Constants.v"

module AndTerminalCase(
		input wire `INDEX_DEF f,
		input wire `INDEX_DEF g,

		output wire hit,
		output reg `INDEX_DEF result
	);

wire equal_f_zero,equal_g_zero,equal_negate,equal_f_g,equal_g_one,equal_f_one;

assign equal_f_zero = (f == `BDD_ZERO);
assign equal_g_zero = (g == `BDD_ZERO);
assign equal_negate = (`EQUAL_NEGATE(f,g));
assign equal_f_g = (f == g);
assign equal_g_one = (g == `BDD_ONE);
assign equal_f_one = (f == `BDD_ONE);

wire return_zero,return_f,return_g;

assign return_zero = (equal_f_zero | equal_g_zero | equal_negate);
assign return_f = (equal_f_g | equal_g_one);
assign return_g = equal_f_one;

assign hit = (return_zero | return_f | return_g);

always @*
begin
	casez({return_zero,return_f,return_g}) // If at the same time, return the most to the right
	3'b000: begin
		result = 30'h00000000; // Can be xxxxxxxx
	end
	3'b001: begin
		result = g;
	end
	3'b01?: begin
		result = f;
	end
	3'b1??: begin
		result = `BDD_ZERO;
	end
	endcase

end

endmodule