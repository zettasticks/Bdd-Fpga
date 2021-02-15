`timescale 10 ns / 10 ns

`include "Constants.v"

module Apply (
		input wire `INDEX_DEF f,
		input wire `INDEX_DEF g,
		input wire `TYPE_DEF  typeIn,
		input wire valid,
		input wire negate,
		input [2:0] index,
		output wire applyBusy,

		output wire simpleCaseHit,
		output wire `INDEX_DEF simpleCaseResult,
		output wire `INDEX_DEF fn,
		output wire `INDEX_DEF fnv,
		output wire `INDEX_DEF gn,
		output wire `INDEX_DEF gnv,
		output wire quantifyOut,
		output wire `INDEX_DEF outF,
		output wire `INDEX_DEF outG,
		output wire `TYPE_DEF outType,
		output wire `VAR_DEF topOut,
		output wire validOut,
		output wire negateOut,
		input wire applyBusyIn,
		output wire [2:0] outIndex,

		input wire insertValueIntoSimpleCache,
		input wire [`CACHE_BIT_SIZE-1:0] simpleCacheIndex,
		input wire [`SIMPLE_CACHE_ENTRY_BIT_SIZE-1:0] simpleCacheValue, 

		// Configuration port to write quantification data
		input wire [15:0] avs_config_address, 
		input wire avs_config_write,
		input wire [31:0] avs_config_writedata,
		input wire avs_config_read,
		output wire [31:0] avs_config_readdata,
		output wire avs_config_readdatavalid,
		output wire avs_config_waitrequest,

		// Memory interfaces with Memory Access (they are separated to allow the outside controller to decide the arbitration)
		output reg [127:0] aso_var_request_data,
		input wire aso_var_request_ready,
		output reg aso_var_request_valid,

		output reg [127:0] aso_node_request_data,
		input wire aso_node_request_ready,
		output reg aso_node_request_valid,

		output reg [127:0] aso_cache_request_data,
		input wire aso_cache_request_ready,
		output reg aso_cache_request_valid,

		// Input result from Memory Access
		input wire [95:0] asi_result_data,
		input wire [1:0] asi_result_channel,
		output wire asi_result_ready,
		input wire asi_result_valid,

		// Input var from Memory Access
		input wire [15:0] asi_var_data,
		output wire asi_var_ready,
		input wire asi_var_valid,

		input wire clk,
		input wire reset
	);

assign avs_config_waitrequest = 1'b0;
assign asi_var_ready = 1'b1;
assign asi_result_ready = 1'b1;

// At the end make the following structures depend on REQUEST_DATA_END
`define APPLY_INPUT_$_REQUEST_ID_END 2
`define APPLY_INPUT_$_NEGATE_END 3
`define APPLY_INPUT_$_BDDINDEX_F_END 33
`define APPLY_INPUT_$_BDDINDEX_G_END 63
`define APPLY_INPUT_$_TYPE_END 66
`define APPLY_INPUT_END 66

`define APPLY_INPUT_$_REQUEST_ID(in) in[2:0]
`define APPLY_INPUT_$_NEGATE(in) in[3:3]
`define APPLY_INPUT_$_BDDINDEX_F(in) in[33:4]
`define APPLY_INPUT_$_BDDINDEX_G(in) in[63:34]
`define APPLY_INPUT_$_TYPE(in) in[66:64]
`define APPLY_INPUT_DEF [66:0]
`define APPLY_INPUT_RANGE 66:0

`define APPLY_STAGE_12_$_APPLY_INPUT_END 66
`define APPLY_STAGE_12_$_SIMPLE_RESULT_END 96
`define APPLY_STAGE_12_$_HIT_END 97
`define APPLY_STAGE_12_END 97

`define APPLY_STAGE_12_$_APPLY_INPUT(in) in[66:0]
`define APPLY_STAGE_12_$_SIMPLE_RESULT(in) in[96:67]
`define APPLY_STAGE_12_$_HIT(in) in[97:97]
`define APPLY_STAGE_12_DEF [97:0]
`define APPLY_STAGE_12_RANGE 97:0

`define APPLY_STAGE_23_$_APPLY_STAGE_12_END 97
`define APPLY_STAGE_23_$_FETCH_F_END 98
`define APPLY_STAGE_23_$_FETCH_G_END 99
`define APPLY_STAGE_23_$_TOP_END 109
`define APPLY_STAGE_23_END 109

`define APPLY_STAGE_23_$_APPLY_STAGE_12(in) in[97:0]
`define APPLY_STAGE_23_$_FETCH_F(in) in[98:98]
`define APPLY_STAGE_23_$_FETCH_G(in) in[99:99]
`define APPLY_STAGE_23_$_TOP(in) in[109:100]
`define APPLY_STAGE_23_DEF [109:0]
`define APPLY_STAGE_23_RANGE 109:0

`define APPLY_STAGE_34_$_APPLY_STAGE_23_END 109
`define APPLY_STAGE_34_$_QUANTIFY_END 110
`define APPLY_STAGE_34_$_NEXT_TOP_END 120
`define APPLY_STAGE_34_END 120

`define APPLY_STAGE_34_$_APPLY_STAGE_23(in) in[109:0]
`define APPLY_STAGE_34_$_QUANTIFY(in) in[110:110]
`define APPLY_STAGE_34_$_NEXT_TOP(in) in[120:111]
`define APPLY_STAGE_34_DEF [120:0]
`define APPLY_STAGE_34_RANGE 120:0

