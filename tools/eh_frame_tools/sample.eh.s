.equ fde1_start_addr, 0x601234
.equ fde2_start_addr, 0x605678

.equ fde1_cs1, fde1_start_addr+0x0a
.equ fde1_cs1_len, 0x0b
.equ fde1_cs1_lp, fde1_cs1+fde1_cs1_len+0x0c

.equ fde1_cs2, fde1_start_addr+0x1a
.equ fde1_cs2_len, 0x1b
.equ fde1_cs2_lp, fde1_cs2+fde1_cs2_len+0x1c

.equ fde2_cs1, fde2_start_addr+0x2a
.equ fde2_cs1_len, 0x2b
.equ fde2_cs1_lp, fde2_cs1+fde1_cs1_len+0x2c

.equ fde2_cs2, fde2_start_addr+0x3a
.equ fde2_cs2_len, 0x3b
.equ fde2_cs2_lp, fde2_cs2+fde1_cs2_len+0x3c


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

	# CIE program
	.byte 0x0e, 0x10,0x0e, 0x18,0x0f, 0x0b, 0x77, 0x08, 0x80, 0x00, 0x3f, 0x1a, 0x3b, 0x2a, 0x33, 0x24, 0x22

	# pad with nops
	.align 4, 0
Lcie1_end:

#fde 1
Lfde1:
	.int Lfde1_end - Lfde1 - 4  	# length of this record. -4 because length doesn't include this field.
	.int . - Lcie1  	   	# this is an FDE (not a cie), and it's cie is CIE1.  byte offset from start of field.
	.int 0x601234 - . 		# FDE start addr
	.int 0xdead 			# fde range length (i.e., can calc the fde_end_addr from this -- note that pcrel is ignored here!)
	#encode Z (length)
	.uleb128 Lfde1_aug_data_end-Lfde1_aug_data_start
Lfde1_aug_data_start:
	#encode L (LSDA) 
	.int LSDA1	# LSDA hard coded here
Lfde1_aug_data_end:

	# FDE1 program
	.byte 0x0e, 0x10,0x0e, 0x18,0x0f, 0x0b, 0x77, 0x08, 0x80, 0x00, 0x3f, 0x1a, 0x3b, 0x2a, 0x33, 0x24, 0x22
	.align 4, 0
	Lfde1_end:



#fde 2
Lfde2:
	.int Lfde2_end - Lfde2 - 4   	# length of this record. -4 because length doesn't include this field
	.int . - Lcie1 			# this is an FDE (not a cie), and it's cie is CIE1.  byte offset from start of field.
	.int 0x605678 - . 		# FDE start addr
	.int 0xbeef 			# fde range length (i.e., can calc the fde_end_addr from this -- note that pcrel is ignored here!)
	#encode Z (length)
	.uleb128 Lfde1_aug_data_end-Lfde1_aug_data_start
Lfde2_aug_data_start:
	#encode L (LSDA) 
	.int LSDA2	# LSDA hard coded here
Lfde2_aug_data_end:

	# FDE2 program
	.byte 0x0e, 0x10,0x0e, 0x18,0x0f, 0x0b, 0x77, 0x08, 0x80, 0x00, 0x3f, 0x1a, 0x3b, 0x2a, 0x33, 0x24, 0x22
	.align 8, 0
Lfde2_end:


.section gcc_except_table, "a", @progbits

LSDA1:

	# 1) encoding of next field 
	.byte 0xff # DW_EH_PE_omit (0xff)

	# 2) landing pad base, if omitted, use FDE start addr
	# .<fdebasetype> <fdebase> -- omitted.  

	# 3) encoding of type table entries
	.byte 0x3  # DW_EH_PE_udata4

	# 4) type table pointer -- always a uleb128
	.uleb128 LSDA1_type_table_end - LSDA1_tt_ptr_end
