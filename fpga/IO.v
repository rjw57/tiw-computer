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
wire [12:0] VRAM_A;
wire [7:0] VRAM_Q;
assign RAM_SEL = A[15:13] == 3'b000;
RAM #( .ADDR_WIDTH(13) ) (
	.data_a(DI), .addr_a(A[12:0]), .clk_a(CLK), .we_a(RAM_SEL & (~Wr_n)), .q_a(RAM_Q),
	.data_b(8'b0), .addr_b(VRAM_A), .clk_b(CLK), .we_b(1'b0), .q_b(VRAM_Q)
);

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

// CRTC at $D010-$D011
wire CRTC_SEL;
wire [7:0] CRTC_Q;
assign CRTC_SEL = A[15:1] == 15'b1101_0000_0001_000;

video (
	.CLK(CLK),
	.RESET_n(RESET_n),
	.SEL(CRTC_SEL), .A(A[0]), .R_W_n(R_W_n), .DI(DI), .DO(CRTC_Q),
	.DOT_CLK(DOT_CLK),
	.VRAM_A(VRAM_A), .VRAM_Q(VRAM_Q),
	.HSYNC(HSYNC), .VSYNC(VSYNC), .VR(VR), .VG(VG), .VB(VB)
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
	else if(CRTC_SEL)
	begin
		DO <= CRTC_Q;
	end
	else
		DO <= 8'hA5;
end
	
endmodule
