/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

// Includes
#include "main.h"
#include "stdio.h"
#include "stdbool.h"
#include "string.h"

// Init Functions
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
void SysTick_Initialize(uint32_t);

// Delay Function
void Delay(int delay);

// Keypad Functions
unsigned char detectkey(void);
bool iskeypressed(void);
void codeentry(char* entry, bool admin);

// LCD Functions
void LCD_nibble_write(uint8_t temp, uint8_t s); //configures message for data (1) or instructions (0) with s
void Write_SR_LCD(uint8_t temp); //writes data to lcd shift register
void Write_Instr_LCD(uint8_t code); //writes instructions to LCD
void Write_Char_LCD(uint8_t code); //writes character to LCD
void Write_String_LCD(char *temp); //writes string to LCD

// Passcode Functions
uint8_t checkcode(char* entry, char codes[][4], uint16_t total); // Validates codes, 0 = incorrect, 1 = correct, 2 = admin
uint16_t editcodes(char codes[][4], uint16_t total, bool mode); // Add/Removes codes
void clearcodes(char codes[][4]); // Clear all codes
void displaycodes(char codes[][4], uint16_t total); // Display available codes

// LED Functions
void setleds(GPIO_PinState); // Sets LED states
void flashleds(bool state); // Flashes LEDs if true is passed

// Speaker Functions
void buzz(int time); // Buzz speaker for given time in ms

// Admin functions
uint16_t adminmenu(char codes[][4], uint16_t total);

// Global Variables
const int CODESIZE = 5; // Sets max codes, max 999, min 1
const char ADMIN[4] = {'2' , '5', '8', '0'}; // Used for admin functions of lock

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();
  /* Configure the system clock */
  SystemClock_Config();
  /* Initialize all configured peripherals */
  MX_GPIO_Init();
	
	// Variables
	char codes[CODESIZE][4]; // Stores all codes for comparison
	char entry[4] = {' ' , ' ', ' ', ' '}; // Stores current code
	uint16_t totalcodes = 0;
	char* line = NULL;
	char num[3], unlocked[16];
	int lockstate = 0, unlockedcount = 0;
		
	// Reset LED
	flashleds(false);
	setleds(GPIO_PIN_RESET);
	
/* LCD controller reset sequence*/ 
	Delay(20);
	LCD_nibble_write(0x30,0);
	Delay(5);
	LCD_nibble_write(0x30,0);
	Delay(1);
	LCD_nibble_write(0x30,0);
	Delay(1);
	LCD_nibble_write(0x20,0);
	Delay(1);
	Write_Instr_LCD(0x28); /* set 4 bit data LCD - two line display - 5x8 font*/
	Write_Instr_LCD(0x0C); /* turn on display, turn off cursor*/
	Write_Instr_LCD(0x01); /* clear display screen and return to home position*/
	Write_Instr_LCD(0x06); /* set write direction */
	
	// Write Digital Lock to screen
	line = "Digital Lock";
	Write_String_LCD(line);
	Delay(1500);
	Write_Instr_LCD(0x01); // Clear Screen
	
	// Ask for initial code
	line = "Enter Code:";
	Write_String_LCD(line);
	Write_Instr_LCD(0xC0); // Go to bottom line
	
	// Wait for inital code
	codeentry(entry, false);
	
	//save initial code
	for (int i = 0; i < 4; i++) {
		codes[0][i] = entry[i];
	}
	totalcodes++;
	
	// End startup
	Write_Instr_LCD(0x01); // Clear Screen
	
	while (1) {
		// Default to locked
		Write_Instr_LCD(0x01); // Clear Screen
		line = "LOCKED";
		Write_String_LCD(line); // Write locked
		setleds(GPIO_PIN_SET); // Turn on LEDS
		Write_Instr_LCD(0xC0); // Go to bottom line
		
		// Wait for code entry
		codeentry(entry, true);
		
		// Check code against others
		lockstate = checkcode(entry, codes, totalcodes);
		
		switch (lockstate) {
			case 0: // Incorrect Code
				Write_Instr_LCD(0x01); // Clear Screen
				line = "INVALID CODE";
				Write_String_LCD(line); // Write invalid code
				flashleds(true); // Enable flashing systick
				buzz(3500); // Buzz speaker
				flashleds(false); // Disable flashing systick
				break;
			case 1: // Correct Code
				Write_Instr_LCD(0x01); // Clear Screen
				line = "UNLOCKED";
				Write_String_LCD(line); // Write unlocked
				setleds(GPIO_PIN_RESET); // Turn off LEDS
				Write_Instr_LCD(0xC0); // Go to bottom line
				line = "10";
				Write_String_LCD(line); // Write 10
				Delay(1000);

				for (int i=9; i > 0; i--) { // Screen Countdown
					Write_Instr_LCD(0x10); // Decrement cursor
					Write_Instr_LCD(0x10);
					line = "  ";
					Write_String_LCD(line); // Erase previous output
					Write_Instr_LCD(0x10); // Decrement cursor
					Write_Instr_LCD(0x10);
					snprintf(num, 3, "0%d", i); // Convert i to string
					Write_String_LCD(num); // Write i
					Delay(1000);
				}
				Write_Instr_LCD(0x01); // Clear Screen
				unlockedcount++;
				line = "# OF UNLOCKS:";
				Write_String_LCD(line); // Write unlocked
				Write_Instr_LCD(0xC0); // Go to bottom line
				snprintf(unlocked, 16, "%d", unlockedcount); // Convert i to string
				Write_String_LCD(unlocked); // Write unlocked array
				Delay(1500);
				break;
			case 2: // Admin Code
				totalcodes = adminmenu(codes, totalcodes);
		}
		
	}
}