LSDA1_tt_ptr_end:

	# 5) call site table encoding
	.byte 0x1 # DW_EH_PE_uleb128 

	# 6) the length of the call site table
	.uleb128 LSDA1_cs_tab_end-LSDA1_cs_tab_start

LSDA1_cs_tab_start:
LSDA1_cs_tab_entry1_start:
	# 1) start of call site relative to FDE start addr
	.uleb128 fde1_cs1 - fde1_start_addr
	# 2) length of call site
	.uleb128 fde1_cs1_len
	# 3) the landing pad, or 0 if none exists.
	.uleb128 fde1_cs1_lp - fde1_start_addr
	# 4) index into action table + 1 -- 0 indicates unwind only
	.uleb128  0
LSDA1_cs_tab_entry1_end:
LSDA1_cs_tab_entry2_start:
	# 1) start of call site relative to FDE start addr
	.uleb128 fde1_cs2 - fde1_start_addr
	# 2) length of call site
	.uleb128 fde1_cs2_len
	# 3) the landing pad, or 0 if none exists.
	.uleb128 fde1_cs2_lp - fde1_start_addr
	# 4) index into action table + 1 -- 0 indicates unwind only
	.uleb128  1 + LSDA1_act2_start - LSDA1_action_tab_start
LSDA1_cs_tab_entry2_end:
LSDA1_cs_tab_end:

LSDA1_action_tab_start:

LSDA1_act1_start:
	.uleb128 2
	.uleb128 0
LSDA1_act1_end:
LSDA1_act2_start:
	.uleb128 1
	.uleb128 LSDA1_act1_start - .
LSDA1_act2_end:
LSDA1_action_tab_end:

LSDA1_type_table_start:
	.int 0x902200
	.int 0
LSDA1_type_table_end:
LSDA1_end:


LSDA2:
	# 1) encoding of next field 
	.byte 0xff # DW_EH_PE_omit (0xff)

	# 2) landing pad base, if omitted, use FDE start addr
	# .<fdebasetype> <fdebase> -- omitted.  

	# 3) encoding of type table entries
	.byte 0x3  # DW_EH_PE_udata4

	# 4) type table pointer
	.uleb128 LSDA2_type_table_end - LSDA2_tt_ptr_end
LSDA2_tt_ptr_end:

	# 5) call site table encoding
	.byte 0x1 # DW_EH_PE_uleb128 

	# 6) the length of the call site table
	.uleb128 LSDA2_cs_tab_end-LSDA2_cs_tab_start

LSDA2_cs_tab_start:
LSDA2_cs_tab_entry1_start:
	# 1) start of call site relative to FDE start addr
	.uleb128 fde1_cs1 - fde1_start_addr
	# 2) length of call site
	.uleb128 fde1_cs1_len
	# 3) the landing pad, or 0 if none exists.
	.uleb128 fde1_cs1_lp - fde2_start_addr
	# 4) index into action table + 1 -- 0 indicates unwind only
	.uleb128  0
LSDA2_cs_tab_entry1_end:
LSDA2_cs_tab_entry2_start:
	# 1) start of call site relative to FDE start addr
	.uleb128 fde1_cs2 - fde1_start_addr
	# 2) length of call site
	.uleb128 fde1_cs2_len
	# 3) the landing pad, or 0 if none exists.
	.uleb128 fde1_cs2_lp - fde2_start_addr
	# 4) index into action table + 1 -- 0 indicates unwind only
	.uleb128  1 + LSDA2_act2_start - LSDA2_action_tab_start
LSDA2_cs_tab_entry2_end:
LSDA2_cs_tab_end:

LSDA2_action_tab_start:

LSDA2_act1_start:
	.uleb128 2
	.uleb128 0
LSDA2_act1_end:
LSDA2_act2_start:
	.uleb128 1
	.uleb128 LSDA2_act1_start - .
LSDA2_act2_end:
LSDA2_action_tab_end:

LSDA2_type_table_start:
	.int 0x902200
	.int 0
LSDA2_type_table_end:
LSDA2_end:
