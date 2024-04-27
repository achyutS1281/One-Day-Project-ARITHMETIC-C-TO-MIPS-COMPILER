#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include<stdbool.h>
#include<math.h>
#include<time.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>

//Function that reads the file and parses it line by line
char* letterArray = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

//Enum for the operations
enum Operation {
	ADDITION,
	SUBTRACTION,
	MULTIPLICATION,
	DIVISION,
	MODULUS
};
bool is_power_of_two(int n) { // Function to check if a number is a power of 2
	if (n <= 0) {
		return false; // Non-positive numbers aren't powers of 2
	}

	while (n > 1) {
		if (n % 2 != 0) {
			return false; // Not divisible by 2
		}
		n /= 2;
	}

	return true;
}
//Function to check if a string is empty
bool is_empty_string(const char* str) {
	return str[0] == '\0';
}
bool isLetter(char c) { // Function to check if a character is a letter
	for (int i = 0; i < strlen(letterArray); i++) {
		if (c == letterArray[i]) {
			return true;
		}
	}
	return false;
}
void find_power_of_two_series(int i, int* result_array) { // Function to find the powers of 2 in a number
	int power = 0;
	int index = 0;
	// Store the powers of 2 in the array (limit to 10)
	while (i > 0 && index < 10) {  // Modified condition
		if (i & 1) { // Check if the bit is set
			result_array[index] = power; // Store the power
			index++; // Increment the index
		}
		i >>= 1; // Shift the bits to the right
		power++;
	}
}
//Function to generate the MIPS code for a single line of C code. NOTE: This function takes PEMDAS into account. Did not see the specifications that I did not need to take PEMDAS into account until after I had already implemented it.
char* generateLine(char* line, char** args, int numArgs, enum Operation* ops, int numOps, int* immediate, char* target, int* temp_offset, int* label_offset) {
	char* out = (char*)malloc(5000 * sizeof(char)); // Allocate memory for the output
	out[0] = '\0'; // Set the output to an empty string
	char temp_str[100];  // Temporary buffer 
	temp_str[0] = '\0'; // Set the temporary buffer to an empty string
	sprintf(temp_str, "# %s\n", line);
	strcat(out, temp_str);  // Append the result to 'out'
	char* op;
	char** currTemps = (char**)malloc(20 * sizeof(char*)); // Allocate memory for the current temp registers
	bool useTarget = false; // Flag to check if the target register is used
	for (int i = 0; i < 20; i++) { // Loop through the current temp registers
		currTemps[i] = (char*)malloc(10 * sizeof(char)); // Allocate memory for the current temp register
		currTemps[i][0] = '\0'; // Set the current temp register to an empty string
	}
	int currOp = *temp_offset; // Set the current operation to the temp offset from other lines
	int* PEMDAS = (int*)malloc(10 * sizeof(int)); // Allocate memory for the PEMDAS array
	int currImm = 0; // Set the current immediate to index 0
	for (int i = 0; i < 10; i++) {
		PEMDAS[i] = -1; // Set the PEMDAS array to -1
	}
	if (numOps == 0) { // If there are no operations adn only an immediate
		op = "li";
		sprintf(temp_str, "%s %s,%d\n", op, target, immediate[0]); // Generate the MIPS code for the immediate
		strcat(out, temp_str);
	}
	else {
		for (int i = 0; i < numOps; i++) {
			bool useTarget = false; // Flag to check if the target register is used
			bool mod = false; // Flag to check if the operation is a modulus
			if (i == numOps - 1) { // If the operation is the last one
				useTarget = true;
			}
			if (ops[i] == MULTIPLICATION || ops[i] == DIVISION || ops[i] == MODULUS) { // If the operation is multiplication, division, or modulus
				*temp_offset += 1;
				if (ops[i] == MULTIPLICATION) { // Set the operation to multiplication or division
					op = "mult";
				}
				else {
					op = "div";
				}
				if (ops[i] == MODULUS) { // If the operation is a modulus
					mod = true;
				}
				sprintf(temp_str, "$t%d", currOp); // Generate the new temp register
				strcpy(currTemps[i], temp_str);
				if (i > 0 && PEMDAS[i - 1] == -1) { // * In case of multiple types of expressions (PEMDAS Order). If the previous and next PEMDAS are -1, meaning no operations have been done on either side
					if (strcmp(args[i + 1], "immediate") == 0) { // If the next argument is an immediate
						if (strcmp(op, "mult") == 0) { // Immediate multiplication algorithm
							if (immediate[currImm] == 1 || immediate[currImm] == -1) { // If the immediate is 1 or -1
								sprintf(temp_str, "$t%d", currOp + 1); // Generate the new temp register
								char* currMove = (char*)malloc(10 * sizeof(char));	// Allocate memory for the current move
								strcpy(currMove, temp_str);
								sprintf(temp_str, "move %s,%s\n", currTemps[i], args[i]); // Generate the MIPS code for the move
								strcat(out, temp_str);
								if (immediate[currImm] == -1) { // If the immediate is -1
									if (useTarget) { // If the target register is used
										sprintf(temp_str, "sub %s,$zero,%s\n", target, currTemps[i]);
									}
									else {
										*temp_offset += 1;
										sprintf(temp_str, "sub %s,$zero,%s\n", currMove, currTemps[i]);
									}
								}
								else { // If the immediate is 1
									if (useTarget) { // If the target register is used
										sprintf(temp_str, "move %s,%s\n", target, currTemps[i]);
									}
									else {
										sprintf(temp_str, "move %s,%s\n", currMove, currTemps[i]);
									}
								}
								strcat(out, temp_str); // Append the result to 'out'
								strcpy(currTemps[i], currMove); // Copy the current move to the current highest temp register for this operation
							}
							else if (immediate[currImm] == 0) { // If the immediate is 0
								if (useTarget) { // If the target register is used
									*temp_offset -= 1;
									sprintf(temp_str, "li %s,0\n", target);
								}
								else { // If the target register is not used
									sprintf(temp_str, "li %s,0\n", currTemps[i]);
								}
							}
							else {
								int* powers = (int*)malloc(10 * sizeof(int)); // Allocate memory for the powers of 2
								find_power_of_two_series(abs(immediate[currImm]), powers); // Find the powers of 2 in the immediate
								sprintf(temp_str, "$t%d", currOp + 1); // Generate the new temp register
								char* currMove = (char*)malloc(10 * sizeof(char)); // Allocate memory for the current move
								strcpy(currMove, temp_str);
								bool happened = false; // Flag to check if the first sll operation has happened
								for (int j = 5; j > 0; j--) { // Loop through the powers of 2
									if (powers[j] != 0) {
										sprintf(temp_str, "sll %s,%s,%d\n", currTemps[i], args[i], powers[j]); // Generate the MIPS code for the sll operation
										strcat(out, temp_str);  // Append the result to 'out'
										if (!happened) { // If the first sll operation has not happened
											sprintf(temp_str, "move %s,%s\n", currMove, currTemps[i]);
											strcat(out, temp_str);  // Append the result to 'out'
										}
										else {
											sprintf(temp_str, "add %s,%s,%s\n", currMove, currMove, currTemps[i]);
											strcat(out, temp_str);  // Append the result to 'out'
										}
										happened = true;
									}

								}
								if (immediate[currImm] % 2 != 0) { // If the immediate is not divisible by 2, we need to add the argument again to account for 2^0
									sprintf(temp_str, "add %s,%s,%s\n", currMove, currMove, args[i]);
									strcat(out, temp_str);
								}
								if (immediate[currImm] < 0) { // If the immediate is negative
									if (useTarget) {
										*temp_offset -= 1; // Decrement the temp offset as no new temp register is needed
										sprintf(temp_str, "sub %s,$zero,%s\n", target, currMove);
									}
									else {
										sprintf(temp_str, "sub %s,$zero,%s\n", currTemps[i], currMove);
									}
									strcat(out, temp_str);  // Append the result to 'out'
								}
								else {
									if (useTarget) { // If the target register is used
										*temp_offset -= 1; // Decrement the temp offset, as no new temp register is needed
										sprintf(temp_str, "move %s,%s\n", target, currMove);
										strcat(out, temp_str);  // Append the result to 'out'
									}
									else {
										sprintf(temp_str, "move $t%d,%s\n", currOp + 2, currMove);
										strcat(out, temp_str);  // Append the result to 'out'
										sprintf(temp_str, "$t%d", currOp + 2);
										strcpy(currTemps[i], temp_str);
									}
								}

								sprintf(temp_str, "$t%d", currOp + 2); // Generate the new temp register
								strcpy(currTemps[i], temp_str); // Copy the new temp register to the current highest temp register for this operation
								currOp++; // Increment the current operation as the multiplication process requires an extra temp register
							}
						}
						else {
							if (immediate[currImm] == 1) { // If the immediate is 1
								if (useTarget) {
									*temp_offset -= 1; // Decrement the temp offset as no new temp register is needed
									sprintf(temp_str, "move %s,%s\n", target, args[i]); // Generate the MIPS code for moving the argument to the target register
								}
								else {
									sprintf(temp_str, "move %s,%s\n", currTemps[i], args[i]); // Generate the MIPS code for moving the argument to the temp register
								}
								strcat(out, temp_str);
							}
							else if (immediate[currImm] == -1) { // If the immediate is -1
								if (useTarget) {
									*temp_offset -= 1; // Decrement the temp offset as no new temp register is needed
									sprintf(temp_str, "sub %s,$zero,%s\n", target, args[i]); // Generate the MIPS code for subtracting the argument from zero
								}
								else {
									sprintf(temp_str, "sub %s,$zero,%s\n", currTemps[i], args[i]);	// Generate the MIPS code for subtracting the argument from zero
								}
								strcat(out, temp_str);
							}
							else { // If the immediate is not 1 or -1
								if (mod) { // If the operation is a modulus immediate
									sprintf(temp_str, "li $t%d,%d\n", currOp, immediate[currImm]); // Generate the MIPS code for loading the immediate
									strcat(out, temp_str);
									sprintf(temp_str, "div %s,$t%d\n", args[i], currOp); // Generate the MIPS code for dividing the argument by the immediate
									strcat(out, temp_str);
									if (useTarget) { // If the target register is used. Use mfhi for getting the remainder
										sprintf(temp_str, "mfhi %s\n", target);
									}
									else {
										sprintf(temp_str, "mfhi $t%d\n", currOp + 1);
									}
									strcat(out, temp_str);
								}
								else {

									bool power = is_power_of_two(abs(immediate[currImm])); // Check if the immediate is a power of 2
									if (power) { // If the immediate is a power of 2
										sprintf(temp_str, "bltz %s,L%d\n", args[i], *label_offset);
										strcat(out, temp_str);
										int powerLog = log2(abs(immediate[currImm])); // Find the log base 2 of the immediate
										if (useTarget) { // If the target register is used. Right shift the argument by the log base 2 of the immediate
											*temp_offset -= 1;
											sprintf(temp_str, "srl %s,%s,%d\n", target, args[i], powerLog);
										}
										else {
											sprintf(temp_str, "srl %s,%s,%d\n", currTemps[i], args[i], powerLog);
										}
										strcat(out, temp_str);
										if (immediate[currImm] < 0) { // If the immediate is negative
											if (useTarget) {
												*temp_offset -= 1;
												sprintf(temp_str, "sub %s,$zero,%s\n", target, target); // Generate the MIPS code for subtracting the target from zero for the negaive immediate
											}
											else {
												sprintf(temp_str, "sub %s,$zero,%s\n", currTemps[i], currTemps[i]); // Generate the MIPS code for subtracting the temp register from zero for the negaive immediate
											}

											strcat(out, temp_str);
										}

										sprintf(temp_str, "j L%d\n", *label_offset + 1); // Generate the MIPS code for jumping to the next label
										strcat(out, temp_str);
										sprintf(temp_str, "L%d:\n", *label_offset); // Generate the MIPS code for the next label
										strcat(out, temp_str);
									} // This code is run if no matter what the immediate is
									sprintf(temp_str, "li $t%d,%d\n", currOp, immediate[currImm]); // Generate the MIPS code for loading the immediate
									strcat(out, temp_str);
									sprintf(temp_str, "div %s,$t%d\n", args[i], currOp); // Generate the MIPS code for dividing the argument by the immediate
									strcat(out, temp_str);
									if (useTarget) { // If the target register is used
										sprintf(temp_str, "mflo %s\n", target); //mflo for division
									}
									else {
										sprintf(temp_str, "mflo $t%d\n", currOp + 1); //mflo for division
									}
									strcat(out, temp_str);
									if (power) { // If the immediate is a power of 2
										sprintf(temp_str, "L%d:\n", *label_offset + 1);
										strcat(out, temp_str);
									}
									*label_offset += 2; // Increment the label offset by 2 for the two labels used
									*temp_offset += 1; // Increment the temp offset by 1 for the new temp register
								}
								sprintf(temp_str, "$t%d", currOp + 1); // Generate the new temp register
								strcpy(currTemps[i], temp_str);
								currOp++;
							}
						}
					}
					else {
						sprintf(temp_str, "%s %s,%s\n", op, args[i], args[i + 1]); // Generate the MIPS code for the operation
						strcat(out, temp_str); // Append the result to 'out'
						if (mod) {
							if (useTarget) {
								*temp_offset -= 1; // Decrement the temp offset as no new temp register is needed. mfhi used for modulus
								sprintf(temp_str, "mfhi %s\n", target);
							}
							else {
								sprintf(temp_str, "mfhi %s\n", currTemps[i]);
							}
						}
						else {
							if (useTarget) {
								*temp_offset -= 1;	// Decrement the temp offset as no new temp register is needed. mflo used for division
								sprintf(temp_str, "mflo %s\n", target);
							}
							else {
								sprintf(temp_str, "mflo %s\n", currTemps[i]);
							}
						}
						strcat(out, temp_str);
					}
				}
				else if (i > 0 && PEMDAS[i - 1] != -1) { //In case the previous expression has used a temp register (any operation after the first one)
					if (strcmp(args[i + 1], "immediate") == 0) { // If the next argument is an immediate
						if (strcmp(op, "mult") == 0) { // Immediate multiplication algorithm
							if (immediate[currImm] == 1 || immediate[currImm] == -1) { // If the immediate is 1 or -1
								sprintf(temp_str, "$t%d", currOp + 1);
								char* currMove = (char*)malloc(10 * sizeof(char)); // Allocate memory for the current move register
								sprintf(temp_str, "move %s,%s\n", currTemps[i], currTemps[i - 1]);
								strcat(out, temp_str);
								if (immediate[currImm] == -1) { // If the immediate is -1, use sub to subtract the temp register from zero
									if (useTarget) {
										sprintf(temp_str, "sub %s,$zero,%s\n", target, currTemps[i]);
									}
									else {
										*temp_offset += 1;
										sprintf(temp_str, "sub %s,$zero,%s\n", currMove, currTemps[i]);
									}
								}
								else { // If the immediate is 1, use move to move the temp register to the target register
									if (useTarget) {
										sprintf(temp_str, "move %s,%s\n", target, currTemps[i]);
									}
									else {
										*temp_offset += 1;
										sprintf(temp_str, "move %s,%s\n", currMove, currTemps[i]);
									}
								}
								strcat(out, temp_str); // Append the result to 'out'
								strcpy(currTemps[i], currMove); // Copy the current move to the current highest temp register for this operation
							}
							else if (immediate[currImm] == 0) { // If the immediate is 0, use li to load 0 to the target register
								if (useTarget) {
									*temp_offset -= 1;
									sprintf(temp_str, "li %s,0\n", target);
								}
								else {
									sprintf(temp_str, "li %s,0\n", currTemps[i]);
								}
								strcat(out, temp_str);
							}
							else { // If the immediate is not 1 or -1 or 0
								int* powers = (int*)malloc(10 * sizeof(int)); // Allocate memory for the powers of 2
								find_power_of_two_series(abs(immediate[currImm]), powers); // Find the powers of 2 in the immediate
								sprintf(temp_str, "$t%d", currOp + 1); // Generate the new temp register
								char* currMove = (char*)malloc(10 * sizeof(char)); // Allocate memory for the current move
								strcpy(currMove, temp_str);
								bool happened = false; 
								for (int j = 5; j > 0; j--) { // Loop through the powers of 2
									if (powers[j] != 0) {

										sprintf(temp_str, "sll %s,%s,%d\n", currTemps[i], currTemps[i - 1], powers[j]); // Generate the MIPS code for the sll operation
										strcat(out, temp_str);  // Append the result to 'out'
										if (!happened) { // If the first sll operation has not happened
											sprintf(temp_str, "move %s,%s\n", currMove, currTemps[i]);
											strcat(out, temp_str);  // Append the result to 'out'
										}
										else {
											sprintf(temp_str, "add %s,%s,%s\n", currMove, currMove, currTemps[i]); // Generate the MIPS code for the addition of next powers of 2
											strcat(out, temp_str);  // Append the result to 'out'
										}
										happened = true;
									}

								}
								if (immediate[currImm] % 2 != 0) { // If the immediate is not divisible by 2, we need to add the argument again to account for 2^0
									sprintf(temp_str, "add %s,%s,%s\n", currMove, currMove, args[i]);
									strcat(out, temp_str);
								}
								if (immediate[currImm] < 0) { // If the immediate is negative
									if (useTarget) {
										*temp_offset -= 1;
										sprintf(temp_str, "sub %s,$zero,%s\n", target, currMove);
									}
									else {
										sprintf(temp_str, "sub %s,$zero,%s\n", currTemps[i], currMove);
									}
									strcat(out, temp_str);  // Append the result to 'out'
								}
								else { // If the immediate is not negative
									if (useTarget) {
										*temp_offset -= 1;
										sprintf(temp_str, "move %s,%s\n", target, currMove);
										strcat(out, temp_str);  // Append the result to 'out'
									}
									else {
										sprintf(temp_str, "move $t%d,%s\n", currOp + 2, currMove);
										strcat(out, temp_str);  // Append the result to 'out'
										sprintf(temp_str, "$t%d", currOp + 2);
										strcpy(currTemps[i], temp_str);
									}

								}
								sprintf(temp_str, "$t%d", currOp + 2);
								strcpy(currTemps[i], temp_str);
								currOp++;
							}
						}
						else {
							if (immediate[currImm] == 1) { // If the immediate is 1
								if (useTarget) {
									*temp_offset -= 1; // Decrement the temp offset as no new temp register is needed
									sprintf(temp_str, "move %s,%s\n", target, currTemps[i - 1]);
								}
								else {
									sprintf(temp_str, "move %s,%s\n", currTemps[i], currTemps[i - 1]);
								}
								strcat(out, temp_str);
							}
							else if (immediate[currImm] == -1) { // If the immediate is -1
								if (useTarget) {
									*temp_offset -= 1;
									sprintf(temp_str, "sub %s,$zero,%s\n", target, currTemps[i - 1]);
								}
								else {
									sprintf(temp_str, "sub %s,$zero,%s\n", currTemps[i], currTemps[i - 1]);
								}
								strcat(out, temp_str);
							}
							else { // If the immediate is not 1 or -1
								if (mod) {	// If the operation is a modulus immediate
									sprintf(temp_str, "li $t%d,%d\n", currOp, immediate[currImm]);
									strcat(out, temp_str);
									sprintf(temp_str, "div %s,$t%d\n", currTemps[i - 1], currOp);
									strcat(out, temp_str);
									if (useTarget) {
										sprintf(temp_str, "mfhi %s\n", target);
									}
									else {
										sprintf(temp_str, "mfhi $t%d\n", currOp + 1);
									}
									strcat(out, temp_str);
								}
								else { // If the operation is not a modulus immediate

									bool power = is_power_of_two(abs(immediate[currImm])); // Check if the immediate is a power of 2
									if (power) { // If the immediate is a power of 2
										sprintf(temp_str, "bltz %s,L%d\n", currTemps[i - 1], *label_offset);
										strcat(out, temp_str);
										int powerLog = log2(abs(immediate[currImm])); // Find the log base 2 of the immediate
										if (useTarget) { // If the target register is used. Right shift the argument by the log base 2 of the immediate
											*temp_offset -= 1;
											sprintf(temp_str, "srl %s,%s,%d\n", target, currTemps[i - 1], powerLog);
										}
										else {
											sprintf(temp_str, "srl %s,%s,%d\n", currTemps[i], currTemps[i - 1], powerLog);
										}
										strcat(out, temp_str); // Append the result to 'out'
										if (immediate[currImm] < 0) {
											if (useTarget) {
												*temp_offset -= 1;
												sprintf(temp_str, "sub %s,$zero,%s\n", target, target);
											}
											else {
												sprintf(temp_str, "sub %s,$zero,%s\n", currTemps[i], currTemps[i]);
											}

											strcat(out, temp_str);
										}
										sprintf(temp_str, "j L%d\n", *label_offset + 1); // Generate the MIPS code for jumping to the next label
										strcat(out, temp_str);
										sprintf(temp_str, "L%d:\n", *label_offset); // Generate the MIPS code for the next label
										strcat(out, temp_str);
									} // This code is run if no matter what the immediate is
									sprintf(temp_str, "li $t%d,%d\n", currOp, immediate[currImm]); // Generate the MIPS code for loading the immediate
									strcat(out, temp_str);
									sprintf(temp_str, "div %s,$t%d\n", currTemps[i - 1], currOp); // Generate the MIPS code for dividing the argument by the immediate
									strcat(out, temp_str);
									if (useTarget) { // If the target register is used
										sprintf(temp_str, "mflo %s\n", target);
									}
									else {
										sprintf(temp_str, "mflo $t%d\n", currOp + 1);
									}
									strcat(out, temp_str);
									if (power) {
										sprintf(temp_str, "L%d:\n", *label_offset + 1); // Generate the MIPS code for the next label
										strcat(out, temp_str);
									}
									*label_offset += 2; // Increment the label offset by 2 for the two labels used
									*temp_offset += 1; // Increment the temp offset by 1 for the new temp register
								}
								sprintf(temp_str, "$t%d", currOp + 1); // Generate the new temp register
								strcpy(currTemps[i], temp_str);
								currOp++;
							}
						}
					}
					else {
						sprintf(temp_str, "%s %s,%s\n", op, currTemps[i - 1], args[i + 1]); // Generate the MIPS code for the operation
						strcat(out, temp_str);
						if (mod) {
							if (useTarget) { // If the target register is used
								*temp_offset -= 1; // Decrement the temp offset as no new temp register is needed. mfhi used for modulus
								sprintf(temp_str, "mfhi %s\n", target);
							}
							else {
								sprintf(temp_str, "mfhi %s\n", currTemps[i]);
							}
						}
						else {
							if (useTarget) { // If the target register is used
								*temp_offset -= 1; // Decrement the temp offset as no new temp register is needed. mflo used for division
								sprintf(temp_str, "mflo %s\n", target);
							}
							else {
								sprintf(temp_str, "mflo %s\n", currTemps[i]);
							}
						}
						strcat(out, temp_str);
					}
					strcpy(currTemps[i - 1], currTemps[i]);
				}
				else if (i == 0 && strcmp(args[i + 1], "immediate") == 0) { // In case of the first expression being an immediate

					if (strcmp(op, "mult") == 0) { // Immediate multiplication algorithm, commented above
						if (immediate[currImm] == 1 || immediate[currImm] == -1) {
							sprintf(temp_str, "$t%d", currOp + 1);
							char* currMove = (char*)malloc(10 * sizeof(char));
							sprintf(temp_str, "move %s,%s\n", currTemps[i], args[i]);
							strcat(out, temp_str);
							if (immediate[currImm] == -1) {
								if (useTarget) {
									sprintf(temp_str, "sub %s,$zero,%s\n", target, currTemps[i]);
								}
								else {
									*temp_offset += 1;
									sprintf(temp_str, "sub %s,$zero,%s\n", currMove, currTemps[i]);
								}
							}
							else {
								if (useTarget) {
									sprintf(temp_str, "move %s,%s\n", target, currTemps[i]);
								}
								else {
									*temp_offset += 1;
									sprintf(temp_str, "move %s,%s\n", currMove, currTemps[i]);
								}
							}
							strcat(out, temp_str);
							strcpy(currTemps[i], currMove);
						}
						else if (immediate[currImm] == 0) {
							if (useTarget) {
								*temp_offset -= 1;
								sprintf(temp_str, "li %s,0\n", target);
							}
							else {
								sprintf(temp_str, "li %s,0\n", currTemps[i]);
							}
							strcat(out, temp_str);
						}
						else {
							int* powers = (int*)malloc(10 * sizeof(int));
							find_power_of_two_series(abs(immediate[currImm]), powers);
							sprintf(temp_str, "$t%d", currOp + 1);
							char* currMove = (char*)malloc(10 * sizeof(char));
							strcpy(currMove, temp_str);
							bool happened = false;
							for (int j = 5; j > 0; j--) {
								if (powers[j] != 0) {
									sprintf(temp_str, "sll %s,%s,%d\n", currTemps[i], args[i], powers[j]);
									strcat(out, temp_str);  // Append the result to 'out'
									if (!happened) {
										sprintf(temp_str, "move %s,%s\n", currMove, currTemps[i]);
										strcat(out, temp_str);  // Append the result to 'out'
									}
									else {
										sprintf(temp_str, "add %s,%s,%s\n", currMove, currMove, currTemps[i]);
										strcat(out, temp_str);  // Append the result to 'out'
									}
									happened = true;
								}

							}
							if (immediate[currImm] % 2 != 0) {
								sprintf(temp_str, "add %s,%s,%s\n", currMove, currMove, args[i]);
								strcat(out, temp_str);
							}
							if (immediate[currImm] < 0) {
								if (useTarget) {
									*temp_offset -= 1;
									sprintf(temp_str, "sub %s,$zero,%s\n", target, currMove);
								}
								else {
									sprintf(temp_str, "sub %s,$zero,%s\n", currTemps[i], currMove);
								}
								strcat(out, temp_str);  // Append the result to 'out'
							}
							else {
								if (useTarget) {
									*temp_offset -= 1;
									sprintf(temp_str, "move %s,%s\n", target, currMove);
									strcat(out, temp_str);  // Append the result to 'out'
								}
								else {
									sprintf(temp_str, "move $t%d,%s\n", currOp + 2, currMove);
									strcat(out, temp_str);  // Append the result to 'out'
									sprintf(temp_str, "$t%d", currOp + 2);
									strcpy(currTemps[i], temp_str);
								}
							}
							sprintf(temp_str, "$t%d", currOp + 2);
							strcpy(currTemps[i], temp_str);
							currOp++;
						}
					}
					else { // Immediate division algorithm, commented above
						if (immediate[currImm] == 1) {
							if (useTarget) {
								*temp_offset -= 1;
								sprintf(temp_str, "move %s,%s\n", target, args[i]);
							}
							else {
								sprintf(temp_str, "move %s,%s\n", currTemps[i], args[i]);
							}
							strcat(out, temp_str);
						}
						else if (immediate[currImm] == -1) {
							if (useTarget) {
								*temp_offset -= 1;
								sprintf(temp_str, "sub %s,$zero,%s\n", target, args[i]);
							}
							else {
								sprintf(temp_str, "sub %s,$zero,%s\n", currTemps[i], args[i]);
							}
							strcat(out, temp_str);
						}
						else {
							if (mod) {
								sprintf(temp_str, "li $t%d,%d\n", currOp, immediate[currImm]);
								strcat(out, temp_str);
								sprintf(temp_str, "div %s,$t%d\n", args[i], currOp);
								strcat(out, temp_str);
								if (useTarget) {
									sprintf(temp_str, "mfhi %s\n", target);
								}
								else {
									sprintf(temp_str, "mfhi $t%d\n", currOp + 1);
								}
								strcat(out, temp_str);
							}
							else {

								bool power = is_power_of_two(abs(immediate[currImm]));
								if (power) {
									sprintf(temp_str, "bltz %s,L%d\n", args[i], *label_offset);
									strcat(out, temp_str);
									int powerLog = log2(abs(immediate[currImm]));
									if (useTarget) {
										*temp_offset -= 1;
										sprintf(temp_str, "srl %s,%s,%d\n", target, args[i], powerLog);
									}
									else {
										sprintf(temp_str, "srl %s,%s,%d\n", currTemps[i], args[i], powerLog);
									}
									strcat(out, temp_str);
									if (immediate[currImm] < 0) {
										if (useTarget) {
											*temp_offset -= 1;
											sprintf(temp_str, "sub %s,$zero,%s\n", target, target);
										}
										else {
											sprintf(temp_str, "sub %s,$zero,%s\n", currTemps[i], currTemps[i]);
										}
										strcat(out, temp_str);
									}

									sprintf(temp_str, "j L%d\n", *label_offset + 1);
									strcat(out, temp_str);
									sprintf(temp_str, "L%d:\n", *label_offset);
									strcat(out, temp_str);
								}

								sprintf(temp_str, "li $t%d,%d\n", currOp, immediate[currImm]);
								strcat(out, temp_str);
								sprintf(temp_str, "div %s,$t%d\n", args[i], currOp);
								strcat(out, temp_str);
								if (useTarget) {
									sprintf(temp_str, "mflo %s\n", target);
								}
								else {
									sprintf(temp_str, "mflo $t%d\n", currOp + 1);
								}
								strcat(out, temp_str);
								if (power) {
									sprintf(temp_str, "L%d:\n", *label_offset + 1);
									strcat(out, temp_str);
								}
								*label_offset += 2;
								*temp_offset += 1;
							}
							sprintf(temp_str, "$t%d", currOp + 1);
							strcpy(currTemps[i], temp_str);
							currOp++;
						}
					}
				}
				else { // If the immediate is not in the first operation
					sprintf(temp_str, "%s %s,%s\n", op, args[i], args[i + 1]); // Generate the MIPS code for the operation
					strcat(out, temp_str); // Append the result to 'out'
					if (mod) {  // If the operation is a modulus
						if (useTarget) {
							*temp_offset -= 1;
							sprintf(temp_str, "mfhi %s\n", target);
						}
						else {
							sprintf(temp_str, "mfhi %s\n", currTemps[i]);
						}
					}
					else { // If the operation is not a modulus
						if (useTarget) {
							*temp_offset -= 1;
							sprintf(temp_str, "mflo %s\n", target);
						}
						else {
							sprintf(temp_str, "mflo %s\n", currTemps[i]);
						}
					}
					strcat(out, temp_str);
				}
				if (strcmp(args[i + 1], "immediate") == 0) { // If the second argument is an immediate
					currImm++; // Increment the current immediate
				}
				PEMDAS[i] = currOp; // Set the current PEMDAS to the current operation
				currOp++;
			}
		}
		for (int i = 0; i < numOps; i++) { // Loop through the number of operations
			if (i == numOps - 1) {
				useTarget = true;
			}
			int currImm = 0;
			if (ops[i] == ADDITION || ops[i] == SUBTRACTION) { // If the operation is addition or subtraction
				*temp_offset += 1;
				sprintf(temp_str, "$t%d", currOp);
				strcat(currTemps[i], temp_str);
				if (ops[i] == ADDITION) { // Set op to add or sub
					op = "add";
				}
				else {
					op = "sub";
				}
				if (i > 0 && PEMDAS[i - 1] == -1 && PEMDAS[i + 1] == -1) { // If the previous and next expressions have not used a temp register
					if (strcmp(args[i + 1], "immediate") == 0) { // If the next argument is an immediate
						if (strcmp(op, "add") == 0) { // Immediate addition algorithm
							if (useTarget) { // If the target register is used
								*temp_offset -= 1; // Decrement the temp offset as no new temp register is needed
								sprintf(temp_str, "addi %s,%s,%d\n", target, args[i], immediate[currImm]); // Generate the MIPS code for adding the immediate to the argument and storing in target
							}
							else {
								sprintf(temp_str, "addi %s,%s,%d\n", currTemps[i], args[i], immediate[currImm]); // Generate the MIPS code for adding the immediate to the argument and storing in temp register
							}

							// Append to 'out'
							strcat(out, temp_str);
						}
						else { // Immediate subtraction algorithm
							if (useTarget) {
								*temp_offset -= 1;
								sprintf(temp_str, "addi %s,%s,%d\n", target, args[i], (-immediate[currImm])); // Generate the MIPS code for subtracting the immediate from the argument and storing in target
							}
							else {
								sprintf(temp_str, "addi %s,%s,%d\n", currTemps[i], args[i], (-immediate[currImm])); // Generate the MIPS code for subtracting the immediate from the argument and storing in temp register
							}

							// Append to 'out'
							strcat(out, temp_str);
						}
					}
					else { // If the next argument is not an immediate
						if (useTarget) { // If the target register is used
							*temp_offset -= 1;
							sprintf(temp_str, "%s %s,%s,%s\n", op, target, args[i], args[i + 1]); // Generate the MIPS code for the operation
						}
						else {
							sprintf(temp_str, "%s %s,%s,%s\n", op, currTemps[i], args[i], args[i + 1]); // Generate the MIPS code for the operation
						}
					}
				}
				else if (i > 0 && PEMDAS[i + 1] != -1 && PEMDAS[i - 1] != -1) { // If the previous and next expressions have used a temp register
					if (strcmp(args[i + 1], "immediate") == 0) { // If the next argument is an immediate
						if (strcmp(op, "add") == 0) { // Immediate addition algorithm
							if (useTarget) { // If the target register is used
								*temp_offset -= 1; // Decrement the temp offset as no new temp register is needed
								sprintf(temp_str, "addi %s,%s,%d\n", target, currTemps[i - 1], immediate[currImm]); // Generate the MIPS code for adding the immediate to the temp register and storing in target
							}
							else {
								sprintf(temp_str, "addi %s,%s,%d\n", currTemps[i], currTemps[i - 1], immediate[currImm]); // Generate the MIPS code for adding the immediate to the temp register and storing in temp register
							}

							// Append to 'out'
							strcat(out, temp_str);
						}
						else { // Immediate subtraction algorithm
							if (useTarget) { // If the target register is used
								*temp_offset -= 1;	// Decrement the temp offset as no new temp register is needed
								sprintf(temp_str, "addi %s,%s,%d\n", target, currTemps[i - 1], (-immediate[currImm])); // Generate the MIPS code for subtracting the immediate from the temp register and storing in target
							}
							else {
								sprintf(temp_str, "addi %s,%s,%d\n", currTemps[i], currTemps[i - 1], (-immediate[currImm])); // Generate the MIPS code for subtracting the immediate from the temp register and storing in temp register
							}

							// Append to 'out'
							strcat(out, temp_str);
						}
					}
					else { // If the next argument is not an immediate
						if (useTarget) { // If the target register is used
							*temp_offset -= 1;
							sprintf(temp_str, "%s %s,%s,%s\n", op, target, currTemps[i - 1], currTemps[i + 1]); // Generate the MIPS code for the operation
						}
						else {
							sprintf(temp_str, "%s %s,%s,%s\n", op, currTemps[i], currTemps[i - 1], currTemps[i + 1]); // Generate the MIPS code for the operation
						}
					}

				}
				else if (PEMDAS[i + 1] != -1) { // If the next expression has used a temp register
					sprintf(temp_str, "%s %s,%s,%s\n", op, currTemps[i], args[i], currTemps[i + 1]); // Generate the MIPS code for the operation
					strcat(out, temp_str);
				}
				else if (i == 0 && strcmp(args[i + 1], "immediate") != 0) { // If the first expression is not an immediate
					if (useTarget) { // If the target register is used
						*temp_offset -= 1;
						sprintf(temp_str, "%s %s,%s,%s\n", op, target, args[i], args[i + 1]);
					}
					else { // If the target register is not used
						sprintf(temp_str, "%s %s,%s,%s\n", op, currTemps[i], args[i], args[i + 1]);
					}
					strcat(out, temp_str);
				}
				else if (i == 0 && strcmp(args[i + 1], "immediate") == 0) {
					if (strcmp(op, "add") == 0) { // Immediate addition algorithm, commented above
						if (useTarget) {
							*temp_offset -= 1;
							sprintf(temp_str, "addi %s,%s,%d\n", target, args[i], immediate[currImm]);
						}
						else {
							sprintf(temp_str, "addi %s,%s,%d\n", currTemps[i], args[i], immediate[currImm]);
						}
						// Append to 'out'
						strcat(out, temp_str);
					}
					else { // Immediate subtraction algorithm, commented above
						if (useTarget) {
							*temp_offset -= 1;
							sprintf(temp_str, "addi %s,%s,%d\n", target, args[i], (-immediate[currImm]));
						}
						else {
							sprintf(temp_str, "addi %s,%s,%d\n", currTemps[i], args[i], (-immediate[currImm]));
						}
						// Append to 'out'
						strcat(out, temp_str);
					}
				}
				else { // If the immediate is not in the first operation
					if (strcmp(args[i + 1], "immediate") == 0) { // If the next argument is an immediate
						if (strcmp(op, "add") == 0) { // Immediate addition algorithm, commented above
							if (useTarget) {
								*temp_offset -= 1;
								sprintf(temp_str, "addi %s,%s,%d\n", target, currTemps[i - 1], immediate[currImm]);
							}
							else {
								sprintf(temp_str, "addi %s,%s,%d\n", currTemps[i], currTemps[i - 1], immediate[currImm]);
							}
							// Append to 'out'
							strcat(out, temp_str);
						}
						else { // Immediate subtraction algorithm, commented above
							if (useTarget) {
								*temp_offset -= 1;
								sprintf(temp_str, "addi %s,%s,%d\n", target, currTemps[i - 1], (-immediate[currImm]));
							}
							else {
								sprintf(temp_str, "addi %s,%s,%d\n", currTemps[i], currTemps[i - 1], (-immediate[currImm]));
							}
							// Append to 'out'
							strcat(out, temp_str);
						}
					}
					else {
						if (useTarget) {
							*temp_offset -= 1;
							sprintf(temp_str, "%s %s,%s,%s\n", op, target, currTemps[i - 1], args[i + 1]);
						}
						else {
							sprintf(temp_str, "%s %s,%s,%s\n", op, currTemps[i], currTemps[i - 1], args[i + 1]);
						}
						// Append to 'out'
						strcat(out, temp_str);
					}
				}
				PEMDAS[i] = currOp;
				currOp++;
			}
		}
	}
	for(int i = 0; i < 20; i++) {
		free(currTemps[i]);
	}
	free(currTemps);
	free(PEMDAS);
	return out;
}
//Function to parse the input file and generate the MIPS code
bool parseLine(char* input, char** arr, int* num_lines_ptr) {
	int currVar = 0;  // Current variable index
	char temp_str[100] = "";  // Temporary buffer 
	char* vars = (char*)malloc(10 * sizeof(char)); // Allocate memory for the variables
	vars[0] = '\0'; // Initialize the variables
	char** varsMIPS = (char**)malloc(10 * sizeof(char*)); // Allocate memory for the MIPS variables used in place of the letters
	for (int i = 0; i < 10; i++) { // Loop through the variables and initialize their strings
		varsMIPS[i] = (char*)malloc(10 * sizeof(char));
		varsMIPS[i][0] = '\0';
	}
	char line[128]; // Allocate memory for the line
	int* immediate = (int*)malloc(10 * sizeof(int)); // Allocate memory for the immediate values
	int indexImm = 0; // Immediate index
	int numArgs = 0; // Number of arguments
	FILE* file = fopen(input, "r"); // Open the file
	if (file == NULL) {
		printf("Error: File not found\n");
		return false;
	}
	char** currLineArgs = (char**)malloc(20 * sizeof(char*)); // Allocate memory for the current line arguments
	for (int i = 0; i < 20; i++) {
		currLineArgs[i] = (char*)malloc(10 * sizeof(char));
		currLineArgs[i][0] = '\0';
	}
	enum Operation* currOps = (enum Operation*)malloc(20 * sizeof(enum Operation)); // Allocate memory for the current operations
	int numOps = 0; // Number of operations
	int* temp_offset = (int*)malloc(sizeof(int)); // Allocate memory for the temp offset used in generateLine
	*temp_offset = 0; // Initialize the temp offset
	int* label_offset = (int*)malloc(sizeof(int)); // Allocate memory for the label offset used in generateLine
	*label_offset = 0; // Initialize the label offset
	char* targetReg = (char*)malloc(10 * sizeof(char)); // Allocate memory for the target register
	int currLine = 0; // Current line index
	while (fgets(line, sizeof(line), file)) { // Loop through the lines of the file

		for (int i = 0; i < strlen(line); i++) { // Loop through the characters of the line
			if (line[i] == ' ') { // If the character is a space, move past it
				continue;
			}
			else if (isLetter(line[i])) { // If the character is a letter, set it as the target register and record it in the variables arrays
				for (int j = 0; j < currVar; j++) { // Loop through the variables
					if (line[i] == vars[j]) { // If the variable is already in the variables array, break
						break;
					}
				}
				vars[currVar] = line[i]; // Set the variable in the variables array
				sprintf(temp_str, "$s%d", currVar); // Generate the MIPS variable
				strcat(varsMIPS[currVar], temp_str); // Append the MIPS variable to the MIPS variables array
				strcpy(targetReg, temp_str); // Set the target register
				currVar++; // Increment the current variable

			}
			else if (line[i] == '=') { // If the character is an equals sign, start to process the expression
				for (int j = i + 1; j < strlen(line); j++) { // Loop through the characters of the expression after the equals sign
					if (line[j] == ' ') { // If the character is a space, move past it
						continue;
					}
					else if (isdigit(line[j])) { // If the character is a digit, set it as an immediate value
						if (line[j - 1] == '-') { // If the immediate is negative, set it as negative
							immediate[indexImm] = -atoi(&line[j]);
						}
						else { // If the immediate is not negative, set it as positive
							immediate[indexImm] = atoi(&line[j]);
						}
						while (isdigit(line[j])) { // Move past the immediate value
							j++;
						}
						j--;
						indexImm++; // Increment the immediate index
						strcpy(currLineArgs[numArgs], "immediate"); // Set the argument as immediate
						numArgs++;
					}
					else if (isLetter(line[j])) { // If the character is a letter, set it as an argument
						for (int k = 0; k < currVar; k++) {
							if (line[j] == vars[k]) {
								strcpy(currLineArgs[numArgs], varsMIPS[k]); // Set the argument as the MIPS variable
								numArgs++; 
							}
						}
					}
					else if (line[j] == '+') { // If the character is a plus sign, set the operation as addition
						currOps[numOps] = ADDITION;
						numOps++;
					}
					else if (line[j] == '-' && line[j + 1] == ' ') { // If the character is a minus sign, set the operation as subtraction
						currOps[numOps] = SUBTRACTION;
						numOps++;
					}
					else if (line[j] == '*') { // If the character is an asterisk, set the operation as multiplication
						currOps[numOps] = MULTIPLICATION;
						numOps++;
					}
					else if (line[j] == '/') { // If the character is a forward slash, set the operation as division
						currOps[numOps] = DIVISION;
						numOps++;
					}
					else if (line[j] == '%') { // If the character is a percent sign, set the operation as modulus
						currOps[numOps] = MODULUS;
						numOps++;
					}
					else if (line[j] == ';') { // If the character is a semicolon, break to generate the line
						i = strlen(line);
					}
					else if (line[j] == '\n') { // If the character is a newline, break to generate the line
						line[j] = '\0';
						i = strlen(line);
					}
				}
			}
		}
		arr[currLine] = generateLine(line, currLineArgs, numArgs, currOps, numOps, immediate, targetReg, temp_offset, label_offset); // Generate the MIPS code for the line
		currLine++; // Increment the current line
		*num_lines_ptr = currLine; // Set the number of lines
		//Reset the variables
		numOps = 0;
		indexImm = 0;
		numArgs = 0;
	}
	for (int i = 0; i < 20; i++) { // Loop through the current line arguments and free them
	
		free(currLineArgs[i]);
	}
	free(currLineArgs); // Free the current line arguments
	for (int i = 0; i < 10; i++) { // Loop through the variables and free the MIPS variables
		free(varsMIPS[i]);
	}
	free(varsMIPS); // Free the MIPS variables
	free(immediate); // Free the immediate values
	free(targetReg); // Free the target register
	free(temp_offset); // Free the temp offset
	free(label_offset); // Free the label offset
	free(vars); // Free the variables
	free(currOps); // Free the current operations
	fclose(file); // Close the file
	return true; 
}
int main(int argc, char* argv[]) {
	char** lines = (char**)malloc(20 * sizeof(char*));
	int* num_lines_ptr = (int*)malloc(sizeof(int));
	*num_lines_ptr = 0;
	// Run the parsing an generating and check if it fails
	if (!parseLine(argv[1], lines, num_lines_ptr)) {
		return 1;
	}
	// Print the MIPS code
	for (int i = 0; i < *num_lines_ptr; i++) {
		printf("%s", lines[i]);
	}
	for(int i = 0; i < *num_lines_ptr; i++) {
		free(lines[i]);
	}
	free(lines);
	free(num_lines_ptr);
	return 0;
}