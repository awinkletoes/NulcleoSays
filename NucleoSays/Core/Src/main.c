/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  * @authors        : Hayley Jamiola and Annaliese Winklosky
  * @program        : Nucleo Says
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "lcd.h"
#include "math.h"

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart2;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SPI1_Init(void);
static void MX_I2C1_Init(void);

// Wii NunChuck related variables and functions;
#define BUFSIZE 64
#define NUNCHUK_ADDRESS_SLAVE1 0xA4
#define NUNCHUK_ADDRESS_SLAVE2 0xA5
#define DEFAULT_FONT FONT_6X8
// function declarations;
HAL_StatusTypeDef Write_To_NunChuck(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t len);
HAL_StatusTypeDef Read_From_NunChuck(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t len);
void NunChuck_print_data_init(void);
void NunChuck_phase1_init(void);
void NunChuck_phase2_read(void);
void NunChuck_translate_data(void);
void NunChuck_print_data(void);
// two fixed length buffers I use to put data to sent or where to put data
// received over the I2C bus;
uint8_t I2CMasterBuffer[BUFSIZE];
uint8_t I2CSlaveBuffer[BUFSIZE];
char text_buffer[8]; // used for printing only to LCD display;
// variable that store updated information read from NunChuck;
uint16_t joy_x_axis = 0;
uint16_t joy_y_axis = 0;
uint16_t accel_x_axis = 0;
uint16_t accel_y_axis = 0;
uint16_t accel_z_axis = 0;
uint16_t z_button = 0;
uint16_t c_button = 0;
int score = 0;
int correct = 0;

// Debugging Messages
#define Z_IT "Z Press\r\n"
#define C_IT "C Press\r\n"
#define PUSH_IT "Blue Pushbutton Press\r\n"
#define TWIST_IT "Joystick movement\r\n"
#define SHAKE_IT "Nunchuck shaking\r\n"
#define EXPECTED_ACTION "Expected Action: "
#define ACTUAL_ACTION "Actual Action: "
#define CORRECT "Correct\r\n"
#define INCORRECT "Incorrect\r\n"

///////////////////////////////////////////////////////////////////////////////
//
// main
//
///////////////////////////////////////////////////////////////////////////////