// Set up the GPIO pins
static void MX_GPIO_Init(void)
{
  uint32_t temp;
	
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	
  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	
	/*Configure KeypadCol GPIO pin : PB1 - PB4 */
  GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	/*Configure KeypadRow GPIO pin : PB8 - PB11 */
  GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	/*Configure LCD GPIO pin : PA5 + PA10*/
	temp = GPIOA->MODER;
	temp &= ~(0x03<<(2*5));
	temp|=(0x01<<(2*5));
	temp &= ~(0x03<<(2*10));
	temp|=(0x01<<(2*10));
	GPIOA->MODER = temp;
	temp=GPIOA->OTYPER;
	temp &=~(0x01<<5);
	temp &=~(0x01<<10);
	GPIOA->OTYPER=temp;temp=GPIOA->PUPDR;
	temp&=~(0x03<<(2*5));
	temp&=~(0x03<<(2*10));
	GPIOA->PUPDR=temp;
	
	/*Configure LCD GPIO pin : PB5*/
	temp = GPIOB->MODER;
	temp &= ~(0x03<<(2*5));
	temp|=(0x01<<(2*5));
	GPIOB->MODER = temp;
	temp=GPIOB->OTYPER;
	temp &=~(0x01<<5);
	GPIOB->OTYPER=temp;
	temp=GPIOB->PUPDR;
	temp&=~(0x03<<(2*5));
	GPIOB->PUPDR=temp;
	
	/*Configure LED GPIO pins : PA0 + PA1*/
	GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	
	/*Configure LED + Speaker GPIO pins : PC7 - PC9*/
	GPIO_InitStruct.Pin = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
	
}


// Detects what key is pressed
unsigned char detectkey(void) 
{
	unsigned char keymap[4][4] =
		{{'1', '2', '3', 'A'},
		{'4', '5', '6', 'B'},
		{'7', '8', '9', 'C'},
		{'*', '0', '#', 'D'}};
	int col = 0, row = 0;
	uint16_t colpins[4] = {GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3, GPIO_PIN_4};
	uint16_t rowpins[4] = {GPIO_PIN_8, GPIO_PIN_9, GPIO_PIN_10, GPIO_PIN_11};
	
	// Setting all columns high
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4, GPIO_PIN_SET);
	
	// Detecting if a key is pressed
	while (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11) == GPIO_PIN_RESET);	
	
	// Detecting which row was pressed
	for (int i = 0; i < 4; i++) {
		
		// Checking test row
		if (HAL_GPIO_ReadPin(GPIOB, rowpins[i]) == GPIO_PIN_SET) {
			row = i;
			break;
		}
		
	}
	
	// Detecting which column was pressed
	for (int i = 0; i < 4; i++) {
		
		// Setting all columns low
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4, GPIO_PIN_RESET);
		
		// Setting test column high
		HAL_GPIO_WritePin(GPIOB, colpins[i], GPIO_PIN_SET);
		
		// Checking rows
		if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11) == GPIO_PIN_SET) {
			col = i;
			break;
		}
		
	}
	
	// Handle held key
	Delay(10);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4, GPIO_PIN_SET);
	while (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11) == GPIO_PIN_SET);	
	Delay(10);
	
	// Return character
	return keymap[row][col];
		
}

