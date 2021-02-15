`timescale 10 ns / 10 ns

`include "Constants.v"

module FindOrInsert (
		input wire `INDEX_DEF e,
		input wire `INDEX_DEF t,
		input wire `VAR_DEF   top,
		input wire valid,

		output wire busy,
		output reg `INDEX_DEF result,
		output reg validOut,

		// Configuration port to write hashTable data
		input wire [15:0] avs_config_address, 
		input wire avs_config_write,
		input wire [31:0] avs_config_writedata,
		input wire avs_config_read,
		output wire [31:0] avs_config_readdata,
		output wire avs_config_readdatavalid,
		output wire avs_config_waitrequest,

		output reg [127:0] aso_request_data,
		input wire aso_request_ready,
		output reg aso_request_valid,

		// Input result from Memory Access
		input wire [95:0] asi_result_data,
		input wire [1:0] asi_result_channel,
		input wire asi_result_valid,

		input wire clk,
		input wire reset
	);

`define FIND_OR_INSERT_$_T_END 29
`define FIND_OR_INSERT_$_E_END 59
`define FIND_OR_INSERT_$_TOP_END 69
`define FIND_OR_INSERT_END 69

`define FIND_OR_INSERT_$_T(in) in[29:0]
`define FIND_OR_INSERT_$_E(in) in[59:30]
`define FIND_OR_INSERT_$_TOP(in) in[69:60]
`define FIND_OR_INSERT_DEF [69:0]
`define FIND_OR_INSERT_RANGE 69:0

reg `INDEX_DEF hashTable[`HASH_SIZE-1:0] /* synthesis ramstyle = "no_rw_check,M10K" */;

reg `FIND_OR_INSERT_DEF dataIn;
wire `FIND_OR_INSERT_DEF data;

always @*
begin
	`FIND_OR_INSERT_$_T(dataIn) = t;
	`FIND_OR_INSERT_$_E(dataIn) = e;
	`FIND_OR_INSERT_$_TOP(dataIn) = top;
end

reg [3:0] nextState;
wire [3:0] currentState;
wire running;

FSMBufferedPipeline #(.DATA_END(`FIND_OR_INSERT_END),.MAXIMUM_STATE_BITS(4)) findOrInsert(
		.validIn(valid),
		.busyOut(busy),
		.busyIn(1'b0), // For now, assume no busy from next stage
		.dataIn(dataIn),
		.validData(running),
		.currentData(data),
		.currentState(currentState),
		.nextState(nextState),
		.memoryTransfer(),
		.validOut(validOut),
		.clk(clk),
		.reset(reset)
	);

localparam STAGE_ID = 2'b01; // ID used when communicating with MA

// Input
wire `INDEX_DEF stored_e = `FIND_OR_INSERT_$_E(data);
wire `INDEX_DEF stored_t = `FIND_OR_INSERT_$_T(data);
wire `VAR_DEF   stored_top = `FIND_OR_INSERT_$_TOP(data);

// Temporaries
wire [31:0] hashTableHashValue = `HASH_TABLE_HASH(stored_e,stored_t,stored_top);
wire [1:0] resultID = asi_result_channel;
wire `NODE_DEF resultNode = asi_result_data;
wire resultValid = (asi_result_valid && resultID == STAGE_ID);

// State variables
reg storeFinalResultInHashTable;
reg `INDEX_DEF previousIndex,currentIndex;
reg `NODE_DEF storedNode;
reg `INDEX_DEF newIndex;
reg storeNewIndexIntoHashTable;
reg [31:0] hashTableHash;

// State variable temporaries
wire `INDEX_DEF storedNodeNext = `NODE_NEXT(storedNode);
wire `INDEX_DEF storedNodeThen = `NODE_THEN(storedNode);
wire `INDEX_DEF storedNodeElse = `NODE_ELSE(storedNode);

// Control variables
reg getHashTableValue;
reg setStoreFinalResultInHashTable;
reg storeNode;
reg setNextCurrentIndex;
reg storeNewIndex;
reg storeHash;
reg storeResult;
reg `INDEX_DEF resultToStore;

// Iteraction with HPS
reg [`HASH_BITS:0] hpsHashTableIndex;
reg `INDEX_DEF hpsValueToInsert,hpsValueReaded;
reg hpsPerformWrite;
reg hpsPerformRead;
reg hpsReturnRead;
reg requestHashTableReadFromHPS;
reg requestHashTableWriteFromHPS;
reg resetPreviousIndex;


localparam 	SET_HASH_TABLE_INDEX = 16'h0003,
			SET_GET_HASH_TABLE_VALUE = 16'h0004;

assign avs_config_readdata = currentIndex;
assign avs_config_readdatavalid = hpsReturnRead;
assign avs_config_waitrequest = 0;

always @(posedge clk)
begin
	hpsPerformWrite <= 1'b0;
	hpsPerformRead <= 1'b0;
	hpsReturnRead <= 1'b0;
	hpsValueToInsert <= 0;
	requestHashTableReadFromHPS <= 1'b0;
	requestHashTableWriteFromHPS <= 1'b0;

	if(avs_config_write)
	begin
		if(avs_config_address == SET_HASH_TABLE_INDEX)
		begin
			hashTableHash <= avs_config_writedata;
		end
		if(avs_config_address == SET_GET_HASH_TABLE_VALUE)
		begin
			newIndex <= avs_config_writedata[29:0];
			hpsPerformWrite <= 1'b1;
			requestHashTableWriteFromHPS <= 1'b1;
		end
	end

	if(avs_config_read && avs_config_address == SET_GET_HASH_TABLE_VALUE)
	begin
		hpsPerformRead <= 1'b1;
		requestHashTableReadFromHPS <= 1'b1;
	end

	if(hpsPerformRead)
	begin
		hpsReturnRead <= 1'b1;
	end

	if(getHashTableValue)
	begin
		currentIndex <= hashTable[hashTableHash];
	end

	if(storeResult)
	begin
		result <= resultToStore;
	end

	if(storeNewIndexIntoHashTable)
	begin
		hashTable[hashTableHash] <= newIndex;
	end 

	if(storeHash)
	begin
		hashTableHash <= hashTableHashValue;
	end

	if(resetPreviousIndex)
	begin
		previousIndex <= `BDD_ZERO;
	end

	if(setNextCurrentIndex)
	begin
		previousIndex <= currentIndex;
		currentIndex <= storedNodeNext;
	end

	if(storeNode)
	begin
		storedNode <= resultNode;
	end

	if(storeNewIndex)
	begin
		newIndex <= resultNode[29:0];
	end

	if(setStoreFinalResultInHashTable)
	begin
		storeFinalResultInHashTable <= 1'b1;
	end

	if(reset | validOut)
	begin
		storeFinalResultInHashTable <= 0;
		previousIndex <= 0;
		currentIndex <= 0;
		storedNode <= 0;
		newIndex <= 0;
		hpsValueReaded <= 0;
		result <= 0;
	end

	if(reset)
	begin
		hpsHashTableIndex <= 0;
	end
end

reg `NODE_DEF nodeToInsert;

always @*
begin
	nodeToInsert = 0;
	`NODE_THEN(nodeToInsert) = stored_t;
	`NODE_ELSE(nodeToInsert) = stored_e;
	`NODE_NEXT(nodeToInsert) = currentIndex;
end

always @*
begin
	nextState = 0;
	getHashTableValue = 1'b0;
	setStoreFinalResultInHashTable = 1'b0;
	setNextCurrentIndex = 1'b0;
	validOut = 1'b0;
	storeResult = 1'b0;
	resultToStore = 0;
	aso_request_data = 0;
	aso_request_valid = 1'b0;
	storeNewIndex = 1'b0;
	storeNewIndexIntoHashTable = 1'b0;
	storeNode = 1'b0;
	storeHash = 1'b0;
	resetPreviousIndex = 1'b0;

	if(requestHashTableWriteFromHPS)
	begin
		storeNewIndexIntoHashTable = 1'b1;
	end

	if(requestHashTableReadFromHPS)
	begin
		getHashTableValue = 1'b1;
	end

	if(running)
	begin
		case(currentState)
		4'h0: begin // Fetch and save hashtable value
			resetPreviousIndex = 1'b1;
			storeHash = 1'b1;
			nextState = 4'h8;
		end
		4'h1: begin // If value empty
			if(currentIndex == `BDD_ZERO)  // BDD_ZERO indicates an empty position
			begin
				setStoreFinalResultInHashTable = 1'b1;
				nextState = 4'h5;
			end
			else
			begin  // Send first node fetch request
				aso_request_valid = 1'b1;
				`MA_REQUEST_$_BDDINDEX_NO_NEGATE(aso_request_data) = currentIndex[29:1];
				`MA_REQUEST_$_TYPE_2(aso_request_data) = `MA_TYPE_2_FETCH_NODE;

				if(aso_request_ready)
				begin
					nextState = 4'h2;
				end
				else 
				begin
					nextState = 4'h1;	
				end
			end
		end
		4'h2: begin // Wait to receive node data
			if(resultValid)
			begin
				storeNode = 1'b1;
				nextState = 4'h3;
			end
			else
			begin
				nextState = 4'h2;
			end
		end
		4'h3: begin // Compare values
			if(storedNodeElse == stored_e && storedNodeThen == stored_t) // Node equal, return it
			begin
				nextState = 4'h9; 
				storeResult = 1'b1;
				resultToStore = currentIndex;
			end
			else 
			if(storedNodeElse > stored_e || (storedNodeElse == stored_e && storedNodeThen > stored_t))
			begin
				nextState = 4'h5;// The node does not exist
			end
			else // Fetch next value, if not the end of the chain
			begin
				setNextCurrentIndex = 1'b1;
				if(storedNodeNext == `BDD_ZERO)
				begin
					nextState = 4'h5;
				end
				else
				begin
					nextState = 4'h4;
				end
			end
		end
		4'h4: begin // Fetch node (loop back)
			aso_request_valid = 1'b1;
			`MA_REQUEST_$_BDDINDEX_NO_NEGATE(aso_request_data) = currentIndex[29:1];
			`MA_REQUEST_$_TYPE_2(aso_request_data) = `MA_TYPE_2_FETCH_NODE;

			if(aso_request_ready)
			begin
				nextState = 4'h2;
			end
			else 
			begin
				nextState = 4'h4;	
			end
		end
		4'h5: begin // Insert node
			aso_request_valid = 1'b1;
			`MA_REQUEST_$_BDDINDEX_NO_NEGATE(aso_request_data) = stored_top;
			`MA_REQUEST_$_DATA(aso_request_data) = nodeToInsert;
			`MA_REQUEST_$_TYPE_2(aso_request_data) = `MA_TYPE_2_INSERT_NODE;

			if(aso_request_ready)
			begin
				nextState = 4'h6;
			end
			else
			begin
				nextState = 4'h5;
			end
		end
		4'h6: begin // Wait for node insertion index
			if(resultValid)
			begin
				storeNewIndex = 1'b1;
				nextState = 4'h7;
			end
			else 
			begin
				nextState = 4'h6;	
			end
		end
		4'h7: begin // Save the bddIndex in the previous Node or in the HashTable
			storeResult = 1'b1;
			resultToStore = newIndex;

			if(storeFinalResultInHashTable | previousIndex == `BDD_ZERO)
			begin
				storeNewIndexIntoHashTable = 1'b1;
				nextState = 4'h9;
			end
			else
			begin
				aso_request_valid = 1'b1;
				`MA_REQUEST_$_BDDINDEX_NO_NEGATE(aso_request_data) = previousIndex[29:1];
				`MA_REQUEST_$_DATA(aso_request_data) = newIndex;
				`MA_REQUEST_$_TYPE_2(aso_request_data) = `MA_TYPE_2_WRITE_NODE_NEXT;

				if(aso_request_ready)
				begin
					nextState = 4'h9; 
				end
				else
				begin
					nextState = 4'h7;
				end
			end
		end
		4'h8: begin
			getHashTableValue = 1'b1;
			nextState = 4'h1;
		end
		4'h9: begin // Output result
			validOut = 1'b1;
			nextState = 4'h0;
		end
		default: begin
			nextState = 4'hf;
		end
		endcase
	end
end

endmodule