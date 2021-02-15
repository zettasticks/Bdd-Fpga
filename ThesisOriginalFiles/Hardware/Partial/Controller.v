`timescale 10 ns / 10 ns

`include "Constants.v"

// Interface - Dword 0 -> Set first 32 bits of request
//			   Dword 1 -> Set second 32 bits of request
//			   Dword 2 -> Set third 32 bits of request, also starts the controller
//			   Dword 3 -> Set hash value to use when inserting / reading from Hashtable 
//			   Dword 4 -> Set/Get value for hashTable
//			   Dword 5 -> Set cube last var
//			   Dword 6 -> Get final result
// 			   Dword 128 - 255 -> Set quantify data
//			   Dword 256 - (256 + 4096) -> Set nextVar data

module Controller (
		// Config
		input wire [15:0] avs_config_address,
		output wire avs_config_waitrequest,
		input wire avs_config_write,
		input wire [31:0] avs_config_writedata,
		input wire avs_config_read,
		output wor [31:0] avs_config_readdata,
		output wor avs_config_readdatavalid,

		// Send request to MA
		output wire [127:0] aso_var_data,
		input wire aso_var_ready,
		output wire aso_var_valid,

		output wor [127:0] aso_node_data,
		input wire aso_node_ready,
		output wor aso_node_valid,

		output wor [127:0] aso_cache_data,
		input wire aso_cache_ready,
		output wor aso_cache_valid,

		// Receive results from MA
		input wire [95:0] asi_result_data,
		input wire [1:0] asi_result_channel,
		output wire asi_result_ready,
		input wire asi_result_valid,

		// Receive variable data from MA
		input wire [15:0] asi_var_data,
		output wire asi_var_ready,
		input wire asi_var_valid,

		input wire clk,
		input wire reset
	);

localparam 	ACTION_APPLY = 1'b0,
			ACTION_REDUCE = 1'b1;

assign avs_config_waitrequest = reset;
assign asi_result_ready = 1'b1;
assign asi_var_ready = 1'b1;

