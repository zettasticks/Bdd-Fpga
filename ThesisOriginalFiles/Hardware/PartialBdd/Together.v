`timescale 1 ps / 1 ps

`include "Constants.v"

`define CLK_EDGE \
		#1 clk = 1; \
		#1 clk = 0

module Together(
		output wire [25:0] avm_node_address,
		output wire avm_node_read,
		input wire [15:0] avm_node_readdata,
		input wire avm_node_readdatavalid,
		output wire avm_node_write,
		output wire [15:0] avm_node_writedata,
		output wire [5:0] avm_node_burstcount,
		input wire avm_node_waitrequest,

		input wire [15:0] avs_config_address,
		output wire avs_config_waitrequest,
		input wire avs_config_write,
		input wire [31:0] avs_config_writedata,
		input wire avs_config_read,
		output wor [31:0] avs_config_readdata,
		output wor avs_config_readdatavalid,

		input wire dma_irq,
		output wire hps_irq,

		input wire CLOCK_50,
		input wire reset
	);

wire [15:0] asi_var_data;
wire asi_var_ready;
wire asi_var_valid;

wire [95:0] asi_result_data;
wire asi_result_channel;
wire asi_result_ready;
wire asi_result_valid;

wire [127:0] asi_requests_data;
wire asi_requests_channel;
wire asi_requests_ready;
wire asi_requests_valid;

wire mReadValid,cReadValid;
wire [31:0] mReadData,cReadData;

assign avs_config_readdatavalid = mReadValid;
assign avs_config_readdatavalid = cReadValid;

assign avs_config_readdata = mReadData;
assign avs_config_readdata = cReadData;

MemoryAccess m(
		.avm_node_address(avm_node_address),
		.avm_node_read(avm_node_read),
		.avm_node_readdata(avm_node_readdata),
		.avm_node_readdatavalid(avm_node_readdatavalid),
		.avm_node_write(avm_node_write),
		.avm_node_writedata(avm_node_writedata),
		.avm_node_burstcount(avm_node_burstcount),
		.avm_node_waitrequest(avm_node_waitrequest),
		.avs_config_address(avs_config_address),
		.avs_config_write(avs_config_write),
		.avs_config_writedata(avs_config_writedata),
		.avs_config_read(avs_config_read),
		.avs_config_readdata(mReadData),
		.avs_config_waitrequest(avs_config_waitrequest),
		.avs_config_readdatavalid(mReadValid),
		.asi_requests_data(asi_requests_data),
		.asi_requests_channel(asi_requests_channel),
		.asi_requests_ready(asi_requests_ready),
		.asi_requests_valid(asi_requests_valid),
		.hps_irq(hps_irq),
		.dma_irq(dma_irq),
		.aso_var_data(asi_var_data),
		.aso_var_ready(asi_var_ready),
		.aso_var_valid(asi_var_valid),
		.aso_result_data(asi_result_data),
		.aso_result_ready(asi_result_ready),
		.aso_result_valid(asi_result_valid),
		.clk(CLOCK_50),
		.reset(reset)
	);

CoPartial c(
		.avs_config_address(avs_config_address),
		.avs_config_waitrequest(),
		.avs_config_write(avs_config_write),
		.avs_config_writedata(avs_config_writedata),
		.avs_config_read(avs_config_read),
		.avs_config_readdata(cReadData),
		.avs_config_readdatavalid(cReadValid),
		.avs_co_write(avs_co_write),
		.avs_co_writedata(avs_co_writedata),
		.avs_co_read(avs_co_read),
		.avs_co_readdata(avs_co_readdata),
		.avs_co_waitrequest(avs_co_waitrequest),
		.avs_co_byteenable(avs_co_byteenable),
		.aso_var_data(aso_var_data),
		.aso_var_ready(aso_var_ready),
		.aso_var_valid(aso_var_valid),
		.aso_cache_data(aso_cache_data),
		.aso_cache_ready(aso_cache_ready),
		.aso_cache_valid(aso_cache_valid),
		.asi_result_data(asi_result_data),
		.asi_result_channel(asi_result_channel),
		.asi_result_ready(asi_result_ready),
		.asi_result_valid(asi_result_valid),
		.asi_var_data(asi_var_data),
		.asi_var_ready(asi_var_ready),
		.asi_var_valid(asi_var_valid),
		.clk(CLOCK_50),
		.reset(reset)
	);

endmodule