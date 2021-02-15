`timescale 1 ps / 1 ps

`include "Constants.v"

/*
`define CLK_EDGE \
		#1 clk = 1; \
		#1 clk = 0
*/

`define CLK_EDGE \
		if(st.controller.controllerState == 32'h4 & st.controller.controllerNextState != 32'h4) \
		begin \
			for(i = 0; i < 8; i = i + 1) \
			begin \
				$fscanf(contextApplyEndFile,"%x ",value96); \
				if(value96 != st.controller.newContext[i]) \
				begin \
					$display("%t Different request on new context for index %x, value expected is :%x\n",$time,i,value96); \
					$stop; \
				end \
			end \
			for(i = 0; i < 8; i = i + 1) \
			begin \
				$fscanf(contextApplyEndFile,"%x ",value96); \
				if(value96 != st.controller.newNextContext[i]) \
				begin \
					$display("%t Different request on new next context for index %x, value expected is :%x\n",$time,i,value96); \
					$stop; \
				end \
			end \
			$fscanf(requestsCreatedFile,"%x\n",value32); \
			if(value32 != st.controller.apply_out_requestsCreated) \
			begin \
				$display("%t Different requests created, value is: %x, expected: %x\n",$time,i,value32,st.controller.apply_out_requestsCreated); \
				$stop; \
			end \
		end \
		#1 clk = 1; \
		#1 clk = 0
		/*
		if(st.controller.controllerState == 32'h1) \
		begin \
			for(i = 0; i < 8; i = i + 1) \
			begin \
				$fscanf(contextStartFile,"%x ",value96); \
				if(value96 != st.controller.currentContext[i]) \
				begin \
					$display("%t Different request on current context for index %x, value expected is :%x\n",$time,i,value96); \
					$stop; \
				end \
			end \
			$fscanf(contextStartFile,"%x ",value32); \
			if(st.controller.currentApply != value32) \
			begin \
				$display("%t Different Apply value, should be:%x\n",$time,value32); \
				$stop; \
			end \
			$fscanf(contextStartFile,"%x\n",value32); \
			if(st.controller.currentReduce != value32) \
			begin \
				$display("%t Different Reduce value, should be:%x\n",$time,value32); \
				$stop; \
			end \
		end \
		if(st.controller.validReduceIn & !st.controller.reduceBusy) \
		begin \
			$fscanf(reduceRequestStartFile,"%x\n",value96); \
			if(`REDUCE_IN_$_REDUCE_REQUEST(st.controller.reduceIn) != value96) \
			begin \
				$display("%t Different Reduce Request start value, should be:%x\n",$time,value96); \
				$stop; \
			end \
		end \
		if(st.controller.reduce_2_validOut) \
		begin \
			$fscanf(reduceRequestEndFile,"%x\n",value96); \
			if(st.controller.reduce_2_newRequest != value96) \
			begin \
				$display("%t Different Reduce Request end value, should be:%x\n",$time,value96); \
				$stop; \
			end \
			$fscanf(reduceEndValids,"%x %x %x\n",validApply,validReduce,validNextApply); \
			if( st.controller.reduce_2_validReduce != validReduce | \
				st.controller.reduce_2_validApply != validApply |  \
				st.controller.reduce_2_validNextApply != validNextApply) \
			begin \
				$display("%t Different Valids should be (A,R,NA): %x:%x:%x, but is %x:%x:%x\n",$time,validApply,validReduce,validNextApply,st.controller.reduce_2_validApply,st.controller.reduce_2_validReduce,st.controller.reduce_2_validNextApply); \
				$stop; \
			end \
		end \
		*/

`define APPLY_INPUT_$_BDDINDEX_F_END 29
`define APPLY_INPUT_$_BDDINDEX_G_END 59
`define APPLY_INPUT_$_TYPE_END 61
`define APPLY_INPUT_$_REQUEST_END 138
`define APPLY_INPUT_END 138

`define APPLY_INPUT_$_BDDINDEX_F(in) in[29:0]
`define APPLY_INPUT_$_BDDINDEX_G(in) in[59:30]
`define APPLY_INPUT_$_TYPE(in) in[61:60]
`define APPLY_INPUT_$_REQUEST(in) in[138:62]
`define APPLY_INPUT_DEF [138:0]
`define APPLY_INPUT_RANGE 138:0

`define APPLY_STAGE_12_$_APPLY_INPUT_END 138
`define APPLY_STAGE_12_$_SIMPLE_RESULT_END 168
`define APPLY_STAGE_12_$_HIT_END 169
`define APPLY_STAGE_12_END 169

`define APPLY_STAGE_12_$_APPLY_INPUT(in) in[138:0]
`define APPLY_STAGE_12_$_SIMPLE_RESULT(in) in[168:139]
`define APPLY_STAGE_12_$_HIT(in) in[169:169]
`define APPLY_STAGE_12_DEF [169:0]
`define APPLY_STAGE_12_RANGE 169:0

