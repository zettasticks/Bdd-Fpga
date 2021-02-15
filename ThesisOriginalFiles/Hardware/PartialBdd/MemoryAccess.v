`timescale 10 ns / 10 ns

`include "Constants.v"

// Interface - 	Read 0x0 - Get page info
//				Read 0x4 - Get request info (page (20 bits) or variable (12 bits), 0 if page otherwise 1 for variable (1 bit))

//				Write - 0x0 - Set page to read info (also set nodecount) (9 bits for nodecount,20 bits for page)
//				Write - 0x4 - Set page info (with nodecount from 0x0) and save it into the page
//						Address | Variable |  CurrentVarPage | Wait for DMA | Valid Address | Valid Variable
//						14 bits     12 bits       1 bit           1 bit            1 bit          1 bit

module MemoryAccess (
		// Access nodes		
		output wor [25:0] avm_node_address,
		output wor avm_node_read,
		input wire [15:0] avm_node_readdata,
		input wire avm_node_readdatavalid,
		output wor avm_node_write,
		output wor [15:0] avm_node_writedata,
		output wor [5:0] avm_node_burstcount,
		input wire avm_node_waitrequest,

		// Configuration port
		input wire [1:0] avs_config_address, 
		input wire avs_config_write,
		input wire [31:0] avs_config_writedata,
		input wire avs_config_read,
		output wire [31:0] avs_config_readdata,
		output wire avs_config_readdatavalid,
		output wire avs_config_waitrequest,

		// Accept memory requests from Controller
		input wire [127:0] asi_requests_data,
		input wire asi_requests_channel,
		output wire asi_requests_ready,
		input wire asi_requests_valid,

		output reg hps_irq,
		input wire dma_irq,  // The DMA connects directly to Memory Access so it reports the end of the transfer immediatly

		// Output result to controller
		output wire [95:0] aso_result_data,
		output wire aso_result_channel,
		input wire aso_result_ready,
		output wire aso_result_valid,

		// Outputs variable to controller
		output wire [15:0] aso_var_data,
		input wire aso_var_ready,
		output wire aso_var_valid,

		input wire clk,
		input wire reset
	);

// Read interface with HPS
assign avs_config_waitrequest = reset;

`define MA_TAG_SIZE 6
`define TAG_DEF [5:0]

// On chip memory (consider grouping them together, since we pretty much access them together. Quartus might not detect this possibility)
reg `VAR_DEF pageToVar[2**14-1:0] /* synthesis ramstyle = "no_rw_check,M10K" */;
reg `TAG_DEF  pageToAddressTag[2**14-1:0] /* synthesis ramstyle = "no_rw_check,M10K" */;
reg pageToValidVariable[2**14-1:0] /* synthesis ramstyle = "no_rw_check,M10K" */;

// Write operations
reg hpsWaitForDMA;
reg hpsCurrentVarPage;
reg [13:0] hpsSavedPageIndex;
reg saveDataIntoMem;

localparam [1:0] HPS_SET_PAGE_AND_COUNT = 2'b00,
				 HPS_SET_PAGE_INFO = 2'b01;

// Read operations
localparam [1:0] HPS_GET_PAGE_INFO = 2'b00,
				 HPS_GET_REQUEST = 2'b01;

reg [31:0] dataToHPS;

reg saveDataToHPS;
reg [31:0] dataToSaveToHPS;

reg [17:0] hpsAddressToRead;
reg hpsDoingARead;

reg [31:0] hpsDataOut;
reg hpsDataOutValid;

assign avs_config_readdata = hpsDataOut;
assign avs_config_readdatavalid = hpsDataOutValid;