`define APPLY_STAGE_45_$_APPLY_STAGE_23_END 109
`define APPLY_STAGE_45_$_QUANTIFY_END 110
`define APPLY_STAGE_45_$_SIMPLE_CACHE_HASH_END 112
`define APPLY_STAGE_45_$_CACHE_INDEX_END 130
`define APPLY_STAGE_45_END 130

`define APPLY_STAGE_45_$_APPLY_STAGE_23(in) in[109:0]
`define APPLY_STAGE_45_$_QUANTIFY(in) in[110:110]
`define APPLY_STAGE_45_$_SIMPLE_CACHE_HASH(in) in[112:111]
`define APPLY_STAGE_45_$_CACHE_INDEX(in) in[130:113]
`define APPLY_STAGE_45_DEF [130:0]
`define APPLY_STAGE_45_RANGE 130:0

`define APPLY_STAGE_56_$_APPLY_STAGE_45_END 130
`define APPLY_STAGE_56_$_STORED_HASH_END 132
`define APPLY_STAGE_56_END 132

`define APPLY_STAGE_56_$_APPLY_STAGE_45(in) in[130:0]
`define APPLY_STAGE_56_$_STORED_HASH(in) in[132:131]
`define APPLY_STAGE_56_DEF [132:0]
`define APPLY_STAGE_56_RANGE 132:0

reg `VAR_DEF cubeLastVar;
reg [31:0] quantify [(`VAR_COUNT/32)-1:0] /* synthesis ramstyle = "no_rw_check,M10K" */;
reg [1:0] simpleCache [`CACHE_SIZE-1:0] /* synthesis ramstyle = "no_rw_check,M10K" */;
reg `VAR_DEF nextVar [`VAR_END:0] /* synthesis ramstyle = "no_rw_check,M10K" */;

// Perform insertion into the simple cache
reg doInsertValueIntoSimpleCache;
reg [`CACHE_BIT_SIZE-1:0] storedSimpleCacheIndex;
reg [`SIMPLE_CACHE_ENTRY_BIT_SIZE-1:0] storedSimpleCacheValue; 

always @(posedge clk)
begin
	doInsertValueIntoSimpleCache <= insertValueIntoSimpleCache;
	storedSimpleCacheIndex <= simpleCacheIndex;
	storedSimpleCacheValue <= simpleCacheValue; 

	if(doInsertValueIntoSimpleCache)
	begin
		simpleCache[storedSimpleCacheIndex] <= storedSimpleCacheValue;
	end

	if(reset)
	begin
		doInsertValueIntoSimpleCache <= 0;
		storedSimpleCacheIndex <= 0;
		storedSimpleCacheValue <= 0;
	end
end

reg hpsPerformWrite;
reg hpsCubeLastVarOperation;
reg [15:0] hpsQuantifyIndex;
reg [31:0] valueToWriteOrRead,quantifyValueRead;
reg hpsPerformRead;
reg hpsReturnData;
reg quantifyDoRead,hpsPerformNextVarRead;
reg hpsPerformNextVarWrite;
reg `VAR_DEF hpsNextVarIndex,valueForNextVar,hpsNextVarIndexToRead;

localparam 	SET_CUBE_LAST_VAR = 15'h0005,
			SET_QUANTIFY = 128,
			SET_NEXT_VAR = 256;

always @(posedge clk)
begin
	hpsPerformWrite <= 1'b0;
	hpsCubeLastVarOperation <= 1'b0;
	hpsQuantifyIndex <= 0;
	valueToWriteOrRead <= 0;
	hpsPerformRead <= 1'b0;
	hpsReturnData <= 1'b0;
	quantifyDoRead <= 0;
	hpsPerformNextVarWrite <= 1'b0;
	hpsPerformNextVarRead <= 1'b0;

	if(avs_config_write)
	begin
		if(avs_config_address >= SET_NEXT_VAR)
		begin
			hpsPerformNextVarWrite <= 1'b1;
			hpsNextVarIndex <= avs_config_address - SET_NEXT_VAR;
			valueForNextVar <= avs_config_writedata; 
		end
		else if(avs_config_address >= SET_QUANTIFY)
		begin
			hpsPerformWrite <= 1'b1;
			hpsQuantifyIndex <= avs_config_address - SET_QUANTIFY;
			valueToWriteOrRead <= avs_config_writedata;
		end
		else 
		if(avs_config_address == SET_CUBE_LAST_VAR)
		begin
			hpsPerformWrite <= 1'b1;
			hpsCubeLastVarOperation <= 1'b1;
			valueToWriteOrRead <= avs_config_writedata;
		end
	end

	if(hpsPerformNextVarWrite)
	begin
		nextVar[hpsNextVarIndex] <= valueForNextVar;
	end

	if(avs_config_read)
	begin
		if(avs_config_address >= SET_NEXT_VAR)
		begin
			hpsPerformNextVarRead <= 1'b1;
			hpsNextVarIndexToRead <= avs_config_address - SET_NEXT_VAR;
		end
		else if(avs_config_address >= SET_QUANTIFY)
		begin
			hpsPerformRead <= 1'b1;
			hpsQuantifyIndex <= avs_config_address - SET_QUANTIFY;
		end
		else if(avs_config_address == SET_CUBE_LAST_VAR)
		begin
			hpsPerformRead <= 1'b1;
			hpsCubeLastVarOperation <= 1'b1;
		end
	end

	if(hpsPerformNextVarRead)
	begin
		hpsReturnData <= 1'b1;
		valueToWriteOrRead <= nextVar[hpsNextVarIndexToRead];
	end

	if(hpsPerformWrite)
	begin
		if(hpsCubeLastVarOperation)
		begin
			cubeLastVar <= valueToWriteOrRead[11:0];
		end
		else 
		begin
			quantify[hpsQuantifyIndex] <= valueToWriteOrRead;	
		end
	end
	else
	if(hpsPerformRead)
	begin
		if(hpsCubeLastVarOperation)
		begin
			hpsReturnData <= 1'b1;
			valueToWriteOrRead <= cubeLastVar;
		end
		else 
		begin
			quantifyValueRead <= quantify[hpsQuantifyIndex];	
			quantifyDoRead <= 1;
		end
	end

	if(quantifyDoRead)
	begin
		hpsReturnData <= 1'b1;
		valueToWriteOrRead <= quantifyValueRead;
	end

	if(reset)
	begin
		quantifyValueRead <= 0;
		hpsNextVarIndex <= 0;
		valueForNextVar <= 0;
		hpsNextVarIndexToRead <= 0;
	end
end

assign avs_config_readdata = hpsReturnData ? valueToWriteOrRead : 0;
assign avs_config_readdatavalid = hpsReturnData;

reg `APPLY_INPUT_DEF apply_input;

always @*
begin
	apply_input = 0;

	`APPLY_INPUT_$_REQUEST_ID(apply_input) = index;
	`APPLY_INPUT_$_NEGATE(apply_input) = negate;
	`APPLY_INPUT_$_BDDINDEX_F(apply_input) = f;
	`APPLY_INPUT_$_BDDINDEX_G(apply_input) = g;
	`APPLY_INPUT_$_TYPE(apply_input) = typeIn;
end

wire stage_1_validData;
wire stage_21_busy;
wire `APPLY_INPUT_DEF stage_1_data;

// Stage 1 - Compute simple cases that do not require variable information
BufferedPipeline #(.DATA_END(`APPLY_INPUT_END)) stage_1(
		.validIn(valid),
		.busyOut(applyBusy),
		.busyIn(stage_21_busy),
		.dataIn(apply_input),
		.validData(stage_1_validData),
		.currentData(stage_1_data),
		.memoryTransfer(),
		.validOut(stage_1_validData),
		.clk(clk),
		.reset(reset)
	);

