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

module CoDepth (
		// Config
		input wire [15:0] avs_config_address,
		output wire avs_config_waitrequest,
		input wire avs_config_write,
		input wire [31:0] avs_config_writedata,
		input wire avs_config_read,
		output wor [31:0] avs_config_readdata,
		output wor avs_config_readdatavalid,

		// CoBdd
		input wire avs_co_write,
		input wire [127:0] avs_co_writedata,
		input wire avs_co_read,
		output wire [127:0] avs_co_readdata,
		output wire avs_co_waitrequest,
		input wire [15:0] avs_co_byteenable,

		// Send request to MA
		output wor [127:0] aso_requests_data,
		input wor aso_requests_ready,
		output wor aso_requests_valid,
		output wor [1:0] aso_requests_channel,

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

assign avs_config_waitrequest = reset;

assign asi_result_ready = 1'b1; // Is always ready to receive data
assign asi_var_ready = 1'b1;

reg finish; // Set to perform the last step (pop from result)

reg `REQUEST_DEF requestLifo[`VAR_COUNT*3-1:0] /* synthesis ramstyle = "no_rw_check,M10K" */;
reg `INDEX_DEF resultLifo[`VAR_COUNT*2-1:0] /* synthesis ramstyle = "no_rw_check,M10K" */;

// Result index
reg [`VAR_SIZE:0] resultLifoIndex; // Points to a space after the top
reg [`VAR_SIZE:0] resultLifoTop; // Points to the top index
reg [`VAR_SIZE:0] resultLifoTopMinusOne; // Points to the index after the top

// Request index
reg [`VAR_SIZE+1:0] requestLifoIndex; // Points to a space after the top
reg [`VAR_SIZE+1:0] requestLifoTop; // Points to the top index
reg [`VAR_SIZE+1:0] requestLifoTopMinusOne; // Points to the index after the top

// Wires to insert result
wor insertResult;
wor `INDEX_DEF resultToInsert;
wor popTwoResults;
wor popOneResult;

assign insertResult = 0;
assign resultToInsert = 0;
assign popTwoResults = 0;
assign popOneResult = 0;

always @(posedge clk)
begin
	if(popOneResult)
	begin
		resultLifoIndex <= resultLifoIndex - 13'd1; // Don't know if its better (or should use a minus 2)
		resultLifoTop <= resultLifoTop - 13'd1;
		resultLifoTopMinusOne <= resultLifoTopMinusOne - 13'd1;
	end

	if(popTwoResults)
	begin
		resultLifoIndex <= resultLifoTopMinusOne; // Don't know if its better (or should use a minus 2)
		resultLifoTop <= resultLifoTop - 13'd2;
		resultLifoTopMinusOne <= resultLifoTopMinusOne - 13'd2;
	end

	if(insertResult)
	begin
		resultLifo[resultLifoIndex] <= resultToInsert;
		resultLifoIndex <= resultLifoIndex + 13'd1;
		resultLifoTop <= resultLifoIndex;
		resultLifoTopMinusOne <= resultLifoTop;
	end

	if(reset | finish)
	begin
		resultLifoIndex <= 0;
		resultLifoTop <= - 13'd1;
		resultLifoTopMinusOne <= - 13'd2;
	end
end

// Read the top two values from result Lifo
reg `INDEX_DEF topResult;
reg `INDEX_DEF topMinusOneResult;

always @(posedge clk)
begin
	topResult <= resultLifo[resultLifoTop];
	topMinusOneResult <= resultLifo[resultLifoTopMinusOne];

	if(insertResult)
	begin
		topResult <= resultToInsert; // Remove one cycle from having to wait for top result
	end

	if(reset | finish)
	begin
		topResult <= 0;
		topMinusOneResult <= 0;
	end
end

reg `REQUEST_DEF topRequest;

always @(posedge clk)
begin
	topRequest <= requestLifo[requestLifoTop];

	if(reset | finish)
	begin
		topRequest <= 0;
	end
end

reg `REQUEST_DEF currentRequest;
reg validCurrentRequest;

wor insertRequest;
wor `REQUEST_DEF requestToInsert;