always @(posedge clk)
begin
	hpsDataOutValid <= 1'b0;
	hpsDoingARead <= 1'b0;
	hpsDataOut <= 0;

	if(avs_config_read)
	begin
		if(avs_config_address == HPS_GET_PAGE_INFO)
		begin
			hpsAddressToRead <= hpsSavedPageIndex;
			hpsDoingARead <= 1'b1;
		end
		if(avs_config_address == HPS_GET_REQUEST)
		begin
			hpsDataOut <= dataToHPS;
			hpsDataOutValid <= 1'b1;
		end
	end

	if(hpsDoingARead)
	begin
		// Read the tag to the page stored in this index, as well as node count
		hpsDataOutValid <= 1'b1;
	end

	if(reset)
	begin
		hpsAddressToRead <= 0;
	end
end

// Interface with HPS
always @(posedge clk)
begin
	if(saveDataToHPS)
	begin
		dataToHPS <= dataToSaveToHPS;
	end

	if(reset)
	begin
		dataToHPS <= 0;
	end
end

reg `MA_INTRO_DEF stage_1_in;

always @*
begin
	`MA_INTRO_$_MA_REQUEST(stage_1_in) = asi_requests_data;
	`MA_INTRO_$_ID(stage_1_in) = asi_requests_channel;
end

wire stage_1_busyOut;
assign asi_requests_ready = !stage_1_busyOut;

// Buffered pipeline logic
wire stage_32_busy,stage_2_validData;
wire `MA_INTRO_DEF stage_2_data;
wire stage_23_valid;

wire [5:0] stage_2_state;
reg  [5:0] stage_2_nextState;

FSMBufferedPipeline #(.DATA_END(`MA_INTRO_END),.MAXIMUM_STATE_BITS(6)) stage_2(
		.validIn(asi_requests_valid),
		.dataIn(stage_1_in),
		.busyIn(stage_32_busy), // What happens if next stage is forever busy
		.validOut(stage_23_valid),
		.busyOut(stage_1_busyOut),
		.currentData(stage_2_data),
		.validData(stage_2_validData),
		.memoryTransfer(),
		.currentState(stage_2_state),
		.nextState(stage_2_nextState),
		.clk(clk),
		.reset(reset)
	);

// Temporaries
wire `INDEX_DEF stage_2_bdd_index;
wire [19:0] stage_2_page;
wire [13:0] stage_2_page_index;
wire [`MA_TAG_SIZE:0] stage_2_page_tag_part;
wire stage_2_page_full;
wire stage_2_page_hit;