// Input
wire `INDEX_DEF stage_1_f = `APPLY_INPUT_$_BDDINDEX_F(stage_1_data);
wire `INDEX_DEF stage_1_g = `APPLY_INPUT_$_BDDINDEX_G(stage_1_data);
wire `TYPE_DEF stage_1_type = `APPLY_INPUT_$_TYPE(stage_1_data);

// Output from Apply Simple Case no Var
wire `INDEX_DEF stage_1_simpleResult;
wire stage_1_hit;

wire `TYPE_DEF stage_1_typeOut;
wire `INDEX_DEF stage_1_fOut,stage_1_gOut;

ApplySimpleCasesNoVar stage_1_simple_case_no_var(
		.typeIn(stage_1_type),
		.f(stage_1_f),
		.g(stage_1_g),
		.outType(stage_1_typeOut),
		.outF(stage_1_fOut),
		.outG(stage_1_gOut),
		.hit(stage_1_hit),
		.result(stage_1_simpleResult)
	);

// Change the order so that f is smaller than g
wire stage_1_f_bigger = (stage_1_f > stage_1_g);
wire stage_1_reorder = (stage_1_typeOut != `BDD_EXIST && stage_1_typeOut != `BDD_PERMUTE);

wire `INDEX_DEF stage_1_finalF = (stage_1_reorder & stage_1_f_bigger) ? stage_1_gOut : stage_1_fOut;
wire `INDEX_DEF stage_1_finalG = (stage_1_reorder & stage_1_f_bigger) ? stage_1_fOut : stage_1_gOut;

reg `APPLY_STAGE_12_DEF stage_12_data;

always @*
begin
	stage_12_data = 0;

	`APPLY_INPUT_$_REQUEST_ID(stage_12_data) = `APPLY_INPUT_$_REQUEST_ID(stage_1_data);
	`APPLY_INPUT_$_NEGATE(stage_12_data) = `APPLY_INPUT_$_NEGATE(stage_1_data);
	`APPLY_INPUT_$_BDDINDEX_F(stage_12_data) = stage_1_finalF;
	`APPLY_INPUT_$_BDDINDEX_G(stage_12_data) = stage_1_finalG;
	`APPLY_INPUT_$_TYPE(stage_12_data) = stage_1_typeOut;
	`APPLY_STAGE_12_$_SIMPLE_RESULT(stage_12_data) = stage_1_simpleResult;
	`APPLY_STAGE_12_$_HIT(stage_12_data) = stage_1_hit;
end

// Stage 2 - Fetch variables (performs two memory accesses if needed)
wire stage_2_validData;
wire stage_32_busy;
wire `APPLY_STAGE_12_DEF stage_2_data;
reg stage_2_validOut;
wire [3:0] stage_2_state;
reg [3:0] stage_2_next_state;