// Writes to the LCD
void Write_SR_LCD(uint8_t temp)
{
	int i;
	uint8_t mask=0x80;
	for(i=0; i<8; i++) {
		if((temp&mask)==0)
		GPIOB->ODR&=~(1<<5);
		else
		GPIOB->ODR|=(1<<5);
		
		/* Sclck */
		GPIOA->ODR&=~(1<<5);
		GPIOA->ODR|=(1<<5);
		Delay(1);
		mask=mask>>1;
	}
	/*Latch*/
	GPIOA->ODR|=(1<<10);
	GPIOA->ODR&=~(1<<10);

}


// Sets up the nibbles for writing to LCD
void LCD_nibble_write(uint8_t temp, uint8_t s)
{
	/*writing instruction*/
	if (s==0){
		temp=temp&0xF0;
		temp=temp|0x02; /*RS (bit 0) = 0 for Command EN (bit1)=high */
		Write_SR_LCD(temp);
		temp=temp&0xFD; /*RS (bit 0) = 0 for Command EN (bit1) = low*/
		Write_SR_LCD(temp); }
	/*writing data*/
	else if (s==1) {
		temp=temp&0xF0;
		temp=temp|0x03; /*RS(bit 0)=1 for data EN (bit1) = high*/
		Write_SR_LCD(temp);
		temp=temp&0xFD; /*RS(bit 0)=1 for data EN(bit1) = low*/
		Write_SR_LCD(temp);
}}

// Writes instructions to LCD
void Write_Instr_LCD(uint8_t code)
{
	LCD_nibble_write(code&0xF0,0);
	code=code<<4;
	LCD_nibble_write(code,0);
}


// Writes characters to LCD
void Write_Char_LCD(uint8_t code)
{
	LCD_nibble_write(code&0xF0,1);
	code=code<<4;
	LCD_nibble_write(code,1);
}


// Writes strings to LCD
void Write_String_LCD(char *temp)
{
	int i=0;
	while(temp[i]!=0)
	{
		Write_Char_LCD(temp[i]);
		i=i+1;
	}
}


// Detects if a key is pressed
bool iskeypressed(void)
{
	// Setting all columns high
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4, GPIO_PIN_SET);
	
	// Detecting if a key is pressed
	if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11) == GPIO_PIN_RESET) {
		return false;
	} else {
		return true;
	}
}