wire [11:0] stage_2_requested_var_for_insert = `MA_REQUEST_$_BDDINDEX_NO_NEGATE(stage_2_data);
assign stage_2_bdd_index = {`MA_REQUEST_$_BDDINDEX_NO_NEGATE(stage_2_data),1'b0};
wire [28:0] stage_2_cache_index = `MA_REQUEST_$_BDDINDEX_NO_NEGATE(stage_2_data);
assign stage_2_page = `INDEX_$_PAGE(stage_2_bdd_index);
assign stage_2_page_index = stage_2_page[13:0];
assign stage_2_page_tag_part = {1'b0,stage_2_page[17:12]};

// Variables saved by the FSM
reg [13:0] stage_2_page_address;
reg [`MA_TAG_SIZE:0] stage_2_tag_to_compare;
reg stage_2_page_index_valid;
reg [8:0] stage_2_node_count;
reg [11:0] stage_2_var;
reg stage_2_var_valid;
reg [13:0] stage_2_fetched_var_to_page_value;

assign stage_2_page_hit = (stage_2_page_tag_part == stage_2_tag_to_compare);
assign stage_2_page_full = (stage_2_node_count >= 341); // Check this part do not know if I should be checking 342 or 341. In software, being == to 341 is a problem. Do not know if this wire represents the node count or 1 plus the node count

wire [2:0] stage_2_type_2 = `MA_REQUEST_$_TYPE_2(stage_2_data);
wire stage_2_insertion = (stage_2_type_2 == `MA_TYPE_2_INSERT_NODE);
wire stage_2_cacheOperation = (	stage_2_type_2 == `MA_TYPE_2_FETCH_CACHE |
							 	stage_2_type_2 == `MA_TYPE_2_WRITE_CACHE );

// Control variables set by the FSM logic
reg fetchVarToPage;
reg useFetchedVarToPage;
reg fetchPageInformation;
reg incrementNodeCount;

reg [13:0] stage_2_page_to_get_info;
reg storeCurrentVarPage;
reg saveNodeCount;
reg [13:0] stage_2_pageToChangeNodeCount;

always @(posedge clk)
begin
	saveDataIntoMem <= 1'b0;

	if(avs_config_write)
	begin
		if(avs_config_address == HPS_SET_PAGE_AND_COUNT) // Set page to insert or retrieve info (also send nodecount)
		begin
			stage_2_tag_to_compare <= avs_config_writedata[19:14];
			hpsSavedPageIndex <= avs_config_writedata[13:0];
			stage_2_pageToChangeNodeCount <= avs_config_writedata[13:0];
			stage_2_node_count <= avs_config_writedata[20+9:20]; // Experimenting something
		end
		if(avs_config_address == HPS_SET_PAGE_INFO) 	 // Set address and variable info (writing to this address also acknowledges the interrupt)
		begin
			stage_2_var_valid <= avs_config_writedata[0];
			stage_2_page_index_valid <= avs_config_writedata[1];
			hpsWaitForDMA <= avs_config_writedata[2];
			hpsCurrentVarPage <= avs_config_writedata[3];
			stage_2_var <= avs_config_writedata[15:4];
			stage_2_page_address <= avs_config_writedata[16+13:16]; // Address is the page
			saveDataIntoMem <= 1'b1;
		end
	end

	if(saveDataIntoMem)
	begin
		pageToAddressTag[hpsSavedPageIndex] <= stage_2_tag_to_compare;
		pageToVar[hpsSavedPageIndex] <= stage_2_var;
		pageToValidVariable[hpsSavedPageIndex] <= stage_2_var_valid;
	end

	if(incrementNodeCount)
	begin
		stage_2_node_count <= stage_2_node_count + 1;
	end

	begin
		stage_2_page_to_get_info <= stage_2_page_index;
	end

	if(fetchPageInformation)
	begin
		stage_2_tag_to_compare <= pageToAddressTag[stage_2_page_to_get_info];
		stage_2_var <= pageToVar[stage_2_page_to_get_info];
		stage_2_var_valid <= pageToValidVariable[stage_2_page_to_get_info];
		stage_2_pageToChangeNodeCount <= stage_2_page_to_get_info;
	end

	if(reset)
	begin
		stage_2_page_address <= 0;
		stage_2_tag_to_compare <= 0;		
		stage_2_var <= 0;
		stage_2_var_valid <= 0;
		stage_2_page_index_valid <= 0;
		stage_2_node_count <= 0;
		stage_2_fetched_var_to_page_value <= 0;
		hpsCurrentVarPage <= 0;
		hpsWaitForDMA <= 0;
		hpsSavedPageIndex <= 0;
		stage_2_pageToChangeNodeCount <= 0;
	end
end

// State computation
always @*
begin
	stage_2_nextState = 4'h0;
	saveDataToHPS = 1'b0;
	dataToSaveToHPS = 0;
	hps_irq = 1'b0;
	fetchVarToPage = 1'b0;
	fetchPageInformation = 1'b0;
	useFetchedVarToPage = 1'b0;
	incrementNodeCount = 1'b0;
	storeCurrentVarPage = 1'b0;
	saveNodeCount = 1'b0;

	if(stage_2_validData)
	begin
		case(stage_2_state)
		4'h0:begin
			stage_2_nextState = 4'h0;
			/*
			if(stage_2_type_2 == `MA_TYPE_2_INSERT_NODE)  // Fetch Var to page if bdd insert node
			begin			
				storeCurrentVarPage = 1'b1;
			end

			if(stage_2_cacheOperation)
			begin
				stage_2_nextState = 4'h0; // Just let it through
			end
			else begin
				stage_2_nextState = 4'h6;	
			end
			*/
		end
		4'h6:begin
			fetchPageInformation = 1'b1;
			stage_2_nextState = 4'h2;
		end
		4'h2:begin
			stage_2_nextState = 4'h0;
			case(stage_2_type_2)
				`MA_TYPE_2_INSERT_NODE:begin
					if(!stage_2_page_hit | (stage_2_page_hit & (!stage_2_page_index_valid | !stage_2_var_valid | stage_2_page_full | stage_2_var != stage_2_requested_var_for_insert)))
					begin
						stage_2_nextState = 4'h3;
						saveDataToHPS = 1'b1;
						dataToSaveToHPS = {stage_2_requested_var_for_insert,1'b1}; 
					end
					else 
					begin
						incrementNodeCount = 1'b1;
						stage_2_nextState = 4'h8;
					end
				end
				`MA_TYPE_2_FETCH_NODE,`MA_TYPE_2_FETCH_SIMPLE_NODE,`MA_TYPE_2_WRITE_NODE_NEXT:begin
				if(!stage_2_page_hit | (stage_2_page_hit & !stage_2_page_index_valid))
				begin
					stage_2_nextState = 4'h3;
					// Save request value into reg
					saveDataToHPS = 1'b1;
					dataToSaveToHPS = {stage_2_page,1'b0}; // Page request
				end
			end
				`MA_TYPE_2_GET_VAR:begin
				if(!stage_2_page_hit | (stage_2_page_hit & !stage_2_var_valid))
				begin
					stage_2_nextState = 4'h3;
					// Save request value into reg
					saveDataToHPS = 1'b1;
					dataToSaveToHPS = {stage_2_page,1'b0}; // Page request
				end
			end
				`MA_TYPE_2_FETCH_CACHE,`MA_TYPE_2_WRITE_CACHE:begin
				stage_2_nextState = 4'h0;
			end
			endcase
		end
		4'h3:begin // Assert IRQ and await for a HPS write
			hps_irq = 1'b1;
			
			if(avs_config_write & avs_config_address == HPS_SET_PAGE_INFO)
			begin
				stage_2_nextState = 4'h4;

				if(stage_2_type_2 == `MA_TYPE_2_INSERT_NODE)
				begin
					incrementNodeCount = 1'b1;
				end
			end
			else begin
				stage_2_nextState = 4'h3; // Remain in this state waiting for HPS write	
			end
		end
		4'h4:begin // Wait for DMA or End FMS, use saved address which is the correct one
			if(hpsWaitForDMA)
			begin
				stage_2_nextState = 4'h5;
			end
			else begin
				stage_2_nextState = 4'h0;
			end
		end
		4'h5:begin // Wait for DMA to complete. Do not know if really needed, can change later
			if(!dma_irq)
			begin
				stage_2_nextState = 4'h5;
			end
			else 
			begin
				stage_2_nextState = 4'h0;
			end
		end
		4'h8:begin
			saveNodeCount = 1'b1;
			stage_2_nextState = 4'h0;
		end
		default:begin
			stage_2_nextState = 4'h0;
		end
		endcase
	end
end

assign stage_23_valid = (stage_2_validData & stage_2_nextState == 4'h0);

wire [13:0] stage_2_pageIndexOut;
wire [11:0] stage_2_varOut;

assign stage_2_pageIndexOut = stage_2_page_address;
assign stage_2_varOut = stage_2_var;

// Temporaries for final address calculation
wire [8:0] stage_2_index;

reg [2:0] stage_2_burstCount;
reg [2:0] stage_2_offset;
reg stage_2_write;
always @*
begin
	stage_2_burstCount = 6;
	stage_2_offset = 0;
	stage_2_write = 1'b0;

	case(stage_2_type_2)
	`MA_TYPE_2_FETCH_SIMPLE_NODE: begin 
		stage_2_burstCount = 4;
		stage_2_offset = 2;
	end
	`MA_TYPE_2_GET_VAR: begin
		stage_2_burstCount = 1;
		stage_2_offset = 0;
		stage_2_write = 1'b0;
	end
	`MA_TYPE_2_INSERT_NODE: stage_2_write = 1'b1;
	`MA_TYPE_2_WRITE_CACHE: stage_2_write = 1'b1;
	`MA_TYPE_2_WRITE_NODE_NEXT: begin
		stage_2_write = 1'b1;
		stage_2_burstCount = 2;
	end
	endcase
end

wire [9:0] stage_2_true_nodecount = (stage_2_node_count - 1);

assign stage_2_index = (stage_2_insertion ? stage_2_true_nodecount : `INDEX_$_INDEX(stage_2_bdd_index));

// Calculate BddIndex for an insertion
reg `INDEX_DEF stage_2_newBddIndex;

always @*
begin
	stage_2_newBddIndex = 0;
	`INDEX_$_PAGE(stage_2_newBddIndex) = stage_2_pageIndexOut;
	`INDEX_$_INDEX(stage_2_newBddIndex) = stage_2_true_nodecount;
end

// Calculate final address (Only get var)
wire [25:0] stage_2_finalAddress;

assign stage_2_finalAddress = stage_2_page * 2;

wire [25:0] stage_2_cache_address;
assign stage_2_cache_address = stage_2_cache_index * 12 + `MA_CACHE_START;

reg `MA_STAGE_23_DEF stage_23_data;
always @*
begin
	// Missing - Insert node and cache request
	stage_23_data = 0;

	stage_23_data[0] = 0; // Do var
	stage_23_data[1] = 0; // Insert node
	`MA_STAGE_23_$_WRITE(stage_23_data) = stage_2_write;
	`MA_STAGE_23_$_ID(stage_23_data) = `MA_INTRO_$_ID(stage_2_data);
	`MA_STAGE_23_$_VARIABLE(stage_23_data) = stage_2_varOut;
	`MA_STAGE_23_$_FINAL_ADDRESS(stage_23_data) = stage_2_cacheOperation ? stage_2_cache_address : stage_2_finalAddress;
	`MA_STAGE_23_$_BURSTCOUNT(stage_23_data) = stage_2_burstCount;
	`MA_STAGE_23_$_DATA(stage_23_data) = `MA_REQUEST_$_DATA(stage_2_data);
	`MA_STAGE_23_$_BDDINDEX(stage_23_data) = stage_2_insertion ? stage_2_newBddIndex : stage_2_bdd_index; // If node insertion, have to change this part
end

// Stage 3 - Start memory access or start aso sending data

// Buffered pipeline logic
wire stage_43_busy,stage_3_validData,stage_34_valid;

wire `MA_STAGE_23_DEF stage_3_data;

wire [3:0] stage_3_currentCycle;
wire [3:0] stage_3_maximumCycles;

wire stage_3_actual_valid;

wire stage_3_do_var;

MultiCycleBufferedPipeline #(.DATA_END(`MA_STAGE_23_END),.MAXIMUM_CYCLES_BITS(4)) stage_3(
		.validIn(stage_23_valid),
		.dataIn(stage_23_data),
		.busyIn(stage_43_busy),
		.validOut(stage_34_valid),
		.busyOut(stage_32_busy),
		.currentData(stage_3_data),
		.validData(stage_3_validData),
		.currentCycles(stage_3_currentCycle),
		.maximumCycles(stage_3_maximumCycles),
		.increment(stage_3_validData & (!avm_node_waitrequest | stage_3_do_var)),
		.memoryTransfer(),
		.earlyEnd(),
		.clk(clk),
		.reset(reset)
	);