FSMBufferedPipeline #(.DATA_END(`APPLY_STAGE_12_END),.MAXIMUM_STATE_BITS(4)) stage_2(
		.validIn(stage_1_validData),
		.busyOut(stage_21_busy),
		.busyIn(stage_32_busy),
		.dataIn(stage_12_data),
		.validData(stage_2_validData),
		.currentData(stage_2_data),
		.currentState(stage_2_state),
		.nextState(stage_2_next_state),
		.memoryTransfer(),
		.validOut(stage_2_validOut),
		.clk(clk),
		.reset(reset)
	);

wire stage_2_responseValid = asi_var_valid;

reg `VAR_DEF stage_2_stored_f_var,stage_2_stored_g_var;
reg stage_2_storeFirstRegister,stage_2_storeSecondRegister,stage_2_clearStoredRegisters;

always @(posedge clk)
begin
	if(stage_2_storeFirstRegister)
	begin
		stage_2_stored_f_var <= asi_var_data;
	end

	if(stage_2_storeSecondRegister)
	begin
		stage_2_stored_g_var <= asi_var_data;
	end

	if(stage_2_clearStoredRegisters)
	begin
		stage_2_stored_f_var <= 12'hfff; // Clear to maximum value since it is the value we must return for indexes we do not read
		stage_2_stored_g_var <= 12'hfff;
	end

	if(reset)
	begin
		stage_2_stored_f_var <= 0;
		stage_2_stored_g_var <= 0;
	end
end

wire stage_2_simpleCase = `APPLY_STAGE_12_$_HIT(stage_2_data);
wire `INDEX_DEF stage_2_f = `APPLY_INPUT_$_BDDINDEX_F(stage_2_data);
wire `INDEX_DEF stage_2_g = `APPLY_INPUT_$_BDDINDEX_G(stage_2_data);
wire `TYPE_DEF stage_2_type = `APPLY_INPUT_$_TYPE(stage_2_data);

always @*
begin
	stage_2_validOut = 1'b0;
	stage_2_next_state = 0;
	aso_var_request_data = 0;
	aso_var_request_valid = 0;
	stage_2_storeFirstRegister = 1'b0;
	stage_2_storeSecondRegister = 1'b0;
	stage_2_clearStoredRegisters = 1'b0;

	if(stage_2_validData)
	begin
		stage_2_next_state = stage_2_state; // By default, wait
		casez(stage_2_state)
		// Received Second | Received First | Sent all | Sent First
		4'b0000:begin // Just started, check if simple case
			stage_2_clearStoredRegisters = 1'b1;
			if(stage_2_simpleCase)
			begin
				stage_2_validOut = 1'b1;
				stage_2_next_state = 0;
			end
			else
			begin
				aso_var_request_valid = 1'b1;
				`MA_REQUEST_$_BDDINDEX_NO_NEGATE(aso_var_request_data) = stage_2_f[29:1];
				`MA_REQUEST_$_TYPE_2(aso_var_request_data) = `MA_TYPE_2_GET_VAR;

				if(aso_var_request_ready)
				begin
					stage_2_next_state[0] = 1; // We only sent one request
					if(stage_2_type == `BDD_EXIST | stage_2_type == `BDD_PERMUTE)
					begin
						stage_2_next_state[1] = 1; // We sent all requests already
					end
				end
			end
		end
		4'b0001: begin // Sent only one request and are awaiting for one response
			// Send the second request
			aso_var_request_valid = 1'b1;
			`MA_REQUEST_$_BDDINDEX_NO_NEGATE(aso_var_request_data) = stage_2_g[29:1];
			`MA_REQUEST_$_TYPE_2(aso_var_request_data) = `MA_TYPE_2_GET_VAR;

			if(stage_2_responseValid)
			begin
				stage_2_storeFirstRegister = 1'b1;
				stage_2_next_state[2] = 1; // We have one result
			end
			if(aso_var_request_ready)
			begin
				stage_2_next_state[1] = 1; // We sent all requests already
			end
		end
		4'b001?: begin // Sent both requests and are still awaiting for the first
			if(stage_2_responseValid)
			begin
				stage_2_storeFirstRegister = 1'b1;
				stage_2_next_state[2] = 1; // We have one result
				if(stage_2_type == `BDD_EXIST | stage_2_type == `BDD_PERMUTE)
				begin
					stage_2_next_state[3] = 1; // We sent all requests already
				end
			end
		end
		4'b010?: begin // Sent one and got one. Need to still send second request
			aso_var_request_valid = 1'b1;
			`MA_REQUEST_$_BDDINDEX_NO_NEGATE(aso_var_request_data) = stage_2_g[29:1];
			`MA_REQUEST_$_TYPE_2(aso_var_request_data) = `MA_TYPE_2_GET_VAR;

			if(aso_var_request_ready)
			begin
				stage_2_next_state[1] = 1; // Sent both requests
			end
		end
		4'b011?: begin // Are awaiting for the second request
			if(stage_2_responseValid)
			begin
				stage_2_storeSecondRegister = 1'b1;
				stage_2_next_state[3] = 1; // We have both result
			end
		end
		4'b1???: begin // We have both results
			stage_2_next_state = 0;
			stage_2_validOut = 1'b1;
		end
		endcase
	end
end

// Data for next stage
reg `APPLY_STAGE_23_DEF stage_23_data;