// Handles code entry
void codeentry(char* entry, bool admin)
{
	char keypressed;
	uint8_t length = 0, admincode = 0, temp1 = 0, temp2 = 0;
	char* line;
	bool seecode = 0;
	
	while (1) { // Breaks when user hits A
		keypressed = detectkey(); // Wait for input
		switch (keypressed) {
				case '*': // Do nothing (unused)
					break;
				case '#': // Do nothing (unused)
					break;
				case 'A': // Enter
					if (length == 4) { // Confirms full passcode is entered
						for (int i = 0; i < 4; i++) { // Checks for admin code
							if (entry[i] == ADMIN[i]) {
								admincode++;
							}
						}
						if (admincode == 4 && admin == false) { // If admin code is not an option, inform user
							for (int i = 0; i < 4; i++) {
								Write_Instr_LCD(0x10); // Move cursor left
								Write_Char_LCD(' '); // Print space
								Write_Instr_LCD(0x10); // Move cursor left
							}
							line = "INVALID CODE";
							Write_String_LCD(line);
							Delay(1250);
							for (int i = 0; i < 12; i++) {
								Write_Instr_LCD(0x10); // Move cursor left
								Write_Char_LCD(' '); // Print space
								Write_Instr_LCD(0x10); // Move cursor left
							}
							line = "CHANGE CODE";
							Write_String_LCD(line);
							Delay(1250);
							for (int i = 0; i < 11; i++) {
								Write_Instr_LCD(0x10); // Move cursor left
								Write_Char_LCD(' '); // Print space
								Write_Instr_LCD(0x10); // Move cursor left
							}
							admincode = 0;
							length = 0;
						} else {
							return;
						}
					}
					break;
				case 'B': // Backspace
					if (length > 0) {
						Write_Instr_LCD(0x10); // Move cursor left
						Write_Char_LCD(' '); // Print space
						Write_Instr_LCD(0x10); // Move cursor left
						length--;
						entry[length] = ' '; // Remove character from array
					}
					break;
				case 'C': // Clear
					while (length > 0) { // Loop for all characters
						Write_Instr_LCD(0x10); // Move cursor left
						Write_Char_LCD(' '); // Print space
						Write_Instr_LCD(0x10); // Move cursor left
						length--;
						entry[length] = ' '; // Remove character from array
					}
					break;
				case 'D': // Changes * to proper characters
					if (seecode == 0) {
					temp1 = length - 1;
					temp2 = length;
						while (temp2 != 0) { // Loop for all characters
								Write_Instr_LCD(0x10); // Move cursor left
								Write_Char_LCD(entry[temp1]); // Print space
								Write_Instr_LCD(0x10); // Move cursor left
								temp1--;
								temp2--;
							}
							while (temp2 < length) {
								Write_Instr_LCD(0x14);
								temp2++;
							}
							seecode = 1;
						}
						else if (seecode == 1) {
							temp2 = length;
							while (temp2 > 0) {
								Write_Instr_LCD(0x10);
								temp2--;
							}
							while (temp2 < length) {
									Write_Char_LCD('*');
									temp2++;
							}
							seecode = 0;
						}
					break;
				default:
					if (seecode == 0) {
						if (length < 4) {
							Write_Char_LCD('*'); // Write pressed character
							entry[length] = keypressed; // Add character to array
							length++;
						}
					}
					else if (seecode == 1){
						if (length < 4) {
							Write_Char_LCD(keypressed); // Write pressed character
							entry[length] = keypressed; // Add character to array
							length++;
						}						
					}
					break;
			}
	}
}


// Sets led states
void setleds(GPIO_PinState state)
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, state);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, state);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, state);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, state);
}


// Flashes LED with SysTick
void flashleds(bool state)
{
	if (state == true) {
		SysTick_Initialize(SystemCoreClock/2);
	} else {
		SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk; // Disable Interrupt
		SysTick->VAL = 0; // Reset Counter
	}
}

// Validates codes (2 = Admin, 1 = Correct, 0 = Incorrect)
uint8_t checkcode(char* entry, char codes[][4], uint16_t total) 
{
	// Checking for admin code
	for (int j = 0; j < 4; j++) { // compares each character of the codes
		if (ADMIN[j] == entry[j] && j != 3) { // checks first three characters
			continue; // continues to next character if correct
		} else if (ADMIN[j] == entry[j] && j == 3) {
			return 2; // Return 2 for admin code
		} else {
			break;
		}
	}
	
	
	// Checking against all available codes
	for (int i = 0; i < total; i++) { // loops for length of total OR until code is found
		for (int j = 0; j < 4; j++) { // compares each character of the codes
			if (codes[i][j] == entry[j] && j != 3) { // checks first three characters
				continue; // continues to next character if correct
			} else if (codes[i][j] == entry[j] && j == 3) {
				return 1; // Return 1 for correct
			} else {
				break;
			}
		}
	}

	// If no codes match
	return 0;
}