wire stage_3_write;
wire [25:0] stage_3_finalAddress;
wire [3:0] stage_3_burstcount;
wire [127:0] stage_3_writedata;
wire [29:0] stage_3_bddIndex;
wire [31:0] stage_3_id;
wire [11:0] stage_3_var;

assign stage_3_write = `MA_STAGE_23_$_WRITE(stage_3_data);
assign stage_3_finalAddress = `MA_STAGE_23_$_FINAL_ADDRESS(stage_3_data);
assign stage_3_burstcount = `MA_STAGE_23_$_BURSTCOUNT(stage_3_data);
assign stage_3_writedata = `MA_STAGE_23_$_DATA(stage_3_data);
assign stage_3_bddIndex = `MA_STAGE_23_$_BDDINDEX(stage_3_data);
assign stage_3_id = `MA_STAGE_23_$_ID(stage_3_data);
assign stage_3_var = `MA_STAGE_23_$_VARIABLE(stage_3_data);

assign stage_3_do_var = `MA_STAGE_23_$_DO_VAR(stage_3_data);
wire stage_3_do_insertion = `MA_STAGE_23_$_DO_INSERTION(stage_3_data);

// Deal with potential busy from next stage by just pretending data is not valid, thus delaying start doing anything
// Also do not start access if we are just returning a var 
assign stage_3_actual_valid = stage_3_validData & !stage_3_do_var & !stage_43_busy;

// Temporaries
assign stage_3_maximumCycles = (stage_3_writedata ? stage_3_burstcount : 4'h1); // Reads are just sending burstcount

assign avm_node_address = stage_3_actual_valid ? stage_3_finalAddress : 26'h0000000;
assign avm_node_read = stage_3_actual_valid & !stage_3_write;
assign avm_node_write = stage_3_actual_valid & stage_3_write;
assign avm_node_burstcount = stage_3_actual_valid ? stage_3_burstcount : 4'h0;
assign avm_node_writedata = stage_3_actual_valid ? stage_3_writedata[stage_3_currentCycle*16 +: 16] : 16'h0000;

reg `MA_STAGE_34_DEF stage_34_data;