reg finish;
reg finished; // Set to indicate the computation is finished
reg `INDEX_DEF finalResult;

reg `REQUEST_DEF contextStorage[`VAR_END * 8 - 1:0] /* synthesis ramstyle = "no_rw_check,M10K" */;
reg [7:0] contextApply[`VAR_END:0] /* synthesis ramstyle = "no_rw_check,M10K" */;
reg [7:0] contextReduce[`VAR_END:0] /* synthesis ramstyle = "no_rw_check,M10K" */;

reg [`VAR_SIZE-1:0] contextIndex;
reg [`VAR_SIZE-1:0] contextIndexPlusOne;
reg currentAction;

reg `REQUEST_DEF currentContext[7:0];
reg [7:0] currentApply;
reg [7:0] currentReduce;
reg `REQUEST_DEF currentNextContext[7:0];

integer i;

reg [31:0] controllerState,controllerNextState;
reg resetPipeline;

// Perform Apply
reg `INDEX_DEF f,g;
reg `TYPE_DEF typeIn;
reg validIn;
reg negate;
wire applyBusy;

reg `REQUEST_DEF newContext[7:0];
reg [7:0] newApply;
reg [7:0] newReduce;

// Sends data from Current Context to the Apply module
reg StartSendingDataToApply;

reg [3:0] SendContextApplyCounter;
reg [3:0] ValidApplyRequestsSent;
reg sendingData;
reg SendContextApplyFinished;

always @*
begin
	f = 0;
	g = 0;
	typeIn = 0;
	validIn = 0;
	negate = 0;

	if(sendingData & currentApply[SendContextApplyCounter])
	begin
		negate = `REQUEST_NEGATE(currentContext[SendContextApplyCounter]);
		f = `REQUEST_F(currentContext[SendContextApplyCounter]);
		g = `REQUEST_G(currentContext[SendContextApplyCounter]);
		typeIn = `REQUEST_TYPE(currentContext[SendContextApplyCounter]);
		validIn = 1'b1;
	end
end

wire simpleCaseHit;
wire `INDEX_DEF simpleCaseResult;
wire `INDEX_DEF fn;
wire `INDEX_DEF fnv;
wire `INDEX_DEF gn;
wire `INDEX_DEF gnv;
wire `INDEX_DEF outF;
wire `INDEX_DEF outG;
wire quantifyOut;
wire `TYPE_DEF outType;
wire validOut;
wire `VAR_DEF topOut;
wire applyBusyIn;
wire negateOut;
wire [2:0] outIndex;

reg insertValueIntoSimpleCache;
reg [`CACHE_BIT_SIZE-1:0] simpleCacheIndex;
reg [`SIMPLE_CACHE_ENTRY_BIT_SIZE-1:0] simpleCacheValue;

wire [31:0] apply_avs_config_readdata;
wire apply_avs_config_readdatavalid;

assign avs_config_readdata = apply_avs_config_readdatavalid ? apply_avs_config_readdata : 0;
assign avs_config_readdatavalid = apply_avs_config_readdatavalid;

wire [127:0] apply_node_data,apply_cache_data;
wire apply_node_valid,apply_cache_valid;

assign aso_node_data = apply_node_data;
assign aso_node_valid = apply_node_valid;
assign aso_cache_data = apply_cache_data;
assign aso_cache_valid = apply_cache_valid;

Apply apply (
		.f(f),
		.g(g),
		.typeIn(typeIn),
		.valid(validIn),
		.negate(negate),
		.applyBusy(applyBusy),
		.index(SendContextApplyCounter[2:0]),
		.simpleCaseHit(simpleCaseHit),
		.simpleCaseResult(simpleCaseResult),
		.fn(fn),
		.fnv(fnv),
		.gn(gn),
		.gnv(gnv),
		.outF(outF),
		.outG(outG),
		.quantifyOut(quantifyOut),
		.topOut(topOut),
		.outType(outType),
		.validOut(validOut),
		.applyBusyIn(applyBusyIn),
		.negateOut(negateOut),
		.outIndex(outIndex),
		.insertValueIntoSimpleCache(insertValueIntoSimpleCache),
		.simpleCacheIndex(simpleCacheIndex),
		.simpleCacheValue(simpleCacheValue),
		.avs_config_address(avs_config_address), 
		.avs_config_write(avs_config_write),
		.avs_config_writedata(avs_config_writedata),
		.avs_config_read(avs_config_read),
		.avs_config_readdata(apply_avs_config_readdata),
		.avs_config_readdatavalid(apply_avs_config_readdatavalid),
		.avs_config_waitrequest(),
		.aso_var_request_data(aso_var_data),
		.aso_var_request_ready(aso_var_ready),
		.aso_var_request_valid(aso_var_valid),
		.aso_node_request_data(apply_node_data),
		.aso_node_request_ready(aso_node_ready),
		.aso_node_request_valid(apply_node_valid),
		.aso_cache_request_data(apply_cache_data),
		.aso_cache_request_ready(aso_cache_ready),
		.aso_cache_request_valid(apply_cache_valid),
		.asi_result_data(asi_result_data),
		.asi_result_ready(),
		.asi_result_valid(asi_result_valid),
		.asi_result_channel(asi_result_channel),
		.asi_var_data(asi_var_data),
		.asi_var_ready(),
		.asi_var_valid(asi_var_valid),
		.clk(clk),
		.reset(reset | finish)
	);

`define APPLY_OUT_$_FN_END 29
`define APPLY_OUT_$_FNV_END 59
`define APPLY_OUT_$_GN_END 89
`define APPLY_OUT_$_GNV_END 119
`define APPLY_OUT_$_SIMPLE_CASE_END 120
`define APPLY_OUT_$_SIMPLE_CASE_RESULT_END 150
`define APPLY_OUT_$_QUANTIFY_END 151
`define APPLY_OUT_$_OUT_F_END 181
`define APPLY_OUT_$_OUT_G_END 211
`define APPLY_OUT_$_OUT_TYPE_END 213
`define APPLY_OUT_$_OUT_TOP_END 223
`define APPLY_OUT_$_EXIST_SIMPLE_CASE_END 224
`define APPLY_OUT_$_NEGATE_END 225
`define APPLY_OUT_$_INDEX_END 228
`define APPLY_OUT_END 228

`define APPLY_OUT_$_FN(in) in[29:0]
`define APPLY_OUT_$_FNV(in) in[59:30]
`define APPLY_OUT_$_GN(in) in[89:60]
`define APPLY_OUT_$_GNV(in) in[119:90]
`define APPLY_OUT_$_SIMPLE_CASE(in) in[120:120]
`define APPLY_OUT_$_SIMPLE_CASE_RESULT(in) in[150:121]
`define APPLY_OUT_$_QUANTIFY(in) in[151:151]
`define APPLY_OUT_$_OUT_F(in) in[181:152]
`define APPLY_OUT_$_OUT_G(in) in[211:182]
`define APPLY_OUT_$_OUT_TYPE(in) in[213:212]
`define APPLY_OUT_$_OUT_TOP(in) in[223:214]
`define APPLY_OUT_$_EXIST_SIMPLE_CASE(in) in[224:224]
`define APPLY_OUT_$_NEGATE(in) in[225:225]
`define APPLY_OUT_$_INDEX(in) in[228:226]
`define APPLY_OUT_DEF [228:0]
`define APPLY_OUT_RANGE 228:0


reg `APPLY_OUT_DEF apply_out_dataIn;
always @*
begin
	apply_out_dataIn = 0;
	`APPLY_OUT_$_FN(apply_out_dataIn) = (fn > gn ? gn : fn);
	`APPLY_OUT_$_GN(apply_out_dataIn) = (fn > gn ? fn : gn);
	`APPLY_OUT_$_FNV(apply_out_dataIn) = (fnv > gnv ? gnv : fnv);
	`APPLY_OUT_$_GNV(apply_out_dataIn) = (fnv > gnv ? fnv : gnv);
	`APPLY_OUT_$_SIMPLE_CASE(apply_out_dataIn) = simpleCaseHit;
	`APPLY_OUT_$_SIMPLE_CASE_RESULT(apply_out_dataIn) = simpleCaseResult;
	`APPLY_OUT_$_QUANTIFY(apply_out_dataIn) = quantifyOut;
	`APPLY_OUT_$_OUT_F(apply_out_dataIn) = outF;
	`APPLY_OUT_$_OUT_G(apply_out_dataIn) = outG;
	`APPLY_OUT_$_OUT_TYPE(apply_out_dataIn) = outType;
	`APPLY_OUT_$_OUT_TOP(apply_out_dataIn) = topOut;
	`APPLY_OUT_$_EXIST_SIMPLE_CASE(apply_out_dataIn) = (outType == `BDD_EXIST & quantifyOut & ((fn == `BDD_ONE) | (fnv == `BDD_ONE) | (`EQUAL_NEGATE(fn,fnv))));
	`APPLY_OUT_$_NEGATE(apply_out_dataIn) = negateOut;
	`APPLY_OUT_$_INDEX(apply_out_dataIn) = outIndex;
end

// Perform search and request insertion
wire apply_out_validData;
wire `APPLY_OUT_DEF apply_out_data;

// Perform everything in one cycle, hopefully
BufferedPipeline #(.DATA_END(`APPLY_OUT_END)) apply_out(
		.validIn(validOut),
		.busyOut(applyBusyIn),
		.busyIn(1'b0),
		.dataIn(apply_out_dataIn),
		.validData(apply_out_validData),
		.currentData(apply_out_data),
		.memoryTransfer(),
		.validOut(apply_out_validData),
		.clk(clk),
		.reset(reset | finish)
	);

reg [3:0] apply_out_requestsCreated;

wire `INDEX_DEF apply_out_fn = `APPLY_OUT_$_FN(apply_out_data);
wire `INDEX_DEF apply_out_gn = `APPLY_OUT_$_GN(apply_out_data);
wire `INDEX_DEF apply_out_fnv =	`APPLY_OUT_$_FNV(apply_out_data);
wire `INDEX_DEF apply_out_gnv = `APPLY_OUT_$_GNV(apply_out_data);
wire `INDEX_DEF apply_out_f = `APPLY_OUT_$_OUT_F(apply_out_data);
wire `INDEX_DEF apply_out_g = `APPLY_OUT_$_OUT_G(apply_out_data);
wire `VAR_DEF apply_out_top = `APPLY_OUT_$_OUT_TOP(apply_out_data);
wire `TYPE_DEF apply_out_type = `APPLY_OUT_$_OUT_TYPE(apply_out_data);
wire apply_out_negate = `APPLY_OUT_$_NEGATE(apply_out_data);
wire apply_out_quantify = `APPLY_OUT_$_QUANTIFY(apply_out_data);
wire apply_out_simpleCase = `APPLY_OUT_$_SIMPLE_CASE(apply_out_data);
wire apply_out_existSimpleCase = `APPLY_OUT_$_EXIST_SIMPLE_CASE(apply_out_data);
wire `INDEX_DEF apply_out_simpleCaseResult = `APPLY_OUT_$_SIMPLE_CASE_RESULT(apply_out_data);
wire [2:0] apply_out_index = `APPLY_OUT_$_INDEX(apply_out_data);

reg apply_out_equalThen[7:0];
reg apply_out_equalElse[7:0];

reg [3:0] apply_out_equalThenIndex;
reg [3:0] apply_out_equalElseIndex;

reg apply_out_equalThenCase,apply_out_equalElseCase;
reg apply_out_two_equals;
reg apply_out_equal;

reg `REQUEST_DEF requestToSaveApply;
reg `REQUEST_DEF leftRequestToCreate;
reg `REQUEST_DEF rightRequestToCreate;

reg apply_out_valid_reduce,apply_out_valid_apply;

reg `REQUEST_DEF newNextContext[7:0];
reg [7:0] newNextApply;
reg [3:0] requestsApplyProcessed;
reg applyOneRequestApplied;

wire applyStoreLeft = (((apply_out_type == `BDD_EXIST) | (apply_out_type == `BDD_ABSTRACT)) & apply_out_quantify);

always @*
begin
	apply_out_equalThenIndex = 0;
	apply_out_equalElseIndex = 0;
	apply_out_equalThenCase = 0;
	apply_out_equalElseCase = 0;
	apply_out_two_equals = 0;
	apply_out_equal = 0;

	for(i = 0; i < 8; i = i + 1)
	begin
		apply_out_equalThen[i] = (`REQUEST_F(newNextContext[i]) == apply_out_fn && `REQUEST_G(newNextContext[i]) == apply_out_gn && `REQUEST_TYPE(newNextContext[i]) == apply_out_type && !`REQUEST_STORE_ONLY(newNextContext[i]));
		apply_out_equalElse[i] = (`REQUEST_F(newNextContext[i]) == apply_out_fnv && `REQUEST_G(newNextContext[i]) == apply_out_gnv && `REQUEST_TYPE(newNextContext[i]) == apply_out_type && !`REQUEST_STORE_ONLY(newNextContext[i]));
	end

	for(i = 0; i < 8; i = i + 1)
	begin
		if(apply_out_equalThen[i] & (i < apply_out_requestsCreated))
		begin
			apply_out_equalThenIndex = i;
			apply_out_equal = 1;
			apply_out_equalThenCase = 1;
		end
		if(apply_out_equalElse[i] & (i < apply_out_requestsCreated))
		begin
			apply_out_equalElseIndex = i;
			apply_out_equal = 1;
			apply_out_equalElseCase = 1;
		end
	end

	if(apply_out_equalThenCase & apply_out_equalElseCase)
	begin
		apply_out_two_equals = 1'b1;
	end

	apply_out_valid_apply = 1'b0;
	apply_out_valid_reduce = 1'b1;

	requestToSaveApply = 0;
	`REQUEST_F(requestToSaveApply) = apply_out_f;
	`REQUEST_G(requestToSaveApply) = apply_out_g;
	`REQUEST_TOP(requestToSaveApply) = apply_out_top;
	`REQUEST_TYPE(requestToSaveApply) = apply_out_type;
	`REQUEST_NEGATE(requestToSaveApply) = apply_out_negate;
	`REQUEST_QUANTIFY(requestToSaveApply) = apply_out_quantify;
	//`REQUEST_LEFT_INDEX(requestToSaveApply) = apply_out_equalElseIndex;
	//`REQUEST_RIGHT_INDEX(requestToSaveApply) = apply_out_equalThenIndex;

	if(apply_out_validData)
	begin
		if(apply_out_simpleCase | apply_out_existSimpleCase)
		begin
			apply_out_valid_reduce = 1'b0;
			if(apply_out_simpleCase)
			begin
				requestToSaveApply = 0;
				`REQUEST_F(requestToSaveApply) = apply_out_simpleCaseResult ^ apply_out_negate;
			end
			else begin
				requestToSaveApply = 0;
				`REQUEST_F(requestToSaveApply) = `BDD_ONE; // No negate, since exist does not have it
			end
		end
		else if(apply_out_two_equals)
		begin
			`REQUEST_LEFT_INDEX(requestToSaveApply) = apply_out_equalElseIndex;
			`REQUEST_RIGHT_INDEX(requestToSaveApply) = apply_out_equalThenIndex;
		end
		else if(apply_out_equal & (apply_out_requestsCreated + 1 <= 8))
		begin
			if(apply_out_equalThenCase)
			begin
				`REQUEST_LEFT_INDEX(requestToSaveApply) = apply_out_requestsCreated;
				`REQUEST_RIGHT_INDEX(requestToSaveApply) = apply_out_equalThenIndex;
			end
			else 
			begin
				`REQUEST_LEFT_INDEX(requestToSaveApply) = apply_out_equalElseIndex;
				`REQUEST_RIGHT_INDEX(requestToSaveApply) = apply_out_requestsCreated;
			end
		end
		else if (apply_out_requestsCreated + 2 <= 8)
		begin
			`REQUEST_LEFT_INDEX(requestToSaveApply) = apply_out_requestsCreated + 1;
			`REQUEST_RIGHT_INDEX(requestToSaveApply) = apply_out_requestsCreated;
		end
		else 
		begin
			apply_out_valid_apply = 1'b1;
			apply_out_valid_reduce = 1'b0;
		end
	end

	rightRequestToCreate = 0;
	`REQUEST_F(rightRequestToCreate) = apply_out_fn;
	`REQUEST_G(rightRequestToCreate) = apply_out_gn;
	`REQUEST_TYPE(rightRequestToCreate) = apply_out_type;

	leftRequestToCreate = 0;
	`REQUEST_F(leftRequestToCreate) = apply_out_fnv;
	`REQUEST_G(leftRequestToCreate) = apply_out_gnv;
	`REQUEST_TYPE(leftRequestToCreate) = apply_out_type;
	`REQUEST_STORE_ONLY(leftRequestToCreate) = applyStoreLeft;
end

// Reduce 
reg StartSendingDataToReduce;

reg [3:0] SendContextReduceCounter;
reg [3:0] ValidReduceRequestsSent;
reg sendingReduceData;
reg SendContextReduceFinished;

wire reduceBusy;

`define REDUCE_IN_$_REDUCE_REQUEST_END 84
`define REDUCE_IN_$_E_REQUEST_END 169
`define REDUCE_IN_$_T_REQUEST_END 254
`define REDUCE_IN_$_REQUEST_ID_END 257
`define REDUCE_IN_END 257

`define REDUCE_IN_$_REDUCE_REQUEST(in) in[84:0]
`define REDUCE_IN_$_E_REQUEST(in) in[169:85]
`define REDUCE_IN_$_T_REQUEST(in) in[254:170]
`define REDUCE_IN_$_REQUEST_ID(in) in[257:255]
`define REDUCE_IN_DEF [257:0]
`define REDUCE_IN_RANGE 257:0

reg `REDUCE_IN_DEF reduceIn;
reg validReduceIn;

wire [3:0] reduceInLeftIndex = `REQUEST_LEFT_INDEX(currentContext[SendContextReduceCounter]);
wire [3:0] reduceInRightIndex = `REQUEST_RIGHT_INDEX(currentContext[SendContextReduceCounter]);

always @*
begin
	reduceIn = 0;
	validReduceIn = 1'b0;

	if(sendingReduceData & currentReduce[SendContextReduceCounter])
	begin
		`REDUCE_IN_$_REDUCE_REQUEST(reduceIn) = currentContext[SendContextReduceCounter];
		`REDUCE_IN_$_E_REQUEST(reduceIn) = currentNextContext[reduceInLeftIndex];
		`REDUCE_IN_$_T_REQUEST(reduceIn) = currentNextContext[reduceInRightIndex];
		`REDUCE_IN_$_REQUEST_ID(reduceIn) = SendContextReduceCounter;
		validReduceIn = 1'b1;
	end
end

wire reduce_1_validData;
wire reduce_21_busy;
wire `REDUCE_IN_DEF reduce_1_data;
wire reduce_1_state;
reg reduce_1_nextState;
reg reduce_1_validOut;

// Stage 1 - Compute simple case. If not simple, perform FindOrInsert
FSMBufferedPipeline #(.DATA_END(`REDUCE_IN_END),.MAXIMUM_STATE_BITS(1)) reduce_1(
		.validIn(validReduceIn),
		.busyOut(reduceBusy),
		.busyIn(reduce_21_busy),
		.dataIn(reduceIn),
		.validData(reduce_1_validData),
		.currentData(reduce_1_data),
		.currentState(reduce_1_state),
		.nextState(reduce_1_nextState),
		.memoryTransfer(),
		.validOut(reduce_1_validOut),
		.clk(clk),
		.reset(reset | finish)
	);

wire `REQUEST_DEF reduce_1_request = `REDUCE_IN_$_REDUCE_REQUEST(reduce_1_data);
wire `REQUEST_DEF reduce_1_eRequest = `REDUCE_IN_$_E_REQUEST(reduce_1_data);
wire `REQUEST_DEF reduce_1_tRequest = `REDUCE_IN_$_T_REQUEST(reduce_1_data);

wire `INDEX_DEF reduce_1_f = `REQUEST_F(reduce_1_request);
wire `INDEX_DEF reduce_1_g = `REQUEST_G(reduce_1_request);

wire [1:0] reduce_1_type = `REQUEST_TYPE(reduce_1_request);
wire reduce_1_quantify = `REQUEST_QUANTIFY(reduce_1_request);
wire `VAR_DEF reduce_1_top = `REQUEST_TOP(reduce_1_request);

wire reduce_1_storeOnly = `REQUEST_STORE_ONLY(reduce_1_eRequest);

wire `INDEX_DEF reduce_1_e = `REQUEST_F(reduce_1_eRequest);
wire `INDEX_DEF reduce_1_t = `REQUEST_F(reduce_1_tRequest);

wire `INDEX_DEF reduce_1_fe = `REQUEST_F(reduce_1_eRequest);
wire `INDEX_DEF reduce_1_ge = `REQUEST_G(reduce_1_eRequest);

wire reduce_1_special = ((reduce_1_type == `BDD_EXIST | reduce_1_type == `BDD_ABSTRACT) & reduce_1_quantify);

wire reduce_1_exist_simple_case = (reduce_1_special & reduce_1_t == `BDD_ONE);