wor setCurrentRequest;
wor `REQUEST_DEF requestToSetCurrent;

wor popNextRequest;

assign insertRequest = 0;
assign requestToInsert = 0;
assign setCurrentRequest = 0;
assign requestToSetCurrent = 0;
assign popNextRequest = 1'b0;

wire disableValidCurrentRequest;

// Pop next request from request lifo
always @(posedge clk)
begin
	validCurrentRequest <= 1'b0;
	finish <= 0;

	if(disableValidCurrentRequest)
	begin
		validCurrentRequest <= 1'b0;
	end

	if(setCurrentRequest)
	begin
		validCurrentRequest <= 1'b1;
		currentRequest <= requestToSetCurrent;
	end

	if(avs_config_write)
	begin
		if(avs_config_address == 16'h0000)
		begin
			`REQUEST_F(currentRequest) <= avs_config_writedata; 
		end
		if(avs_config_address == 16'h0001)
		begin
			`REQUEST_G(currentRequest) <= avs_config_writedata; 
		end
		if(avs_config_address == 16'h0002)
		begin
			`REQUEST_TYPE(currentRequest) <= avs_config_writedata;
			validCurrentRequest <= 1'b1;
		end
	end

	if(popNextRequest)
	begin
		if(requestLifoIndex != 0)
		begin	
			validCurrentRequest <= 1'b1;
			currentRequest <= topRequest;
			requestLifoIndex <= requestLifoTop;
			requestLifoTop <= requestLifoTopMinusOne;
			requestLifoTopMinusOne <= requestLifoTopMinusOne - 1;
		end
		else begin
			finish <= 1;
		end
	end

	if(insertRequest)
	begin
		requestLifo[requestLifoIndex] <= requestToInsert;
		requestLifoIndex <= requestLifoIndex + 1;
		requestLifoTop <= requestLifoIndex;
		requestLifoTopMinusOne <= requestLifoTop;
	end

	if(reset | finish)
	begin
		currentRequest <= 0;
		requestLifoIndex <= 0;
		requestLifoTop <= -1;
		requestLifoTopMinusOne <= -2;
	end
end

reg finished; // Set to indicate the computation is finished
reg `INDEX_DEF finalResult;

assign popOneResult = finish;

always @(posedge clk)
begin
	if(validCurrentRequest)
	begin
		finished <= 0;
	end

	if(finish)
	begin
		if(insertResult)
		begin
			finalResult <= resultToInsert;
		end
		else begin
			finalResult <= topResult;			
		end

		finished <= 1;
	end

	if(reset)
	begin
		finalResult <= 0;
		finished <= 1; // Starts at one since the controller starts finished
	end
end

reg performFindOrInsert;
reg performExistReturn0;
reg performExistReturn1;
reg performAbstractReturn0;
reg performAbstractReturn1;
reg performApply;
reg performCacheStore; 

wire [`STATE_SIZE-1:0] currentState;
wire [`TYPE_SIZE-1:0] currentType;

assign currentType = `REQUEST_TYPE(currentRequest);
assign currentState = `REQUEST_STATE(currentRequest);

assign disableValidCurrentRequest = validCurrentRequest;

always @(posedge clk)
begin
	performFindOrInsert <= 0;
	performExistReturn0 <= 0;
	performExistReturn1 <= 0;
	performAbstractReturn0 <= 0;
	performAbstractReturn1 <= 0;
	performApply <= 0;
	performCacheStore <= 0; 

	if(validCurrentRequest)
	begin
		case(currentType)
		`BDD_AND:begin
			case(currentState)
			`BDD_APPLY:begin
				performApply <= !`REQUEST_CACHE(currentRequest);
			end
			`BDD_REDUCE:begin
				performFindOrInsert <= 1;
			end
			endcase
		end
		`BDD_EXIST:begin
			case(currentState)
			`BDD_APPLY:begin
				performApply <= !`REQUEST_CACHE(currentRequest);
			end
			`BDD_RETURN_0:begin
				performExistReturn0 <= 1;
			end
			`BDD_RETURN_1:begin
				performExistReturn1 <= 1;
			end
			`BDD_REDUCE:begin
				performFindOrInsert <= 1;
			end
			endcase
		end
		`BDD_ABSTRACT:begin
			case(currentState)
			`BDD_APPLY:begin
				performApply <= !`REQUEST_CACHE(currentRequest);
			end
			`BDD_RETURN_0:begin
				performAbstractReturn0 <= 1;
			end
			`BDD_RETURN_1:begin
				performAbstractReturn1 <= 1;
			end
			`BDD_REDUCE:begin
				performFindOrInsert <= 1;
			end
			endcase
		end
		`BDD_PERMUTE:begin
			case(currentState)
			`BDD_APPLY:begin
				performApply <= !`REQUEST_CACHE(currentRequest);
			end
			`BDD_REDUCE:begin
				performFindOrInsert <= 1;
			end
			endcase
		end
		endcase

		if(`REQUEST_CACHE(currentRequest) & `REQUEST_STATE(currentRequest) == `BDD_STORE_INTO_CACHE) // Cache bit
		begin
			performCacheStore <= 1; 
		end
	end
end

