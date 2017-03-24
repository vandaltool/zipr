.section eh_frame_hdr, "a",@progbits

eh_frame_hdr_start:
	.byte 1 	  	# version
	.byte 0x10 | 0x0B 	# encoding for pointer to eh-frame -- DH_EH_PE_pcrel (0x10) | DH_EH_PE_sdata4 (0x0B)
	.byte 0x03 		# encoding for ; of entries in eh-frame-hdr  -- BDH_EH_PE_udata4 (0x03)
	.byte 0x10 | 0x0B 	# encoding for pointers (to fdes) held in the eh-frame-hdr header  -- DH_EH_PE_pcrel | DH_EH_PE_sdata4
	.int Lfde_table - . 	# pointer to fde_table, encoded as an sdata4, pcrel
	.byte (eh_frame_table_end-eh_frame_table)/4	# number of FDEs in the header.
	.align 4
eh_frame_table:
	.int Lfde1 - . # fde pointers
	.int Lfde2 - .
eh_frame_table_end:


.section eh_frame, "a", @progbits

Lfde_table:

# cie 1
Lcie1:
.int Lcie1_end - Lcie1 - 4 # length of this record. -4 because length doesn't include this field
.int 0			# cie (not fde)
.byte 3 		# version
.asciz "zPLR"		# aug string.
.uleb128 1		# code alignment factor
.sleb128 -8		# data alignment factor
.uleb128 16		# return address reg.

Lcie1_aug_data_start:
# encode the Z (length)
.sleb128 Lcie1_aug_data_end-Lcie1_aug_data_start # Z -- handle length field

#encode the P (personality encoding + personality routine)
.byte 0x80 | 0x10 | 0x0B 	#  personality pointer encoding DH_EH_PE_indirect (0x80) | pcrel | sdata4
.int 0x900aa0 - .		# actual personality routine, encoded as noted in prev line.

# encode L (lsda encoding) 
.byte  0x03	# LSDA encoding (udata4 -- or maybe later pcrel|sdata4 ? )

# encode R (FDE encoding) 
.byte  0x10 | 0x0B 	# FDE encoding (pcrel | sdata4)
Lcie1_aug_data_end:

.byte 0x0e, 0x10,0x0e, 0x18,0x0f, 0x0b, 0x77, 0x08, 0x80, 0x00, 0x3f, 0x1a, 0x3b, 0x2a, 0x33, 0x24, 0x22
.align 4, 0
Lcie1_end:

#fde 1
Lfde1:
.int Lfde1_end - Lfde1 -4  	# length of this record. -4 because length doesn't include this field.
.int . - Lcie1  	   	# this is an FDE (not a cie), and it's cie is CIE1.  byte offset from start of field.
.int 0x601234 - . 		# FDE start addr
.int 0xdead 			# fde range length (i.e., can calc the fde_end_addr from this -- note that pcrel is ignored here!)
#encode Z (length)
.uleb128 Lfde1_aug_data_end-Lfde1_aug_data_start
Lfde1_aug_data_start:
#encode L (LSDA) 
.int 0x900fec	# LSDA hard coded here
Lfde1_aug_data_end:
.byte 0x0e, 0x10,0x0e, 0x18,0x0f, 0x0b, 0x77, 0x08, 0x80, 0x00, 0x3f, 0x1a, 0x3b, 0x2a, 0x33, 0x24, 0x22
.align 4, 0
Lfde1_end:


#fde 1
Lfde2:
.int Lfde2_end - Lfde2 -4   	# length of this record. -4 because length doesn't include this field
.int . - Lcie1 			# this is an FDE (not a cie), and it's cie is CIE1.  byte offset from start of field.
.int 0x605678 - . 		# FDE start addr
.int 0xbeef 			# fde range length (i.e., can calc the fde_end_addr from this -- note that pcrel is ignored here!)
#encode Z (length)
.uleb128 Lfde1_aug_data_end-Lfde1_aug_data_start
Lfde2_aug_data_start:
#encode L (LSDA) 
.int 0x900fec	# LSDA hard coded here
Lfde2_aug_data_end:
.byte 0x0e, 0x10,0x0e, 0x18,0x0f, 0x0b, 0x77, 0x08, 0x80, 0x00, 0x3f, 0x1a, 0x3b, 0x2a, 0x33, 0x24, 0x22
.align 8, 0
Lfde2_end:

