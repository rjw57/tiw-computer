module IO (
	input CLK,				// FPGA clock
	input PHI2,				// processor clock
	input RESET_n,			// inverted reset signal
	input [15:0] A,		// address bus
	input [7:0] DI,		// data bus input
	input R_W_n,			// 1 - read from IO, 0 - write to IO
	
	input UART_IN,			// input signal to UART
	output UART_OUT,		// output from UART
	
	output reg [7:0] DO	// data bus output
	);

wire Rd_n;
wire Wr_n;

assign Rd_n = ~R_W_n;
assign Wr_n = R_W_n;
	
// 8KB ROM
wire ROM_SEL;
wire [7:0] ROM_Q;
assign ROM_SEL = A[15:13] == 3'b111;
ROM #( .ADDR_WIDTH(13), .CONTENTS_FILE("../os/rom.txt") ) ( .addr(A[12:0]), .clk(CLK), .q(ROM_Q) );

// 8KB RAM
wire RAM_SEL;
wire [7:0] RAM_Q;
assign RAM_SEL = A[15:13] == 3'b000;
RAM #( .ADDR_WIDTH(13) ) ( .data(DI), .addr(A[12:0]), .clk(CLK), .we(RAM_SEL & (~Wr_n)), .q(RAM_Q) );

// UART
wire BaudOut;
wire UART_SEL;
wire [7:0] UART_Q;
assign UART_SEL = A[15:13] == 3'b110;
T16450 (
	.MR_n(RESET_n), .XIn(PHI2), .RClk(BaudOut),
	.CS_n(~UART_SEL), .Rd_n(Rd_n), .Wr_n(Wr_n),
	.A(A[2:0]), .D_In(DI), .D_Out(UART_Q),
	.SIn(UART_IN), .SOut(UART_OUT),
	.BaudOut(BaudOut)
);

// Mux data lines
always @(posedge CLK)
begin
	if(ROM_SEL)
	begin
		DO <= ROM_Q;
	end
	else if(RAM_SEL)
	begin
		DO <= RAM_Q;
	end
	else if(UART_SEL)
	begin
		DO <= UART_Q;
	end
	else
		DO <= 8'hA5;
end
	
endmodule