int main(void)
{
  uint32_t i;
  uint32_t x_prev=120, y_prev=160;
  uint32_t x_new=120, y_new=160;
  uint32_t dx=4, dy=4, delta=5, radius=16;

  // MCU Configuration--------------------------------------------------------
  // Reset of all peripherals, Initializes the Flash interface and the Systick.
  HAL_Init();
  // Configure the system clock
  SystemClock_Config();
  // Initialize all configured peripherals
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_SPI1_Init();
  MX_I2C1_Init();

  // LCD display initialization; black screen;
  LCD_init();

  // one time print of message and variable names;
  // note this text will be having pixels erased as the yellow
  // circle will be moved and overlapping with the text;
  NunChuck_print_data_init();
  // NunChuck phase 1;
  NunChuck_phase1_init();


  while (1)
  {
    correct = 0;
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

    // clear receive buffer;
    for ( i = 0; i < BUFSIZE; i++ ) {
      I2CSlaveBuffer[i] = 0x00;
    }

    int random_action = rand() % 5;
    // 0: C-it!       The program expects c to be pushed.
    // 1: Z-it!       The program expects z to be pushed.
    // 2: Push-it!    The program expects the blue push button to be pushed.
    // 3: Twist-it!   The program expects joystick movement from the Wii Nunchuck.
    // 4: Shake-it!   The program expected joystick acceleration from the Wii Nunchuck.
    HAL_UART_Transmit(&huart2, (uint8_t*)EXPECTED_ACTION, strlen(EXPECTED_ACTION), HAL_MAX_DELAY);

    if (random_action == 0) {
      LCD_PutStr(32,32,  (char *)"C-it!", DEFAULT_FONT, C_WHITE, C_BLACK);
      HAL_UART_Transmit(&huart2, (uint8_t*)C_IT, strlen(C_IT), HAL_MAX_DELAY);
    } else if (random_action == 1){
      LCD_PutStr(32,32,  (char *)"Z-it!", DEFAULT_FONT, C_WHITE, C_BLACK);
      HAL_UART_Transmit(&huart2, (uint8_t*)Z_IT, strlen(Z_IT), HAL_MAX_DELAY);
    } else if (random_action == 2) {
      LCD_PutStr(32,32,  (char *)"Push-it!", DEFAULT_FONT, C_WHITE, C_BLACK);
      HAL_UART_Transmit(&huart2, (uint8_t*)PUSH_IT, strlen(PUSH_IT), HAL_MAX_DELAY);
    } else if (random_action == 3) {
      LCD_PutStr(32,32,  (char *)"Twist-it!", DEFAULT_FONT, C_WHITE, C_BLACK);
      HAL_UART_Transmit(&huart2, (uint8_t*)TWIST_IT, strlen(TWIST_IT), HAL_MAX_DELAY);
    } else {
      LCD_PutStr(32,32,  (char *)"Shake-it!", DEFAULT_FONT, C_WHITE, C_BLACK);
      HAL_UART_Transmit(&huart2, (uint8_t*)SHAKE_IT, strlen(SHAKE_IT), HAL_MAX_DELAY);
    }

    HAL_Delay(2000);
    if (random_action == 0) {
      LCD_PutStr(32,32,  (char *)"C-it!", DEFAULT_FONT, C_BLACK, C_BLACK);
    } else if (random_action == 1){
      LCD_PutStr(32,32,  (char *)"Z-it!", DEFAULT_FONT, C_BLACK, C_BLACK);
    } else if (random_action == 2) {
      LCD_PutStr(32,32,  (char *)"Push-it!", DEFAULT_FONT, C_BLACK, C_BLACK);
    } else if (random_action == 3) {
      LCD_PutStr(32,32,  (char *)"Twist-it!", DEFAULT_FONT, C_BLACK, C_BLACK);
    } else {
      LCD_PutStr(32,32,  (char *)"Shake-it!", DEFAULT_FONT, C_BLACK, C_BLACK);
    }

    // NunChuck phase 2
    NunChuck_phase2_read();
    NunChuck_translate_data();
    NunChuck_print_data();

    HAL_Delay(1000);
    HAL_UART_Transmit(&huart2, (uint8_t*)ACTUAL_ACTION, strlen(ACTUAL_ACTION), HAL_MAX_DELAY);

    if (c_button == 0 && random_action == 0) {
      score++;
      HAL_UART_Transmit(&huart2, (uint8_t*)CORRECT, strlen(CORRECT), HAL_MAX_DELAY);
      correct = 1;
    }

    if (z_button == 0 && random_action == 1) {
      score++;
      HAL_UART_Transmit(&huart2, (uint8_t*)CORRECT, strlen(CORRECT), HAL_MAX_DELAY);
      correct = 1;
    }

    if (!HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) && random_action == 2) {
      score++;
      HAL_UART_Transmit(&huart2, (uint8_t*)CORRECT, strlen(CORRECT), HAL_MAX_DELAY);
      correct = 1;
    }

    if (joy_x_axis != 128 && joy_y_axis != 128 && random_action == 3) {
      score++;
      HAL_UART_Transmit(&huart2, (uint8_t*)CORRECT, strlen(CORRECT), HAL_MAX_DELAY);
      correct = 1;
    }

    if (accel_x_axis != 400 && accel_y_axis != 550 && accel_z_axis != 700 && random_action == 4) {
      score++;
      HAL_UART_Transmit(&huart2, (uint8_t*)CORRECT, strlen(CORRECT), HAL_MAX_DELAY);
      correct = 1;
    }

    if (correct == 0) {
      HAL_UART_Transmit(&huart2, (uint8_t*)INCORRECT, strlen(INCORRECT), HAL_MAX_DELAY);
    }

    LCD_PutStr(180, 180,  (char *)"Score: ", DEFAULT_FONT, C_WHITE, C_BLACK);
    sprintf(text_buffer, "%01d", score);
    LCD_PutStr(200, 200, (char *)text_buffer, DEFAULT_FONT, C_YELLOW, C_BLACK);

    HAL_Delay(10);
  }

}