// Perform Apply
wire `INDEX_DEF f,g;

assign f = `REQUEST_F(currentRequest);
assign g = `REQUEST_G(currentRequest);

wire [127:0] aso_var_request_data,aso_cache_request_data;
wire aso_var_request_valid,aso_cache_request_valid;

assign aso_requests_channel = aso_var_request_valid ? 2'b00 : 0;
assign aso_requests_data = aso_var_request_valid ? aso_var_request_data : 0;
assign aso_requests_valid = aso_var_request_valid;

assign aso_requests_channel = aso_cache_request_valid ? 2'b10 : 0;
assign aso_requests_data = aso_cache_request_valid ? aso_cache_request_data : 0;
assign aso_requests_valid = aso_cache_request_valid;

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

// For now keep it at zero
wor insertValueIntoSimpleCache;
wor [`CACHE_BIT_SIZE-1:0] simpleCacheIndex;
wor [`SIMPLE_CACHE_ENTRY_BIT_SIZE-1:0] simpleCacheValue;

wire [31:0] apply_avs_config_readdata;
wire apply_avs_config_readdatavalid;

assign avs_config_readdata = apply_avs_config_readdatavalid ? apply_avs_config_readdata : 0;
assign avs_config_readdatavalid = apply_avs_config_readdatavalid;

wire `INDEX_DEF coBdd_f;
wire `INDEX_DEF coBdd_g;
wire coBdd_fetchF;
wire coBdd_fetchG;
wire coBdd_validOut;
wire `INDEX_DEF coBdd_fnv;
wire `INDEX_DEF coBdd_fv;
wire `INDEX_DEF coBdd_gnv;
wire `INDEX_DEF coBdd_gv;
wire coBdd_validIn;
wire coBdd_busy;

ApplyNoNodes apply (
		.f(f),
		.g(g),
		.typeIn(currentType),
		.valid(performApply),
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
		.outIndex(),
		.negate(0),
		.index(0),
		.applyBusy(),
		.negateOut(),
		.applyBusyIn(0),
		.asi_result_ready(),
		.asi_var_ready(),
		.coBdd_f(coBdd_f),
		.coBdd_g(coBdd_g),
		.coBdd_fetchF(coBdd_fetchF),
		.coBdd_fetchG(coBdd_fetchG),
		.coBdd_validOut(coBdd_validOut),
		.coBdd_fnv(coBdd_fnv),
		.coBdd_fv(coBdd_fv),
		.coBdd_gnv(coBdd_gnv),
		.coBdd_gv(coBdd_gv),
		.coBdd_busy(coBdd_busy),
		.coBdd_validIn(coBdd_validIn),

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
		.aso_var_request_data(aso_var_request_data),
		.aso_var_request_ready(aso_requests_ready),
		.aso_var_request_valid(aso_var_request_valid),
		.aso_cache_request_data(aso_cache_request_data),
		.aso_cache_request_ready(aso_requests_ready),
		.aso_cache_request_valid(aso_cache_request_valid),
		.asi_result_data(asi_result_data),
		.asi_result_valid(asi_result_valid),
		.asi_result_channel(asi_result_channel),
		.asi_var_data(asi_var_data),
		.asi_var_valid(asi_var_valid),
		.clk(clk),
		.reset(reset | finish)
	);

reg apply_simpleCaseHit;
reg `INDEX_DEF apply_simpleCaseResult;
reg `INDEX_DEF apply_fn;
reg `INDEX_DEF apply_fnv;
reg `INDEX_DEF apply_gn;
reg `INDEX_DEF apply_gnv;
reg `INDEX_DEF apply_outF;
reg `INDEX_DEF apply_outG;
reg apply_quantifyOut;
reg `TYPE_DEF apply_outType;
reg `VAR_DEF apply_topOut;

reg [1:0] apply_state,apply_next_state;

reg apply_reset_valid_out;

reg apply_exist_simple;

always @(posedge clk)
begin
	if(validOut)
	begin
		apply_simpleCaseHit <= simpleCaseHit;
		apply_simpleCaseResult <= simpleCaseResult;
		apply_fn <= fn;
		apply_fnv <= fnv;
		apply_gn <= gn;
		apply_gnv <= gnv;
		apply_outF <= outF;
		apply_outG <= outG;
		apply_topOut <= topOut;
		apply_quantifyOut <= quantifyOut;
		apply_outType <= outType;
		apply_state <= 2'b01;
		apply_exist_simple <= (outType == `BDD_EXIST & quantifyOut & ((fn == `BDD_ONE) | (fnv == `BDD_ONE) | (`EQUAL_NEGATE(fn,fnv))));
	end

	if(^apply_state)
	begin
		apply_state <= apply_next_state;
	end

	if(reset | finish)
	begin
		apply_simpleCaseHit <= 0;
		apply_simpleCaseResult <= 0;
		apply_fn <= 0;
		apply_fnv <= 0;
		apply_gn <= 0;
		apply_gnv <= 0;
		apply_outF <= 0;
		apply_outG <= 0;
		apply_topOut <= 0;
		apply_quantifyOut <= 0;
		apply_outType <= 0;
		apply_state <= 0;
		apply_exist_simple <= 0;
	end
