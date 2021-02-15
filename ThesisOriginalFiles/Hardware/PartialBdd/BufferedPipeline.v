`timescale 10 ns / 10 ns

`include "Constants.v"

// A buffered pipeline is a stage control unit.
// Data is registed at the beginning of the stage
// Registers when a data transfer is performed and "consumes" validity if no valid 
// Logic is performed by outside code using the currentData. Input registration and selection is taken care by this module
// Logic is expected to produce the validOut signal, indicating that it produced valid data
// Logic uses combinatorial signals.
module BufferedPipeline #(parameter
		DATA_END = 0
	)(
		// The 3 signals used by standard pipeline
		input wire validIn, // Valid from previous stage
		input wire [DATA_END:0] dataIn, // Data from previous stage
		input wire busyIn,	 // Busy from next stage

		input wire validOut, // Outside logic computes this signal and needs to pass it here to known when the memory transfer is done
							 // It indicates when the stage is ready to send data to the next stage

		output wire busyOut, // Sent by this stage and indicates it cannot accept the data
		output wire [DATA_END:0] currentData, // Data to be used to perform the stage operation
		output wire validData, // Indicates that currentData is valid, validOut can only be asserted if this signal is also asserted
		
		output wire memoryTransfer, // Asserted when the data from this stage is successfully transfer to the next stage (this stage data becomes invalid unless the previous stage or data is stored)

		input clk,
		input reset
	);

// Simulation checks
always @(posedge clk)
begin
	if(validOut == 1'b1 && validData == 1'b0)
	begin
		$display("Outside logic is not taking validData into account, ValidOut cannot be asserted while validData is deasserted, in %m");
    	$finish;
	end
end

reg [DATA_END:0] inputData,storedData;
reg validInput,validStored;

assign busyOut = validStored; // Stage is busy when the stored register is "full"
assign validData = validInput;
assign currentData = inputData; // The data always comes from input. StoredData is sent to input.
assign memoryTransfer = (validOut & !busyIn); // Memory transfer is done when 

always @(posedge clk)
begin
	if(!validInput) // First case, the stage is empty
	begin
		if(validIn)
		begin
			inputData <= dataIn;
			validInput <= 1'b1;
		end
	end
	else if(memoryTransfer) // Second case, the stage has data and is about to perform a transfer
	begin
		if(validStored) // If we have data stored, flush it out
		begin
			inputData <= storedData;
			storedData <= 0; // Not neeed, but makes it easier to see when debugging
			validStored <= 1'b0;
		end
		else if(validIn) // If the data at the input is valid, perform transfer, keep data valid
		begin
			inputData <= dataIn;
			validInput <= 1'b1; // In theory at this point valid input should already be 1 so not needed
		end
		else 
		begin
			validInput <= 1'b0; // Otherwise data becomes invalid, already used
			inputData <= 0;
		end
	end
	else if(validIn & !validStored) // Third case, the stage cannot perform a transfer and it has valid data coming in and some place to store it
	begin
		storedData <= dataIn;
		validStored <= 1'b1; 
	end

	if(reset)
	begin
		validInput <= 1'b0;
		validStored <= 1'b0;
		inputData <= 0;
		storedData <= 0;
	end
end

endmodule

// A multi cycle version of Buffered Pipeline
module MultiCycleBufferedPipeline #(parameter
		DATA_END = 0,
		MAXIMUM_CYCLES_BITS = 8
	)(
		input wire validIn,
		input wire [DATA_END:0] dataIn,
		input wire busyIn,

		output wire busyOut,
		output wire [DATA_END:0] currentData,
		output wire validData,

		output wire [MAXIMUM_CYCLES_BITS-1:0] currentCycles,
		input  wire [MAXIMUM_CYCLES_BITS-1:0] maximumCycles,

		input wire increment,		// Assert when incrementing the counter. Almost the same as validOut, except more clear what is happening
		output wire memoryTransfer, // When this is asserted, this stage will be in the last counter before resetting back to zero
									// This takes into account the increment variable
		output wire earlyEnd,

		output wire validOut, 		// This stage takes care of the logic to decide when data is valid

		input clk,
		input reset
	);