`define REDUCE_IN_$_REDUCE_REQUEST(in) in[84:0]

/*
`define WRITE_32(address,data) \
		st.hpsmemory.the_altsyncram.m_default.altsyncram_inst.mem_data[address+0] = (data & 32'h0000ffff); \
		st.hpsmemory.the_altsyncram.m_default.altsyncram_inst.mem_data[address+1] = (data & 32'hffff0000) >> 16

`define WRITE_64(address,data) \
		st.hpsmemory.the_altsyncram.m_default.altsyncram_inst.mem_data[address+0] = (data & 64'h000000000000ffff); 		 \
		st.hpsmemory.the_altsyncram.m_default.altsyncram_inst.mem_data[address+1] = (data & 64'h00000000ffff0000) >> 16; \
		st.hpsmemory.the_altsyncram.m_default.altsyncram_inst.mem_data[address+2] = (data & 64'h0000ffff00000000) >> 32; \
		st.hpsmemory.the_altsyncram.m_default.altsyncram_inst.mem_data[address+3] = (data & 64'hffff000000000000) >> 48

`define WRITE_128(address,data) \
		st.hpsmemory.the_altsyncram.m_default.altsyncram_inst.mem_data[address+0] = (data & 128'h0000000000000000000000000000ffff); 	  \
		st.hpsmemory.the_altsyncram.m_default.altsyncram_inst.mem_data[address+1] = (data & 128'h000000000000000000000000ffff0000) >> 16; \
		st.hpsmemory.the_altsyncram.m_default.altsyncram_inst.mem_data[address+2] = (data & 128'h00000000000000000000ffff00000000) >> 32; \
		st.hpsmemory.the_altsyncram.m_default.altsyncram_inst.mem_data[address+3] = (data & 128'h0000000000000000ffff000000000000) >> 48; \
		st.hpsmemory.the_altsyncram.m_default.altsyncram_inst.mem_data[address+4] = (data & 128'h000000000000ffff0000000000000000) >> 64; \
		st.hpsmemory.the_altsyncram.m_default.altsyncram_inst.mem_data[address+5] = (data & 128'h00000000ffff00000000000000000000) >> 70; \
		st.hpsmemory.the_altsyncram.m_default.altsyncram_inst.mem_data[address+6] = (data & 128'h0000ffff000000000000000000000000) >> 96; \
		st.hpsmemory.the_altsyncram.m_default.altsyncram_inst.mem_data[address+7] = (data & 128'hffff0000000000000000000000000000) >> 112
*/

`define WRITE_32(address,data) \
		sdram.soc_system_DRAM_CONTROLLER_test_component_ram.mem_array[address+0] = (data & 32'h0000ffff); \
		sdram.soc_system_DRAM_CONTROLLER_test_component_ram.mem_array[address+1] = (data & 32'hffff0000) >> 16

`define WRITE_64(address,data) \
		sdram.soc_system_DRAM_CONTROLLER_test_component_ram.mem_array[address+0] = (data & 64'h000000000000ffff); 		 \
		sdram.soc_system_DRAM_CONTROLLER_test_component_ram.mem_array[address+1] = (data & 64'h00000000ffff0000) >> 16; \
		sdram.soc_system_DRAM_CONTROLLER_test_component_ram.mem_array[address+2] = (data & 64'h0000ffff00000000) >> 32; \
		sdram.soc_system_DRAM_CONTROLLER_test_component_ram.mem_array[address+3] = (data & 64'hffff000000000000) >> 48