end

reg apply_setCurrentRequest,apply_popNextRequest,apply_insertRequest,apply_insertResult;
reg `REQUEST_DEF apply_requestToSetCurrent,apply_requestToInsert;
reg `INDEX_DEF apply_resultToInsert;

assign setCurrentRequest = apply_setCurrentRequest;
assign popNextRequest = apply_popNextRequest;
assign insertRequest = apply_insertRequest;
assign requestToSetCurrent = apply_requestToSetCurrent;
assign requestToInsert = apply_requestToInsert;
assign resultToInsert = apply_resultToInsert;
assign insertResult = apply_insertResult;

always @*
begin
	apply_reset_valid_out = 1'b0;
	apply_setCurrentRequest = 1'b0;
	apply_popNextRequest = 1'b0;
	apply_insertRequest = 1'b0;
	apply_requestToSetCurrent = 0;
	apply_requestToInsert = 0;
	apply_resultToInsert = 0;
	apply_insertResult = 0;
	apply_next_state = 2'b00;

	case(apply_state)
	2'b01:begin 
		if(apply_exist_simple | apply_simpleCaseHit) // Compute the last remaining simple case (Exist), insert simple result and get next request if any
		begin
			apply_popNextRequest = 1'b1;
			apply_resultToInsert = (apply_simpleCaseHit ? apply_simpleCaseResult ^ `REQUEST_NEGATE(currentRequest) : `BDD_ONE);
			apply_insertResult = 1'b1;
			apply_next_state = 2'b00;
		end
		else 
		begin // Insert Store Result into Cache request
			// By default assume it is a normal FindOrInsert with cache
			apply_insertRequest = 1'b1;
			`REQUEST_CACHE(apply_requestToInsert) = 1'b1;
			`REQUEST_TYPE(apply_requestToInsert) = apply_outType;
			`REQUEST_STATE(apply_requestToInsert) = `BDD_REDUCE;
			`REQUEST_F(apply_requestToInsert) = apply_outF;
			`REQUEST_G(apply_requestToInsert) = apply_outG;
			`REQUEST_NEGATE(apply_requestToInsert) = `REQUEST_NEGATE(currentRequest);
			`REQUEST_TOP(apply_requestToInsert) = apply_topOut;
			`REQUEST_QUANTIFY(apply_requestToInsert) = apply_quantifyOut;

			if(apply_outType == `BDD_PERMUTE)
			begin
				`REQUEST_F(apply_requestToInsert) = apply_outF;
				`REQUEST_NEGATE(apply_requestToInsert) = 0;
			end

			if(apply_quantifyOut)
			begin
				`REQUEST_STATE(apply_requestToInsert) = `BDD_STORE_INTO_CACHE;
				`REQUEST_TOP(apply_requestToInsert) = 0;
			end

			// T request
			apply_setCurrentRequest = 1'b1;
			`REQUEST_F(apply_requestToSetCurrent) = apply_fn;
			`REQUEST_G(apply_requestToSetCurrent) = apply_gn;
			`REQUEST_TYPE(apply_requestToSetCurrent) = apply_outType;
			`REQUEST_STATE(apply_requestToSetCurrent) = `BDD_APPLY;

			apply_next_state = 2'b10;
		end
	end
	2'b10:begin 
		// Calculate E
		apply_insertRequest = 1'b1;
		`REQUEST_F(apply_requestToInsert) = apply_fnv;
		`REQUEST_G(apply_requestToInsert) = apply_gnv;
		`REQUEST_TYPE(apply_requestToInsert) = apply_outType;
		`REQUEST_STATE(apply_requestToInsert) = `BDD_APPLY;

		if(apply_quantifyOut)
		begin
			`REQUEST_STATE(apply_requestToInsert) = `BDD_RETURN_0;
			`REQUEST_TOP(apply_requestToInsert) = apply_topOut;
			if(apply_outType == `BDD_EXIST)
			begin
				`REQUEST_F(apply_requestToInsert) = `BDD_ZERO;
				`REQUEST_G(apply_requestToInsert) = apply_fnv;
			end
		end
	end
	endcase
end

// Perform Reduce
reg `INDEX_DEF e,t;
reg doFindOrInsert;

assign popTwoResults = performFindOrInsert;

