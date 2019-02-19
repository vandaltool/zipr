

SECTIONS
{
	eh_frame_hdr : 
	{
		*(eh_frame_hdr)
	}

	eh_frame : 
	{
		*(eh_frame)
	}

	gcc_except_table : 
	{
		*(gcc_except_table)
	}
}