reg [MAXIMUM_CYCLES_BITS-1:0] cycleCounter;
assign currentCycles = cycleCounter;
assign validOut = (validData & (earlyEnd | (cycleCounter == maximumCycles)));

// A early end is a 1 cycle gain that asserts when its possible to end one cycle early, instead of having to register the input an extra time when the next stage already registers that same input almost immediatly
// Basically, the data on the other cycles must be stored but in a early end the last data is directly sent instead

assign earlyEnd = ((cycleCounter + 1 == maximumCycles) & increment & !busyIn);

BufferedPipeline #(.DATA_END(DATA_END)) pipeline(
		.validIn(validIn),
		.dataIn(dataIn),
		.busyIn(busyIn),
		.validOut(earlyEnd | validOut), // The buffered pipeline is controlled by only asserting validOut when we have performed the last cycle and have all the data awaiting to move to the next stage
		.busyOut(busyOut),
		.currentData(currentData),
		.validData(validData),
		.memoryTransfer(memoryTransfer),
		.clk(clk),
		.reset(reset)
	);

always @(posedge clk)
begin
	if(increment)
	begin
		cycleCounter <= cycleCounter + 1;
	end
	
	if(reset | memoryTransfer | earlyEnd) // Memory transfer resets the cycle counter so its ready for the next stage
	begin
		cycleCounter <= 0;
	end
end

// Some assertions needed because outside logic must follow some rules
always @(posedge clk)
begin
	if(increment == 1'b1 && validData == 1'b0)
	begin
		$display("Outside logic is not taking validData into account, Increment cannot be asserted while validData is deasserted, in %m");
    	$finish;
	end

	if((cycleCounter == maximumCycles) && increment == 1'b1)
	begin
		$display("Increment is being asserted when we have reached the last cycle, in %m");
    	$finish;
	end
end

endmodule

// A Buffered Pipeline that implements a Finite State Machine
// The only thing is it manages the memory transfer process by noticing when the next state is zero
// Meaning that the FSM finished and it is going back to the initial state
// The change of state (from any state to state zero) only occurs if the next state is not busy
// At which point memoryTransfer is asserted indicating the change of data.

// The programming model is simple. State zero always checks if valid data is asserted, if not next state remains zero
// Otherwise next state changes to another state and the FSM begins.
// When on the last state, before changing to state zero (at which point a memory transfer occurs), the FSM machine can use memoryTransfer
// To perform an operation at the same cycle data is transfered 
module FSMBufferedPipeline #(parameter
		DATA_END = 0,
		MAXIMUM_STATE_BITS = 8
	)(
		input wire validIn,
		input wire [DATA_END:0] dataIn,
		input wire busyIn,

		output wire busyOut,
		output wire [DATA_END:0] currentData,
		output wire validData,

		output reg [MAXIMUM_STATE_BITS-1:0] currentState,
		input  wire [MAXIMUM_STATE_BITS-1:0] nextState,

		output wire memoryTransfer, // When this is asserted, the data is transfered.

		input wire validOut,

		input clk,
		input reset
	);

BufferedPipeline #(.DATA_END(DATA_END)) pipeline(
		.validIn(validIn),
		.dataIn(dataIn),
		.busyIn(busyIn),
		.validOut(validOut), 
		.busyOut(busyOut),
		.currentData(currentData),
		.validData(validData),
		.memoryTransfer(memoryTransfer),
		.clk(clk),
		.reset(reset)
	);

wire cannotProgress = (validOut & busyIn);

always @(posedge clk)
begin
	if(validData & !cannotProgress) // If next stage is busy we need to stall
	begin
		currentState <= nextState;
	end

	if(reset)
	begin
		currentState <= 0;
	end
end

endmodule