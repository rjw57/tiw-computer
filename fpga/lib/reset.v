module reset (
	input CLK,
	input reset_req,
	output reg reset = 1'b1
	);
	
reg [1:0] counter = 2'b00;

always @(posedge CLK, posedge reset_req)
begin
	if(reset_req)
	begin
		counter <= 2'b00;
		reset <= 1'b1;
	end
	else
	begin
		if(counter !== 2'b11)
		begin
			counter <= counter + 2'b01;
			reset <= 1'b1;
		end
		else
			reset <= 1'b0;
	end
end
	
endmodule
