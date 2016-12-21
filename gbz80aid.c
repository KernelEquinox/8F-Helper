#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "gbz80aid.h"


void usage(char*);
void strip_spaces(char*);
void lowercase(char*);
void uppercase(char*);
void nullify_char(char*, char);
char* normalize_param(char*);
void ascii2hex(char*, int);
void jump2addr(char*, int);
char* op2hex(char*, char*, char*);
void hex_to_asm(char*, int);
void asm_to_hex(char*);
void hex_to_gen(char*, int);
void hex_to_joy(char*);


// Struct for holding label information
struct Label {
	unsigned int address;
	char *name;
};

// Struct for holding error detections
struct Error {
	unsigned char key_quantity;
	unsigned char duplicates;
	unsigned char glitches;
};


// Global offset of current byte
int cur_offset = 0;

// Offset to begin calulating at
int print_offset = 0;

// Ability to define multiple labels
int jmp_num = 0;
struct Label *label;

// File line offset
int line_num = 1;

// Read 6 chars at a time
int hex_size = 1;
char *hex_string;

// Show warnings by default
char show_warnings = 1;


int main(int argc, char* argv[])
{
	char *format = 0;
	char *input, *filename;
	char file_mode = 0;

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
			if(i == argc)
			{
				// The user supplied a "-f" option but no file to read from.
				printf("Error : a file to read from is expected after \"-f\" !\n");
				exit(1);
			}
		}

		else if (!strcmp(argv[i], "-o"))
		{
			format = argv[++i];
			// I'm just bounds checkin' ~
			if(i == argc)
			{
				printf("Error : \"-o\" expects a format but nothing was found !\n");
				exit(1);
			}
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
			if (i++ == argc)
			{
				printf("Error : \"-ofs\" expects an offset but nothing was found !\n");
				exit(1);
			}
			
			if (!sscanf(argv[i], "%4X", &print_offset))
			{
				printf("Error : \'-ofs\" expects an hexadecimal offset (ie. D322 or 1f49)\n");
				exit(1);
			}
		}

		else if (!strcmp(argv[i], "-w"))
			show_warnings = 0;

		else
			input = argv[i];
	}

	// Set the current offset to the specified offset
	cur_offset = print_offset;

	// Default to asm if no format was selected
	if (!format)
		format = "asm";

	// Allocate space for the hex string
	hex_string = calloc(hex_size, sizeof(char));

	// Parse file as input
	if (file_mode)
	{
		FILE* file = fopen(filename, "r");
		
		if(!file)
		{
			printf("Error : specified file \"%s\" does not exist !\n", filename);
			exit(1);
		}
		
		char line[256];
		while (fgets(line, sizeof(line), file))
			asm_to_hex(line);
		
		uppercase(hex_string);
		input = hex_string;
		
		fclose(file);
	}

	// Reset the current offset, since it was probably modified
	cur_offset = print_offset;

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
	else if (!strcmp(format, "joy"))
		hex_to_joy(input);
	else if (!strcmp(format, "bgb"))
		hex_to_asm(input, 1);
	else
		hex_to_asm(input, 0);
	
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
	printf("                 (Ignored in other formats)\n");
	printf("  -w           Disable item warning messages\n");
	printf("  -h           Print this help message and exit\n");
	printf("  -v           Print version information and exit\n\n");
	printf("Formats:\n");
	printf("  asm          GB-Z80 assembly language\n");
	printf("  bgb          BGB-style assembly language\n");
	printf("  hex          Hexadecimal machine code format\n");
	printf("  joy          Joypad values\n");
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
	char *parenthesis = strchr(param, '[');
	if(parenthesis)
	{
		*parenthesis = '(';
		
		parenthesis = strchr(param, ']');
		if(!parenthesis)
		{
			// Please, be consistent.
			printf("Couldn't parse [%s %s] on line %d\n", opcode, param, line_num);
			exit(1);
		}
		*parenthesis = ')';
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
			if ((!strcmp(opcode, cb_opcode_table[i][k]) &&
				!strcmp(param, cb_param_table[i][k])) ||
				(!strcmp(opcode, bgb_opcode_table[i][k]) &&
				!strcmp(param, bgb_param_table[i][k])) ||
				(!strcmp(opcode, opcode_table[i][k]) &&
				!strcmp(param, param_table[i][k])))
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
void jump2addr(char *param, int type)
{
	for (int i = 0; i < jmp_num; i++)
	{
		if (!strcmp(label[i].name, param))
		{
			param[0] = '$';
			if (type)
				sprintf(param + 1, "%x", (label[i].address - cur_offset - 2) & 0xFF);
			else
				sprintf(param + 1, "%04x", label[i].address);
			param[(type ? 3 : 5)] = 0;
		}
	}
}





// Converts hex to asm and prints the results
void hex_to_asm(char *str, int bgb)
{
	// Remove spaces and calculate length
	strip_spaces(str);
	int len = strlen(str);

	// Make modifiable copy of input string
	char bytes[len + 1];
	strcpy(bytes, str);

	// Translate the hex string into a byte array
	ascii2hex(str, len);

	printf("\n%sgbz80 Assembly:\n\n", (bgb ? "BGB " : ""));

	// Loop through each opcode
	for (int i = 0; i < len;)
	{
		// Place horizontal cursor at line start
		int h_cursor = 0;

		// Print current offset
		printf("% 4X  ", cur_offset);
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

		// Special case for the STOP instruction
		if (h == 1 && l == 0)
		{
			printf("01 ");
			cur_offset++;
			h_cursor += 3;
			i += 2;
		}

		// Mnemonic formatting variables
		char *instruction;
		int param_len = 0;
		int arg_size = 0;
		int offset = 0;

		if (cb)
		{
			instruction = cb_opcode_table[h][l];
			param_len = strlen(cb_param_table[h][l]) + 1;
		}
		else if (bgb)
		{
			instruction = bgb_opcode_table[h][l];
			param_len = strlen(bgb_param_table[h][l]) + 1;
			arg_size = size_table[h][l];
			offset = bgb_offset_table[h][l];
		}
		else
		{
			instruction = opcode_table[h][l];
			param_len = strlen(param_table[h][l]) + 1;
			arg_size = size_table[h][l];
			offset = offset_table[h][l];
		}

		// Create modifiable copy of parameter string
		char *parameters = malloc(param_len * sizeof(char));
		memset(parameters, 0, param_len);
		if (cb)
			strcpy(parameters, cb_param_table[h][l]);
		else if (bgb)
			strcpy(parameters, bgb_param_table[h][l]);
		else
			strcpy(parameters, param_table[h][l]);

		// Modify mnemonic string if required
		for (int x = 0; x < (arg_size * 2); x++)
		{
			// Swap endianness if necessary
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

		for (; h_cursor < 28; h_cursor++)
			printf(" ");
		printf("%s\n", parameters);
	}
}


void asm_to_hex(char *str)
{
	int i = 0;
	int len = 0;
	char opcode[5] = {0};
	char *param;
	char *param2, *args;

	// Convert to lowercase and calculate line length
	lowercase(str);
	len = strlen(str);

	// Trim leading spaces
	for (; str[i] == '\t' || str[i] == ' '; i++);

	// Force comments and newlines to null bytes
	nullify_char(str, '\n');
	nullify_char(str, '\r');
	nullify_char(str, ';');

	// No instruction on this line
	if (!str[i])
	{
		line_num++;
		return;
	}

	// Update the label array if a label was detected
	if (str[i] == '.' || strchr(str, ':'))
	{
		strip_spaces(str);
		nullify_char(str, ':');
		// Dynamically expand array as needed
		label = realloc(label, (jmp_num + 1) * sizeof(*label));
		// Add current address and label name to array
		label[jmp_num].address = cur_offset;
		label[jmp_num].name = calloc(len, sizeof(char));
		strcpy(label[jmp_num++].name, (str[i] == '.' ? str + 1 : str));
		line_num++;
		return;
	}

	// Fetch the instruction
	for (int k = 0; str[i] > 0x20;)
		opcode[k++] = str[i++];

	// Remove all the remaining spaces
	strip_spaces(str + i);

	// Detect comma location and parse accordingly
	char *comma = strchr(str, ',');
	if (opcode[0] == 'j')
		if (comma)
	 		jump2addr(comma + 1, (opcode[1] == 'r' ? 1 : 0));
	 	else
	 		jump2addr(str + i, (opcode[1] == 'r' ? 1 : 0));
	args = normalize_param(str);

	// The rest of the string is parameter data
	param = calloc(len, sizeof(char));
	strcpy(param, str + i);

	// Special case for the STOP instruction
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
	if(strcmp(opcode, "stop"))
		free(args);
}


// Converts hex string into Gen I items for 8F
void hex_to_gen(char *str, int gen)
{
	// Error handler for weird item setups
	struct Error errors = {0};

	strip_spaces(str);
	int len = strlen(str);
	unsigned char seen_items[16][16] = {0};

	// Warning placeholders
	unsigned char glitch_items = 0;
	unsigned char multi_key_items = 0;
	unsigned char duplicate_items = 0;

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
		char *quantity = calloc(4, sizeof(char));
		unsigned char conversion = 0;

		h_cursor += strlen(item);

		// Grab next byte as quantity
		if (str[i])
		{
			ascii2hex(str + i, 2);
			conversion = str[i++] << 4;
			conversion |= str[i++];
			sprintf(quantity, "%d", conversion);
		}
		// Set as "Any" if any quantity will do
		else
			quantity = "Any";

		// Print item/quantity pairs
		printf("%s", item);
		for (; h_cursor < 16; h_cursor++)
			printf(" ");
		printf("x%s\n", quantity);

		// [Error] Key items with 2+ quantity
		if ((gen == 1 ? gen1_key_items[h][l] : gen2_key_items[h][l]))
			if (conversion && conversion != 1)
				errors.key_quantity = 1;
		// [Error] Invalid or glitch items
		if ((gen == 1 ? gen1_glitch_items[h][l] : gen2_glitch_items[h][l]))
			errors.glitches = 1;
		// [Error] Duplicate item stacks
		if (seen_items[h][l] == 1)
			errors.duplicates = 1;
		else
			seen_items[h][l] = 1;
	}

	// Print errors
	if (show_warnings)
		if (errors.key_quantity || errors.glitches || errors.duplicates)
		{
			printf("\n\n-- WARNING! --\n");
			if (errors.duplicates)
				printf(" * Duplicate item stacks detected!\n");
			if (errors.key_quantity)
				printf(" * Key item with 2+ quantity detected!\n");
			if (errors.glitches)
				printf(" * Invalid and/or glitch items detected!\n");
		}
}

// Converts hex string into joypad values for use with Full Control method
// http://forums.glitchcity.info/index.php?topic=7744.0
void hex_to_joy(char* str)
{
	strip_spaces(str);
	int len = strlen(str);
	ascii2hex(str, len);

	// Placeholder for the last button value
	char *last;

	printf("\nJoypad Values:\n\n");

	// Print an initial A to skip the junk byte
	printf("A\n");

	// Total number of button presses, just for funsies
	// 3 = The initial A and the ending START + SELECT
	int presses = 3;

	for (int i = 0; i < len;)
	{
		char p14 = str[i++];
		char p15 = str[i++];
		char *buttons[12] = {0};
		int index = 0;

		// Collect all D-Pad values
		for (int x = 0; x < 4; x++)
			if (joy_vals[x] <= p14)
			{
				buttons[index++] = joy_high[x];
				p14 -= joy_vals[x];
				presses++;
			}

		// Collect all other values
		for (int x = 0; x < 4; x++)
			if (joy_vals[x] <= p15)
			{
				buttons[index++] = joy_low[x];
				p15 -= joy_vals[x];
				presses++;
			}

		// Check if 1st button = NEXT code
		if (buttons[0] == last)
			// Swap buttons if more than 1 exists
			if (buttons[1] != 0)
			{
				char *tmp = buttons[0];
				buttons[0] = buttons[1];
				buttons[1] = tmp;
			}
			// Overflow the byte for correction if only 1 exists
			else
				if (last == "DOWN")
				{
					char *tmp = buttons[0];
					buttons[0] = "UP";
					buttons[1] = "DOWN";
					buttons[2] = "UP";
					buttons[3] = tmp;
					index += 3;
					presses += 3;
				}
				else
				{
					char *tmp = buttons[0];
					buttons[0] = "DOWN";
					buttons[1] = tmp;
					buttons[2] = "DOWN";
					presses += 2;
					index += 2;
				}

		// Keep track of the last button for correction
		last = buttons[index - 1];

		// Print out the button combination for this byte
		for (int x = 0; x < index; x++)
			printf("%s ", buttons[x]);
		printf("%s\n", last);
		presses++;
	}

	// Print the EXIT code and number of button presses
	printf("START + SELECT\n\n");
	printf("Total number of button presses: %d\n", presses);
}
