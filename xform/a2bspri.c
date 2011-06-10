/*
 * a2bspri.c - convert assembly SPRI format to binary SPRI format
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Anh Nguyen-Tuong
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>


#include "a2bspri.h"

/*
 * Convert from assembly spri to binary spri file format
 */
void a2bspri(FILE *f, FILE *output)
{
#define BREAK_IF_EOF(f) if(feof(f)) break;
	aspri_address_t from_address, to_address;
	char type[100];

	unsigned int virtualPC = 0xFF000000;

	/* read each line of the file */
	int line=0;
	while(!feof(f))
	{
		/* read an address */
		aspri_get_address(f, &from_address);
		BREAK_IF_EOF(f);

//		fprintf(stderr,"line %d: from_address: 0x%x\n", line, from_address.offset);

		/* and a type */
		fscanf(f, "%10s", type);

		BREAK_IF_EOF(f);
	
		/* address mapping */
		if (strcmp(type, "->")==0)
		{
			/* read target address */
			aspri_get_address(f, &to_address);
			BREAK_IF_EOF(f);

			if (from_address.isCurrentPC)
			{
				adjust_aspri_address(&from_address, virtualPC);
				virtualPC++;
			}

			if (to_address.isCurrentPC)
			{
				fprintf(output,"\n");
				adjust_aspri_address(&to_address, virtualPC);
			}

			emit_spri_mapping(output, from_address, to_address);
			discard_to_eol_or_eof(f);
		}
		/* instruction mapping */
		else if (strcmp(type, "**")==0)
		{
			/* 
				format is:
				. ** <assembly isntruction>
			*/
			char asm_instr[2048];
			unsigned char bin_instr[2048];
			int size;

			fgets(asm_instr, 1024, f);

			/* pass the assembly to nasm, get the actual binary + size out */
			call_assembler(asm_instr, &size, bin_instr);

			/* emit SPRI rule */
			emit_spri_instruction(output, virtualPC, size, bin_instr, asm_instr);

			/* advance virtual PC */
			virtualPC += size;
		}
		/* check for comment */
		else if(type[0]=='#')
		{
			discard_to_eol_or_eof(f);
			BREAK_IF_EOF(f);
		}
		/* unknown type */
		else
		{
			aspri_fatal("SPRI input file is malformed", line);
		}

		line++;
	}
}

/*
 * aspri_get_address - read an address into from_address 
 */
void aspri_get_address(FILE* f, aspri_address_t *from_address)
{
	int c = 0;

 	discard_whitespace(f);

	c = fgetc(f);

	if (c != '.')
	{
		ungetc(c, f);
		from_address->library_name = strdup("a.out");
		fscanf(f,"%x", &from_address->offset);
	}
	else
	{
		from_address->library_name = strdup("a.out");
		from_address->offset = 0;
		from_address->isCurrentPC = true;
	}
}


/*
 * discard_to_eol_or_eof - discard characters from file f until EOL or EOF is reached.
 */
void discard_to_eol_or_eof(FILE* f)
{
  int c=0;
  /* discard until EOL or EOF */
  while(c!=EOF && c!='\n')
    c=fgetc(f);
}

/*
 * discard_whitespace - discard whitespace from file f until a non-whitespace is reached
 *
 * whitespace is defined to be a space or a tab in this case
 */
void discard_whitespace(FILE* f)
{
  int c = 0;
  /* discard until non whitespace */
  do {
    c = fgetc(f);
  }
  while(c == ' ' || c == '\t');
  ungetc(c, f);
}

/*
 * If "current PC" specified, need to concretize it with an address
 */
void adjust_aspri_address(aspri_address_t *from_address, app_iaddr_t virtualPC)
{
  if (from_address->isCurrentPC)
  {
    from_address->offset = virtualPC;
    from_address->isCurrentPC = false;
  }
}

/*
*  asm_instr:  the assembly instruction 
*  size     :  size of compiled assembly instruction
*  bin_instr:  resulting binary encoding of assembly instruction
*/
void call_assembler(char *asm_instr, int *size, unsigned char *bin_instr)
{
  char inputfile[1024], outputfile[1024];

  sprintf(inputfile,"apsri.in.%d", getpid());
  sprintf(outputfile,"aspri.out.%d", getpid());

  FILE* asmfile = fopen(inputfile, "w+");
  assert(asmfile != NULL);
  fprintf(asmfile, "BITS 32\n%s\n", asm_instr);
  fclose(asmfile);

//  fprintf(stderr,"Calling nasm with input instruction: %s\n", asm_instr);

  // invoke nasm assembler
  char cmd[2048];
  sprintf(cmd,"nasm -f bin -O2 %s -o %s", inputfile, outputfile);
  int retval = system(cmd);
  assert(retval == 0);

  // output file should contain the binary code
  FILE* bin = fopen(outputfile, "r");
  assert(bin != NULL);
  fseek(bin, 0L, SEEK_END);
  *size = ftell(bin);
  rewind(bin);
  fread(bin_instr, sizeof(unsigned char), *size, bin);
  fclose(bin);

  // cleanup by deleting the two files
  remove(inputfile);
  remove(outputfile);
}

void aspri_fatal(char *msg, int line)
{
  if (line > 0)
    fprintf(stderr, "line %d: %s\n", line, msg);
  else
    fprintf(stderr, "%s\n", msg);
  exit(1);
}

void emit_spri_mapping(FILE *output, aspri_address_t from_address, aspri_address_t to_address)
{
  fprintf(output,"0x%08x -> 0x%08x\n", from_address.offset, to_address.offset);
}

void emit_spri_instruction(FILE *output, int virtualPC, int size, unsigned char *bin_instr, char *asm_instr)
{
  fprintf(output,"0x%08x ** %d", virtualPC, size);
  for (int i = 0; i < size; ++i)
  {
    unsigned char c = bin_instr[i];
    fprintf(output," %02x", c);
  }

  fprintf(output, "  # %s", asm_instr);
}