wire `VAR_DEF stage_2_top = (stage_2_stored_f_var < stage_2_stored_g_var ? stage_2_stored_f_var : stage_2_stored_g_var);

always @*
begin
	stage_23_data = 0;	
	`APPLY_STAGE_23_$_APPLY_STAGE_12(stage_23_data) = stage_2_data;
	`APPLY_STAGE_23_$_TOP(stage_23_data) = stage_2_top;
	`APPLY_STAGE_23_$_FETCH_F(stage_23_data) = (stage_2_stored_f_var == stage_2_top);
	`APPLY_STAGE_23_$_FETCH_G(stage_23_data) = (stage_2_stored_g_var == stage_2_top);
end

// Stage 3 - Perform the Abstract transformation, fetch quantify and nextVar info
wire `APPLY_STAGE_23_DEF stage_3_data;
wire stage_3_validData;
wire stage_43_busy;

BufferedPipeline #(.DATA_END(`APPLY_STAGE_23_END)) stage_3(
		.validIn(stage_2_validOut),
		.dataIn(stage_23_data),
		.busyIn(stage_43_busy),
		.validOut(stage_3_validData), // Just passes data through and appends a few things
		.busyOut(stage_32_busy),
		.currentData(stage_3_data),
		.validData(stage_3_validData),
		.memoryTransfer(),
		.clk(clk),
		.reset(reset)
	);

wire `VAR_DEF stage_3_top = `APPLY_STAGE_23_$_TOP(stage_3_data);
wire `TYPE_DEF stage_3_type = `APPLY_INPUT_$_TYPE(stage_3_data);
wire `INDEX_DEF stage_3_f = `APPLY_INPUT_$_BDDINDEX_F(stage_3_data);
wire `INDEX_DEF stage_3_g = `APPLY_INPUT_$_BDDINDEX_G(stage_3_data);

wire `INDEX_DEF stage_3_outF,stage_3_outG,stage_3_simpleResult;
wire `TYPE_DEF stage_3_typeOut;
wire stage_3_hit;

ApplySimpleCases stage_3_simple_case(
		.typeIn(stage_3_type),
		.f(stage_3_f),
		.g(stage_3_g),
		.top(stage_3_top),
		.cubeLastVar(cubeLastVar),
		.outType(stage_3_typeOut),
		.outF(stage_3_outF),
		.outG(stage_3_outG),
		.hit(stage_3_hit),
		.result(stage_3_simpleResult)
	);

reg `APPLY_STAGE_34_DEF stage_34_data;

wire [31:0] stage_3_quantifyData = quantify[stage_3_top / 32];

always @*
begin
	stage_34_data = 0;
	`APPLY_STAGE_34_$_APPLY_STAGE_23(stage_34_data) = stage_3_data;
	`APPLY_INPUT_$_BDDINDEX_F(stage_34_data) = stage_3_outF;
	`APPLY_INPUT_$_BDDINDEX_G(stage_34_data) = stage_3_outG;
	`APPLY_INPUT_$_TYPE(stage_34_data) = stage_3_typeOut;
	`APPLY_STAGE_12_$_HIT(stage_34_data) = stage_3_hit; // Just replace the value. If without var is a simple case, then with var is also a simple case. We do not remove any logic that could be duplicated from stage 1 to this
	`APPLY_STAGE_12_$_SIMPLE_RESULT(stage_34_data) = stage_3_simpleResult;
	`APPLY_STAGE_34_$_QUANTIFY(stage_34_data) = stage_3_quantifyData[stage_3_top % 32];
	`APPLY_STAGE_34_$_NEXT_TOP(stage_34_data) = nextVar[stage_3_top];
end

// Stage 4 - Calculate the cache hash values
wire `APPLY_STAGE_34_DEF stage_4_data;
wire stage_4_validData;
wire stage_54_busy;

BufferedPipeline #(.DATA_END(`APPLY_STAGE_34_END)) stage_4(
		.validIn(stage_3_validData),
		.dataIn(stage_34_data),
		.busyIn(stage_54_busy),
		.validOut(stage_4_validData), // Simple passes data through
		.busyOut(stage_43_busy),
		.currentData(stage_4_data),
		.validData(stage_4_validData),
		.memoryTransfer(),
		.clk(clk),
		.reset(reset)
	);

wire `INDEX_DEF stage_4_f = `APPLY_INPUT_$_BDDINDEX_F(stage_4_data);
wire `INDEX_DEF stage_4_g = `APPLY_INPUT_$_BDDINDEX_G(stage_4_data);
wire `TYPE_DEF stage_4_type = `APPLY_INPUT_$_TYPE(stage_4_data);
wire stage_4_stored_quantify = `APPLY_STAGE_34_$_QUANTIFY(stage_4_data);

// Change top to nextTop if doing a permute
wire `VAR_DEF stage_4_top = `APPLY_STAGE_23_$_TOP(stage_4_data);
wire `VAR_DEF stage_4_nextTop = `APPLY_STAGE_34_$_NEXT_TOP(stage_4_data);

// Use this stage to set quantify to zero if And or Permute
wire stage_4_quantify = (stage_4_type == `BDD_AND | stage_4_type == `BDD_PERMUTE ? 1'b0 : stage_4_stored_quantify);

reg `APPLY_STAGE_45_DEF stage_45_data;

