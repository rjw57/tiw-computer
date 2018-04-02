module tiw (
	input CLK_48M, // 48MHz FPGA clock
	input [3:0] KEY,
	
	output reg [7:0] SEG,
	output reg [3:0] DIGIT
	);

wire CLK_4M;
	
wire PHI2;
wire RESET;
wire [15:0] A;
wire [7:0] DI; // data in to the CPU
wire [7:0] DO; // date out from the CPU
wire R_W_n; // 1 - CPU reading, 0 - CPU writing
wire SYNC;

wire [3:0] DEBOUNCED_KEY;

assign PHI2 = DEBOUNCED_KEY[0] ? CLK_4M : ~DEBOUNCED_KEY[2];

pll (.inclk0(CLK_48M), .c0(CLK_4M));

debouncer (.CLK(CLK_48M), .switch_input(KEY[0]), .state(DEBOUNCED_KEY[0]));
debouncer (.CLK(CLK_48M), .switch_input(KEY[1]), .state(DEBOUNCED_KEY[1]));
debouncer (.CLK(CLK_48M), .switch_input(KEY[2]), .state(DEBOUNCED_KEY[2]));
debouncer (.CLK(CLK_48M), .switch_input(KEY[3]), .state(DEBOUNCED_KEY[3]));

reset ( .CLK(PHI2), .reset_req(~DEBOUNCED_KEY[3]), .reset(RESET) );

wire [7:0] display_seg;
wire [3:0] display_digit;
display_7_seg (
	.CLK(CLK_48M),
	.value(
		DEBOUNCED_KEY[1] ? A : (R_W_n ? {8'b0, DI} : {8'b0, DO})
	),
	.seg(display_seg),
	.digit(display_digit)
);

always @(posedge CLK_48M)
begin
	SEG[7:1] <= display_seg[7:1];
	DIGIT <= display_digit;
	case(display_digit)
		4'b1110: SEG[0] <= R_W_n;
		4'b1101: SEG[0] <= SYNC;
		default: SEG[0] <= 1'b0;
	endcase
end

wire [23:0] A_FULL;
assign A = A_FULL[15:0];

T65 (
	.Mode(2'b00), // 6502
	.Res_n(~RESET),
	.Clk(PHI2),
	.Rdy(1'b1),
	.Abort_n(1'b1),
	.IRQ_n(1'b1),
	.NMI_n(1'b1),
	.SO_n(1'b1),
	
	.A(A_FULL),
	.DI(DI),
	.DO(DO),
	
	.R_W_n(R_W_n),
	.Sync(SYNC)
	
//		EF		: out std_logic;
//		MF		: out std_logic;
//		XF		: out std_logic;
//		ML_n	: out std_logic;
//		VP_n	: out std_logic;
//		VDA		: out std_logic;
//		VPA		: out std_logic;
);

IO (
	.CLK(CLK_48M),
	.PHI2(PHI2),
	.A(A),
	.DI(DO),
	.DO(DI),
	.R_W_n(R_W_n)
);
	
endmodule

module IO (
	input CLK,			// FPGA clock
	input PHI2,			// processor clock
	input [15:0] A,	// address bus
	input [7:0] DI,	// data bus input
	input R_W_n,		// 1 - read from IO, 0 - write to IO
	
	output reg [7:0] DO	// data bus output
	);

// 8KB ROM
wire ROM_SEL;
wire [7:0] ROM_Q;
assign ROM_SEL = A[15:13] == 3'b111;
ROM #( .ADDR_WIDTH(13), .CONTENTS_FILE("../os/rom.txt") ) ( .addr(A[12:0]), .clk(CLK), .q(ROM_Q) );

// 8KB RAM
wire RAM_SEL;
wire [7:0] RAM_Q;
assign RAM_SEL = A[15:13] == 3'b000;
RAM #( .ADDR_WIDTH(13) ) ( .data(DI), .addr(A[12:0]), .clk(CLK), .we(RAM_SEL & ~R_W_n), .q(RAM_Q) );

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
end
	
endmodule
