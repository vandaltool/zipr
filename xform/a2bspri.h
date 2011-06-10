/*
 * a2bspri.h - header file
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Anh Nguyen-Tuong
 *
 */

#ifndef a2bspri_h
#define a2bspri_h

#include "all.h"
#include "aspri.h"

void a2bspri(FILE *aspriInputFile, FILE *bspriOutputFile);
void aspri_get_address(FILE* f, aspri_address_t *from_address);
void discard_to_eol_or_eof(FILE* f);
void discard_whitespace(FILE* f);
void adjust_aspri_address(aspri_address_t *from_address, app_iaddr_t virtualPC);
void call_assembler(char *asm_instr, int *size, unsigned char *bin_instr);
void aspri_fatal(char *msg, int line);
void emit_spri_mapping(FILE *output, aspri_address_t from_address, aspri_address_t to_address);
void emit_spri_instruction(FILE *output, int virtualPC, int size, unsigned char *bin_instr, char *asm_instr);

#endif
