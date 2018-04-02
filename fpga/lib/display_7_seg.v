module display_7_seg(
	input CLK,
	input [15:0] value,
	
	output [7:0] seg,
	output reg [3:0] digit
	);
	
reg [1:0] cur_digit = 2'b0;
reg [11:0] counter = 12'b0;
reg [7:0] slice;

always @(posedge CLK)
begin
	counter <= counter + 12'd1;
	if(&counter)
	begin
		cur_digit <= cur_digit + 2'b01;
		case(cur_digit)
			2'b00: begin slice <= value[3:0]; digit <= 4'b1110; end
			2'b01: begin slice <= value[7:4]; digit <= 4'b1101; end
			2'b10: begin slice <= value[11:8]; digit <= 4'b1011; end
			2'b11: begin slice <= value[15:12];	digit <= 4'b0111; end
		endcase
	end
end

decoder_7_seg( .CLK(CLK), .D(slice), .SEG(seg) );
	
endmodule