always @*
begin
	stage_45_data = 0;	

	`APPLY_STAGE_45_$_APPLY_STAGE_23(stage_45_data) = `APPLY_STAGE_34_$_APPLY_STAGE_23(stage_4_data);
	`APPLY_STAGE_45_$_QUANTIFY(stage_45_data) = stage_4_quantify;
	`APPLY_STAGE_45_$_CACHE_INDEX(stage_45_data) = `CACHE_HASH(stage_4_f,stage_4_g,stage_4_type,stage_4_quantify);
	`APPLY_STAGE_45_$_SIMPLE_CACHE_HASH(stage_45_data) = `SIMPLE_HASH(stage_4_f,stage_4_g,stage_4_type,stage_4_quantify);
	`APPLY_STAGE_23_$_TOP(stage_45_data) = (stage_4_type == `BDD_PERMUTE ? stage_4_nextTop : stage_4_top);
end

// Stage 5 - Get Simple Hash value
wire `APPLY_STAGE_45_DEF stage_5_data;
wire stage_5_validData;
wire stage_65_busy;

BufferedPipeline #(.DATA_END(`APPLY_STAGE_45_END)) stage_5(
		.validIn(stage_4_validData),
		.dataIn(stage_45_data),
		.busyIn(stage_65_busy),
		.validOut(stage_5_validData), // Simple passes data through
		.busyOut(stage_54_busy),
		.currentData(stage_5_data),
		.validData(stage_5_validData),
		.memoryTransfer(),
		.clk(clk),
		.reset(reset)
	);

wire `CACHE_HASH_DEF stage_5_hashIndex = `APPLY_STAGE_45_$_CACHE_INDEX(stage_5_data);

reg `APPLY_STAGE_56_DEF stage_56_data;

always @*
begin
	stage_56_data = 0;	

	`APPLY_STAGE_56_$_APPLY_STAGE_45(stage_56_data) = stage_5_data;
	`APPLY_STAGE_56_$_STORED_HASH(stage_56_data) = simpleCache[stage_5_hashIndex];
end

// Stage 6 - Access cache
wire stage_6_validData;
wire stage_76_busy;
wire `APPLY_STAGE_56_DEF stage_6_data;
reg stage_6_validOut;
wire [1:0] stage_6_state;
reg [1:0] stage_6_next_state;

FSMBufferedPipeline #(.DATA_END(`APPLY_STAGE_56_END),.MAXIMUM_STATE_BITS(2)) stage_6(
		.validIn(stage_5_validData),
		.busyOut(stage_65_busy),
		.busyIn(stage_76_busy),
		.dataIn(stage_56_data),
		.validData(stage_6_validData),
		.currentData(stage_6_data),
		.currentState(stage_6_state),
		.nextState(stage_6_next_state),
		.memoryTransfer(),
		.validOut(stage_6_validOut),
		.clk(clk),
		.reset(reset)
	);

localparam [1:0] STAGE_6_ID = 2'b10;

wire stage_6_responseValid = (asi_result_valid & asi_result_channel == STAGE_6_ID);

reg `CACHE_ENTRY_DEF stage_6_storedCacheEntry;
reg stage_6_storeRegister;

always @(posedge clk)
begin
	if(stage_6_storeRegister)
	begin
		stage_6_storedCacheEntry <= asi_result_data;
	end

	if(reset)
	begin
		stage_6_storedCacheEntry <= 0;
	end
end

wire stage_6_simpleCase = `APPLY_STAGE_12_$_HIT(stage_6_data);
wire `CACHE_HASH_DEF stage_6_cache_hash = `APPLY_STAGE_45_$_CACHE_INDEX(stage_6_data);

wire `SIMPLE_CACHE_DEF stage_6_simpleHash = `APPLY_STAGE_45_$_SIMPLE_CACHE_HASH(stage_6_data);
wire `SIMPLE_CACHE_DEF stage_6_storedSimpleHash = `APPLY_STAGE_56_$_STORED_HASH(stage_6_data);

always @*
begin
	stage_6_validOut = 1'b0;
	stage_6_next_state = 0;
	aso_cache_request_data = 0;
	aso_cache_request_valid = 0;
	stage_6_storeRegister = 1'b0;

	if(stage_6_validData)
	begin
		stage_6_next_state = stage_6_state; // By default, wait
		casez(stage_6_state)
		// Received | Sent request
		4'b00:begin // Just started, check if simple case
			if(stage_6_simpleCase | stage_6_simpleHash != stage_6_storedSimpleHash) // If simple case or the simple hash does not coincide, just move to next stage
			begin
				stage_6_validOut = 1'b1;
				stage_6_next_state = 0;
			end
			else
			begin
				aso_cache_request_valid = 1'b1;
				`MA_REQUEST_$_BDDINDEX_NO_NEGATE(aso_cache_request_data) = stage_6_cache_hash;
				`MA_REQUEST_$_TYPE_2(aso_cache_request_data) = `MA_TYPE_2_FETCH_CACHE;

				if(aso_cache_request_ready)
				begin
					stage_6_next_state[0] = 1;
				end
			end
		end
		4'b01: begin // Sent only one request and are awaiting for one response
			if(stage_6_responseValid)
			begin
				stage_6_storeRegister = 1'b1;
				stage_6_next_state[1] = 1; // We have one result
			end
		end
		4'b1?: begin // Await for next stage to not be busy
			stage_6_next_state = 0;
			stage_6_validOut = 1'b1;
		end
		endcase
	end
end

wire `INDEX_DEF stage_6_f = `APPLY_INPUT_$_BDDINDEX_F(stage_6_data);
wire `INDEX_DEF stage_6_g = `APPLY_INPUT_$_BDDINDEX_G(stage_6_data);
wire `TYPE_DEF stage_6_type = `APPLY_INPUT_$_TYPE(stage_6_data);
wire stage_6_quantify = `APPLY_STAGE_34_$_QUANTIFY(stage_6_data);
wire `INDEX_DEF stage_6_cache_result = `CACHE_ENTRY_$_BDDINDEX_RESULT(stage_6_storedCacheEntry);

