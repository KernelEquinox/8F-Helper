#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "gbz80aid.h"


void usage(char*) __attribute__ ((noreturn));
void strip_spaces(char*);
void lowercase(char*);
void uppercase(char*);
void nullify_char(char*, char);
char* normalize_param(char*);
void ascii2hex(char*, int);
void jump2addr(char*, int);
char* op2hex(char*, char*, char*);
void hex_to_asm(char*);
void asm_to_hex(char*);
void hex_to_gen(char*, int);


// Global offset of current byte
int cur_offset = 0;
// Printing offset (for ASM format)
int print_offset = 0;
// Ability to define one named jump
int jmp_offset = 0;
// File line offset
int line_num = 1;
// Read 6 chars at a time
int hex_size = 1;
char *hex_string;


int main(int argc, char* argv[])
{
	char *format = 0, *input, *filename;
	char interactive = 0, file_mode = 0;

	// Show help
	if (argc == 1)
		usage(argv[0]);

	// Parse arguments
	for (int i = 0; i < argc; i++)
	{
		// GIVE US DEM FILE
		if (!strcmp(argv[i], "-f"))
		{
			file_mode = 1;
			filename = argv[++i];
			// This means the previous line did an affectation from over the boundaries of argv !!
			if (i == argc)
			{
				// The user supplied a "-f" option but no file to read from.
				printf("Error : a file to read from is expected after \"-f\" !\n");
				exit(1);
			}
		}
		else if (!strcmp(argv[i], "-o"))
			format = argv[++i];
			// I'm just bounds checkin' ~
			if (i == argc)
			{
				printf("Error : \"-o\" expects a format but nothing was found !\n");
				exit(1);
			}
		else if (!strcmp(argv[i], "-h"))
			// Note : -h will be accepted even if other options were read.
			// Example : gbz80aid -o gen1 -h
			// is valid and will simply print the usage.
			usage(argv[0]);
			
			// Does not return.
		else if (!strcmp(argv[i], "-v"))
		{
			printf("GBZ80 Aid\n");
			printf("Version 1.1\n\n");
			printf("Created by KernelEquinox\n");
			printf("Contributions by ISSOtm\n\n");
			printf("Homepage : http://github.com/KernelEquinox/8F-Helper/\n\n");
			
			exit(0);
		}
		else if (!strcmp(argv[i], "-ofs"))
		{
			i++;
			if (i == argc)
			{
				printf("Error : \"-ofs\" expects an offset but nothing was found !\n");
				exit(1);
			}
			
			if (!sscanf(argv[i], "%4X", &print_offset))
			{
				printf("Error : \'-ofs\" expects an hexadecimal offset (ie. $D322 or 1f49h)\n");
				exit(1);
			}
		}
		else
			input = argv[i];
	}

	// Default to asm if no format was selected
	if (!format)
		format = "asm";

	// Allocate space for the hex string
	hex_string = calloc(hex_size, sizeof(char));

	// Parse file as input
	if (file_mode)
	{
		FILE* file = fopen(filename, "r");
		
		if (!file)
		{
			printf("Error : specified file \"%s\" does not exist !\n", filename);
			perror(filename);
			
			exit(1);
		}
		
		char line[256];
		while (fgets(line, sizeof(line), file))
			asm_to_hex(line);
		
		uppercase(hex_string);
		input = hex_string;
		
		fclose(file);
	}

	// Print results
	if (!strcmp(format, "hex"))
	{
		if (!file_mode)
			asm_to_hex(input);
		uppercase(hex_string);
		printf("\nMachine code: %s\n", hex_string);
	}
	else if (!strcmp(format, "gen1"))
		hex_to_gen(input, 1);
	else if (!strcmp(format, "gen2"))
		hex_to_gen(input, 2);
	else
		hex_to_asm(input);
	
	printf("\n");
	return 0;
}


// Print the default help message
void usage(char *str)
{
	printf("Usage: %s [options] [hex]\n\n", str);
	printf("Options:\n");
	printf("  -f file      File mode (read input from file)\n");
	printf("  -o format    Display output in a specific format\n");
	printf("  -ofs offset  Specify memory offset to display in asm format.\n");
	printf("                 Ignored in other formats\n");
	printf("  -h           Print this help message and exit\n");
	printf("  -v           Print version information and exit\n\n");
	printf("Formats:\n");
	printf("  asm          GB-Z80 assembly language\n");
	printf("  hex          Hexadecimal GB-Z80 opcodes\n");
	printf("  gen1         R/B/Y item codes for use with ACE\n");
	printf("  gen2         G/S/C item codes for use with ACE\n\n");
	printf("Examples:\n");
	printf("  %s EA14D7C9\n", str);
	printf("  %s -o asm -f bgb_mem.dump\n", str);
	printf("  %s -o hex -f zzazz.asm\n", str);
	printf("  %s -o gen1 0E1626642EBB4140CDD635C9\n", str);
	printf("  %s -o gen2 -f coin_case.asm\n", str);
	exit(0);
}


