// Quartus II Verilog Template
// Single Port ROM

module ROM #(
	parameter DATA_WIDTH=8, parameter ADDR_WIDTH=8, parameter CONTENTS_FILE="rom.txt"
	)(
	input [(ADDR_WIDTH-1):0] addr,
	input clk, 
	output reg [(DATA_WIDTH-1):0] q
	);

	// Declare the ROM variable
	reg [DATA_WIDTH-1:0] rom[2**ADDR_WIDTH-1:0];

	// Initialize the ROM with $readmemb.  Put the memory contents
	// in the file single_port_rom_init.txt.  Without this file,
	// this design will not compile.

	// See Verilog LRM 1364-2001 Section 17.2.8 for details on the
	// format of this file, or see the "Using $readmemb and $readmemh"
	// template later in this section.

	initial
	begin
		$readmemh(CONTENTS_FILE, rom);
	end

	always @ (posedge clk)
	begin
		q <= rom[addr];
	end

endmodule
