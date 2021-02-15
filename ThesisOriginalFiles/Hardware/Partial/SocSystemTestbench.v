`timescale 1 ps / 1 ps

`include "Constants.v"

`define CLK_EDGE \
		#1 clk = 1; \
		#1 clk = 0

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

`define RESET_MEMORY_ACCESS_INTERFACE memory_access_config_address = 0; \
				memory_access_config_write = 1'b0; \
				memory_access_config_writedata = 0

`define MEMORY_ACCESS_WRITE(address,value) memory_access_config_address = address; \
				memory_access_config_write = 1'b1; \
				memory_access_config_writedata = value

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

reg [12:0] hps_irq_counter;

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

soc_system_simul st(
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
integer reduceEndValids,requestsCreatedFile;

integer pageInfo;
reg [31:0] index,value32,another32;
reg [63:0] value64;
reg [19:0] page;
reg [9:0] nodeCount;
reg [13:0] pageAddress;
reg [95:0] value96;
reg [11:0] variable;
reg [95:0] testRequestValue;
reg [29:0] fn,fnv,gn,gnv;
reg [1:0] simpleHash;
reg validApply,validReduce,validNextApply;

initial
begin

	hashDataEndFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/And1/hashTableEnd.dat", "r");
	nodePageInfoDataStartFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/And1/nodePageInfoDataStart.dat", "r");
	nodePageInfoDataEndFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/And1/nodePageInfoDataEnd.dat", "r");
	nodeDataStartFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/And1/nodeDataStart.dat", "r");
	nodeDataEndFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/And1/nodeDataEnd.dat", "r");
	hashDataStartFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/And1/hashTableStart.dat", "r");
	quantifyDataFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/And1/quantifyData.dat","r");
	cacheInsertFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/And1/cacheInsert.dat","r");
	contextStartFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/And1/contextStartFile.dat","r");
	nextContextStartFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/And1/nextContextStartFile.dat","r");
	requestDataFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/And1/requestStart.dat","r");
	contextApplyEndFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/And1/contextApplyEndFile.dat","r");
	//permutFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/Permute1/permutData.dat","r");
	reduceRequestStartFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/And1/reduceRequestStart.dat","r");
	reduceRequestEndFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/And1/reduceRequestEnd.dat","r");
	reduceEndValids = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/And1/reduceEndValids.dat","r");
	requestsCreatedFile =  $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/And1/requestsCreated.dat","r");
	finalResultFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Partial/Testdata/And1/finalResult.dat","r");

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
		finalResultFile == 0)
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

	memory_access_config_write = 0;
	memory_access_config_writedata = 0;
	controller_config_address = 0;
	controller_config_read = 0;
	controller_config_write = 0;
	controller_config_writedata = 0;
	memory_access_config_address = 0;
	memory_access_config_read = 0;
	validApplies = 0;
	hps_irq_counter = 0;

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

	//$monitor("%d %d\n",st.controller.contextIndex,hps_irq_counter);

	// Needed initialization
	for(i = 0; i < `HASH_SIZE; i = i + 1)
	begin
		st.controller.findOrInsert.hashTable[i] <= `BDD_ZERO;
	end

	while(!$feof(hashDataStartFile))
	begin
		$fscanf(hashDataStartFile,"%x %x\n",index,value32);

		controller_config_address = 3;
		controller_config_write = 1'b1;
		controller_config_writedata = index;

		`CLK_EDGE;

		controller_config_address = 4;
		controller_config_write = 1'b1;
		controller_config_writedata = value32;

		`CLK_EDGE;

		controller_config_address = 0;
		controller_config_write = 1'b0;
	end

	for(i = 0; i < 1024; i = i + 1)
	begin
		st.memoryaccess.pageToAddress[i] <= 0;
		st.memoryaccess.pageToVar[i] <= 0;
		st.memoryaccess.pageToAddressTag[i] <= 0;
		st.memoryaccess.pageToValidVariable[i] <= 0;
		st.memoryaccess.pageToValidAddress[i] <= 0;
		st.memoryaccess.pageToNodeCount[i] <= 0;
		st.memoryaccess.varToCurrentPage[i] <= 0;
	end

	while(!$feof(nodePageInfoDataStartFile))
	begin
		$fscanf(nodePageInfoDataStartFile,"%x %x %x\n",index,nodeCount,variable);

		$display("%x %x %x",index,nodeCount,variable);

		memory_access_config_address = 0;
		memory_access_config_write = 1'b1;
		memory_access_config_writedata[19:0] = index;
		memory_access_config_writedata[29:20] = nodeCount;

		`CLK_EDGE;

		memory_access_config_address = 1;
		memory_access_config_write = 1'b1;
		memory_access_config_writedata[0] = 1;
		memory_access_config_writedata[1] = 1;
		memory_access_config_writedata[2] = 0;
		memory_access_config_writedata[3] = 1;
		memory_access_config_writedata[15:4] = variable;
		memory_access_config_writedata[29:16] = index;

		`CLK_EDGE;

		memory_access_config_address = 0;
		memory_access_config_write = 1'b0;
		memory_access_config_writedata = 0;


		/*
		st.memoryaccess.pageToAddress[index] <= index;
		st.memoryaccess.pageToVar[index] <= variable;
		st.memoryaccess.pageToAddressTag[index] <= 0; // We do not use more than 16k pages, for now
		st.memoryaccess.pageToValidVariable[index] <= 1;
		st.memoryaccess.pageToValidAddress[index] <= 1;
		st.memoryaccess.pageToNodeCount[index] <= nodeCount;
		st.memoryaccess.varToCurrentPage[variable] <= index;
		*/
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

	`CLK_EDGE;

	controller_config_write = 1'b0;

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
					$stop;
					$display("%t Different request on current context for index %x, value expected is :%x\n",$time,i,value96);
				end
			end
			$fscanf(contextStartFile,"%x ",value32);
			if(st.controller.currentApply != value32)
			begin
				$stop;
				$display("%t Different Apply value, should be:%x\n",$time,value32);
			end

			$fscanf(contextStartFile,"%x\n",value32);
			if(st.controller.currentReduce != value32)
			begin
				$stop;
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
					$stop;
					$display("%t Different request on new context for index %x, value expected is :%x\n",$time,i,value96);
				end
			end
			for(i = 0; i < 8; i = i + 1)
			begin
				$fscanf(contextApplyEndFile,"%x ",value96);
				if(value96 != st.controller.newNextContext[i])
				begin
					$stop;
					$display("%t Different request on new next context for index %x, value expected is :%x\n",$time,i,value96);
				end
			end
			$fscanf(requestsCreatedFile,"%x\n",value32);
			if(value32 != st.controller.apply_out_requestsCreated)
			begin
				$stop;
				$display("%t Different requests created, value is: %x, expected: %x\n",$time,i,value32,st.controller.apply_out_requestsCreated);
			end
		end

		if(st.controller.validReduceIn & !st.controller.reduceBusy)
		begin
			$fscanf(reduceRequestStartFile,"%x\n",value96);
			if(`REDUCE_IN_$_REDUCE_REQUEST(st.controller.reduceIn) != value96)
			begin
				$stop;
				$display("%t Different Reduce Request start value, should be:%x\n",$time,value96);
			end
		end

		if(st.controller.reduce_2_validOut)
		begin
			$fscanf(reduceRequestEndFile,"%x\n",value96);
			if(st.controller.reduce_2_newRequest != value96)
			begin
				$stop;
				$display("%t Different Reduce Request end value, should be:%x\n",$time,value96);
			end
			$fscanf(reduceEndValids,"%x %x %x\n",validApply,validReduce,validNextApply);
			if( st.controller.reduce_2_validReduce != validReduce |
				st.controller.reduce_2_validApply != validApply | 
				st.controller.reduce_2_validNextApply != validNextApply)
			begin
				$stop;
				$display("%t Different Valids should be (A,R,NA): %x:%x:%x, but is %x:%x:%x\n",$time,validApply,validReduce,validNextApply,st.controller.reduce_2_validApply,st.controller.reduce_2_validReduce,st.controller.reduce_2_validNextApply);
			end
		end
		*/

		`CLK_EDGE;
	end

	// Pass a few cycles to give time to finish writing any data potentially still in the Memory Access pipeline
	for(i = 0; i < 50; i = i + 1)
	begin
		`CLK_EDGE;
	end

	// Check memory out
	/*
	while(!$feof(hashDataEndFile))
	begin
		$fscanf(hashDataEndFile,"%x %x\n",index,value32);
		if(st.controller.findOrInsert.hashTable[index] != value32)
		begin
			$display("%t Different hash for index %x - Is: %x, should be: %x",$time,index,st.controller.findOrInsert.hashTable[index],value32);
		end
	end
	*/

	while(!$feof(nodePageInfoDataEndFile))
	begin
		$fscanf(nodePageInfoDataEndFile,"%x %x %x\n",index,nodeCount,variable);
		if(st.memoryaccess.pageToNodeCount[index] != nodeCount)
		begin
			$display("%t Different NodeCount for index %x - Is: %x, should be: %x",$time,index,st.memoryaccess.pageToNodeCount[index],nodeCount);
		end
	end

	while(!$feof(nodeDataEndFile))
	begin
		$fscanf(nodeDataEndFile,"%x %x\n",value32,value96);
	
		`CHECK_96(value32,value96);
	end

	$fscanf(finalResultFile,"%x\n",value32);
	$display("Result should be:%x Final result is:%x\n",value32,st.controller.finalResult);
	$display("The end\n");
	$stop;
end

endmodule


// Old code, it shows the interfaces and such
/*

Adicionar casos de teste ao FindOrInsert.
Preciso de saber se o node foi encontrado ou se nao.
E se foi criada uma nova entrada na tabela.








*/