always @*
begin
	stage_34_data = 0;

	`MA_STAGE_34_$_BDDINDEX(stage_34_data) = stage_3_bddIndex;
	`MA_STAGE_34_$_SIZE(stage_34_data) = stage_3_burstcount;
	`MA_STAGE_34_$_ID(stage_34_data) = stage_3_id;
	`MA_STAGE_34_$_WRITE(stage_34_data) = stage_3_write; // Write of one indicates an insertion
end

wire stage_34_validOut;
assign stage_34_validOut = (stage_34_valid & !stage_3_do_var & (!stage_3_write | stage_3_do_insertion));

// Stage 4 (Read data and match with ID)

// We do not use a buffered pipeline here. The fifo is the "buffer".
// This is done so that we have only 1 cycle of latency from the moment a request is made
// To the moment we can receive the response.

wire stage_54_busy;

wire stage_4_pop_fifo_value;
wire stage_4_fifo_empty;
wire stage_4_fifo_full;
wire `MA_STAGE_34_DEF stage_4_fifo_out;

Fifo #(.DATA_SIZE_END(`MA_STAGE_34_END),.DEPTH_BITS(3)) stage_4_fifo(
		.insertValue(stage_34_validOut),
		.popValue(stage_4_pop_fifo_value), // Perform this when a memory transfer occurs
		.inValue(stage_34_data),
		.outValue(stage_4_fifo_out),
		.empty(stage_4_fifo_empty),
		.full(stage_4_fifo_full),
		.clk(clk),
		.reset(reset)
	);