`define WRITE_96(address,data) \
		sdram.soc_system_DRAM_CONTROLLER_test_component_ram.mem_array[address+0] = (data & 96'h00000000000000000000ffff); 		 \
		sdram.soc_system_DRAM_CONTROLLER_test_component_ram.mem_array[address+1] = (data & 96'h0000000000000000ffff0000) >> 16; \
		sdram.soc_system_DRAM_CONTROLLER_test_component_ram.mem_array[address+2] = (data & 96'h000000000000ffff00000000) >> 32; \
		sdram.soc_system_DRAM_CONTROLLER_test_component_ram.mem_array[address+3] = (data & 96'h00000000ffff000000000000) >> 48; \
		sdram.soc_system_DRAM_CONTROLLER_test_component_ram.mem_array[address+4] = (data & 96'h0000ffff0000000000000000) >> 64; \
		sdram.soc_system_DRAM_CONTROLLER_test_component_ram.mem_array[address+5] = (data & 96'hffff00000000000000000000) >> 80

`define WRITE_128(address,data) \
		sdram.soc_system_DRAM_CONTROLLER_test_component_ram.mem_array[address+0] = (data & 128'h0000000000000000000000000000ffff); 	  \
		sdram.soc_system_DRAM_CONTROLLER_test_component_ram.mem_array[address+1] = (data & 128'h000000000000000000000000ffff0000) >> 16; \
		sdram.soc_system_DRAM_CONTROLLER_test_component_ram.mem_array[address+2] = (data & 128'h00000000000000000000ffff00000000) >> 32; \
		sdram.soc_system_DRAM_CONTROLLER_test_component_ram.mem_array[address+3] = (data & 128'h0000000000000000ffff000000000000) >> 48; \
		sdram.soc_system_DRAM_CONTROLLER_test_component_ram.mem_array[address+4] = (data & 128'h000000000000ffff0000000000000000) >> 64; \
		sdram.soc_system_DRAM_CONTROLLER_test_component_ram.mem_array[address+5] = (data & 128'h00000000ffff00000000000000000000) >> 70; \
		sdram.soc_system_DRAM_CONTROLLER_test_component_ram.mem_array[address+6] = (data & 128'h0000ffff000000000000000000000000) >> 96; \
		sdram.soc_system_DRAM_CONTROLLER_test_component_ram.mem_array[address+7] = (data & 128'hffff0000000000000000000000000000) >> 112