wire stage_6_equal_f = (stage_6_f == `CACHE_ENTRY_$_BDDINDEX_F(stage_6_storedCacheEntry));
wire stage_6_equal_g = (stage_6_g == `CACHE_ENTRY_$_BDDINDEX_G(stage_6_storedCacheEntry));
wire stage_6_equal_type = (stage_6_type == `CACHE_ENTRY_$_TYPE(stage_6_storedCacheEntry));
wire stage_6_equal_quantify = (stage_6_quantify == `CACHE_ENTRY_$_QUANTIFY(stage_6_storedCacheEntry));

wire stage_6_equal_to_cache_entry = (stage_6_equal_f & stage_6_equal_g & stage_6_equal_type & stage_6_equal_quantify);

// Data for next stage
reg `APPLY_STAGE_56_DEF stage_67_data;

always @*
begin
	stage_67_data = stage_6_data;	

	// Not simplecase so we know the value was fetched
	// The comparision between simple hashes is not needed, but its present in order to match the testbench expected results
	// Basically, in the off chanche the data from a previous search matches the current data, and the value in the cache was overrided
	if(stage_6_equal_to_cache_entry & !stage_6_simpleCase & stage_6_simpleHash == stage_6_storedSimpleHash)
	begin
		`APPLY_STAGE_12_$_HIT(stage_67_data) = 1;
		`APPLY_STAGE_12_$_SIMPLE_RESULT(stage_67_data) = stage_6_cache_result;
	end
end

// Stage 7 - Access Nodes
wire stage_7_validData;
wire `APPLY_STAGE_56_DEF stage_7_data;
reg stage_7_validOut;
wire [3:0] stage_7_state;
reg [3:0] stage_7_next_state;

FSMBufferedPipeline #(.DATA_END(`APPLY_STAGE_56_END),.MAXIMUM_STATE_BITS(4)) stage_7(
		.validIn(stage_6_validOut),
		.busyOut(stage_76_busy),
		.busyIn(applyBusyIn),
		.dataIn(stage_67_data),
		.validData(stage_7_validData),
		.currentData(stage_7_data),
		.currentState(stage_7_state),
		.nextState(stage_7_next_state),
		.memoryTransfer(),
		.validOut(stage_7_validOut),
		.clk(clk),
		.reset(reset)
	);

localparam [1:0] STAGE_7_ID = 2'b01;

wire stage_7_responseValid = (asi_result_valid & asi_result_channel == STAGE_7_ID);

// Data
wire `INDEX_DEF stage_7_f,stage_7_g;
assign stage_7_f = `APPLY_INPUT_$_BDDINDEX_F(stage_7_data);
assign stage_7_g = `APPLY_INPUT_$_BDDINDEX_G(stage_7_data);

wire stage_7_fetchF = `APPLY_STAGE_23_$_FETCH_F(stage_7_data);
wire stage_7_fetchG = `APPLY_STAGE_23_$_FETCH_G(stage_7_data);

reg `SIMPLE_NODE_DEF stage_7_stored_f_node,stage_7_stored_g_node;
reg stage_7_storeFirstRegister,stage_7_storeSecondRegister,stage_7_clearStoredRegisters;