// Debug values
wire `INDEX_DEF debug_stage_4_bddIndex = `MA_STAGE_34_$_BDDINDEX(stage_4_fifo_out);
wire [3:0] debug_stage_4_size = `MA_STAGE_34_$_SIZE(stage_4_fifo_out);
wire [1:0] debug_stage_4_id = `MA_STAGE_34_$_ID(stage_4_fifo_out);
wire 	   debug_stage_4_write = `MA_STAGE_34_$_WRITE(stage_4_fifo_out);

assign stage_43_busy = stage_4_fifo_full;

wire stage_4_valid = !stage_4_fifo_empty;

reg [3:0] stage_4_currentCycle;
reg [127:0] stage_4_dataBuffer;

wire stage_4_earlyEnd;
wire [3:0] stage_4_maximumCycles;
wire stage_45_valid;
wire stage_4_memoryTransfer;

wire stage_4_insertion = `MA_STAGE_34_$_WRITE(stage_4_fifo_out);

assign stage_4_maximumCycles = `MA_STAGE_34_$_SIZE(stage_4_fifo_out);
assign stage_4_earlyEnd = ((stage_4_currentCycle + 1 == stage_4_maximumCycles) & avm_node_readdatavalid & !stage_54_busy);
assign stage_45_valid = (stage_4_valid & (stage_4_insertion | stage_4_earlyEnd | (stage_4_currentCycle == stage_4_maximumCycles)));
assign stage_4_memoryTransfer = (stage_45_valid & !stage_54_busy);

assign stage_4_pop_fifo_value = stage_4_memoryTransfer;

reg [127:0] stage_4_currentDataStored;

// Calculates the current data in the current cycle.
// This is the data that is sent to the next stage, as the stage becomes valid in the same cycle the last data is sent
always @*
begin
	stage_4_currentDataStored = stage_4_dataBuffer;
	stage_4_currentDataStored[stage_4_currentCycle*16 +: 16] = avm_node_readdata;
end

always @(posedge clk)
begin
	if(avm_node_readdatavalid)
	begin
		stage_4_currentCycle <= stage_4_currentCycle + 1;
		stage_4_dataBuffer <= stage_4_currentDataStored;
	end
	
	if(reset | stage_4_memoryTransfer | stage_4_earlyEnd) // Memory transfer resets the cycle counter so its ready for the next stage
	begin
		stage_4_currentCycle <= 0;
		stage_4_dataBuffer <= 0;
	end
end

reg `MA_STAGE_45_DEF stage_45_data;
wire [31:0] stage_4_id;