`define CHECK_96(address,data) \
		if(sdram.soc_system_DRAM_CONTROLLER_test_component_ram.mem_array[address+0] != (data & 96'h00000000000000000000ffff)) \
		 	$display("Different data on line address:%x",address); \
		if(sdram.soc_system_DRAM_CONTROLLER_test_component_ram.mem_array[address+1] != (data & 96'h0000000000000000ffff0000) >> 16) \
		 	$display("Different data on line address:%x",address); \
		if(sdram.soc_system_DRAM_CONTROLLER_test_component_ram.mem_array[address+2] != (data & 96'h000000000000ffff00000000) >> 32) \
		 	$display("Different data on line address:%x",address); \
		if(sdram.soc_system_DRAM_CONTROLLER_test_component_ram.mem_array[address+3] != (data & 96'h00000000ffff000000000000) >> 48) \
		 	$display("Different data on line address:%x",address); \
		if(sdram.soc_system_DRAM_CONTROLLER_test_component_ram.mem_array[address+4] != (data & 96'h0000ffff0000000000000000) >> 64) \
		 	$display("Different data on line address:%x",address); \
		if(sdram.soc_system_DRAM_CONTROLLER_test_component_ram.mem_array[address+5] != (data & 96'hffff00000000000000000000) >> 80) \
		 	$display("Different data on line address:%x",address)

module Test();

reg clk;
reg reset;

reg [1:0]  memory_access_config_address;
wire       memory_access_config_waitrequest;
reg        memory_access_config_write;
reg [31:0] memory_access_config_writedata;
reg 		memory_access_config_read;
wire [31:0] memory_access_config_readdata;

reg [15:0]  controller_config_address;
wire        controller_config_waitrequest;
reg         controller_config_write;
reg 		controller_config_read;
wire [31:0] controller_config_readdata;
reg [31:0]  controller_config_writedata;

wire [12:0] sdram_wire_addr;
wire [1:0]  sdram_wire_ba;
wire        sdram_wire_cas_n;
wire        sdram_wire_cke;
wire        sdram_wire_cs_n;
wire [15:0] sdram_wire_dq;
wire [1:0]  sdram_wire_dqm;
wire        sdram_wire_ras_n;
wire        sdram_wire_we_n;

wire controller_config_readdatavalid;
wire memory_access_config_readdatavalid;

wire hps_irq;

reg [4:0] hps_irq_counter;

soc_system_DRAM_CONTROLLER_test_component sdram(
            .clk(clk),
            .zs_addr(sdram_wire_addr),
            .zs_ba(sdram_wire_ba),
            .zs_cas_n(sdram_wire_cas_n),
            .zs_cke(sdram_wire_cke),
            .zs_cs_n(sdram_wire_cs_n),
            .zs_dqm(sdram_wire_dqm),
            .zs_ras_n(sdram_wire_ras_n),
            .zs_we_n(sdram_wire_we_n),
            .zs_dq(sdram_wire_dq)
         );

reg avs_co_write;
reg [127:0] avs_co_writedata;
reg avs_co_read;
wire [127:0] avs_co_readdata;
reg [15:0] avs_co_byteenable;
wire avs_co_waitrequest;

soc_system_cobdd_simul st(
	.controller_co_write(avs_co_write),
	.controller_co_writedata(avs_co_writedata),
	.controller_co_read(avs_co_read),
	.controller_co_readdata(avs_co_readdata),
	.controller_co_byteenable(avs_co_byteenable),
	.controller_co_waitrequest(avs_co_waitrequest),
	.controller_config_address(controller_config_address),
	.controller_config_waitrequest(controller_config_waitrequest),
	.controller_config_write(controller_config_write),
	.controller_config_writedata(controller_config_writedata),
	.controller_config_read(controller_config_read),
	.controller_config_readdata(controller_config_readdata),
	.controller_config_readdatavalid(controller_config_readdatavalid),
	.memory_access_config_address(memory_access_config_address),
	.memory_access_config_waitrequest(memory_access_config_waitrequest),
	.memory_access_config_write(memory_access_config_write),
	.memory_access_config_writedata(memory_access_config_writedata),
	.memory_access_config_readdatavalid(memory_access_config_readdatavalid),
	.memory_access_config_read(memory_access_config_read),
	.memory_access_config_readdata(memory_access_config_readdata),
	.clk_clk(clk),
	.reset_reset_n(~reset),
	.sdram_wire_addr(sdram_wire_addr),
	.sdram_wire_ba(sdram_wire_ba),
	.sdram_wire_cas_n(sdram_wire_cas_n),
	.sdram_wire_cke(sdram_wire_cke),
	.sdram_wire_cs_n(sdram_wire_cs_n),
	.sdram_wire_dq(sdram_wire_dq),
	.sdram_wire_dqm(sdram_wire_dqm),
	.sdram_wire_ras_n(sdram_wire_ras_n),
	.sdram_wire_we_n(sdram_wire_we_n),
	.memoryaccess_hps_irq_irq(hps_irq)
	);

integer hashDataStartFile,nodePageInfoDataStartFile,nodeDataStartFile,
		hashDataEndFile,nodePageInfoDataEndFile,nodeDataEndFile,
		requestDataFile,resultDataFile,applyResultDataFile,
		requestsInsertedFile,requestsSetFile,finalResultFile,
		quantifyDataFile,cacheInsertFile,resultInsertedFile,permutFile;

integer cacheFile,contextStartFile,nextContextStartFile,contextApplyEndFile,reduceRequestStartFile,reduceRequestEndFile;
integer reduceEndValids,requestsCreatedFile,applyCoBdd,reduceCoBdd;

integer pageInfo;
reg [31:0] index,value32,another32;
reg [63:0] value64;
reg [19:0] page;
reg [9:0] nodeCount;
reg [13:0] pageAddress;
reg [95:0] value96;
reg [11:0] variable;
reg [95:0] testRequestValue;
reg [1:0] simpleHash;
reg [1:0] type;
reg [31:0] f,g,fnv,fv,gnv,gv;
reg validApply,validReduce,validNextApply;
reg sendData;

`define WORD_0(in) in[31:0]
`define WORD_1(in) in[63:32]
`define WORD_2(in) in[95:64]
`define WORD_3(in) in[127:96]

initial
begin
	hashDataEndFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/Abstract9/hashTableEnd.dat", "r");
	nodePageInfoDataStartFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/Abstract9/nodePageInfoDataStart.dat", "r");
	nodePageInfoDataEndFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/Abstract9/nodePageInfoDataEnd.dat", "r");
	nodeDataStartFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/Abstract9/nodeDataStart.dat", "r");
	nodeDataEndFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/Abstract9/nodeDataEnd.dat", "r");
	hashDataStartFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/Abstract9/hashTableStart.dat", "r");
	quantifyDataFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/Abstract9/quantifyData.dat","r");
	cacheInsertFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/Abstract9/cacheInsert.dat","r");
	contextStartFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/Abstract9/contextStartFile.dat","r");
	nextContextStartFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/Abstract9/nextContextStartFile.dat","r");
	requestDataFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/Abstract9/requestStart.dat","r");
	contextApplyEndFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/Abstract9/contextApplyEndFile.dat","r");
	//permutFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/Permute1/permutData.dat","r");
	reduceRequestStartFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/Abstract9/reduceRequestStart.dat","r");
	reduceRequestEndFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/Abstract9/reduceRequestEnd.dat","r");
	reduceEndValids = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/Abstract9/reduceEndValids.dat","r");
	requestsCreatedFile =  $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/Abstract9/requestsCreated.dat","r");
	finalResultFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/Abstract9/finalResult.dat","r");
	applyCoBdd = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/Abstract9/applyCoBdd.dat","r");
	reduceCoBdd = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/Abstract9/reduceCoBdd.dat","r");

	permutFile = 1;

	if(	hashDataStartFile == 0 | 
		hashDataEndFile == 0 | 
		nodePageInfoDataStartFile == 0 | 
		nodePageInfoDataEndFile == 0 | 
		nodeDataStartFile == 0 | 
		nodeDataEndFile == 0 |
		quantifyDataFile == 0 |
		cacheInsertFile == 0 |
		permutFile == 0 |
		contextStartFile == 0 |
		nextContextStartFile == 0 |
		contextApplyEndFile == 0 |
		reduceRequestStartFile == 0 |
		reduceRequestEndFile == 0 |
		reduceEndValids == 0 |
		requestsCreatedFile == 0 |
		finalResultFile == 0 |
		applyCoBdd == 0 |
		reduceCoBdd == 0)
	begin
		$display("Error opening file");
		$finish;
	end
end

integer i,validApplies;
initial
begin
	#1;
	
	if(`CACHE_HASH(66582,66584,0,0) != 'h2082e)
	begin
		$stop;
	end

	avs_co_write = 0;
	avs_co_writedata = 0;
	avs_co_read = 0;
	avs_co_byteenable = 0;
	memory_access_config_write = 0;
	memory_access_config_writedata = 0;
	controller_config_address = 0;
	controller_config_read = 0;
	controller_config_write = 0;
	controller_config_writedata = 0;
	memory_access_config_address = 0;
	memory_access_config_read = 0;
	validApplies = 0;

	// This is really slow for the correct SimpleCacheSize. Do not forget to change the value back and do a proper test
	for(i = 0; i < `CACHE_SIZE; i = i + 1)
	begin
		st.controller.apply.simpleCache[i] <= 0;
	end

	st.controller.apply.cubeLastVar = 90;

	for(i = 0; i < 4096 / 32; i = i + 1)
	begin
		$fscanf(quantifyDataFile,"%x\n",value32);

		st.controller.apply.quantify[i] <= value32;
	end

	for(i = 0; i < 4096; i = i + 1)
	begin
		st.controller.apply.nextVar[i] <= 0;
	end

	/*
	while(!$feof(permutFile))
	begin
		$fscanf(permutFile,"%x %x\n",value32,another32);
		st.controller.apply.nextVar[value32] <= another32;
	end
	*/

	reset = 1;

	`CLK_EDGE;

	reset = 0;

	// Wait some time for reset to propagate
	`CLK_EDGE;
	`CLK_EDGE;
	`CLK_EDGE;
	`CLK_EDGE;
	`CLK_EDGE;
	`CLK_EDGE;
	`CLK_EDGE;
	`CLK_EDGE;
	`CLK_EDGE;
	`CLK_EDGE;
	`CLK_EDGE;
	st.sdram.refresh_counter = 1; // Do not want to wait for 5000 cycles doing nothing. This does not change anything

	`CLK_EDGE;

	// Needed initialization
	/*
	for(i = 0; i < `HASH_SIZE; i = i + 1)
	begin
		st.controller.findOrInsert.hashTable[i] <= `BDD_ZERO;
	end
	*/

	while(!$feof(nodePageInfoDataEndFile))
	begin
		$fscanf(nodePageInfoDataEndFile,"%x %x %x\n",index,nodeCount,variable);

		sdram.soc_system_DRAM_CONTROLLER_test_component_ram.mem_array[index] <= variable;
		st.memoryaccess.pageToVar[index] <= variable;
		st.memoryaccess.pageToAddressTag[index] <= 0; // We do not use more than 16k pages, for now
		st.memoryaccess.pageToValidVariable[index] <= 1;
	end

	while(!$feof(nodeDataStartFile))
	begin
		$fscanf(nodeDataStartFile,"%x %x\n",value32,value96);
	
		`WRITE_96(value32,value96);
	end

	//$monitor("Current:%x",st.controller.currentRequest);

	// Fetch the first request
	$fscanf(requestDataFile,"%x\n",testRequestValue);

	`CLK_EDGE;

	controller_config_address = 0;
	controller_config_write = 1'b1;
	controller_config_writedata = testRequestValue;

	`CLK_EDGE;

	controller_config_address = 1; // Not multiple of 4 since I'm doing a direct value change
	controller_config_writedata = testRequestValue >> 32;

	`CLK_EDGE;

	controller_config_address = 2; // Not multiple of 4 since I'm doing a direct value change
	controller_config_writedata = testRequestValue >> 64;
	avs_co_read = 1'b1;

	`CLK_EDGE;

	controller_config_write = 1'b0;
	avs_co_read = 1'b1;	
	avs_co_byteenable = 16'h000f;

	while(!st.controller.finished)
	begin
		// Need to change INDEX_SIZE to 32 before testing requests again
		/*
		if(st.controller.controllerState == 32'h1)
		begin
			for(i = 0; i < 8; i = i + 1)
			begin
				$fscanf(contextStartFile,"%x ",value96);
				if(value96 != st.controller.currentContext[i])
				begin
					$display("%t Different request on current context for index %x, value expected is :%x\n",$time,i,value96);
				end
			end
			$fscanf(contextStartFile,"%x ",value32);
			if(st.controller.currentApply != value32)
			begin
				$display("%t Different Apply value, should be:%x\n",$time,value32);
			end

			$fscanf(contextStartFile,"%x\n",value32);
			if(st.controller.currentReduce != value32)
			begin
				$display("%t Different Reduce value, should be:%x\n",$time,value32);
			end
		end

		if(st.controller.controllerState == 32'h4 & st.controller.controllerNextState != 32'h4) // End of Apply
		begin
			for(i = 0; i < 8; i = i + 1)
			begin
				$fscanf(contextApplyEndFile,"%x ",value96);
				if(value96 != st.controller.newContext[i])
				begin
					$display("%t Different request on new context for index %x, value expected is :%x\n",$time,i,value96);
				end
			end
			for(i = 0; i < 8; i = i + 1)
			begin
				$fscanf(contextApplyEndFile,"%x ",value96);
				if(value96 != st.controller.newNextContext[i])
				begin
					$display("%t Different request on new next context for index %x, value expected is :%x\n",$time,i,value96);
				end
			end
			$fscanf(requestsCreatedFile,"%x\n",value32);
			if(value32 != st.controller.apply_out_requestsCreated)
			begin
				$display("%t Different requests created, value is: %x, expected: %x\n",$time,i,value32,st.controller.apply_out_requestsCreated);
			end
		end

		if(st.controller.validReduceIn & !st.controller.reduceBusy)
		begin
			$fscanf(reduceRequestStartFile,"%x\n",value96);
			if(`REDUCE_IN_$_REDUCE_REQUEST(st.controller.reduceIn) != value96)
			begin
				$display("%t Different Reduce Request start value, should be:%x\n",$time,value96);
			end
		end

		if(st.controller.reduce_2_validOut)
		begin
			$fscanf(reduceRequestEndFile,"%x\n",value96);
			if(st.controller.reduce_2_newRequest != value96)
			begin
				$display("%t Different Reduce Request end value, should be:%x\n",$time,value96);
			end
			$fscanf(reduceEndValids,"%x %x %x\n",validApply,validReduce,validNextApply);
			if( st.controller.reduce_2_validReduce != validReduce |
				st.controller.reduce_2_validApply != validApply | 
				st.controller.reduce_2_validNextApply != validNextApply)
			begin
				$display("%t Different Valids should be (A,R,NA): %x:%x:%x, but is %x:%x:%x\n",$time,validApply,validReduce,validNextApply,st.controller.reduce_2_validApply,st.controller.reduce_2_validReduce,st.controller.reduce_2_validNextApply);
			end
		end
		*/

		if(!avs_co_waitrequest)
		begin
			
			/*
			avs_co_byteenable = 16'h000f;
			`CLK_EDGE;
			avs_co_byteenable = 16'h00f0;
			`CLK_EDGE;
			avs_co_byteenable = 16'h0f00;
			`CLK_EDGE;
			avs_co_byteenable = 16'hf000;
			`CLK_EDGE;
			
			*/
			avs_co_byteenable = 16'hffff;
			`CLK_EDGE;

			avs_co_read = 1'b0;

			`CLK_EDGE;

			type = avs_co_readdata[1:0];
			if(type == 2'b00) // HPS_APPLY_ONE_NODE
			begin
				$fscanf(applyCoBdd,"%x %x %x\n",f,fnv,fv);
				if(`WORD_1(avs_co_readdata) != f)
				begin
					$stop;
					$display("%t Error 1, Should be:%x, is:%x\n",$time,f,`WORD_1(avs_co_readdata));
				end

				avs_co_write = 1'b1;
				//avs_co_byteenable = 16'h000f;
				avs_co_writedata = {32'h0,32'h0,fv,fnv};
			end
			else if(type == 2'b01) // HPS_APPLY_TWO_NODE
			begin
				$fscanf(applyCoBdd,"%x %x %x %x %x %x\n",f,g,fnv,fv,gnv,gv);
				if(`WORD_1(avs_co_readdata) != f | `WORD_2(avs_co_readdata) != g)
				begin
					$stop;
					$display("%t Error 2, Should be:%x %x, is:%x %x\n",$time,f,g,`WORD_1(avs_co_readdata),`WORD_2(avs_co_readdata));
				end

				avs_co_write = 1'b1;
				//avs_co_byteenable = 16'h000f;
				avs_co_writedata = {gv,gnv,fv,fnv};
			end
			else if(type == 2'b10) // HPS_FIND_OR_INSERT
			begin
				$fscanf(reduceCoBdd,"%x %x %x %x\n",f,g,variable,value32); // f = t, g = e
				if(`WORD_1(avs_co_readdata) != f | `WORD_2(avs_co_readdata) != g | `WORD_3(avs_co_readdata) != variable)
				begin
					$stop;
					$display("%t Error 3, Should be:%x %x %x, is %x %x %x\n",$time,f,g,variable,`WORD_1(avs_co_readdata),`WORD_2(avs_co_readdata),`WORD_3(avs_co_readdata));
				end

				avs_co_write = 1'b1;
				//avs_co_byteenable = 16'h000f;
				avs_co_writedata = {32'h0,32'h0,32'h0,value32};

			end
			else if(type == 2'b11) // HPS_FINISHED
			begin
				$fscanf(finalResultFile,"%x\n",value32);
				$display("Result should be:%x Final result is:%x\n",value32,avs_co_readdata[31:2]);
				$stop;
			end

			/*
			`CLK_EDGE;
			avs_co_byteenable = 16'h00f0;
			`CLK_EDGE;
			avs_co_byteenable = 16'h0f00;
			`CLK_EDGE;
			avs_co_byteenable = 16'hf000;
			*/

			`CLK_EDGE;
			avs_co_write = 1'b0;
			`CLK_EDGE;
			avs_co_read = 1'b1;
		end
		
		`CLK_EDGE;
	end

	// Pass a few cycles to give time to finish writing any data potentially still in the Memory Access pipeline
	for(i = 0; i < 50; i = i + 1)
	begin
		`CLK_EDGE;
	end

	$fscanf(finalResultFile,"%x\n",value32);
	$display("Result should be:%x Final result is:%x\n",value32,st.controller.finalResult);
	$display("The end\n");
	$stop;
end

endmodule