// Removes all spaces from the string
void strip_spaces(char *str)
{
	char *modified = str;
	do
		while (*modified == ' ' || *modified == '\t')
			modified++;
	while ((*str++ = *modified++));
}


// Converts all uppercase chars to lowercase
void lowercase(char *str)
{
	do
		if (*str > 0x40 && *str < 0x5B)
			*str += 0x20;
	while (*str++);
}


// Converts all lowercase chars to uppercase
void uppercase(char *str)
{
	do
		if (*str > 0x60)
			*str -= 0x20;
	while (*str++);
}


// Replaces selected char with a null byte
void nullify_char(char *str, char target)
{
	do
		if (*str == target)
		{
			*str = 0;
			return;
		}
	while (*str++);
}


// Changes input values to $xx or $xxyy
char* normalize_param(char *str)
{
	char *hex = calloc(5, sizeof(char));
	do
		if (*str == '$')
		{
			hex[0] = str[1];
			hex[1] = str[2];
			str[1] = 'x';
			str[2] = 'x';
			if (str[3] > ')')
			{
				hex[2] = hex[0];
				hex[3] = hex[1];
				hex[0] = str[3];
				hex[1] = str[4];
				str[3] = 'y';
				str[4] = 'y';
			}
			break;
		}
	while (*str++);
	return hex;
}


// Reads hexadecimal written in ASCII, and replaces each character by its value.
void ascii2hex(char *str, int len)
{
	char map[256] = {['0'] = 0,
					 ['1'] = 1,
					 ['2'] = 2,
					 ['3'] = 3,
					 ['4'] = 4,
					 ['5'] = 5,
					 ['6'] = 6,
					 ['7'] = 7,
					 ['8'] = 8,
					 ['9'] = 9,
					 ['A'] = 10,
					 ['a'] = 10,
					 ['B'] = 11,
					 ['b'] = 11,
					 ['C'] = 12,
					 ['c'] = 12,
					 ['D'] = 13,
					 ['d'] = 13,
					 ['E'] = 14,
					 ['e'] = 14,
					 ['F'] = 15,
					 ['f'] = 15};

	for (int i = 0; i < len; i++)
		str[i] = map[str[i]];
}


// Converts an instruction into its hexadecimal equivalent.
char* op2hex(char *opcode, char *param, char *args)
{
	char *hex = calloc(7, sizeof(char));
	char opcode_hex[3];
	int cb = 0; // Flags if this instruction is CB-prefixed.
	
	// Some syntaxes use '[' instead of '('. These being strictly equivalent, we'll replace brackets if we find some.
	// We'll only look at the first occurence since no instruction uses parentheses twice.
	char* parenthesis = strchr(param, '[');
	if (parenthesis)
	{
		*parenthesis = '(';
		
		parenthesis = strchr(param, ']');
		if (!parenthesis)
		{
			// Please, be consistent.
			printf("Couldn't parse [%s %s] on line %d\n", opcode, param, line_num);
			exit(1);
		}
		*parenthesis = ')';
	}
	
	// WARNING : puth this code block AFTER the "bracket to parenthesis" block !
	
	// We will look for "ldi (hl)", and alternative to "ld (hli)". Same for ldd.
	if (!strncmp(opcode, "ldi", 3) && !strncmp(opcode, "ldd", 3))
	{
		// Okay. We need to replace the opcode with "ld" and the "hl" argument by "hl+".
		// We need to allocate one more byte to make room for "hli". +2 because +1 for the i, and +1 for the terminator.
		int len = strlen(param) + 2;
		// Keep a pointer to the original pointer for the copy.
		char* old_param = param;
		// This will be the ne parameter.
		param = malloc(len * sizeof(char));
		// We now copy the opcode, adding the character after "hl".
		for(int i = 0; i < len; i++)
		{
			// We check if we just copied a parenthesis, signaling we finished copying the "hl".
			// It cannot be a bracket, since we normalized it right before !
			if (*old_param == ')')
			{
				param[i] = opcode[2];
				i++;
			}
			
			param[i] = *old_param;
			i++;
			old_param++;
		}
		
		opcode[2] = '\0'; // Remove the "i" in "ldi".
	}
	
	char* hl_repl = strstr(param, "hli");
	if (hl_repl)
	{
		// We replace hli with hl+.
		*(hl_repl + 2 * sizeof(char)) = '+';
	}
	
	hl_repl = strstr(param, "hld");
	if (hl_repl)
	{
		// We replace hld with hl-.
		*(hl_repl + 2 * sizeof(char)) = '-';
	}
	
	// Scan the list of prefixed instructions.
	for (int x = 0; x < 11; x++)
		if (!strcmp(opcode, cb_set[x]))
		{
			strcat(hex, "cb");
			cb = 1;
		}

	// Check the lookup table for matches
	for (int i = 0; i < 16; i++)
		for (int k = 0; k < 16; k++)
			if (!strcmp(opcode, (cb ? cb_opcode_table : opcode_table)[i][k]) &&
			    !strcmp(param,  (cb ? cb_param_table  : param_table)[i][k]))
			{
				int index = (i << 4) | k;
				sprintf(opcode_hex, "%02x", index);
				strcat(hex, opcode_hex);
				strcat(hex, args);
				return hex;
			}

	// Ollie into the sun if no match
	printf("Couldn't parse [%s %s] on line %d\n", opcode, param, line_num);
	exit(1);
}


