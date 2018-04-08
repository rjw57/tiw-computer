module video (
	input CLK,					// FPGA clock
	input RESET_n,
	
	input SEL,					// device select
	input A,						// register address
	input R_W_n,				// 1 - read, 0 - write
	input [7:0] DI,			// data bus in
	output [7:0] DO,			// data bus out

	input DOT_CLK, 			// video clock
	output reg HSYNC,				// horizontal sync output
	output reg VSYNC,				// vertical sync output
	output reg [4:0] VR,			// video red signal
	output reg [5:0] VG,			// video green signal
	output reg [4:0] VB,			// video blue signal
	
	output [12:0] VRAM_A,
	input [7:0] VRAM_Q
);


// Construct character clock from dot clock.
// The 6845 treats the high -> low transition as the active edge.
wire CHAR_CLK;
reg [2:0] dot_counter = 0;
assign CHAR_CLK = dot_counter[2];
always @(posedge DOT_CLK)
begin
	dot_counter <= dot_counter + 3'b1;
end

// 2KB Character ROM
wire [7:0] CHAR_ROM_Q;
wire [10:0] CHAR_ROM_A;
ROM #( .ADDR_WIDTH(13), .CONTENTS_FILE("./font/font.txt") ) ( .addr(CHAR_ROM_A), .clk(CLK), .q(CHAR_ROM_Q) );

// CRTC
wire CRTC_DE, CRTC_HS, CRTC_VS;
wire [4:0] RA;
wire [13:0] MA;
mc6845 (
	.LPSTB(1'b0), .nRESET(RESET_n), .CLOCK(CHAR_CLK), .CLKEN(1'b1),
	.ENABLE(SEL), .RS(A), .R_nW(R_W_n), .DI(DI), .DO(DO),
	.HSYNC(CRTC_HS), .VSYNC(CRTC_VS), .DE(CRTC_DE),
	.MA(MA), .RA(RA)
);

assign CHAR_ROM_A[2:0] = RA[3:1];
assign CHAR_ROM_A[10:3] = VRAM_Q;
assign VRAM_A = MA[12:0];

// Wait for half the character clock for retrieved character row to stabilise
reg DE;
always @(posedge CHAR_CLK)
begin
	char_row <= CHAR_ROM_Q;
	HSYNC <= CRTC_HS;
	VSYNC <= CRTC_VS;
	DE <= CRTC_DE;
end

wire px;
reg [7:0] char_row;
reg [2:0] char_col_counter = 0;
assign px = char_row[7-char_col_counter];

always @(posedge DOT_CLK)
begin
	if(~DE)
	begin
		VR <= 'b0;
		VG <= 'b0;
		VB <= 'b1;
	end
	else
	begin
		VR <= px ? 5'b11111 : 5'b0;
		VG <= px ? 6'b111111 : 6'b0;
		VB <= 5'b11111;
	end
end

always @(posedge DOT_CLK)
begin
	if(~DE)
	begin
		char_col_counter <= 0;
	end
	else
	begin
		char_col_counter <= char_col_counter + 1;
	end
end

endmodule