wire stage_7_simpleCase = `APPLY_STAGE_12_$_HIT(stage_7_data);

always @(posedge clk)
begin
	if(stage_7_storeFirstRegister)
	begin
		stage_7_stored_f_node <= asi_result_data;
	end

	if(stage_7_storeSecondRegister)
	begin
		stage_7_stored_g_node <= asi_result_data;
	end

	if(stage_7_clearStoredRegisters)
	begin
		stage_7_stored_f_node <= {2'b0,stage_7_f,2'b0,stage_7_f};
		stage_7_stored_g_node <= {2'b0,stage_7_g,2'b0,stage_7_g};
	end

	if(reset)
	begin
		stage_7_stored_f_node <= 0;
		stage_7_stored_g_node <= 0;
	end
end

always @*
begin
	stage_7_validOut = 1'b0;
	stage_7_next_state = 0;
	aso_node_request_data = 0;
	aso_node_request_valid = 0;
	stage_7_storeFirstRegister = 1'b0;
	stage_7_storeSecondRegister = 1'b0;
	stage_7_clearStoredRegisters = 1'b0;

	if(stage_7_validData)
	begin
		stage_7_next_state = stage_7_state; // By default, wait
		casez(stage_7_state)
		// Received Second | Received First | Fetched G | Fetched F
		4'b0000:begin // Just started, check if simple case
			stage_7_clearStoredRegisters = 1'b1;
			if(stage_7_simpleCase)
			begin
				stage_7_validOut = 1'b1;
				stage_7_next_state = 0;
			end
			else
			begin
				aso_node_request_valid = 1'b1;
				`MA_REQUEST_$_TYPE_2(aso_node_request_data) = `MA_TYPE_2_FETCH_SIMPLE_NODE; // Do not need to read Next value

				if(stage_7_fetchF)
				begin
					`MA_REQUEST_$_BDDINDEX_NO_NEGATE(aso_node_request_data) = stage_7_f[29:1];
					if(aso_node_request_ready)
					begin
						stage_7_next_state[0] = 1;
					end
				end
				else// Fetch G, we cannot arrive to this stage without either one of these being one
				begin 
					`MA_REQUEST_$_BDDINDEX_NO_NEGATE(aso_node_request_data) = stage_7_g[29:1];
					if(aso_node_request_ready)
					begin
						 // We did not fetched f but the result is already stored there so pretend we did
						stage_7_next_state[0] = 1;
						stage_7_next_state[2] = 1;

						stage_7_next_state[1] = 1;
					end
				end
			end
		end
		4'b0001: begin // Sent only one request and are awaiting for one response
			// Send the second request if we have to
			if(stage_7_fetchG)
			begin
				aso_node_request_valid = 1'b1;
				`MA_REQUEST_$_BDDINDEX_NO_NEGATE(aso_node_request_data) = stage_7_g[29:1];
				`MA_REQUEST_$_TYPE_2(aso_node_request_data) = `MA_TYPE_2_FETCH_SIMPLE_NODE;

				if(aso_node_request_ready)
				begin
					stage_7_next_state[1] = 1; // We sent all requests already
				end
			end

			if(stage_7_responseValid)
			begin
				stage_7_storeFirstRegister = 1'b1;
				stage_7_next_state[2] = 1; // We have one result
			end
		end
		4'b001?: begin // Sent both requests and are still awaiting for the first
			if(stage_7_responseValid)
			begin
				stage_7_storeFirstRegister = 1'b1;
				stage_7_next_state[2] = 1; // We have one result
			end
		end
		4'b010?: begin // Sent one and got one. Need to still send second request
			if(stage_7_fetchG)
			begin
				aso_node_request_valid = 1'b1;
				`MA_REQUEST_$_BDDINDEX_NO_NEGATE(aso_node_request_data) = stage_7_g[29:1];
				`MA_REQUEST_$_TYPE_2(aso_node_request_data) = `MA_TYPE_2_FETCH_SIMPLE_NODE;

				if(aso_node_request_ready)
				begin
					stage_7_next_state[1] = 1; // Sent both requests
				end
			end
			else 
			begin
				stage_7_next_state[1] = 1;
				stage_7_next_state[3] = 1;
			end
		end
		4'b011?: begin // Are awaiting for the second request
			if(stage_7_responseValid)
			begin
				stage_7_storeSecondRegister = 1'b1;
				stage_7_next_state[3] = 1; // We have both result
			end
		end
		4'b1???: begin // We have both results
			stage_7_next_state = 0;
			stage_7_validOut = 1'b1;
		end
		endcase
	end
end

wire [1:0] stage_7_type = `APPLY_INPUT_$_TYPE(stage_7_data);
wire stage_7_negateF = (stage_7_f[0]);

// Perform the negation if we fetched them
wire `INDEX_DEF stage_7_finalFn = {stage_7_stored_f_node[61:33],stage_7_stored_f_node[32] ^ (stage_7_fetchF & stage_7_negateF)};
wire `INDEX_DEF stage_7_finalFnv = {stage_7_stored_f_node[29:1],stage_7_stored_f_node[0] ^ (stage_7_fetchF & stage_7_negateF)};
wire `INDEX_DEF stage_7_finalGn = {stage_7_stored_g_node[61:33],stage_7_stored_g_node[32] ^ (stage_7_fetchG & stage_7_g[0])};
wire `INDEX_DEF stage_7_finalGnv = {stage_7_stored_g_node[29:1],stage_7_stored_g_node[0] ^ (stage_7_fetchG & stage_7_g[0])};

assign simpleCaseHit = stage_7_simpleCase; 
assign simpleCaseResult = `APPLY_STAGE_12_$_SIMPLE_RESULT(stage_7_data);
assign quantifyOut = `APPLY_STAGE_34_$_QUANTIFY(stage_7_data);
assign outType = stage_7_type;
assign fn = stage_7_finalFn;
assign fnv = stage_7_finalFnv;
assign gn = stage_7_finalGn;
assign gnv = stage_7_finalGnv;
assign outF = `APPLY_INPUT_$_BDDINDEX_F(stage_7_data);
assign outG = `APPLY_INPUT_$_BDDINDEX_G(stage_7_data);
assign topOut = `APPLY_STAGE_23_$_TOP(stage_7_data);
assign validOut = stage_7_validOut;
assign negateOut = `APPLY_INPUT_$_NEGATE(stage_7_data);
assign outIndex = `APPLY_INPUT_$_REQUEST_ID(stage_7_data);

endmodule