// Converts a jump label into a jump address
void jump2addr(char *param, int rel)
{
	char str[5];
	char *addr = str;
	int i = 0, index = 0;
	int size = (rel ? 3 : 5);

	unsigned char jmp_rel = ~(cur_offset - jmp_offset) - 1;

	if (rel)
		sprintf(str, "%02x", jmp_rel);
	else
		sprintf(str, "%04x", jmp_offset);
	if (param[0] == ',')
		index++;
	param[index++] = '$';
	for (; i < (size - 1); i++)
		param[index + i] = *addr++;
	param[index + i] = 0;
}





// Converts hex to asm and prints the results
void hex_to_asm(char *str)
{
	// Reset current offset for printing purposes
	cur_offset = print_offset;
	
	// Remove spaces and calculate length
	strip_spaces(str);
	int len = strlen(str);

	// Make modifiable copy of input string
	char bytes[len + 1];
	strcpy(bytes, str);

	// Translate the hex string into a byte array
	ascii2hex(str, len);

	printf("\ngbz80 Assembly:\n\n");

	// Loop through each opcode
	for (int i = 0; i < len;)
	{
		// Place horizontal cursor at line start
		int h_cursor = 0;

		// Print current offset
		printf("%04X  ", cur_offset);
		h_cursor += 6;

		// Split high and low nybble as indices
		char h = str[i++];
		char l = str[i++];
		char cb = 0;

		// Check if current byte is prefix CB
		if (h == 0xC && l == 0xB)
			cb = 1;

		// Print initial opcode
		printf("%X%X ", h, l);
		cur_offset++;
		h_cursor += 3;

		// Print next byte for CB opcodes
		if (cb)
		{
			h = str[i++];
			l = str[i++];
			printf("%X%X ", h, l);
			cur_offset++;
			h_cursor += 3;
		}

		if (h == 1 && l == 0)
		{
			printf("01 ");
			cur_offset++;
			h_cursor += 3;
			i += 2;
		}

		// Mnemonic formatting variables
		char *instruction = (cb ? cb_opcode_table[h][l] : opcode_table[h][l]);
		int param_len = (strlen((cb ? cb_param_table[h][l] : param_table[h][l]))) + 1;
		int arg_size = (cb ? 0 : size_table[h][l]);
		int offset = (cb ? 9 : offset_table[h][l]);

		// Create modifiable copy of parameter string
		char parameters[param_len];
		strcpy(parameters, (cb ? cb_param_table : param_table)[h][l]);

		// Modify mnemonic string if required
		for (int x = 0; x < (arg_size * 2); x++)
		{
			if (arg_size == 2)
				parameters[offset + ((x + 2) % 4)] = bytes[i];
			else
				parameters[offset + x] = bytes[i];
			printf("%X", str[i++]);
			if (x % 2)
			{
				printf(" ");
				cur_offset++;
				h_cursor += 3;
			}
		}

		// Uniformly print mnemonics
		for (; h_cursor < 23; h_cursor++)
			printf(" ");
		printf("%s", instruction);
		h_cursor += strlen(instruction);
		
		// The second verification verifies this isn't a "jp (hl)".
		if (instruction[0] == 'j' && parameters[0] != '(')
		{
			// We'll be making special cases of the jump operations.
			if (parameters[0] == 'n')
			{
				// That's NC or NZ.
				// We remove the comma.
				parameters[2] = '\0';
				printf(" %s,", parameters);
				h_cursor += 4;
				
				// Now we copy the hex to the rest.
				strcpy(parameters, &parameters[3]);
			}
			else if (parameters[0] != '$')
			{
				// That's C or Z.
				// Same.
				parameters[1] = '\0';
				printf("  %s,", parameters);
				h_cursor += 4;
				
				strcpy(parameters, &parameters[2]);
			}
			
			if (instruction[1] == 'r')
			{
				// We will compute the address of the JR instead of displaying the offset.
				signed char jump_offset;
				sscanf(&parameters[1], "%2X", &jump_offset);
				int jump_target = (signed int)jump_offset;
				jump_target += cur_offset;
				
				for (; h_cursor < 29; h_cursor++)
					printf(" ");
				printf("$%04X", jump_target);
				parameters[0] = '\0'; // We displayed the address. Nothing to do anymore.
			}
		}
		for (; h_cursor < 29; h_cursor++)
			printf(" ");
		printf("%s\n", parameters);
	}
}