///////////////////////////////////////////////////////////////////////////////
//
// NunChuck functions
//
///////////////////////////////////////////////////////////////////////////////

// Communication with the NunChuk consists of two phases:
// -->phase 1: initialization phase (executed once) in which specific data
// are written to the NunChuk;
// Essentially initialization consists of two write transactions,
// each of which writes a single byte to a register internal to
// the I2C slave ( reg[0xf0] = 0x55, reg[0xfb] = 0x00 ).
// -->phase 2: repeated read phase in which six data bytes are read
// again and again; each read phase consists of two transactions –
// a write transaction which sets the read address to zero, and a
// read transaction.
// NOTE: NunChuck uses 'fast mode' for I2C communication!

HAL_StatusTypeDef Write_To_NunChuck(I2C_HandleTypeDef *hi2c,
    uint16_t DevAddress, uint8_t *pData, uint16_t len)
{
  HAL_StatusTypeDef returnValue;

  // transfer transmit buffer over the I2C bus;
  returnValue = HAL_I2C_Master_Transmit(hi2c, DevAddress, pData, len, HAL_MAX_DELAY);
  if (returnValue != HAL_OK)
    return returnValue;

  return HAL_OK;
}

HAL_StatusTypeDef Read_From_NunChuck(I2C_HandleTypeDef *hi2c,
    uint16_t DevAddress, uint8_t *pData, uint16_t len)
{
  HAL_StatusTypeDef returnValue;

  // retrieve data;
  returnValue = HAL_I2C_Master_Receive(hi2c, DevAddress, pData, len, HAL_MAX_DELAY);

  return returnValue;
}

void NunChuck_print_data_init(void)
{
  // Note: this function should be called once only;
//  LCD_PutStr(32,32,  (char *)"This is I2C example", DEFAULT_FONT, C_WHITE, C_BLACK);
//  LCD_PutStr(32,48,  (char *)"Data from NunChuck:", DEFAULT_FONT, C_WHITE, C_BLACK);
  LCD_PutStr(32,64,  (char *)"joyX =", DEFAULT_FONT, C_WHITE, C_BLACK);
  LCD_PutStr(32,80,  (char *)"joyY =", DEFAULT_FONT, C_WHITE, C_BLACK);
  LCD_PutStr(32,96,  (char *)"accX =", DEFAULT_FONT, C_WHITE, C_BLACK);
  LCD_PutStr(32,112, (char *)"accY =", DEFAULT_FONT, C_WHITE, C_BLACK);
  LCD_PutStr(32,128, (char *)"accZ =", DEFAULT_FONT, C_WHITE, C_BLACK);
  LCD_PutStr(32,144, (char *)"Z    =", DEFAULT_FONT, C_WHITE, C_BLACK);
  LCD_PutStr(32,160, (char *)"C    =", DEFAULT_FONT, C_WHITE, C_BLACK);
}

void NunChuck_phase1_init(void)
{
  // this function should be called once only;

  I2CMasterBuffer[0] = 0xF0; // at address 0xF0 of NunChuck write:
  I2CMasterBuffer[1] = 0x55; // data 0x55
  Write_To_NunChuck(&hi2c1, NUNCHUK_ADDRESS_SLAVE1, I2CMasterBuffer, 2);
  HAL_Delay(10);

  I2CMasterBuffer[0] = 0xFB; // at address 0xFB of NunChuck write:
  I2CMasterBuffer[1] = 0x00; // data 0x00
  Write_To_NunChuck(&hi2c1, NUNCHUK_ADDRESS_SLAVE1, I2CMasterBuffer, 2);
  HAL_Delay(10);
}

