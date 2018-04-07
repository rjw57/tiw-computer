module reset (
	input CLK,
	input reset_req,
	output reg reset = 1'b1
	);
	
reg [8:0] counter = 9'b0;

always @(posedge CLK, posedge reset_req)
begin
	if(reset_req)
	begin
		counter <= 'b0;
		reset <= 1'b1;
	end
	else
	begin
		if(&counter)
		begin
			reset <= 1'b0;
		end
		else
		begin
			counter <= counter + 1;
			reset <= 1'b1;
		end
	end
end
	
endmodule
