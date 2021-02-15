`timescale 10 ns / 10 ns

`include "Constants.v"

module CoBdd(
		output wire busy,

		// Interaction with HPS
		input wire avs_write,
		input wire [127:0] avs_writedata,
		input wire avs_read,
		output wire [127:0] avs_readdata,
		output wire avs_waitrequest,
		input wire [15:0] avs_byteenable,

		// Apply input
		input wire `INDEX_DEF apply_f,
		input wire apply_fetchF,
		input wire `INDEX_DEF apply_g,
		input wire apply_fetchG,
		input wire apply_validIn,

		// Apply output 
		output wire `INDEX_DEF fnv,
		output wire `INDEX_DEF fv,
		output wire `INDEX_DEF gnv,
		output wire `INDEX_DEF gv,
		output reg apply_validOut,

		// FindOrInsert input
		input wire `INDEX_DEF e,
		input wire `INDEX_DEF t,
		input wire `VAR_DEF   top,
		input wire findOrInsert_valid,

		// FindOrInsert output
		output reg `INDEX_DEF findOrInsert_result,
		output reg validFindOrInsertOut,

		// Finish
		input wire finished,
		input wire `INDEX_DEF finalResult,

		input wire reset,
		input wire clk
	);

reg assertedWaitRequest;

assign avs_waitrequest = assertedWaitRequest | reset; // The first read always asserts a waitrequest

reg resetHpsPerformedRead,resetHpsPerformedWrite;
reg [15:0] hpsBytesRead;

wire [15:0] bytesCurrentRead = hpsBytesRead | avs_byteenable;

reg [127:0] hpsWrittenData;
reg [15:0] hpsBytesWritten;

wire [15:0] bytesCurrentWritten = hpsBytesWritten | avs_byteenable;

reg [255:0] dataIn;

`define WORD_0(in) in[31:0]
`define WORD_1(in) in[63:32]
`define WORD_2(in) in[95:64]
`define WORD_3(in) in[127:96]

localparam 	APPLY = 2'b00,
			FIND_OR_INSERT = 2'b01,
			FINISHED = 2'b10;

`define TYPE(in) in[1:0]
`define DATA(in) in[127:2]

always @*
begin
	dataIn = 0;

	if(apply_validIn)
	begin
		`TYPE(dataIn) = APPLY;
		dataIn[2] = apply_fetchF;
		dataIn[3] = apply_fetchG;
		`WORD_1(dataIn) = apply_f;
		`WORD_2(dataIn) = apply_g;
	end
	else if(findOrInsert_valid)
	begin
		`TYPE(dataIn) = FIND_OR_INSERT;
		`WORD_1(dataIn) = t;
		`WORD_2(dataIn) = e;
		`WORD_3(dataIn) = top;
	end 
	else if(finished)
	begin
		`TYPE(dataIn) = FINISHED;
	end
end

wire validData;
wire [255:0] data;
wire [2:0] state;
reg [2:0] nextState;
reg finishedValid;

FSMBufferedPipeline #(.DATA_END(255),.MAXIMUM_STATE_BITS(3)) stage(
		.validIn(apply_validIn | findOrInsert_valid | finished),
		.busyOut(busy),
		.busyIn(1'b0),
		.dataIn(dataIn),
		.validData(validData),
		.currentData(data),
		.memoryTransfer(),
		.currentState(state),
		.nextState(nextState),
		.validOut(apply_validOut | validFindOrInsertOut | finishedValid),
		.clk(clk),
		.reset(reset)
	);

wire [1:0] currentType = `TYPE(data);

reg [127:0] hpsDataOut;

localparam 	HPS_APPLY_ONE_NODE = 2'b00,
			HPS_APPLY_TWO_NODES = 2'b01,
			HPS_FIND_OR_INSERT = 2'b10,
			HPS_FINISHED = 2'b11;

`define HPS_TYPE(in) in[1:0]
`define HPS_INDEX(in) in[31:2]

wire fetchF = data[2];
wire fetchG = data[3];

always @*
begin
	hpsDataOut = 0;

	case(currentType)
	APPLY: begin
		`WORD_1(hpsDataOut) = `WORD_1(data);
		`WORD_2(hpsDataOut) = `WORD_2(data);

		if(fetchF & fetchG)
		begin
			`HPS_TYPE(hpsDataOut) = HPS_APPLY_TWO_NODES;
		end
		else if(fetchG) // If fetchF the value is already in the current word
		begin
			`WORD_1(hpsDataOut) = `WORD_2(data);
		end
	end
	FIND_OR_INSERT: begin
		`HPS_TYPE(hpsDataOut) = HPS_FIND_OR_INSERT;
		`WORD_1(hpsDataOut) = `WORD_1(data);
		`WORD_2(hpsDataOut) = `WORD_2(data);
		`WORD_3(hpsDataOut) = `WORD_3(data);
	end
	FINISHED: begin
		`HPS_TYPE(hpsDataOut) = HPS_FINISHED;
		`HPS_INDEX(hpsDataOut) = finalResult;
	end
	endcase
end

assign avs_readdata = hpsDataOut;

integer i;
always @(posedge clk)
begin
	if(validData & avs_read)
	begin
		hpsBytesRead <= bytesCurrentRead;
	end

	if(validData & avs_write)
	begin
		hpsBytesWritten <= bytesCurrentWritten;

		for(i = 0; i < 16; i = i + 1)
		begin
			if(avs_byteenable[i])
			begin
				hpsWrittenData[i*8+:8] <= avs_writedata[i*8+:8];
			end
		end
	end

	if(reset | resetHpsPerformedRead)
	begin
		hpsBytesRead <= 0;
		hpsBytesWritten <= 0;
		hpsWrittenData <= 0;
	end
end

always @*
begin
	nextState = 0;
	resetHpsPerformedRead = 1'b0;
	resetHpsPerformedWrite = 1'b0;
	finishedValid = 1'b0;
	findOrInsert_result = 0;
	apply_validOut = 1'b0;
	validFindOrInsertOut = 1'b0;
	assertedWaitRequest = 1'b1;

	if(validData)
	begin
		case(state)
		3'h0 : begin // Wait for Hps to perform a read and return the request
			nextState = 0;
			if(avs_read)
			begin
				assertedWaitRequest = 1'b0;
			end

			if(&hpsBytesRead)
			begin
				nextState = 3'h1;
				if(currentType == FINISHED)
				begin
					nextState = 3'h0;
					finishedValid = 1'b1;
					resetHpsPerformedRead = 1'b1;
					resetHpsPerformedWrite = 1'b1;
				end
			end
		end
		3'h1: begin // Wait for Hps to write the result
			nextState = 3'h1;
			if(avs_write)
			begin
				assertedWaitRequest = 1'b0;
			end

			if(&hpsBytesWritten)
			begin
				nextState = 3'h2;
			end
		end
		3'h2: begin // Output the result
			nextState = 3'h0;
			resetHpsPerformedRead = 1'b1;
			resetHpsPerformedWrite = 1'b1;

			case(currentType)
			APPLY: begin
				apply_validOut = 1'b1;
			end
			FIND_OR_INSERT: begin
				findOrInsert_result = `WORD_0(hpsWrittenData);
				validFindOrInsertOut = 1'b1;
			end
			endcase
		end
		endcase
	end
end

wire `SIMPLE_NODE_DEF node0 = {`WORD_1(hpsWrittenData),`WORD_0(hpsWrittenData)};
wire `SIMPLE_NODE_DEF node1 = {`WORD_3(hpsWrittenData),`WORD_2(hpsWrittenData)};

wire `SIMPLE_NODE_DEF nodeF = node0;
wire `SIMPLE_NODE_DEF nodeG = (fetchF ? node1 : node0);

assign fv = nodeF[61:32];
assign fnv = nodeF[29:0];
assign gv = nodeG[61:32];
assign gnv = nodeG[29:0];

endmodule