always @(posedge clk)
begin
	if(performFindOrInsert)
	begin
		e <= topResult;
		t <= topMinusOneResult;
	end
	doFindOrInsert <= performFindOrInsert;

	if(reset | finish)
	begin
		e <= 0;
		t <= 0;
	end
end

wire `VAR_DEF top = `REQUEST_TOP(currentRequest);

wire negateFindOrInsertResult = t[0];

wire simpleReduce = (e == t);

wire `INDEX_DEF inE = {e[29:1],e[0] ^ negateFindOrInsertResult};
wire `INDEX_DEF inT = {t[29:1],1'b0};

wire `INDEX_DEF findOrInsertResult;
wire findOrInsertResultValid;

/*
FindOrInsert findOrInsert (
		.e(inE),
		.t(inT),
		.top(top),
		.valid(doFindOrInsert & !simpleReduce),
		.busy(),
		.result(findOrInsertResult),
		.validOut(findOrInsertResultValid),
		.avs_config_address(avs_config_address), 
		.avs_config_write(avs_config_write),
		.avs_config_writedata(avs_config_writedata),
		.avs_config_read(avs_config_read),
		.avs_config_readdata(findOrInsert_avs_config_readdata),
		.avs_config_readdatavalid(findOrInsert_avs_config_readdatavalid),
		.avs_config_waitrequest(),
		.aso_request_data(aso_findOrInsert_data),
		.aso_request_ready(aso_requests_ready),
		.aso_request_valid(aso_findOrInsert_valid),
		.asi_result_data(asi_result_data),
		.asi_result_valid(asi_result_valid),
		.asi_result_channel(asi_result_channel),
		.clk(clk),
		.reset(reset | finish)
	);
*/

CoBdd coBdd(
		.busy(coBdd_busy),

		.avs_write(avs_co_write),
		.avs_writedata(avs_co_writedata),
		.avs_read(avs_co_read),
		.avs_readdata(avs_co_readdata),
		.avs_byteenable(avs_co_byteenable),
		.avs_waitrequest(avs_co_waitrequest),

		// Apply input 
		.apply_f(coBdd_f),
		.apply_g(coBdd_g),
		.apply_fetchF(coBdd_fetchF),
		.apply_fetchG(coBdd_fetchG),
		
		// Apply output
		.fnv(coBdd_fnv),
		.fv(coBdd_fv),
		.gnv(coBdd_gnv),
		.gv(coBdd_gv),
		.apply_validOut(coBdd_validIn),
		.apply_validIn(coBdd_validOut),

		// FindOrInsert input
		.e(inE),
		.t(inT),
		.top(top),
		.findOrInsert_valid(doFindOrInsert & !simpleReduce),

		// Request id for the Partial implementation
		.findOrInsert_result(findOrInsertResult),
		.validFindOrInsertOut(findOrInsertResultValid),

		// Finish
		.finished(finish),
		.finalResult(finalResult),

		.reset(reset),
		.clk(clk)
	);


reg reduce_popNextRequest,reduce_insertResult;
reg `INDEX_DEF reduce_resultToInsert;

assign popNextRequest = reduce_popNextRequest;
assign resultToInsert = reduce_resultToInsert;
assign insertResult = reduce_insertResult;

reg reduce_performReduceCacheStore;
reg `INDEX_DEF reduce_resultToCache;

always @(posedge clk)
begin
	reduce_performReduceCacheStore <= 0;

	if((findOrInsertResultValid | (doFindOrInsert & simpleReduce)) & `REQUEST_CACHE(currentRequest))
	begin
		reduce_resultToCache <= (reduce_resultToInsert ^ `REQUEST_NEGATE(currentRequest));
		reduce_performReduceCacheStore <= 1'b1;
	end

	if(reset | finish)
	begin
		reduce_resultToCache <= 0;		
	end
end

always @*
begin
	reduce_popNextRequest = 1'b0;
	reduce_resultToInsert = 0;
	reduce_insertResult = 1'b0;

	if(doFindOrInsert & simpleReduce)
	begin
		reduce_resultToInsert = t ^ `REQUEST_NEGATE(currentRequest);

		if(!`REQUEST_CACHE(currentRequest))
		begin
			reduce_popNextRequest = 1'b1;
		end

		reduce_insertResult = 1'b1;
	end

	if(findOrInsertResultValid)
	begin
		reduce_resultToInsert = findOrInsertResult ^ negateFindOrInsertResult ^ `REQUEST_NEGATE(currentRequest);

		if(!`REQUEST_CACHE(currentRequest))
		begin
			reduce_popNextRequest = 1'b1;
		end

		reduce_insertResult = 1'b1;
	end
end

// Perform cache storage for the Reduce operation
wire requestQuantify = `REQUEST_QUANTIFY(currentRequest);

reg [`CACHE_BIT_SIZE-1:0] reduce_performCacheStoredHash;
reg [`SIMPLE_CACHE_ENTRY_BIT_SIZE-1:0] reduce_performCacheStoredSimpleHash; 
reg reduce_performCacheHoldValues;
reg reduce_performCacheHold;
reg reduce_performPopOneResult;

always @(posedge clk)
begin
	if(!reduce_performReduceCacheStore | !reduce_performCacheHoldValues)
	begin
		reduce_performCacheStoredHash <= `CACHE_HASH(f,g,currentType[1:0],requestQuantify);
		reduce_performCacheStoredSimpleHash <= `SIMPLE_HASH(f,g,currentType[1:0],requestQuantify);
	end

	reduce_performCacheHoldValues <= reduce_performCacheHold;
end

assign insertValueIntoSimpleCache = reduce_performReduceCacheStore;
assign simpleCacheIndex = reduce_performCacheStoredHash;
assign simpleCacheValue = reduce_performCacheStoredSimpleHash;
assign popOneResult = reduce_performPopOneResult;

reg [127:0] aso_return_cache_data;
reg aso_return_cache_valid;
reg [1:0] aso_return_cache_channel;
 
assign aso_requests_channel = aso_return_cache_channel;
assign aso_requests_data = aso_return_cache_data;
assign aso_requests_valid = aso_return_cache_valid;

localparam [1:0] CACHE_INSERT_ID = 2'b10;

reg `CACHE_ENTRY_DEF reduce_performCacheEntry;
reg reduce_performCache_popNextRequest;
assign popNextRequest = reduce_performCache_popNextRequest;

always @*
begin
	aso_return_cache_valid = 0;
	aso_return_cache_data = 0;
	aso_return_cache_channel = 0;
	reduce_performCacheHold = 0;
	reduce_performCacheEntry = 0;
	reduce_performPopOneResult = 0;
	reduce_performCache_popNextRequest = 0;

	`CACHE_ENTRY_$_BDDINDEX_RESULT(reduce_performCacheEntry) = reduce_resultToCache;
	`CACHE_ENTRY_$_BDDINDEX_F(reduce_performCacheEntry) = f;
	`CACHE_ENTRY_$_BDDINDEX_G(reduce_performCacheEntry) = g;
	`CACHE_ENTRY_$_QUANTIFY(reduce_performCacheEntry) = requestQuantify;
	`CACHE_ENTRY_$_TYPE(reduce_performCacheEntry) = currentType[1:0];

	if(reduce_performReduceCacheStore | reduce_performCacheHoldValues)
	begin
		aso_return_cache_valid = 1'b1;
		aso_return_cache_channel = CACHE_INSERT_ID;
		`MA_REQUEST_$_BDDINDEX_NO_NEGATE(aso_return_cache_data) = reduce_performCacheStoredHash;
		`MA_REQUEST_$_DATA(aso_return_cache_data) = reduce_performCacheEntry;
		`MA_REQUEST_$_TYPE_2(aso_return_cache_data) = `MA_TYPE_2_WRITE_CACHE;

		if(!aso_requests_ready)
		begin
			reduce_performCacheHold = 1;
		end
		else 
		begin
			reduce_performCache_popNextRequest = 1'b1;	
		end
	end
end

// End of reduce

// Perform cache storage
reg [`CACHE_BIT_SIZE-1:0] performCacheStoredHash;
reg [`SIMPLE_CACHE_ENTRY_BIT_SIZE-1:0] performCacheStoredSimpleHash; 
reg performCacheHoldValues;
reg performCacheHold;
reg performPopOneResult;

always @(posedge clk)
begin
	if(!performCacheStore | !performCacheHoldValues)
	begin
		performCacheStoredHash <= `CACHE_HASH(f,g,currentType[1:0],requestQuantify);
		performCacheStoredSimpleHash <= `SIMPLE_HASH(f,g,currentType[1:0],requestQuantify);
	end

	performCacheHoldValues <= performCacheHold;

	if(reset | finish)
	begin
		performCacheStoredHash <= 0;
		performCacheStoredSimpleHash <= 0;
	end
end

assign insertValueIntoSimpleCache = performCacheStore;
assign simpleCacheIndex = performCacheStoredHash;
assign simpleCacheValue = performCacheStoredSimpleHash;
assign popOneResult = performPopOneResult;

reg [127:0] aso_perform_data;
reg aso_perform_valid;
reg [1:0] aso_perform_channel;

assign aso_requests_data = aso_perform_data;
assign aso_requests_valid = aso_perform_valid;
assign aso_requests_channel = aso_perform_channel;

reg `CACHE_ENTRY_DEF performCacheEntry;
reg performCache_popNextRequest;
assign popNextRequest = performCache_popNextRequest;

always @*
begin
	aso_perform_valid = 0;
	aso_perform_data = 0;
	aso_perform_channel = 0;
	performCacheHold = 0;
	performCacheEntry = 0;
	performPopOneResult = 0;
	performCache_popNextRequest = 0;

	`CACHE_ENTRY_$_BDDINDEX_RESULT(performCacheEntry) = {topResult[29:1],topResult[0] ^ `REQUEST_NEGATE(currentRequest)};
	`CACHE_ENTRY_$_BDDINDEX_F(performCacheEntry) = f;
	`CACHE_ENTRY_$_BDDINDEX_G(performCacheEntry) = g;
	`CACHE_ENTRY_$_QUANTIFY(performCacheEntry) = requestQuantify;
	`CACHE_ENTRY_$_TYPE(performCacheEntry) = currentType[1:0];

	if(performCacheStore | performCacheHoldValues)
	begin
		aso_perform_valid = 1'b1;
		aso_perform_channel = CACHE_INSERT_ID;
		`MA_REQUEST_$_BDDINDEX_NO_NEGATE(aso_perform_data) = performCacheStoredHash;
		`MA_REQUEST_$_DATA(aso_perform_data) = performCacheEntry;
		`MA_REQUEST_$_TYPE_2(aso_perform_data) = `MA_TYPE_2_WRITE_CACHE;

		if(!aso_requests_ready)
		begin
			performCacheHold = 1;
		end
		else 
		begin
			performCache_popNextRequest = 1'b1;	
		end
	end
end

// Perform Return actions
reg performReturnType;
reg [3:0] returnType;

reg `INDEX_DEF returnSavedResult;

reg returnPopOneResult;
reg returnPopRequest;
reg returnInsertResult;
reg `INDEX_DEF returnResultToInsert;
reg return_setCurrentRequest,return_insertRequest;
reg `REQUEST_DEF return_requestToSetCurrent,return_requestToInsert; 
reg returnDelayedPopRequest,doDelayedPopRequest;
reg returnSavedResultEqualToBddOne;
reg returnEqualToSavedRequestF;
reg returnEqualToSavedRequestG;

assign popOneResult = returnPopOneResult;
assign popNextRequest = returnPopRequest;
assign popNextRequest = doDelayedPopRequest;
assign insertResult = returnInsertResult;
assign resultToInsert = returnResultToInsert;
assign insertRequest = return_insertRequest;
assign requestToInsert = return_requestToInsert;
assign requestToSetCurrent = return_requestToSetCurrent;
assign setCurrentRequest = return_setCurrentRequest;

always @(posedge clk)
begin
	performReturnType <= 0;
	doDelayedPopRequest <= 1'b0;
	returnSavedResultEqualToBddOne <= 0;
	returnEqualToSavedRequestF <= 0;
	returnEqualToSavedRequestG <= 0;

	if(performExistReturn0 | performExistReturn1 | performAbstractReturn0 | performAbstractReturn1)
	begin
		performReturnType <= 1'b1;
		returnType <= {performAbstractReturn1,performAbstractReturn0,performExistReturn1,performExistReturn0};
	end

	if(returnPopOneResult)
	begin
		returnSavedResult <= topResult;
		returnEqualToSavedRequestF <= (topResult == `REQUEST_F(currentRequest));
		returnEqualToSavedRequestG <= (topResult == `REQUEST_G(currentRequest));
		returnSavedResultEqualToBddOne <= (topResult == `BDD_ONE);
	end

	if(returnDelayedPopRequest)
	begin
		doDelayedPopRequest <= 1'b1;
	end

	if(reset | finish)
	begin
		returnType <= 0;
		returnSavedResult <= 0;
	end
end

reg `INDEX_DEF returnRequestF;
reg `INDEX_DEF returnRequestG;

always @*
begin
	if(currentType != `BDD_EXIST & currentType != `BDD_PERMUTE & `REQUEST_F(currentRequest) > `REQUEST_G(currentRequest))
	begin
		returnRequestF = `REQUEST_G(currentRequest);
		returnRequestG = `REQUEST_F(currentRequest);
	end
	else begin
		returnRequestF = `REQUEST_F(currentRequest);
		returnRequestG = `REQUEST_G(currentRequest);
	end
end

always @*
begin
	returnPopOneResult = 1'b0;
	returnPopRequest = 1'b0;
	returnInsertResult = 1'b0;
	returnResultToInsert = 0;
	return_insertRequest = 0;
	return_requestToInsert = 0;
	return_requestToSetCurrent = 0;
	return_setCurrentRequest = 0;
	returnDelayedPopRequest = 1'b0;

	if(performExistReturn0 | performExistReturn1 | performAbstractReturn0 | performAbstractReturn1)
	begin
		returnPopOneResult = 1'b1;
	end

	if(performReturnType)
	begin
		casez(returnType)
		4'b???1: begin // Exist 0
			if(returnSavedResultEqualToBddOne)
			begin
				returnInsertResult = 1'b1;
				returnResultToInsert = `BDD_ONE;
				returnPopRequest = 1'b1;
			end
			else 
			begin
				return_setCurrentRequest = 1'b1;
				`REQUEST_F(return_requestToSetCurrent) = returnRequestG;
				`REQUEST_G(return_requestToSetCurrent) = `BDD_ZERO;
				`REQUEST_TYPE(return_requestToSetCurrent) = `BDD_EXIST;
				`REQUEST_STATE(return_requestToSetCurrent) = `BDD_APPLY;
				
				return_insertRequest = 1'b1;
				`REQUEST_F(return_requestToInsert) = returnRequestF;
				`REQUEST_G(return_requestToInsert) = returnSavedResult;
				`REQUEST_TYPE(return_requestToInsert) = `BDD_EXIST;
				`REQUEST_STATE(return_requestToInsert) = `BDD_RETURN_1;
			end
		end
		4'b??1?: begin // Exist 1
			return_setCurrentRequest = 1'b1;
			`REQUEST_F(return_requestToSetCurrent) = returnRequestG ^ 1'b1;
			`REQUEST_G(return_requestToSetCurrent) = returnSavedResult ^ 1'b1;
			`REQUEST_TYPE(return_requestToSetCurrent) = `BDD_AND;
			`REQUEST_STATE(return_requestToSetCurrent) = `BDD_APPLY;
			`REQUEST_NEGATE(return_requestToSetCurrent) = 1'b1;
		end
		4'b?1??: begin // Abstract 0
			if(	returnSavedResultEqualToBddOne |
				returnEqualToSavedRequestF |
				returnEqualToSavedRequestG)
			begin
				returnInsertResult = 1'b1;
				returnResultToInsert = returnSavedResult;
				returnDelayedPopRequest = 1'b1;
			end
			else
			begin
				return_setCurrentRequest = 1'b1;
				`REQUEST_TYPE(return_requestToSetCurrent) = `BDD_ABSTRACT;
				`REQUEST_STATE(return_requestToSetCurrent) = `BDD_APPLY;
				`REQUEST_F(return_requestToSetCurrent) = returnRequestF;
				`REQUEST_G(return_requestToSetCurrent) = returnRequestG;

				if(`EQUAL_NEGATE(returnSavedResult,returnRequestF))
				begin
					`REQUEST_TYPE(return_requestToSetCurrent) = `BDD_EXIST;
					`REQUEST_F(return_requestToSetCurrent) = returnRequestG;
					`REQUEST_G(return_requestToSetCurrent) = `BDD_ZERO;
				end
				else if(`EQUAL_NEGATE(returnSavedResult,returnRequestG))
				begin
					`REQUEST_TYPE(return_requestToSetCurrent) = `BDD_EXIST;
					`REQUEST_F(return_requestToSetCurrent) = returnRequestF;
					`REQUEST_G(return_requestToSetCurrent) = `BDD_ZERO;
				end 

				return_insertRequest = 1'b1;
				`REQUEST_F(return_requestToInsert) = returnSavedResult;
				`REQUEST_G(return_requestToInsert) = `BDD_ZERO;
				`REQUEST_TYPE(return_requestToInsert) = `BDD_ABSTRACT;
				`REQUEST_STATE(return_requestToInsert) = `BDD_RETURN_1;
			end
		end
		4'b1???: begin // Abstract 1
			if(returnEqualToSavedRequestF)
			begin
				returnInsertResult = 1'b1;
				returnResultToInsert = returnRequestF;
				returnPopRequest = 1'b1;
			end
			else 
			begin
				return_setCurrentRequest = 1'b1;
				`REQUEST_F(return_requestToSetCurrent) = returnRequestF ^ 1'b1;
				`REQUEST_G(return_requestToSetCurrent) = returnSavedResult ^ 1'b1;
				`REQUEST_TYPE(return_requestToSetCurrent) = `BDD_AND;
				`REQUEST_STATE(return_requestToSetCurrent) = `BDD_APPLY;
				`REQUEST_NEGATE(return_requestToSetCurrent) = 1'b1;
			end
		end
		endcase
	end
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
	if(validCurrentRequest)
	begin
		counter <= counter + 1;
	end

	if(reset)
	begin
		counter <= 0;
	end
end

endmodule