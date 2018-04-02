module debouncer(
	input CLK,
	input switch_input,
	output reg state,
	output trans_up,
	output trans_down
	);

// Synchronise input switch to clock
reg sync_0, sync_1;
always @(posedge CLK)
begin
	sync_0 = switch_input;
end

always @(posedge CLK)
begin
	sync_1 = sync_0;
end

// Debounce switch
reg [16:0] count;
wire idle = (state == sync_1);
wire finished = &count;

always @(posedge CLK)
begin
	if(idle)
	begin
		count <= 17'd0;
	end
	else
	begin
		count <= count + 17'd1;
		if(finished)
		begin
			state <= ~state;
		end
	end
end

assign trans_up = ~idle & finished & ~state;
assign trans_down = ~idle & finished & state;
	
endmodule
	