assign stage_4_id = `MA_STAGE_34_$_ID(stage_4_fifo_out);

always @*
begin
	stage_45_data = 0;

	`MA_STAGE_45_$_ID(stage_45_data) = stage_4_id;
	`MA_STAGE_45_$_DATA(stage_45_data) = stage_4_insertion ? `MA_STAGE_34_$_BDDINDEX(stage_4_fifo_out) : stage_4_currentDataStored;
end

// Stage 5 (Send data to the ID)
wire stage_5_validData;

wire `MA_STAGE_45_DEF stage_5_data;

BufferedPipeline #(.DATA_END(`MA_STAGE_45_END)) stage_5(
		.validIn(stage_45_valid),
		.dataIn(stage_45_data),
		.busyIn(!aso_result_ready),
		.validOut(stage_5_validData),
		.busyOut(stage_54_busy),
		.currentData(stage_5_data),
		.validData(stage_5_validData),
		.memoryTransfer(),
		.clk(clk),
		.reset(reset)
	);

assign aso_var_data = `MA_STAGE_45_$_DATA(stage_5_data);
assign aso_var_valid = stage_5_validData & `MA_STAGE_45_$_ID(stage_5_data) == 0;

assign aso_result_data = `MA_STAGE_45_$_DATA(stage_5_data);
assign aso_result_valid = stage_5_validData & `MA_STAGE_45_$_ID(stage_5_data) != 0;
assign aso_result_channel = `MA_STAGE_45_$_ID(stage_5_data);

endmodule