// Add/Removes codes (TRUE = ADD, FALSE = RMV)
uint16_t editcodes(char codes[][4], uint16_t total, bool mode)
{
	char key = ' ', ctostr[2] = {' '};
	char* line = "";
	char linearr[4], num[1] = {' '};
	char entry[4] = {' ', ' ', ' ', ' '};
	bool removing[CODESIZE];
	int totalremoved = 0, current = 1;
	
	if (mode == true) { // ADD mode
		
		
		while (key != 'B') { //Runs until user hits B
			
			if (total >= CODESIZE) {
				Write_Instr_LCD(0x01); // Clear Screen
				line = "MAX CODES";
				Write_String_LCD(line); // Write line
				Write_Instr_LCD(0xC0); // Go to bottom line
				line = "REACHED";
				Write_String_LCD(line); // Write line
				Delay(2000);
				Write_Instr_LCD(0x01); // Clear Screen
				return total;
			}
			
			
			
			Write_Instr_LCD(0x01); // Clear Screen
			line = "Enter Code:";
			Write_String_LCD(line); // Write line
			Write_Instr_LCD(0xC0); // Go to bottom line
			
			// Wait for code entry
			codeentry(entry, false);
			
			// Save code, increment total
			for (int i = 0; i < 4; i++) {
				codes[total][i] = entry[i];
			}
			total++;
			
			//Display total codes
			Write_Instr_LCD(0x01); // Clear Screen
			line = "Total Codes:";
			Write_String_LCD(line); // Write line
			Write_Instr_LCD(0xC0); // Go to bottom line
			snprintf(linearr, 4, "%d", total); // Convert total to string
			Write_String_LCD(linearr); // Print total
			line = "/";
			Write_String_LCD(line);
			snprintf(linearr, 4, "%d", CODESIZE); // Convert CODESIZE to string
			Write_String_LCD(linearr); // Print CODESIZE
			Delay(1000);
			
			// Ask if the user would like to add more codes
			Write_Instr_LCD(0x01); // Clear Screen
			line = "Add another?";
			Write_String_LCD(line); // Write line
			Write_Instr_LCD(0xC0); // Go to bottom line
			line = "A=Yes       B=No";
			Write_String_LCD(line);
			
			// Get input + Validate
			do {
				key = detectkey();
			} while (key != 'A' && key != 'B');
		}
	} else { // RMV mode
		
		// Clearing Removing Array
		for (int i = 0; i < CODESIZE; i++) {
			removing[i] = false;
		}
		
		Write_Instr_LCD(0x01); // Clear Screen
		line = "SELECT CODES TO";
		Write_String_LCD(line); // Write line
		Write_Instr_LCD(0xC0); // Go to bottom line
		line = "REMOVE";
		Write_String_LCD(line); // Write line
		Delay(1500);
		Write_Instr_LCD(0x01); // Clear Screen
		line = "WHEN DONE,";
		Write_String_LCD(line); // Write line
		Write_Instr_LCD(0xC0); // Go to bottom line
		line = "PRESS C";
		Write_String_LCD(line); // Write line
		Delay(1500);
		
		while (key != 'C') { // Handles menu navigation + user input
			Write_Instr_LCD(0x01); // Clear Screen
			
			// Convert current to string
			if (current > 9) {
				snprintf(linearr, 4, "0%d", current);
			}	else if (current < 10) {
				snprintf(linearr, 4, "00%d", current);
			} else if (current > 99) {
				snprintf(linearr, 4, "%d", current); 
			}
			
			// Place current code and index into line
			if (removing[current-1] == false) { // Not marked for removal
				// Place current code and index into line
				line = "#";
			} else { // Marked for removal
					line = "X";
			}
			Write_String_LCD(line);
			Write_String_LCD(linearr);
			Write_String_LCD(" =");
			for (int i = 0; i < 4; i++) {
				ctostr[0] = codes[current-1][i];
				ctostr[1] = '\0';
				Write_String_LCD(" ");
				Write_String_LCD(ctostr);
			}
			
			// Write options
			Write_Instr_LCD(0xC0); // Go to bottom line
			line = "<=* A=Y  B=N #=>";
			Write_String_LCD(line); // Write line
			
			// Input validation
			do {
				key = detectkey();
			} while (key != '*' && key != 'A' && key != 'B' && key != '#' && key != 'C');
			
			switch (key) {
				case '*':
					if (current == 1) {
						current = total;
					} else {
						current--;
					}
					break;
				case '#':
					if (current == total) {
						current = 1;
					} else {
						current++;
					}
					break;
				case 'A':
					if (removing[current-1] == false) {
						removing[current-1] = true;
						totalremoved++;
					}
					break;
				case 'B':
					if (removing[current-1] == true) {
						removing[current-1] = false;
						totalremoved--;
					}
					break;
				case 'C':
						// Do nothing, while loop will end
					break;
			}
		}
		
		// Remove message
		Write_Instr_LCD(0x01); // Clear Screen
		line = "REMOVING:";
		Write_String_LCD(line); // Write line
		Write_Instr_LCD(0xC0); // Go to bottom line
		snprintf(linearr, 4, "%d", totalremoved);
		Write_String_LCD(linearr);
		Write_String_LCD(" CODES");
		Delay(1500);
		
		// Removing marked codes
		for (int i = 0; i < totalremoved; i++) { // Runs once for each code marked
			
			// Search for index of a marked code
			for (int j = 0; j < total; j++) { 
				if (removing[j] == true) {
					current = j;
					break;
				}
			}
			
			// Shuffle codes to erase marked code
			if (current != (total-1)) { // Makes sure marked code is not last code in array
				for (int j = current; j < total-1; j++) { // Shuffling
						for (int k = 0; k < 4; k++) {
							codes[j][k] = codes[j+1][k];
						}
				}
				total--; // Adjust total
			} else { // Marked is last
				total--; // Adjust total
			}
			
		}
		Write_Instr_LCD(0x01); // Clear Screen
		line = "REMOVED";
		Write_String_LCD(line); // Write line
		Write_Instr_LCD(0xC0); // Go to bottom line
		line = "";
		snprintf(linearr, 4, "%d", totalremoved);
		Write_String_LCD(linearr);
		Write_String_LCD(" CODES");
		Delay(1500);
		
		
		// Checking if 0 codes left
		if (total == 0) {
			
			// Inform user
			Write_Instr_LCD(0x01); // Clear Screen
			line = "0 CODES REMAIN";
			Write_String_LCD(line); // Write line
			Delay(1500);
			Write_Instr_LCD(0x01); // Clear Screen
			
			// Ask for new code
			line = "Enter New Code:";
			Write_String_LCD(line);
			Write_Instr_LCD(0xC0); // Go to bottom line

			// Wait for new code
			codeentry(entry, false);

			//save initial code
			for (int i = 0; i < 4; i++) {
				codes[0][i] = entry[i];
			}
			total++;
		}
		
	}
	
	Write_Instr_LCD(0x01); // Clear Screen
	
	// Update main total via return
	return total;
}

