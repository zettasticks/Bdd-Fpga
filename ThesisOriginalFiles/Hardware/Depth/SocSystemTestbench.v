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

initial
begin
	hashDataStartFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Testdata/hashTableStart.dat", "r");
	hashDataEndFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Testdata/hashTableEnd.dat", "r");
	nodePageInfoDataStartFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Testdata/nodePageInfoDataStart.dat", "r");
	nodePageInfoDataEndFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Testdata/nodePageInfoDataEnd.dat", "r");
	nodeDataStartFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Testdata/nodeDataStart.dat", "r");
	nodeDataEndFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Testdata/nodeDataEnd.dat", "r");
	requestDataFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Testdata/requestDataFile.dat", "r");
	resultDataFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Testdata/resultDataFile.dat","r");
	applyResultDataFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Testdata/applyResultDataFile.dat","r");
	requestsInsertedFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Testdata/requestsInserted.dat","r");
	requestsSetFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Testdata/requestsSet.dat","r");
	finalResultFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Testdata/finalResult.dat","r");
	quantifyDataFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Testdata/quantifyData.dat","r");
	cacheInsertFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Testdata/cacheInsert.dat","r");
	resultInsertedFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Testdata/resultDataFile.dat","r");
	permutFile = $fopen("/home/ruben/Tese/Projects/CommonIP/New/Testdata/permutData.dat","r");

	if(	hashDataStartFile == 0 | 
		hashDataEndFile == 0 | 
		nodePageInfoDataStartFile == 0 | 
		nodePageInfoDataEndFile == 0 | 
		nodeDataStartFile == 0 | 
		nodeDataEndFile == 0 |
		requestDataFile == 0 |
		resultDataFile == 0 |
		applyResultDataFile == 0 |
		requestsInsertedFile == 0 |
		requestsSetFile == 0 |
		finalResultFile == 0 |
		quantifyDataFile == 0 |
		cacheInsertFile == 0 |
		resultInsertedFile == 0 |
		permutFile == 0)
	begin
		$display("Error opening file");
		$finish;
	end
end