void NunChuck_phase2_read(void)
{
  // this is called repeatedly to realize continued polling of NunChuck

  I2CMasterBuffer[0] = 0x00; // value;
  Write_To_NunChuck(&hi2c1, NUNCHUK_ADDRESS_SLAVE1, I2CMasterBuffer, 1);
  HAL_Delay(10);

  Read_From_NunChuck(&hi2c1, NUNCHUK_ADDRESS_SLAVE2, I2CSlaveBuffer, 6);
  HAL_Delay(10);
}

void NunChuck_translate_data(void)
{
  int byte5 = I2CSlaveBuffer[5];
  joy_x_axis = I2CSlaveBuffer[0];
  joy_y_axis = I2CSlaveBuffer[1];
  accel_x_axis = (I2CSlaveBuffer[2] << 2);
  accel_y_axis = (I2CSlaveBuffer[3] << 2);
  accel_z_axis = (I2CSlaveBuffer[4] << 2);
  z_button = 0;
  c_button = 0;

  // byte I2CSlaveBuffer[5] contains bits for z and c buttons
  // it also contains the least significant bits for the accelerometer data
  if ((byte5 >> 0) & 1)
    z_button = 1;
  if ((byte5 >> 1) & 1)
    c_button = 1;
  accel_x_axis += (byte5 >> 2) & 0x03;
  accel_y_axis += (byte5 >> 4) & 0x03;
  accel_z_axis += (byte5 >> 6) & 0x03;
}

void NunChuck_print_data(void)
{
  // this is called as many times as reads from the NunChuck;
  sprintf(text_buffer, "%03d", joy_x_axis);
  LCD_PutStr(88, 64, (char *)text_buffer, DEFAULT_FONT, C_YELLOW, C_BLACK);
  sprintf(text_buffer, "%03d", joy_y_axis);
  LCD_PutStr(88, 80, (char *)text_buffer, DEFAULT_FONT, C_YELLOW, C_BLACK);
  sprintf(text_buffer, "%04d", accel_x_axis);
  LCD_PutStr(88, 96, (char *)text_buffer, DEFAULT_FONT, C_YELLOW, C_BLACK);
  sprintf(text_buffer, "%04d", accel_y_axis);
  LCD_PutStr(88, 112, (char *)text_buffer, DEFAULT_FONT, C_YELLOW, C_BLACK);
  sprintf(text_buffer, "%04d", accel_z_axis);
  LCD_PutStr(88, 128, (char *)text_buffer, DEFAULT_FONT, C_YELLOW, C_BLACK);
  sprintf(text_buffer, "%01d", z_button);
  LCD_PutStr(88, 144, (char *)text_buffer, DEFAULT_FONT, C_YELLOW, C_BLACK);
  sprintf(text_buffer, "%01d", c_button);
  LCD_PutStr(88, 160, (char *)text_buffer, DEFAULT_FONT, C_YELLOW, C_BLACK);
}


///////////////////////////////////////////////////////////////////////////////
//
// configuration functions
//
///////////////////////////////////////////////////////////////////////////////

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  // Configure the main internal regulator output voltage
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  // Initializes the RCC Oscillators according to the specified parameters
  // in the RCC_OscInitTypeDef structure.
  // NOTE: use the high speed internal clock source, and not the default
  // MultiSpeed Internal (MSI) clock, which is slower;
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  // Initializes the CPU, AHB and APB buses clocks
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
void MX_I2C1_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;

  // Peripheral clock enable
  __HAL_RCC_I2C1_CLK_ENABLE();

  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00707CBB;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  HAL_I2C_Init(&hi2c1);

  // Configure Analog filter
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }
  // Configure Digital filter
  //if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  //{
  //  Error_Handler();
  //}

  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
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

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA5 */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin Output Level - DC*/
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level - RST */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level - CS */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);

  /*Configure GPIO pin : ST7789_DC_Pin */
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : ST7789_RST_Pin */
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : ST7789_CS_Pin */
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT; // GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

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