wire reduce_1_abstract_simple_comparison = (reduce_1_t == `BDD_ONE | reduce_1_t == reduce_1_fe | reduce_1_t == reduce_1_ge);
wire reduce_1_abstract_simple_case = (reduce_1_special & reduce_1_storeOnly & reduce_1_abstract_simple_comparison);
wire reduce_1_abstract_simple_case_2 = (reduce_1_special & !reduce_1_storeOnly & reduce_1_t == reduce_1_e);

wire reduce_1_simpleCase = ((reduce_1_type == `BDD_EXIST & reduce_1_exist_simple_case) |
							(reduce_1_type == `BDD_ABSTRACT & (reduce_1_abstract_simple_case | reduce_1_abstract_simple_case_2)) |
							(!reduce_1_special & reduce_1_t == reduce_1_e));

wire reduce_1_requestCreation = (reduce_1_special & !reduce_1_simpleCase);

wire reduce_1_exist_creation_1 = (reduce_1_type == `BDD_EXIST & reduce_1_quantify & reduce_1_storeOnly);
wire reduce_1_exist_creation_2 = (reduce_1_type == `BDD_EXIST & reduce_1_quantify & !reduce_1_storeOnly);

wire reduce_1_abstract_creation_1 = (reduce_1_type == `BDD_ABSTRACT & reduce_1_quantify & reduce_1_storeOnly);
wire reduce_1_abstract_creation_2 = (reduce_1_type == `BDD_ABSTRACT & reduce_1_quantify & !reduce_1_storeOnly);

wire `INDEX_DEF reduce_1_findOrInsertIndexOut;
wire reduce_1_findOrInsertValid;

reg reduce_1_doFindOrInsert;

wire [127:0] reduce_node_data;
wire reduce_node_valid;

assign aso_node_data = reduce_node_data;
assign aso_node_valid = reduce_node_valid;

wire [31:0] findOrInsert_avs_config_readdata;
wire findOrInsert_avs_config_readdatavalid;
wire findOrInsert_asi_result_ready;

assign avs_config_readdata = findOrInsert_avs_config_readdatavalid ? findOrInsert_avs_config_readdata : 0;
assign avs_config_readdatavalid = findOrInsert_avs_config_readdatavalid;

wire negateFindOrInsertResult = reduce_1_t[0];
wire `INDEX_DEF inE = {reduce_1_e[29:1],reduce_1_e[0] ^ negateFindOrInsertResult};
wire `INDEX_DEF inT = {reduce_1_t[29:1],1'b0};

FindOrInsert findOrInsert (
		.e(inE),
		.t(inT),
		.top(reduce_1_top),
		.valid(reduce_1_doFindOrInsert),
		.busy(),
		.result(reduce_1_findOrInsertIndexOut),
		.validOut(reduce_1_findOrInsertValid),
		.avs_config_address(avs_config_address),
		.avs_config_write(avs_config_write),
		.avs_config_writedata(avs_config_writedata),
		.avs_config_read(avs_config_read),
		.avs_config_readdata(findOrInsert_avs_config_readdata),
		.avs_config_readdatavalid(findOrInsert_avs_config_readdatavalid),
		.avs_config_waitrequest(),
		.aso_request_data(reduce_node_data),
		.aso_request_ready(aso_node_ready),
		.aso_request_valid(reduce_node_valid),
		.asi_result_data(asi_result_data),
		.asi_result_channel(asi_result_channel),
		.asi_result_ready(),
		.asi_result_valid(asi_result_valid),
		.clk(clk),
		.reset(reset | finish)
	);

reg reduce_1_useFindOrInsertIndex;

always @*
begin
	reduce_1_nextState = 0;
	reduce_1_validOut = 0;
	reduce_1_doFindOrInsert = 0;
	reduce_1_useFindOrInsertIndex = 0;

	if(reduce_1_validData)
	begin
		case(reduce_1_state)
			1'b0 : begin
				if(reduce_1_simpleCase | reduce_1_requestCreation)
				begin
					reduce_1_validOut = 1'b1;
				end
				else
				begin
					// Activate FindOrInsert
					reduce_1_nextState = 1'b1;
					reduce_1_doFindOrInsert = 1'b1;
				end
			end
			1'b1 : begin // Wait for end of FindOrInsert
				reduce_1_nextState = 1'b1;
				if(reduce_1_findOrInsertValid)
				begin
					reduce_1_validOut = 1'b1;
					reduce_1_nextState = 1'b0;
					reduce_1_useFindOrInsertIndex = 1'b1;
				end
			end
		endcase
	end
end

`define REDUCE_12_$_REDUCE_REQUEST_END 84
`define REDUCE_12_$_E_REQUEST_END 169
`define REDUCE_12_$_T_REQUEST_END 254
`define REDUCE_12_$_INDEX_END 284
`define REDUCE_12_$_REQUEST_CREATION_END 285
`define REDUCE_12_$_REQUEST_CREATION_TYPE_END 289
`define REDUCE_12_$_SIMPLE_CACHE_HASH_END 291
`define REDUCE_12_$_CACHE_INDEX_END 309
`define REDUCE_12_$_REQUEST_ID_END 312
`define REDUCE_12_END 312

`define REDUCE_12_$_REDUCE_REQUEST(in) in[84:0]
`define REDUCE_12_$_E_REQUEST(in) in[169:85]
`define REDUCE_12_$_T_REQUEST(in) in[254:170]
`define REDUCE_12_$_INDEX(in) in[284:255]
`define REDUCE_12_$_REQUEST_CREATION(in) in[285:285]
`define REDUCE_12_$_REQUEST_CREATION_TYPE(in) in[289:286]
`define REDUCE_12_$_SIMPLE_CACHE_HASH(in) in[291:290]
`define REDUCE_12_$_CACHE_INDEX(in) in[309:292]
`define REDUCE_12_$_REQUEST_ID(in) in[312:310]
`define REDUCE_12_DEF [312:0]
`define REDUCE_12_RANGE 312:0


reg `REDUCE_12_DEF reduce_12_data;
always @*
begin
	reduce_12_data = 0;

	`REDUCE_12_$_REQUEST_ID(reduce_12_data) = `REDUCE_IN_$_REQUEST_ID(reduce_1_data);
	`REDUCE_12_$_REDUCE_REQUEST(reduce_12_data) = reduce_1_request;
	`REDUCE_12_$_E_REQUEST(reduce_12_data) = reduce_1_eRequest;
	`REDUCE_12_$_T_REQUEST(reduce_12_data) = reduce_1_tRequest;
	`REDUCE_12_$_INDEX(reduce_12_data) = (reduce_1_useFindOrInsertIndex ? (reduce_1_findOrInsertIndexOut ^ negateFindOrInsertResult) : reduce_1_t); // Also sends t in the case we have to create requests
	`REDUCE_12_$_REQUEST_CREATION(reduce_12_data) = reduce_1_requestCreation;
	`REDUCE_12_$_REQUEST_CREATION_TYPE(reduce_12_data) = {reduce_1_abstract_creation_2,reduce_1_abstract_creation_1,reduce_1_exist_creation_2,reduce_1_exist_creation_1};
	`REDUCE_12_$_SIMPLE_CACHE_HASH(reduce_12_data) = `SIMPLE_HASH(reduce_1_f,reduce_1_g,reduce_1_type,reduce_1_quantify);
	`REDUCE_12_$_CACHE_INDEX(reduce_12_data) = `CACHE_HASH(reduce_1_f,reduce_1_g,reduce_1_type,reduce_1_quantify);
end

// Stage 2 - If Simple Case or Result, insert into cache. Otherwise, Compute and Insert NewNextRequest
wire reduce_2_validData;
wire `REDUCE_12_DEF reduce_2_data;
reg reduce_2_validOut;

BufferedPipeline #(.DATA_END(`REDUCE_12_END)) reduce_2(
		.validIn(reduce_1_validOut),
		.busyOut(reduce_21_busy),
		.busyIn(1'b0),
		.dataIn(reduce_12_data),
		.validData(reduce_2_validData),
		.currentData(reduce_2_data),
		.memoryTransfer(),
		.validOut(reduce_2_validOut),
		.clk(clk),
		.reset(reset | finish)
	);

wire reduce_2_requestCreation = `REDUCE_12_$_REQUEST_CREATION(reduce_2_data);
wire [3:0] reduce_2_requestCreationType = `REDUCE_12_$_REQUEST_CREATION_TYPE(reduce_2_data);

wire [3:0] reduce_2_request_id = `REDUCE_12_$_REQUEST_ID(reduce_2_data);

wire `INDEX_DEF reduce_2_index = `REDUCE_12_$_INDEX(reduce_2_data);
wire `REQUEST_DEF reduce_2_leftRequest = `REDUCE_12_$_E_REQUEST(reduce_2_data);
wire `REQUEST_DEF reduce_2_rightRequest = `REDUCE_12_$_T_REQUEST(reduce_2_data);

wire `REQUEST_DEF reduce_2_request = `REDUCE_12_$_REDUCE_REQUEST(reduce_2_data);
wire `INDEX_DEF reduce_2_f = `REQUEST_F(reduce_2_request);
wire `INDEX_DEF reduce_2_g = `REQUEST_G(reduce_2_request);
wire reduce_2_quantify = `REQUEST_QUANTIFY(reduce_2_request);
wire `TYPE_DEF reduce_2_type = `REQUEST_TYPE(reduce_2_request);

wire [3:0] reduce_2_left_index = `REQUEST_LEFT_INDEX(reduce_2_request);
wire [3:0] reduce_2_right_index = `REQUEST_RIGHT_INDEX(reduce_2_request);

wire `INDEX_DEF reduce_2_t = `REDUCE_12_$_INDEX(reduce_2_data);
wire `INDEX_DEF reduce_2_e = `REQUEST_F(reduce_2_leftRequest);

wire `INDEX_DEF reduce_2_fe = `REQUEST_F(reduce_2_leftRequest);
wire `INDEX_DEF reduce_2_ge = `REQUEST_G(reduce_2_leftRequest);

wire `INDEX_DEF reduce_2_negated_e = (reduce_2_e ^ 1'b1);
wire `INDEX_DEF reduce_2_negated_t = (reduce_2_t ^ 1'b1);

wire reduce_2_change_order = (reduce_2_negated_t > reduce_2_negated_e);

wire `INDEX_DEF reduce_2_res_1 = (reduce_2_change_order ? reduce_2_negated_e : reduce_2_negated_t);
wire `INDEX_DEF reduce_2_res_2 = (reduce_2_change_order ? reduce_2_negated_t : reduce_2_negated_e);

wire [`CACHE_BIT_SIZE-1:0] reduce_2_cacheIndex = `REDUCE_12_$_CACHE_INDEX(reduce_2_data);
wire [`SIMPLE_CACHE_ENTRY_BIT_SIZE-1:0] reduce_2_simpleCacheValue = `REDUCE_12_$_SIMPLE_CACHE_HASH(reduce_2_data); 

reg `CACHE_ENTRY_DEF reduce_2_cacheEntry;
reg `REQUEST_DEF reduce_2_newLeftRequest,reduce_2_newRequest;

reg reduce_2_validReduce,reduce_2_validApply,reduce_2_validNextApply;

reg [128:0] reduce_cache_data;
reg reduce_cache_valid;

assign aso_cache_data = reduce_cache_data;
assign aso_cache_valid = reduce_cache_valid;

always @*
begin
	reduce_2_cacheEntry = 0;
	reduce_2_validOut = 1'b0;
	reduce_cache_data = 0;
	reduce_cache_valid = 0;
	reduce_2_validReduce = 0;
	reduce_2_validApply = 0;
	reduce_2_validNextApply = 0;

	insertValueIntoSimpleCache = 1'b0;
	simpleCacheIndex = 0;
	simpleCacheValue = 0;

	`CACHE_ENTRY_$_BDDINDEX_RESULT(reduce_2_cacheEntry) = reduce_2_index; // The cache value inserted does not take into account the Request negation
	`CACHE_ENTRY_$_BDDINDEX_F(reduce_2_cacheEntry) = reduce_2_f;
	`CACHE_ENTRY_$_BDDINDEX_G(reduce_2_cacheEntry) = reduce_2_g;
	`CACHE_ENTRY_$_QUANTIFY(reduce_2_cacheEntry) = reduce_2_quantify;
	`CACHE_ENTRY_$_TYPE(reduce_2_cacheEntry) = reduce_2_type;

	reduce_2_newLeftRequest = reduce_2_leftRequest;
	reduce_2_newRequest = reduce_2_request;

	if(reduce_2_validData)
	begin
		// Store in SDRAM
		if(!reduce_2_requestCreation)
		begin
			reduce_cache_valid = 1'b1;
			`MA_REQUEST_$_DATA(reduce_cache_data) = reduce_2_cacheEntry;
			`MA_REQUEST_$_TYPE_2(reduce_cache_data) = `MA_TYPE_2_WRITE_CACHE;
			`MA_REQUEST_$_BDDINDEX_NO_NEGATE(reduce_cache_data) = reduce_2_cacheIndex;

			insertValueIntoSimpleCache = 1'b1;
			simpleCacheIndex = reduce_2_cacheIndex;
			simpleCacheValue = reduce_2_simpleCacheValue;

			 // Save result in F of request
			`REQUEST_F(reduce_2_newRequest) = reduce_2_index ^ `REQUEST_NEGATE(reduce_2_request);

			if(aso_cache_ready)
			begin
				reduce_2_validOut = 1'b1;
			end
		end
		else
		begin
			reduce_2_validNextApply = 1'b0;
			casez(reduce_2_requestCreationType)
			4'b???1: begin // Exist 1
				`REQUEST_STORE_ONLY(reduce_2_newLeftRequest) = 1'b0;

				reduce_2_validReduce = 1'b1;
				reduce_2_validNextApply = 1'b1;
			end
			4'b??1?: begin // Exist 2
				`REQUEST_TYPE(reduce_2_newRequest) = `BDD_AND;
				`REQUEST_NEGATE(reduce_2_newRequest) = 1'b1;
				`REQUEST_F(reduce_2_newRequest) = reduce_2_res_1;
				`REQUEST_G(reduce_2_newRequest) = reduce_2_res_2;

				reduce_2_validApply = 1'b1;
			end
			4'b?1??: begin // Abstract 1
				if(`EQUAL_NEGATE(reduce_2_t,reduce_2_fe))
				begin
					`REQUEST_F(reduce_2_newLeftRequest) = reduce_2_ge;
					`REQUEST_G(reduce_2_newLeftRequest) = `BDD_ZERO;
					`REQUEST_TYPE(reduce_2_newLeftRequest) = `BDD_EXIST;
				end
				else if(`EQUAL_NEGATE(reduce_2_t,reduce_2_ge))
				begin
					`REQUEST_F(reduce_2_newLeftRequest) = reduce_2_fe;
					`REQUEST_G(reduce_2_newLeftRequest) = `BDD_ZERO;
					`REQUEST_TYPE(reduce_2_newLeftRequest) = `BDD_EXIST;
				end

				`REQUEST_STORE_ONLY(reduce_2_newLeftRequest) = 1'b0;
				`REQUEST_NEGATE(reduce_2_newLeftRequest) = 1'b0;

				reduce_2_validNextApply = 1'b1;
				reduce_2_validReduce = 1'b1;
			end
			4'b1???: begin // Abstract 2 - Equal to Exist 2
				`REQUEST_TYPE(reduce_2_newRequest) = `BDD_AND;
				`REQUEST_NEGATE(reduce_2_newRequest) = 1'b1;
				`REQUEST_F(reduce_2_newRequest) = reduce_2_res_1;
				`REQUEST_G(reduce_2_newRequest) = reduce_2_res_2;

				reduce_2_validApply = 1'b1;
			end
			endcase
			
			reduce_2_validOut = 1'b1;
		end
	end
end

reg applyNextContext;
reg [3:0] requestsReduceProcessed; 

// Send new Next context to current context
reg copyContextStart; // Control

localparam 	COPY_CONTEXT_SAME = 2'b00, // Copys the same context from the output into the input (New Current to Current and New Next to Next)
			COPY_CONTEXT_NEXT = 2'b01, // Copy the Next context into the Current Context (NewNext -> Current)
			COPY_CONTEXT_PREVIOUS = 2'b10; // Copy the New Context into the Current Next Context (Finished an Apply and going to perform a Reduce)

reg [1:0] copyContextType;
reg [1:0] copyContextStoredType;

reg [3:0] copyContextCounter;
reg copyContextRunning;
wire copyContextEnd = (!copyContextRunning | copyContextCounter == 7);

// Store new context into current context index storage
reg storeContextStart; // Control

reg [3:0] storeContextCounter;
reg storeContextRunning;
wire storeContextEnd = (!storeContextRunning | copyContextCounter == 7);

// Loads Context
reg loadContextStart; // Control

reg [3:0] loadContextCounter;
reg loadContextRunning;
wire loadContextEnd = (!loadContextRunning | loadContextCounter == 7);

reg incrementContextIndex,decrementContextIndex,switchCurrentAction;

reg [3:0] requestReadStoreIndex;
reg [31:0] contextStorageIndex;

reg requestReadDoLoad;

always @(posedge clk)
begin
	if(apply_out_validData)
	begin
		requestsApplyProcessed <= requestsApplyProcessed + 1;

		newContext[apply_out_index] <= requestToSaveApply;
		newApply[apply_out_index] <= apply_out_valid_apply;
		newReduce[apply_out_index] <= apply_out_valid_reduce;

		if(!apply_out_simpleCase & !apply_out_existSimpleCase & !apply_out_two_equals)
		begin
			if(apply_out_equal & (apply_out_requestsCreated + 1 <= 8))
			begin
				applyOneRequestApplied <= 1;
				apply_out_requestsCreated <= apply_out_requestsCreated + 1;
				if(apply_out_equalThenCase)
				begin // If then equal, need to save else
					newNextContext[apply_out_requestsCreated] <= leftRequestToCreate;
					newNextApply[apply_out_requestsCreated] <= !applyStoreLeft;
				end
				else
				begin
					newNextContext[apply_out_requestsCreated] <= rightRequestToCreate;
					newNextApply[apply_out_requestsCreated] <= 1'b1;
				end
			end
			else if(apply_out_requestsCreated + 2 <= 8)
			begin
				applyOneRequestApplied <= 1;
				apply_out_requestsCreated <= apply_out_requestsCreated + 2;
				newNextContext[apply_out_requestsCreated] <= rightRequestToCreate;
				newNextContext[apply_out_requestsCreated+1] <= leftRequestToCreate;
				newNextApply[apply_out_requestsCreated] <= 1'b1;
				newNextApply[apply_out_requestsCreated+1] <= !applyStoreLeft;
			end
		end
	end

	if(reduce_2_validOut)
	begin
		requestsReduceProcessed <= requestsReduceProcessed + 1;

		if(reduce_2_validNextApply)
		begin
			applyNextContext <= 1'b1;
		end

		newContext[reduce_2_request_id] <= reduce_2_newRequest;
		newApply[reduce_2_request_id] <= reduce_2_validApply;
		newReduce[reduce_2_request_id] <= reduce_2_validReduce;

		newNextContext[reduce_2_left_index] <= reduce_2_newLeftRequest;
		newNextApply[reduce_2_left_index] <= reduce_2_validNextApply;
		newNextContext[reduce_2_right_index] <= reduce_2_rightRequest;
	end

	SendContextApplyFinished <= 1'b0;
	
	if(StartSendingDataToApply)
	begin
		sendingData <= 1;
		SendContextApplyCounter <= 0;
		ValidApplyRequestsSent <= 0;
	end

	if(sendingData)
	begin
		if(currentApply[SendContextApplyCounter] & !applyBusy)
		begin
			SendContextApplyCounter <= SendContextApplyCounter + 1;
			ValidApplyRequestsSent <= ValidApplyRequestsSent + 1;
			if(SendContextApplyCounter == 3'h7)
			begin
				sendingData <= 0;
				SendContextApplyCounter <= 0;
				SendContextApplyFinished <= 1'b1;
			end
		end
		if(!currentApply[SendContextApplyCounter])
		begin
			newContext[SendContextApplyCounter] <= currentContext[SendContextApplyCounter];
			newReduce[SendContextApplyCounter] <= currentReduce[SendContextApplyCounter];

			SendContextApplyCounter <= SendContextApplyCounter + 1;
			if(SendContextApplyCounter == 3'h7)
			begin
				sendingData <= 0;
				SendContextApplyCounter <= 0;
				SendContextApplyFinished <= 1'b1;
			end
		end
	end

	if(copyContextStart)
	begin
		copyContextRunning <= 1;
		copyContextCounter <= 0;
		copyContextStoredType <= copyContextType;
	end

	if(copyContextRunning)
	begin
		if(copyContextStoredType == COPY_CONTEXT_SAME) // Same copy
		begin
			currentContext[copyContextCounter] <= newContext[copyContextCounter];
			currentNextContext[copyContextCounter] <= newNextContext[copyContextCounter];
			currentApply <= newApply;
			currentReduce <= newReduce;
		end
		else if(copyContextStoredType == COPY_CONTEXT_NEXT)// New Next becomes Current
		begin
			currentContext[copyContextCounter] <= newNextContext[copyContextCounter];
			currentApply <= newNextApply;
		end
		else // COPY_CONTEXT_PREVIOUS - New Context becomes Next. 
		begin
			currentNextContext[copyContextCounter] <= newContext[copyContextCounter];
		end

		copyContextCounter <= copyContextCounter + 1;
		if(copyContextEnd)
		begin
			copyContextRunning <= 1'b0;
			copyContextCounter <= 0;
			copyContextStoredType <= 0;
		end
	end

	if(loadContextStart)
	begin
		loadContextRunning <= 1;
		loadContextCounter <= 0;
	end

	requestReadStoreIndex <= 0;
	requestReadDoLoad <= 1'b0;
	contextStorageIndex <= 0;

	if(requestReadDoLoad)
	begin
		currentContext[requestReadStoreIndex] <= contextStorage[contextStorageIndex];
	end

	if(loadContextRunning)
	begin
		contextStorageIndex <= contextIndex * `CONTEXT_SIZE + loadContextCounter;
		requestReadStoreIndex <= loadContextCounter;
		requestReadDoLoad <= 1'b1;

		//currentContext[loadContextCounter] <= contextStorage[contextIndex][loadContextCounter];
		//currentNextContext[loadContextCounter] <= contextStorage[contextIndexPlusOne][loadContextCounter];

		currentApply <= contextApply[contextIndex];
		currentReduce <= contextReduce[contextIndex];

		loadContextCounter <= loadContextCounter + 1;
		if(loadContextEnd)
		begin
			loadContextRunning <= 1'b0;
			loadContextCounter <= 0;
		end
	end

	if(storeContextStart)
	begin
		storeContextRunning <= 1;
		storeContextCounter <= 0;
	end

	if(storeContextRunning)
	begin
		contextStorage[contextIndex * `CONTEXT_SIZE + storeContextCounter] <= newContext[storeContextCounter];
		contextApply[contextIndex] <= newApply;
		contextReduce[contextIndex] <= newReduce;

		storeContextCounter <= storeContextCounter + 1;
		if(storeContextEnd)
		begin
			storeContextRunning <= 1'b0;
			storeContextCounter <= 0;
		end
	end

	if(incrementContextIndex)
	begin
		contextIndex <= contextIndexPlusOne;
		contextIndexPlusOne <= contextIndexPlusOne + 1;
		currentReduce <= 0; // Going to a higher context implies the next has zero valid reduces
	end

	if(decrementContextIndex)
	begin
		contextIndex <= contextIndex - 1;
		contextIndexPlusOne <= contextIndex;
	end

	if(switchCurrentAction)
	begin
		currentAction <= !currentAction;
	end

	controllerState <= controllerNextState;

	if(avs_config_write)
	begin
		if(avs_config_address == 16'h0000)
		begin
			`REQUEST_F(currentContext[0]) <= avs_config_writedata; 
		end
		if(avs_config_address == 16'h0001)
		begin
			`REQUEST_G(currentContext[0]) <= avs_config_writedata; 
		end
		if(avs_config_address == 16'h0002)
		begin
			`REQUEST_TYPE(currentContext[0]) <= avs_config_writedata;
			currentApply[0] <= 1'b1;
			finished <= 0;
		end
	end

	SendContextReduceFinished <= 1'b0;
	
	if(StartSendingDataToReduce)
	begin
		sendingReduceData <= 1;
		SendContextReduceCounter <= 0;
		ValidReduceRequestsSent <= 0;
	end

	if(sendingReduceData)
	begin
		//newNextContext[SendContextReduceCounter] <= currentNextContext[SendContextReduceCounter];
		if(currentReduce[SendContextReduceCounter] & !reduceBusy)
		begin
			SendContextReduceCounter <= SendContextReduceCounter + 1;
			ValidReduceRequestsSent <= ValidReduceRequestsSent + 1;
			if(SendContextReduceCounter == 3'h7)
			begin
				sendingReduceData <= 0;
				SendContextReduceCounter <= 0;
				SendContextReduceFinished <= 1'b1;
			end
		end
		if(!currentReduce[SendContextReduceCounter])
		begin
			newContext[SendContextReduceCounter] <= currentContext[SendContextReduceCounter];
			newApply[SendContextReduceCounter] <= currentApply[SendContextReduceCounter];

			SendContextReduceCounter <= SendContextReduceCounter + 1;
			if(SendContextReduceCounter == 3'h7)
			begin
				sendingReduceData <= 0;
				SendContextReduceCounter <= 0;
				SendContextReduceFinished <= 1'b1;
			end
		end
	end

	if(finish)
	begin
		finalResult <= newContext[0][29:0];
		finished <= 1;
	end

	if(resetPipeline)
	begin
		for(i = 0; i < 8; i = i + 1)
		begin
			newContext[i] <= 0;
			newNextContext[i] <= 0;
		end
		requestsApplyProcessed <= 0;
		requestsReduceProcessed <= 0;
		newNextApply <= 0;
		newApply <= 0;
		newReduce <= 0;
		apply_out_requestsCreated <= 0;
		applyOneRequestApplied <= 0;
		applyNextContext <= 0;
	end

	if(reset | finish)
	begin
		loadContextRunning <= 0;
		loadContextCounter <= 0;
		storeContextRunning <= 0;
		storeContextCounter <= 0;
		copyContextRunning <= 0;
		copyContextCounter <= 0;
		copyContextStoredType <= 0;

		for(i = 0; i < 8; i = i + 1)
		begin
			currentContext[i] <= 0;
			currentNextContext[i] <= 0;
		end	
		currentApply <= 0;
		currentReduce <= 0;
		contextIndex <= 0;
		contextIndexPlusOne <= 1;
		currentAction <= ACTION_APPLY;
		sendingReduceData <= 0;
		SendContextReduceCounter <= 0;
		ValidReduceRequestsSent <= 0;
		finished <= 1; // Starts at one since the controller starts finished
		sendingData <= 0;
		SendContextApplyCounter <= 0;
		ValidApplyRequestsSent <= 0;
		for(i = 0; i < 8; i = i + 1)
		begin
			newContext[i] <= 0;
			newNextContext[i] <= 0;
		end
		requestsApplyProcessed <= 0;
		requestsReduceProcessed <= 0;
		newNextApply <= 0;
		newApply <= 0;
		newReduce <= 0;
		apply_out_requestsCreated <= 0;
		applyOneRequestApplied <= 0;
		applyNextContext <= 0;
	end
end

// Controller logic

always @*
begin
	controllerNextState = 32'h0; // Just wait
	StartSendingDataToApply = 1'b0;
	incrementContextIndex = 1'b0;
	decrementContextIndex = 1'b0;
	switchCurrentAction = 1'b0;
	StartSendingDataToReduce = 1'b0;
	resetPipeline = 1'b0;
	copyContextStart = 1'b0;
	copyContextType = 1'b0;
	storeContextStart = 1'b0;
	loadContextStart = 1'b0;
	finish = 1'b0;

	if(controllerState != 32'h0)
	begin
		controllerNextState = controllerState; // By default wait in the same state	
	end

	case(controllerState)
		32'h0: begin
			controllerNextState = 32'h0;
			if(avs_config_write & avs_config_address == 16'h0002)
			begin
				controllerNextState = 32'h1;
			end
		end
		32'h1: begin // Start data transfer from currentContext to Apply
			if(currentAction == ACTION_APPLY)
			begin
				StartSendingDataToApply = 1'b1;
				controllerNextState = 32'h2;
				resetPipeline = 1'b1;
			end
			else 
			begin
				StartSendingDataToReduce = 1'b1;
				controllerNextState = 32'h20;
				resetPipeline = 1'b1;
			end
		end
		32'h2: begin // End for wait of Apply transfer
			if(SendContextApplyFinished)
			begin
				controllerNextState = 32'h4;
			end
		end
		32'h4: begin
			if(requestsApplyProcessed == ValidApplyRequestsSent) // Wait for the Apply requests to be processed
			begin
				if(applyOneRequestApplied)
				begin
					storeContextStart = 1'b1;
					copyContextStart = 1'b1;
					copyContextType = COPY_CONTEXT_NEXT;
					controllerNextState = 32'h8;
				end
				else if(contextIndex == 0) // We have a simple case in Context 0. Finish 
				begin
					controllerNextState = 32'h0;
					finish = 1'b1;
				end // Just processed an Apply, meaning that the 
				else
				begin
					decrementContextIndex = 1'b1; // Decrement before loading
					switchCurrentAction = 1'b1; // Set it to reduce
					controllerNextState = 32'h10;
					loadContextStart = 1'b1;
					copyContextStart = 1'b1;
					copyContextType = COPY_CONTEXT_PREVIOUS;
				end
			end
		end
		32'h8: begin // Wait for end of copy context and storing it
			if(copyContextEnd & storeContextEnd)
			begin
				incrementContextIndex = 1'b1;
				//controllerNextState = 32'h1;
				controllerNextState = 32'h100;
			end
		end
		32'h10: begin // Wait for end of load
			if(loadContextEnd & storeContextEnd)
			begin
				//controllerNextState = 32'h1;
				controllerNextState = 32'h100;
			end
		end
		32'h20: begin // Wait for end of Reduce transfer
			if(SendContextReduceFinished)
			begin
				controllerNextState = 32'h40;
			end
		end
		32'h40: begin // Wait for requests to finish Reducing
			if(requestsReduceProcessed == ValidReduceRequestsSent)
			begin
				if(applyNextContext) // Change to apply, increment index
				begin
					switchCurrentAction = 1'b1; // Set it to apply
					storeContextStart = 1'b1;
					copyContextStart = 1'b1;
					copyContextType = COPY_CONTEXT_NEXT;
					controllerNextState = 32'h8;
				end
				else if(newApply != 0) // Change to apply, maintain index
				begin
					switchCurrentAction = 1'b1; // Set it to apply
					copyContextStart = 1'b1;
					copyContextType = COPY_CONTEXT_SAME;
					controllerNextState = 32'h80;
				end
				else if(contextIndex == 0) // We just reduced the last context index, finish
				begin 
					controllerNextState = 32'h0;
					finish = 1'b1;
				end 
				else // Change to reduce on the lower index
				begin
					decrementContextIndex = 1'b1; // Decrement before loading
					controllerNextState = 32'h10;
					loadContextStart = 1'b1;
					copyContextStart = 1'b1;
					copyContextType = COPY_CONTEXT_PREVIOUS;
				end
			end
		end
		32'h80: begin // Wait for end of copy context and storing it but does not increment
			if(copyContextEnd & storeContextEnd)
			begin
				//controllerNextState = 32'h1;
				controllerNextState = 32'h100; 
			end
		end
		32'h100: begin // Because of the testing code, we must delay the transition for one cycle, so that the code samples the current request in the moment it has all the requests loaded
			controllerNextState = 32'h1;
		end
	endcase
end

// HPS Get the final result

reg hpsGetFinalResult;

always @(posedge clk)
begin
	hpsGetFinalResult <= 1'b0;

	if(avs_config_read && avs_config_address == 16'h0006)
	begin
		hpsGetFinalResult <= 1'b1;
	end
end

wire [31:0] outputResult = (finished ? finalResult : 32'hffffffff);

assign avs_config_readdata = hpsGetFinalResult ? outputResult : 32'h00000000;
assign avs_config_readdatavalid = hpsGetFinalResult;

reg [31:0] counter;

always @(posedge clk)
begin
	if(controllerState == 32'h1)
	begin
		counter <= counter + 1;
	end

	if(reset)
	begin
		counter <= 0;
	end
end

endmodule