module IO (
	input CLK,				// FPGA clock
	input PHI2,				// processor clock
	input UART_CLK,		// UART clock
	input DOT_CLK, 		// video clock
	
	input RESET_n,			// inverted reset signal
	input [15:0] A,		// address bus
	input [7:0] DI,		// data bus input
	input R_W_n,			// 1 - read from IO, 0 - write to IO
	
	input UART_IN,			// input signal to UART
	output UART_OUT,		// output from UART
	
	output HSYNC,			// horizontol sync output
	output VSYNC,			// vertical sync output
	output [4:0] VR,		// video red signal
	output [5:0] VG,		// video green signal
	output [4:0] VB,		// video blue signal
	
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
// UART at $D000-$D007
assign UART_SEL = A[15:3] == 13'b1101_0000_0000_0;
T16450 (
	.MR_n(RESET_n), .XIn(UART_CLK), .RClk(BaudOut),
	.CS_n(~UART_SEL), .Rd_n(Rd_n), .Wr_n(Wr_n),
	.A(A[2:0]), .D_In(DI), .D_Out(UART_Q),
	.SIn(UART_IN), .SOut(UART_OUT),
	.BaudOut(BaudOut)
);


// Video clocks
wire CHAR_CLK;
reg [2:0] counter = 0;
assign CHAR_CLK = counter[2];
always @(posedge DOT_CLK)
begin
	counter <= counter + 3'b1;
end

// CRTC
wire CRTC_SEL;
wire CRTC_DE;
wire [7:0] CRTC_Q;
// CRTC at $D010-$D011
assign CRTC_SEL = A[15:1] == 15'b1101_0000_0001_000;

wire [4:0] RA;
wire [13:0] MA;

/*
crtc6845 (
	.LPSTBn(1'b0),	.RESETn(RESET_n),	.REG_INIT(1'b0),
	.CLK(CHAR_CLK),
	.E(CLK), .RS(A[0]), .CSn(~(CRTC_SEL & PHI2)), .RW(R_W_n), .DI(DI), .DO(CRTC_Q),
	.HSYNC(HSYNC), .VSYNC(VSYNC), .DE(CRTC_DE),
	.MA(MA), .RA(RA)
);
*/

mc6845 (
	.LPSTB(1'b0), .nRESET(RESET_n), .CLOCK(CHAR_CLK), .CLKEN(1'b1),
	.ENABLE(CRTC_SEL), .RS(A[0]), .R_nW(R_W_n), .DI(DI), .DO(CRTC_Q),
	.HSYNC(HSYNC), .VSYNC(VSYNC), .DE(CRTC_DE),
	.MA(MA), .RA(RA)
);

assign VR = CRTC_DE ? RA : 5'b0;
assign VG = CRTC_DE ? MA[5:0] : 6'b0;
assign VB = CRTC_DE ? 5'b00000 : 5'b0;

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
	else if(CRTC_SEL)
	begin
		DO <= CRTC_Q;
	end
	else
		DO <= 8'hA5;
end
	
endmodule