void asm_to_hex(char *str)
{
	char opcode[5] = {0};
	char param[11] = {0};
	char *param2, *args;

	// Conver to lowercase
	lowercase(str);

	// Trim leading spaces
	for (; *str == '\t' || *str == ' '; *str++);

	// Force comments and newlines to null bytes
	nullify_char(str, '\n');
	nullify_char(str, '\r');
	nullify_char(str, ';');

	// No instruction on this line
	if (!*str)
	{
		line_num++;
		return;
	}

	// Set jump location if a label was found
	if (*str == '.' || strchr(str, ':'))
	{
		if (!jmp_offset)
			jmp_offset = cur_offset;
		else
		{
			printf("More than one jump detected, quitting.\n");
			exit(1);
		}
		line_num++;
		return;
	}

	// Fetch the instruction
	for (int i = 0; *str > 0x20; opcode[i++] = *str++);

	// Remove all the remaining spaces
	strip_spaces(str);

	// Grab the first parameter
	for (int i = 0; *str && *str != ','; param[i++] = *str++);

	if (opcode[0] == 'j' && !strchr(str, ','))
		if (param[0] != '$' && param[0] != '(')
			if (opcode[1] == 'r')
				jump2addr(param, 1);
			else
				jump2addr(param, 0);


	// Normalize first parameter for table lookup
	args = normalize_param(param);

	// Grab the rest as param2
	param2 = str;

	if (opcode[0] == 'j')
		if (*param2 && param2[1] != '$')
			if (opcode[1] == 'r')
				jump2addr(param2, 1);
			else
				jump2addr(param2, 0);

	// Normalize second parameter for table lookup
	if (!*args)
	{
		free(args);
		args = normalize_param(param2);
	}

	// Combine parameters
	strcat(param, param2);

	if (!strcmp(opcode, "stop"))
	{
		free(args);
		args = "01";
	}
	
	// Get hex equivalent of instruction
	char *cur_hex = op2hex(opcode, param, args);

	// Resize hex string to hold additional hex values
	hex_string = realloc(hex_string, (hex_size += strlen(cur_hex)) * sizeof(char));

	// Append instruction hex to output hex string
	strcat(hex_string, cur_hex);

	// These are for jump correction and error handling
	cur_offset += strlen(cur_hex) / 2;
	line_num++;
	
	// Free allocated string as it goes out of scope.
	if (strcmp(opcode, "stop")) // Don't try to free() if "stop" is the opcode, as then "args" has already been free'd.
		free(args);
}


// Converts hex string into Gen I items for 8F
void hex_to_gen(char *str, int gen)
{
	strip_spaces(str);
	int len = strlen(str);

	printf("\nItem            Quantity\n");
	printf("========================\n");

	for (int i = 0; i < len;)
	{
		// Prep bytes for lookup
		ascii2hex(str + i, 2);

		// Place horizontal cursor at line start
		int h_cursor = 0;
		char h = str[i++];
		char l = str[i++];
		char *item = (gen == 1 ? gen1_items[h][l] : gen2_items[h][l]);
		// If we don't specify the quantity, then this is a default.
		char quantity[4] = "Any";

		h_cursor += strlen(item);

		// Grab next byte as quantity
		if (str[i])
		{
			ascii2hex(str + i, 2);
			unsigned char conversion = str[i++] << 4;
			conversion |= str[i++];
			sprintf(quantity, "%d", conversion);
		}

		printf("%s", item);
		for (; h_cursor < 16; h_cursor++)
			printf(" ");
		printf("x%s\n", quantity);
	}
}

