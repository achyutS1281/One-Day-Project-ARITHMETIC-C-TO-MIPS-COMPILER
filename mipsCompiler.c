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
	for(int i = 0; i < strlen(letterArray); i++) {
		if(c == letterArray[i]) {
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
	char temp_str[100];  // Temporary buffer 
	sprintf(temp_str, "# %s\n", line);
	strcat(out, temp_str);  // Append the result to 'out'
	char* op;
	char** currTemps = (char**)malloc(20 * sizeof(char*)); // Allocate memory for the current temp registers
	bool useTarget = false; // Flag to check if the target register is used
	for (int i = 0; i < 20; i++) { // Loop through the current temp registers
		currTemps[i] = (char*)malloc(10 * sizeof(char)); // Allocate memory for the current temp register
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
			if(i == numOps - 1) { // If the operation is the last one
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
				if(ops[i] == MODULUS) { // If the operation is a modulus
					mod = true;
				}
				sprintf(temp_str, "$t%d", currOp); // Generate the new temp register
				strcat(currTemps[i], temp_str);
				if (i > 0 && PEMDAS[i - 1] == -1 && PEMDAS[i + 1] == -1) { // * In case of multiple types of expressions (PEMDAS Order). If the previous and next PEMDAS are -1, meaning no operations have been done on either side
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
								if(immediate[currImm] % 2 != 0) { // If the immediate is not divisible by 2, we need to add the argument again to account for 2^0
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
										sprintf(temp_str, "mflo $t%d\n", currOp+1); //mflo for division
									}
									strcat(out, temp_str);
									if (power) { // If the immediate is a power of 2
										sprintf(temp_str, "L%d:\n", *label_offset + 1);
										strcat(out, temp_str);
									}
									*label_offset += 2; // Increment the label offset by 2 for the two labels used
									*temp_offset += 1; // Increment the temp offset by 1 for the new temp register
								}
								sprintf(temp_str, "$t%d", currOp+1); // Generate the new temp register
								strcpy(currTemps[i], temp_str);
								currOp++;
							}
						}
					}
					else {
						sprintf(temp_str, "%s %s,%s\n", op, args[i], args[i + 1]);
						strcat(out, temp_str);
						if (mod) {
							if (useTarget) {
								*temp_offset -= 1;
								sprintf(temp_str, "mfhi %s\n", target);
							}
							else {
								sprintf(temp_str, "mfhi %s\n", currTemps[i]);
							}
						}
						else {
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
				}
				else if (PEMDAS[i + 1] != -1) {
					sprintf(temp_str, "%s %s,%s\n", op, args[i], currTemps[i + 1]);
					strcat(out, temp_str);
					if (mod) {
						if (useTarget) {
							*temp_offset -= 1;
							sprintf(temp_str, "mfhi %s\n", target);
						}
						else {
							sprintf(temp_str, "mfhi %s\n", currTemps[i]);
						}
					}
					else {
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
				else if (i > 0 && PEMDAS[i + 1] != -1 && PEMDAS[i - 1] != -1) {
					sprintf(temp_str, "%s %s,%s\n", op, currTemps[i - 1], currTemps[i + 1]);
					strcat(out, temp_str);
					if (mod) {
						if (useTarget) {
							*temp_offset -= 1;
							sprintf(temp_str, "mfhi %s\n", target);
						}
						else {
							sprintf(temp_str, "mfhi %s\n", currTemps[i]);
						}
					}
					else {
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
				else if (i > 0 && PEMDAS[i - 1] != -1) {
					if (strcmp(args[i + 1], "immediate") == 0) {
						if (strcmp(op, "mult") == 0) {
							if (immediate[currImm] == 1 || immediate[currImm] == -1) {
								sprintf(temp_str, "$t%d", currOp + 1);
								char* currMove = (char*)malloc(10 * sizeof(char));
								sprintf(temp_str, "move %s,%s\n", currTemps[i], currTemps[i - 1]);
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
										
										sprintf(temp_str, "sll %s,%s,%d\n", currTemps[i], currTemps[i - 1], powers[j]);
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
						else {
							if (immediate[currImm] == 1) {
								if (useTarget) {
									*temp_offset -= 1;
									sprintf(temp_str, "move %s,%s\n", target, currTemps[i - 1]);
								}
								else {
									sprintf(temp_str, "move %s,%s\n", currTemps[i], currTemps[i - 1]);
								}
								strcat(out, temp_str);
							}
							else if (immediate[currImm] == -1) {
								if (useTarget) {
									*temp_offset -= 1;
									sprintf(temp_str, "sub %s,$zero,%s\n", target, currTemps[i - 1]);
								}
								else {
									sprintf(temp_str, "sub %s,$zero,%s\n", currTemps[i], currTemps[i - 1]);
								}
								strcat(out, temp_str);
							}
							else {
								if (mod) {
									sprintf(temp_str, "li $t%d,%d\n", currOp, immediate[currImm]);
									strcat(out, temp_str);
									sprintf(temp_str, "div %s,$t%d\n", currTemps[i - 1], currOp);
									strcat(out, temp_str);
									if(useTarget) {
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
										sprintf(temp_str, "bltz %s,L%d\n", currTemps[i - 1], *label_offset);
										strcat(out, temp_str);
										int powerLog = log2(abs(immediate[currImm]));
										if (useTarget) {
											*temp_offset -= 1;
											sprintf(temp_str, "srl %s,%s,%d\n", target, currTemps[i - 1], powerLog);
										}
										else {
											sprintf(temp_str, "srl %s,%s,%d\n", currTemps[i], currTemps[i - 1], powerLog);
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
									sprintf(temp_str, "div %s,$t%d\n", currTemps[i - 1], currOp);
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
					else {
						sprintf(temp_str, "%s %s,%s\n", op, currTemps[i - 1], args[i + 1]);
						strcat(out, temp_str);
						if (mod) {
							if (useTarget) {
								*temp_offset -= 1;
								sprintf(temp_str, "mfhi %s\n", target);
							}
							else {
								sprintf(temp_str, "mfhi %s\n", currTemps[i]);
							}
						}
						else {
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
					
				}
				else if (i == 0 && strcmp(args[i + 1], "immediate") == 0) {
					
					if (strcmp(op, "mult") == 0) {
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
					else {
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
								if(useTarget) {
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
				else {
					if (strcmp(args[i + 1], "immediate") == 0) {
						if (strcmp(op, "mult") == 0) {
							if(immediate[currImm] == 1 || immediate[currImm] == -1) {
								sprintf(temp_str, "$t%d", currOp + 1);
								char* currMove = (char*)malloc(10 * sizeof(char));
								sprintf(temp_str, "move %s,%s\n", currTemps[i], currTemps[i - 1]);
								if(immediate[currImm] == -1) {
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
							else if(immediate[currImm] == 0){
								if(useTarget) {
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
										sprintf(temp_str, "sll %s,%s,%d\n", currTemps[i], currTemps[i-1], powers[j]);
										strcat(out, temp_str);  // Append the result to 'out'
										if (!happened) {
											sprintf(temp_str, "move %s,%s\n", currMove, currTemps[i]);
											strcat(out, temp_str);  // Append the result to 'out'
										}else {
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
								strcpy(currTemps[i], currMove);
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
						else {
							if (immediate[currImm] == 1) {
								if(useTarget) {
									*temp_offset -= 1;
									sprintf(temp_str, "move %s,%s\n", target, currTemps[i - 1]);
								}
								else {
									sprintf(temp_str, "move %s,%s\n", currTemps[i], currTemps[i - 1]);
								}
								strcat(out, temp_str);
							}
							else if (immediate[currImm] == -1) {
								if(useTarget) {
									*temp_offset -= 1;
									sprintf(temp_str, "sub %s,$zero,%s\n", target, currTemps[i - 1]);
								}
								else {
									sprintf(temp_str, "sub %s,$zero,%s\n", currTemps[i], currTemps[i - 1]);
								}
								strcat(out, temp_str);
							}
							else {
								if (mod) {
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
								else {

									bool power = is_power_of_two(abs(immediate[currImm]));
									if (power) {
										sprintf(temp_str, "bltz %s,L%d\n", currTemps[i - 1], *label_offset);
										strcat(out, temp_str);
										int powerLog = log2(abs(immediate[currImm]));
										if (useTarget) {
											*temp_offset -= 1;
											sprintf(temp_str, "srl %s,%s,%d\n", target, currTemps[i - 1], powerLog);
										}
										else {
											sprintf(temp_str, "srl %s,%s,%d\n", currTemps[i], currTemps[i - 1], powerLog);
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
									sprintf(temp_str, "div %s,$t%d\n", currTemps[i - 1], currOp);
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
					else {
						sprintf(temp_str, "%s %s,%s\n", op, args[i], args[i + 1]);
						strcat(out, temp_str);
						if (mod) {
							if (useTarget) {
								*temp_offset -= 1;
								sprintf(temp_str, "mfhi %s\n", target);
							}
							else {
								sprintf(temp_str, "mfhi %s\n", currTemps[i]);
							}
						}
						else {
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

				}
				if (strcmp(args[i + 1], "immediate") == 0) {
					currImm++;
				}
				PEMDAS[i] = currOp;
				currOp++;
			}
		}
		for (int i = 0; i < numOps; i++) {
			if (i == numOps - 1) {
				useTarget = true;
			}
			int currImm = 0;
			if (ops[i] == ADDITION || ops[i] == SUBTRACTION) {
				*temp_offset += 1;
				sprintf(temp_str, "$t%d", currOp);
				strcat(currTemps[i], temp_str);
				if (ops[i] == ADDITION) {
					op = "add";
				}
				else {
					op = "sub";
				}
				if (i > 0 && PEMDAS[i - 1] == -1 && PEMDAS[i + 1] == -1) {
					if (strcmp(args[i + 1], "immediate") == 0) {
						if (strcmp(op, "add") == 0) {
							if(useTarget) {
								*temp_offset -= 1;
								sprintf(temp_str, "addi %s,%s,%d\n", target, args[i], immediate[currImm]);
							}
							else {
								sprintf(temp_str, "addi %s,%s,%d\n", currTemps[i], args[i], immediate[currImm]);
							}

							// Append to 'out'
							strcat(out, temp_str);
						}
						else {
							if(useTarget) {
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
					else {
						if(useTarget) {
							*temp_offset -= 1;
							sprintf(temp_str, "%s %s,%s,%s\n", op, target, args[i], args[i + 1]);
						}
						else {
							sprintf(temp_str, "%s %s,%s,%s\n", op, currTemps[i], args[i], args[i + 1]);
						}
					}
				}
				else if (i > 0 && PEMDAS[i + 1] != -1 && PEMDAS[i - 1] != -1) {
					if (strcmp(args[i + 1], "immediate") == 0) {
						if (strcmp(op, "add") == 0) {
							if(useTarget) {
								*temp_offset -= 1;
								sprintf(temp_str, "addi %s,%s,%d\n", target, currTemps[i - 1], immediate[currImm]);
							}
							else {
								sprintf(temp_str, "addi %s,%s,%d\n", currTemps[i], currTemps[i - 1], immediate[currImm]);
							}

							// Append to 'out'
							strcat(out, temp_str);
						}
						else {
							if(useTarget) {
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
						if(useTarget) {
							*temp_offset -= 1;
							sprintf(temp_str, "%s %s,%s,%s\n", op, target, currTemps[i - 1], currTemps[i + 1]);
						}
						else {
							sprintf(temp_str, "%s %s,%s,%s\n", op, currTemps[i], currTemps[i - 1], currTemps[i + 1]);
						}
					}
					
				}
				else if (PEMDAS[i + 1] != -1) {
					sprintf(temp_str, "%s %s,%s,%s\n", op, currTemps[i], args[i], currTemps[i + 1]);
					strcat(out, temp_str);
				}
				else if (i == 0 && strcmp(args[i + 1], "immediate") != 0) {
					if (useTarget) {
						*temp_offset -= 1;
						sprintf(temp_str, "%s %s,%s,%s\n", op, target, args[i], args[i + 1]);
					}else {
						sprintf(temp_str, "%s %s,%s,%s\n", op, currTemps[i], args[i], args[i + 1]);
					}
					strcat(out, temp_str);
				}
				else if (i == 0 && strcmp(args[i + 1], "immediate") == 0) {
					if (strcmp(op, "add") == 0) {
						if(useTarget) {
							*temp_offset -= 1;
							sprintf(temp_str, "addi %s,%s,%d\n", target, args[i], immediate[currImm]);
						}
						else {
							sprintf(temp_str, "addi %s,%s,%d\n", currTemps[i], args[i], immediate[currImm]);
						}
						// Append to 'out'
						strcat(out, temp_str);
					}
					else {
						if(useTarget) {
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
				else {
					if (strcmp(args[i + 1], "immediate") == 0) {
						if (strcmp(op, "add") == 0) {
							if(useTarget) {
								*temp_offset -= 1;
								sprintf(temp_str, "addi %s,%s,%d\n", target, currTemps[i-1], immediate[currImm]);
							}
							else {
								sprintf(temp_str, "addi %s,%s,%d\n", currTemps[i], currTemps[i - 1], immediate[currImm]);
							}
							// Append to 'out'
							strcat(out, temp_str);
						}
						else {
							if(useTarget) {
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
						if(useTarget) {
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
	free(currTemps);
	free(PEMDAS);
	return out;
}
bool parseLine(char* input, char** arr, int* num_lines_ptr) {
	int currVar = 0;
	char temp_str[100];  // Temporary buffer 
	char* vars = (char*)malloc(10 * sizeof(char));
	char** varsMIPS = (char**)malloc(10 * sizeof(char*));
	for (int i = 0; i < 10; i++) {
		varsMIPS[i] = (char*)malloc(10 * sizeof(char));
	}
	char line[128];
	int* immediate = (int*)malloc(10*sizeof(int));
	int indexImm = 0;
	int numArgs = 0;
	FILE *file = fopen(input, "r");
	char** currLineArgs = (char**)malloc(20 * sizeof(char*));
	for (int i = 0; i < 20; i++) {
		currLineArgs[i] = (char*)malloc(10 * sizeof(char));

	}
	enum Operation* currOps = (enum Operation*)malloc(20 * sizeof(enum Operation));
	int numOps = 0;
	int* temp_offset = (int*)malloc(sizeof(int));
	*temp_offset = 0;
	int* label_offset = (int*)malloc(sizeof(int));
	*label_offset = 0;
	char* targetReg = (char*)malloc(10 * sizeof(char));
	if(file == NULL) {
		printf("Error: File not found\n");
		return false;
	}
	int currLine = 0;
	while(fgets(line, sizeof(line), file)) {
		
		for (int i = 0; i < strlen(line); i++) {
			if (line[i] == ' ') {
				continue;
			}
			else if (isLetter(line[i])) {
				for(int j = 0; j < strlen(vars); j++) {
					if(line[i] == vars[j]) {
						break;
					}
				}
				vars[currVar] = line[i];
				sprintf(temp_str, "$s%d", currVar);
				strcat(varsMIPS[currVar], temp_str);
				strcpy(targetReg, temp_str);
				currVar++;
				
			}
			else if (line[i] == '=') {
				for(int j = i+1; j < strlen(line); j++) {
					if(line[j] == ' ') {
						continue;
					}
					else if(isdigit(line[j])) {
						if(line[j-1] == '-') {
							immediate[indexImm] = -atoi(&line[j]);
						}
						else {
							immediate[indexImm] = atoi(&line[j]);
						}
						while(isdigit(line[j])) {
							j++;
						}
						j--;
						indexImm++;
						currLineArgs[numArgs] = "immediate";
						numArgs++;
					}
					else if(isLetter(line[j])) {
						for(int k = 0; k < strlen(vars); k++) {
							if(line[j] == vars[k]) {
								currLineArgs[numArgs] = varsMIPS[k];
								numArgs++;
							}
						}
					}
					else if(line[j] == '+') {
						currOps[numOps] = ADDITION;
						numOps++;
					}
					else if(line[j] == '-' && line[j+1] == ' ') {
						currOps[numOps] = SUBTRACTION;
						numOps++;
					}
					else if(line[j] == '*') {
						currOps[numOps] = MULTIPLICATION;
						numOps++;
					}
					else if(line[j] == '/') {
						currOps[numOps] = DIVISION;
						numOps++;
					}
					else if (line[j] == '%') {
						currOps[numOps] = MODULUS;
						numOps++;
					}
					else if (line[j] == ';') {
						i = strlen(line);
					}
					else if(line[j] == '\n') {
						line[j] = '\0';
						i = strlen(line);
					}
				}
			}
		}
		arr[currLine] = generateLine(line, currLineArgs, numArgs, currOps, numOps, immediate, targetReg, temp_offset, label_offset);
		currLine++;
		*num_lines_ptr = currLine;
		numOps = 0;
		indexImm = 0;
		numArgs = 0;
	}
	fclose(file);
	return true;
}
int main(int argc, char* argv[]) {
	char** lines = (char**)malloc(20 * sizeof(char*));
	int* num_lines_ptr = (int*)malloc(sizeof(int));
	*num_lines_ptr = 0;
	if(!parseLine(argv[1], lines, num_lines_ptr)) {
		return 1;
	}
	for(int i = 0; i < *num_lines_ptr; i++) {

		printf("%s", lines[i]);
	}
	return 0;
}