// Add/Removes codes
void clearcodes(char codes[][4])
{
	char* line;
	char entry[4], key = ' ';
	
	// Confirm Clear
	Write_Instr_LCD(0x01); // Clear Screen
	line = "Clear codes?";
	Write_String_LCD(line);
			
	// Write options
	Write_Instr_LCD(0xC0); // Go to bottom line
	line = "A=Yes B=No";
	Write_String_LCD(line); // Write line
	
	// Input validation
	do {
		key = detectkey();
	} while (key != 'A' && key != 'B');
	
	if (key == 'B') { // If B, cancel operation
		return;
	}
	
	// Inform user
	Write_Instr_LCD(0x01); // Clear Screen
	line = "Clearing codes.";
	Write_String_LCD(line); // Write line
	Delay(1500);
	
	// Obtain new code
	Write_Instr_LCD(0x01); // Clear Screen
	line = "0 CODES REMAIN";
	Write_String_LCD(line); // Write line
	Delay(1500);
	Write_Instr_LCD(0x01); // Clear Screen
	
	// Ask for new code
	line = "Enter New Code:";
	Write_String_LCD(line);
	Write_Instr_LCD(0xC0); // Go to bottom line

	// Wait for new code
	codeentry(entry, false);

	//save initial code
	for (int i = 0; i < 4; i++) {
		codes[0][i] = entry[i];
	}
	
}

