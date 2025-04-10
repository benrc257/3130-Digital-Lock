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

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stdio.h"
#include "stdbool.h"

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
int fputc(int, FILE*);

// Keypad Functions
unsigned char detectkey(void);
bool iskeypressed(void);
void codeentry(char* entry);

// LCD Functions
void LCD_nibble_write(uint8_t temp, uint8_t s); //configures message for data (1) or instructions (0) with s
void Write_SR_LCD(uint8_t temp); //writes data to lcd shift register
void Write_Instr_LCD(uint8_t code); //writes instructions to LCD
void Write_Char_LCD(uint8_t code); //writes character to LCD
void Write_String_LCD(char *temp); //writes string to LCD

// Passcode Functions
bool checkcode(char* entry, char* codes, uint16_t total); // Validates codes
char* editcodes(char* entry, char* codes, uint16_t total, uint8_t mode); // Add/Removes codes
void displaycodes(char* codes, uint16_t total); // Display available codes

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
  MX_USART2_UART_Init();

	// Variables
	int* passcodes = NULL; //we should dynamically allocate each passcode using a list, so we can have unlimited passcodes
	char admincode[4] = {'2' , '5', '8', '0'}; //used for admin functions of lock
	char codes[100][4] = {}; // Stores all codes for comparison
	char entry[4] = {' ' , ' ', ' ', ' '}; // Stores current code
	char keypressed;
	int totalcodes = 0;
	char* line;
	bool locked = false;
		
/* LCD controller reset sequence*/ 
	HAL_Delay(20);
	LCD_nibble_write(0x30,0);
	HAL_Delay(5);
	LCD_nibble_write(0x30,0);
	HAL_Delay(1);
	LCD_nibble_write(0x30,0);
	HAL_Delay(1);
	LCD_nibble_write(0x20,0);
	HAL_Delay(1);
	Write_Instr_LCD(0x28); /* set 4 bit data LCD - two line display - 5x8 font*/
	Write_Instr_LCD(0x0E); /* turn on display, turn on cursor , turn off blinking*/
	Write_Instr_LCD(0x01); /* clear display screen and return to home position*/
	Write_Instr_LCD(0x06); /* set write direction */
	
	// Write Digital Lock to screen
	line = "Digital Lock";
	Write_String_LCD(line);
	HAL_Delay(2500);
	
	// Ask for initial code
	line = "Please select a passcode.";
	Write_String_LCD(line);
	Write_Instr_LCD(0xC0); // Go to bottom line
	
	// Wait for inital code
	codeentry(entry);
	
	while (1) {
		
		
		
		
		
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
	
	/*Configure GPIO pin : PB1 - PB4 */
  GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	/*Configure GPIO pin : PB8 - PB11 */
  GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	/*PA5 and PA10 are outputs*/
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
	
	/*PB5 is output*/
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
	
}

unsigned char detectkey(void) {
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
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4, GPIO_PIN_SET);
	while (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11) == GPIO_PIN_SET);	
	
	// Return character
	return keymap[row][col];
		
}

void Write_SR_LCD(uint8_t temp) {
	int i;
	uint8_t mask=0b10000000;
	for(i=0; i<8; i++) {
		if((temp&mask)==0)
		GPIOB->ODR&=~(1<<5);
		else
		GPIOB->ODR|=(1<<5);
		
		/* Sclck */
		GPIOA->ODR&=~(1<<5);
		GPIOA->ODR|=(1<<5);
		HAL_Delay(1);
		mask=mask>>1;
	}
	/*Latch*/
	GPIOA->ODR|=(1<<10);
	GPIOA->ODR&=~(1<<10);

}

void LCD_nibble_write(uint8_t temp, uint8_t s){
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

void Write_Instr_LCD(uint8_t code)
{
	LCD_nibble_write(code&0xF0,0);
	code=code<<4;
	LCD_nibble_write(code,0);
}

void Write_Char_LCD(uint8_t code)
{
	LCD_nibble_write(code&0xF0,1);
	code=code<<4;
	LCD_nibble_write(code,1);
}

void Write_String_LCD(char *temp) {
	int i=0;
	while(temp[i]!=0)
	{
		Write_Char_LCD(temp[i]);
		i=i+1;
	}
}

bool iskeypressed(void) {
	// Setting all columns high
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4, GPIO_PIN_SET);
	
	// Detecting if a key is pressed
	if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11) == GPIO_PIN_RESET) {
		return false;
	} else {
		return true;
	}
}


void codeentry(char* entry) {
	char keypressed;
	uint8_t length = 0;
	
	switch (keypressed) {
			case '*': // Do nothing (unused)
				break;
			case '#': // Do nothing (unused)
				break;
			case 'A': // Enter
				if (length == 4) {
					return;
				}
				break;
			case 'B': // Backspace
				if (length > 0) {
					Write_Instr_LCD(0x10); // Move cursor left
					Write_Char_LCD(' '); // Print space
					Write_Instr_LCD(0x10); // Move cursor left
					length--;
					entry[length] = ' ';
				}
				break;
			case 'C': // Clear
				while (length > 0) { // Loop for all characters
					Write_Instr_LCD(0x10); // Move cursor left
					Write_Char_LCD(' '); // Print space
					Write_Instr_LCD(0x10); // Move cursor left
					length--;
					entry[length] = ' '; // remove character from array
				}
				break;
			case 'D': // Do nothing (unused)
				break;
			default:
				if (length < 4) {
					Write_Char_LCD(keypressed); // write pressed character
					entry[length] = keypressed; // add character to array
					length++;
				}
				break;
		}
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



// USART2 Initialization Function
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

// This function reroutes printf calls to USART2 for external debugging using TeraTerm
int fputc(int ch, FILE *f)
{
	HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xFFFF);
	return ch;
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