reg [12:0] hps_irq_counter;
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

	while(!$feof(permutFile))
	begin
		$fscanf(permutFile,"%x %x\n",value32,another32);
		st.controller.apply.nextVar[value32] <= another32;
	end

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
	for(i = 0; i < `HASH_SIZE; i = i + 1)
	begin
		st.controller.findOrInsert.hashTable[i] <= `BDD_ZERO;
	end

	while(!$feof(hashDataStartFile))
	begin
		$fscanf(hashDataStartFile,"%x %x\n",index,value32);
		st.controller.findOrInsert.hashTable[index] <= value32;
	end

	for(i = 0; i < 1024; i = i + 1)
	begin
		st.memoryaccess.pageToAddress[i] <= 16'hffff;
		st.memoryaccess.pageToVar[i] <= 16'hffff;
		st.memoryaccess.pageToAddressTag[i] <= 16'hffff;
		st.memoryaccess.pageToValidVariable[i] <= 0;
		st.memoryaccess.pageToValidAddress[i] <= 0;
		st.memoryaccess.pageToNodeCount[i] <= 16'hffff;
		st.memoryaccess.varToCurrentPage[variable] <= 16'hffff;
	end

	// Insert from the end to remove cases of page requests

	while(!$feof(nodePageInfoDataEndFile))
	begin
		$fscanf(nodePageInfoDataEndFile,"%x %x %x\n",index,nodeCount,variable);

		st.memoryaccess.pageToAddress[index] <= index;
		st.memoryaccess.pageToVar[index] <= variable;
		st.memoryaccess.pageToAddressTag[index] <= 0; // We do not use more than 16k pages, for now
		st.memoryaccess.pageToValidVariable[index] <= 1;
		st.memoryaccess.pageToValidAddress[index] <= 1;
		st.memoryaccess.pageToNodeCount[index] <= nodeCount;
		st.memoryaccess.varToCurrentPage[variable] <= index;
	end
	$fseek(nodePageInfoDataEndFile,0,0);

	for(i = 0; i < 4096; i = i + 1)
	begin
		st.memoryaccess.pageToNodeCount[i] <= 0;
	end

	pageAddress = 0;
	while(!$feof(nodePageInfoDataStartFile))
	begin
		$fscanf(nodePageInfoDataStartFile,"%x %x %x\n",index,nodeCount,variable);

		st.memoryaccess.pageToAddress[index] <= pageAddress;
		st.memoryaccess.pageToVar[index] <= variable;
		st.memoryaccess.pageToAddressTag[index] <= 0; // We do not use more than 16k pages, for now
		st.memoryaccess.pageToValidVariable[index] <= 1;
		st.memoryaccess.pageToValidAddress[index] <= 1;
		st.memoryaccess.pageToNodeCount[index] <= nodeCount;
		st.memoryaccess.varToCurrentPage[variable] <= index;
		pageAddress = pageAddress + 1;
	end

	while(!$feof(nodeDataStartFile))
	begin
		$fscanf(nodeDataStartFile,"%x %x\n",value32,value96);
	
		`WRITE_96(value32,value96);
	end

	$monitor("Current:%x",st.controller.currentRequest);

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

	`CLK_EDGE;

	force st.memoryaccess.dma_irq = 1'b1; 

	while(!st.controller.finished)
	begin
		// Need to change INDEX_SIZE to 32 before testing requests again

		if(testRequestValue != st.controller.currentRequest) // If different, it might be because we just changed current requests
		begin
			if($feof(requestDataFile))
			begin
				$display("%t Error, there should be no more requests.",$time);
				$stop;
			end
			else
			begin
				$fscanf(requestDataFile,"%x\n",testRequestValue); // Fetch the next value and check if it is correct
				if(testRequestValue != st.controller.currentRequest)
				begin
					$display("%t Different request. Should be: %x\n",$time,testRequestValue);
					$stop;
				end
			end
		end

		if(st.controller.insertRequest)
		begin
			$fscanf(requestsInsertedFile,"%x\n",value96);
			if(st.controller.requestToInsert != value96)
			begin
				$display("%t Different request to insert. Should be: %x\n",$time,value96);
				$stop;
			end
		end

		if(st.controller.setCurrentRequest)
		begin
			$fscanf(requestsSetFile,"%x\n",value96);
			if(st.controller.requestToSetCurrent != value96)
			begin
				$display("%t Different request to set. Should be: %x\n",$time,value96);
				$stop;
			end
		end

		if(st.controller.insertResult)
		begin
			$fscanf(resultInsertedFile,"%x\n",value32);
			if(st.controller.resultToInsert != value32)
			begin
				$display("%t Different result to insert. Should be: %x\n",$time,value32);
				$stop;
			end
		end


		if(st.controller.insertValueIntoSimpleCache)
		begin
			$fscanf(cacheInsertFile,"%x %x %x\n",value32,simpleHash,value96);
			if(st.controller.performCacheStoredHash != value32)
			begin
				$display("%t The cache index should be: %x but his %x",$time,value32,st.controller.performCacheStoredHash);
				$stop;
			end
			if(st.controller.performCacheStoredSimpleHash != simpleHash)
			begin
				$display("%t The simple hash should be: %x but his %x",$time,simpleHash,st.controller.performCacheStoredSimpleHash);
				$stop;
			end
			if(st.controller.performCacheEntry != value96)
			begin
				$display("%t The cache entry should be: %x but his %x",$time,value96,st.controller.performCacheEntry);
				$stop;
			end
		end

		if(st.controller.validOut)
		begin
			validApplies <= validApplies + 1;
			`CLK_EDGE;
			if(!st.controller.apply_simpleCaseHit & !st.controller.apply_exist_simple)
			begin
				$fscanf(applyResultDataFile,"%x %x %x %x\n",fn,fnv,gn,gnv); // Fetch the next value and check if it is correct
				if(fn != st.controller.apply_fn)
				begin
					$display("%t Different fn. Should be: %x [Apply %d]\n",$time,fn,validApplies);
					$stop;
				end
				if(fnv != st.controller.apply_fnv)
				begin
					$display("%t Different fnv. Should be: %x [Apply %d]\n",$time,fnv,validApplies);
					$stop;
				end
				if(gn != st.controller.apply_gn)
				begin
					$display("%t Different gn. Should be: %x [Apply %d]\n",$time,gn,validApplies);
					$stop;
				end
				if(gnv != st.controller.apply_gnv)
				begin
					$display("%t Different gnv. Should be: %x [Apply %d]\n",$time,gnv,validApplies);
					$stop;
				end
			end
		end
		else
		begin
			`CLK_EDGE;
		end

		if(st.memoryaccess.hps_irq)
		begin
			case(hps_irq_counter)
				0: begin
				 `MEMORY_ACCESS_WRITE(0,32'h200009);
				 `CLK_EDGE;
				 `MEMORY_ACCESS_WRITE(1,32'h1f);
				 hps_irq_counter <= hps_irq_counter + 1;
				 `CLK_EDGE;
				 `RESET_MEMORY_ACCESS_INTERFACE;
				end
				1: begin
				 `MEMORY_ACCESS_WRITE(0,32'h200008);
				 `CLK_EDGE;
				 `MEMORY_ACCESS_WRITE(1,32'h1003f);
				 hps_irq_counter <= hps_irq_counter + 1;
				 `CLK_EDGE;
				 `RESET_MEMORY_ACCESS_INTERFACE;
				end
				2: begin
				 `MEMORY_ACCESS_WRITE(0,32'h200007);
				 `CLK_EDGE;
				 `MEMORY_ACCESS_WRITE(1,32'h2007f);
				 hps_irq_counter <= hps_irq_counter + 1;
				 `CLK_EDGE;
				 `RESET_MEMORY_ACCESS_INTERFACE;
				end
				3: begin
				 `MEMORY_ACCESS_WRITE(0,32'h200006);
				 `CLK_EDGE;
				 `MEMORY_ACCESS_WRITE(1,32'h300bf);
				 hps_irq_counter <= hps_irq_counter + 1;
				 `CLK_EDGE;
				 `RESET_MEMORY_ACCESS_INTERFACE;
				end
				4: begin
				 `MEMORY_ACCESS_WRITE(0,32'h200005);
				 `CLK_EDGE;
				 `MEMORY_ACCESS_WRITE(1,32'h400df);
				 hps_irq_counter <= hps_irq_counter + 1;
				 `CLK_EDGE;
				 `RESET_MEMORY_ACCESS_INTERFACE;
				end
				5: begin
				 `MEMORY_ACCESS_WRITE(0,32'h10000b);
				 `CLK_EDGE;
				 `MEMORY_ACCESS_WRITE(1,32'h500cf);
				 hps_irq_counter <= hps_irq_counter + 1;
				 `CLK_EDGE;
				 `RESET_MEMORY_ACCESS_INTERFACE;
				end
				6: begin
				 `MEMORY_ACCESS_WRITE(0,32'h200004);
				 `CLK_EDGE;
				 `MEMORY_ACCESS_WRITE(1,32'h600ff);
				 hps_irq_counter <= hps_irq_counter + 1;
				 `CLK_EDGE;
				 `RESET_MEMORY_ACCESS_INTERFACE;
				end
				7: begin
				 `MEMORY_ACCESS_WRITE(0,32'h10000a);
				 `CLK_EDGE;
				 `MEMORY_ACCESS_WRITE(1,32'h700ef);
				 hps_irq_counter <= hps_irq_counter + 1;
				 `CLK_EDGE;
				 `RESET_MEMORY_ACCESS_INTERFACE;
				end
				8: begin
				 `MEMORY_ACCESS_WRITE(0,32'h100002);
				 `CLK_EDGE;
				 `MEMORY_ACCESS_WRITE(1,32'h8013f);
				 hps_irq_counter <= hps_irq_counter + 1;
				 `CLK_EDGE;
				 `RESET_MEMORY_ACCESS_INTERFACE;
				end
				9: begin
				 `MEMORY_ACCESS_WRITE(0,32'h100003);
				 `CLK_EDGE;
				 `MEMORY_ACCESS_WRITE(1,32'h9010f);
				 hps_irq_counter <= hps_irq_counter + 1;
				 `CLK_EDGE;
				 `RESET_MEMORY_ACCESS_INTERFACE;
				end
				default: begin
					$stop;
				end
			endcase
		end
	end

	// Pass a few cycles to give time to finish writing any data potentially still in the Memory Access pipeline
	for(i = 0; i < 50; i = i + 1)
	begin
		`CLK_EDGE;
	end

	// Check memory out
	while(!$feof(hashDataEndFile))
	begin
		$fscanf(hashDataEndFile,"%x %x\n",index,value32);
		if(st.controller.findOrInsert.hashTable[index] != value32)
		begin
			$display("%t Different hash for index %x - Is: %x, should be: %x",$time,index,st.controller.findOrInsert.hashTable[index],value32);
		end
	end

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