// Display available codes
void displaycodes(char codes[][4], uint16_t total)
{
	int current = 1;
	char key = ' ';
	char linearr[4], ctostr[2];
	char* line;
	
	while (key != 'B') { // Handles menu navigation + user input
		Write_Instr_LCD(0x01); // Clear Screen
		
		// Convert current to string
		if (current > 9) {
			snprintf(linearr, 4, "0%d", current);
		} else if (current > 99) {
			snprintf(linearr, 4, "%d", current); 
		} else {
			snprintf(linearr, 4, "00%d", current);
		}
		
		// Write current code and index
		line = "#";
		Write_String_LCD(line);
		Write_String_LCD(linearr);
		Write_String_LCD(" =");
		for (int i = 0; i < 4; i++) {
			ctostr[0] = codes[current-1][i];
			ctostr[1] = '\0';
			Write_String_LCD(" ");
			Write_String_LCD(ctostr);
		}

		
		// Write options
		Write_Instr_LCD(0xC0); // Go to bottom line
		line = "<=*  B=BACK  #=>";
		Write_String_LCD(line); // Write line
		
		// Input validation
		do {
			key = detectkey();
		} while (key != '*' && key != 'B' && key != '#');
		
		switch (key) {
			case '*':
				if (current == 1) {
					current = total;
				} else {
					current--;
				}
				break;
			case '#':
				if (current == total) {
					current = 1;
				} else {
					current++;
				}
				break;
			case 'B':
				// Do nothing
				break;
		}
	}
	return;
}

// Buzzes speaker for given time in ms
void buzz(int time)
{
	while (time > 0) {
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET); // Enable buzzer
		Delay(2);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_RESET); // Disable buzzer
		Delay(2);
		time-=4; // Decrement time
	}
}

// Creates a delay in ms
void Delay(int delay)
{
	delay = delay * (SystemCoreClock/5000);
	for (int n = 0; n < delay; n++)
	{
		__asm("nop");
	}
}

// Handles Admin State
uint16_t adminmenu(char codes[][4], uint16_t total)
{
	char* line = "";
	uint8_t state = 1;
	unsigned char key = ' ';
	
	Write_Instr_LCD(0x01); // Clear Screen
	line = "ADMIN";
	Write_String_LCD(line); // Write admin
	setleds(GPIO_PIN_RESET); // Turn off LEDS
	Delay(1500);
	
	while (key != 'B') {
		Write_Instr_LCD(0x01); // Clear Screen
		switch (state) { // Handle displaying options
			case 1:
				line = "1 - View Codes";
				Write_String_LCD(line);
				break;
			case 2: 
				line = "2 - Add Codes";
				Write_String_LCD(line);
				break;
			case 3:
				line = "3 - Remove Codes";
				Write_String_LCD(line);
				break;
			case 4:	
				line = "4 - Clear Codes";
				Write_String_LCD(line);
				break;
		}
		// Write hotkeys
		Write_Instr_LCD(0xC0); // Go to bottom line
		line = "A=SEL B=BACK C=>";
		Write_String_LCD(line);
		
		do { // Take input + Validate
			key = detectkey();
		} while (key != 'A' && key != 'B' && key != 'C');
		
		switch (key) { // Handle input
			case 'A': // Select
				switch (state) { // Handles selection based on state
					case 1: // Display Codes
						displaycodes(codes, total);
						break;
					case 2: // Add Codes
						total = editcodes(codes, total, true); 
						break;
					case 3: // Remove Codes
						total = editcodes(codes, total, false);
						break;
					case 4:	// Clear Codes
						clearcodes(codes);
						total = 1;
						break;
				}
				break;
			case 'B': // Back
				return total; // Return to main
			case 'C': // Next
				if (state < 4) {
					state++;
				} else {
					state = 1;
				}
				break;
		}	
	}
	
	// Fallback return statement
	return total;
}

// Initializes and sets systick timing
void SysTick_Initialize(uint32_t ticks)
{
  SysTick->LOAD = ticks;
  SysTick->VAL = 0;

  // Select processor clock to internal: 1 = processor clock; 0 = external clock
  SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk;

  // Enable counting of SysTick
  SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;

  // Enables SysTick interrupt
  SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
}



// Handles Flashing LEDS with SysTick
void SysTick_Handler(void)
{
	HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_0 | GPIO_PIN_1);
	HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_7 | GPIO_PIN_8);
}


// System Clock Configuration
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
