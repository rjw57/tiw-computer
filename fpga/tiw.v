module tiw (
	input CLK_48M, // 48MHz FPGA clock
	input [3:0] KEY,
	
	input UART_IN,
	output UART_OUT,
	
	output reg [7:0] SEG,
	output reg [3:0] DIGIT
	);

// Clocks
wire CLK_6_144M;
wire CLK_4M;
pll (.inclk0(CLK_48M), .c0(CLK_6_144M), .c1(CLK_4M));
	
wire PHI2;			// processor clock
wire RESET;			// reset signal
wire [15:0] A;		// address bus
wire [7:0] DI;		// data in to the CPU
wire [7:0] DO; 	// data out from the CPU
wire R_W_n;			// 1 - CPU reading, 0 - CPU writing
wire SYNC;			// 1 - start of instruction fetch

// Input key debouncing
wire [3:0] DEBOUNCED_KEY;
debouncer (.CLK(CLK_48M), .switch_input(KEY[0]), .state(DEBOUNCED_KEY[0]));
debouncer (.CLK(CLK_48M), .switch_input(KEY[1]), .state(DEBOUNCED_KEY[1]));
debouncer (.CLK(CLK_48M), .switch_input(KEY[2]), .state(DEBOUNCED_KEY[2]));
debouncer (.CLK(CLK_48M), .switch_input(KEY[3]), .state(DEBOUNCED_KEY[3]));

// Processor clock source for single stepping
assign PHI2 = DEBOUNCED_KEY[0] ? CLK_4M : ~DEBOUNCED_KEY[2];

// Reset key
reset ( .CLK(PHI2), .reset_req(~DEBOUNCED_KEY[3]), .reset(RESET) );

// Address/data bus display
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

// Status flag display
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

// CPU
wire [23:0] A_FULL;
assign A = A_FULL[15:0];
T65 (
	.Mode(2'b00), // 6502
	.Res_n(~RESET), .Clk(PHI2), .Rdy(1'b1),
	.Abort_n(1'b1), .IRQ_n(1'b1), .NMI_n(1'b1), .SO_n(1'b1),
	
	.A(A_FULL), .DI(DI), .DO(DO), .R_W_n(R_W_n),
	
	.Sync(SYNC)
);

// IO
IO (
	.CLK(CLK_48M),	.PHI2(PHI2), .UART_CLK(CLK_6_144M), .RESET_n(~RESET),
	.A(A), .DI(DO), .DO(DI), .R_W_n(R_W_n),
	
	.UART_IN(UART_IN), .UART_OUT(UART_OUT)
);
	
endmodule
