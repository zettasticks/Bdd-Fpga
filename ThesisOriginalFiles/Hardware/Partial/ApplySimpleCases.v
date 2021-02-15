`timescale 10 ns / 10 ns

`include "Constants.v"

// Check abstract transform and simple cases
module ApplySimpleCases(
		// Input
		input wire `TYPE_DEF typeIn,
		input wire `INDEX_DEF f,
		input wire `INDEX_DEF g,
		input wire `VAR_DEF top,

		input wire `VAR_DEF cubeLastVar,

		// Abstract transform result
		output reg `TYPE_DEF  outType,
		output reg `INDEX_DEF outF,
		output reg `INDEX_DEF outG,

		// Simple cases
		output reg hit,
		output reg `INDEX_DEF result
	);

wire topBigger,equal_f_one,equal_f_g,equal_g_one;

assign topBigger = (top > cubeLastVar);
assign equal_f_one = (f == `BDD_ONE);
assign equal_f_g = (f == g);
assign equal_g_one = (g == `BDD_ONE);

wire typeEqualAbstract,transformAnd,transformExistG,transformExistF;

assign typeEqualAbstract = (typeIn == `BDD_ABSTRACT);
assign transformAnd = topBigger;
assign transformExistG = (equal_f_one | equal_f_g);
assign transformExistF = equal_g_one;

reg checkExistGSimpleCase;

// Perform transformation
always @*
begin
	checkExistGSimpleCase = 0;
	casez({typeEqualAbstract,transformAnd,transformExistG,transformExistF})
	4'b11??: begin
		outF = f;
		outG = g;
		outType = `BDD_AND;
	end
	4'b101?: begin
		checkExistGSimpleCase = 1;
		outF = g;
		outG = `BDD_ZERO; // This isn't needed, but nice for hashing purposes
		outType = `BDD_EXIST;
	end
	4'b1001:begin
		outF = f; 
		outG = `BDD_ZERO; // This isn't needed, but nice for hashing purposes
		outType = `BDD_EXIST;
	end
	default:begin
		outF = f;
		outG = g;
		outType = typeIn;
	end
	endcase
end

wire andHit;
wire `INDEX_DEF andResult;

AndTerminalCase terminalAnd(
		.f(f),
		.g(g),

		.hit(andHit),
		.result(andResult));

wire existHit;
wire `INDEX_DEF existResult;

ExistTerminalCase terminalExist(
		.f(f),
		.top(top),
		.cubeLastVar(cubeLastVar),

		.hit(existHit),
		.result(existResult));

wire transformedExistHit;
wire `INDEX_DEF transformedExistResult;

ExistTerminalCase transformedTerminalExist( // Calculates the terminal case if we do have a exist transformation (its the only case that needs a separate calculation)
		.f(g),
		.top(top),
		.cubeLastVar(cubeLastVar),

		.hit(transformedExistHit),
		.result(transformedExistResult));

wire abstractHit;
wire `INDEX_DEF abstractResult;

AbstractTerminalCase terminalAbstract(
		.f(f),
		.g(g),

		.hit(abstractHit),
		.result(abstractResult));

wire existFinalHit;
wire `INDEX_DEF existFinalResult;

assign existFinalHit = (checkExistGSimpleCase ? transformedExistHit : existHit);
assign existFinalResult = (checkExistGSimpleCase ? transformedExistResult : existResult);

always @*
begin
	case(typeIn)
		`BDD_AND:begin
			hit = andHit;
			result = andResult;
		end
		`BDD_EXIST:begin
			hit = existFinalHit;
			result = existFinalResult;
		end
		`BDD_ABSTRACT:begin
			hit = abstractHit;
			result = abstractResult;
		end
		`BDD_PERMUTE:begin
			hit = (f == `BDD_ZERO | f == `BDD_ONE);
			result = f;
		end
		default:begin
			hit = 1'b0;
			result = 30'h00000000; // Can be xxxxxxxx
		end
	endcase
end

endmodule