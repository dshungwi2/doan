/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ssd1306.h"
#include "fonts.h"
#include <stdio.h>
#include "fingerprint.h"
#include <stdbool.h>
#include "KeyPad.h"
#define BUTTON_A  1
#define BUTTON_B  2
#define BUTTON_C  3
#define BUTTON_D  4

extern F_Packet rpacket;
uint32_t password;
char buf_tx[100];
char data_rx;
char buf_rx[64];
int rx_index = 0;
// trang thai quet van tay
typedef enum { 
   STATE_IDLE = 0,
   STATE_DISPLAY_RESULT
} FingerState;
FingerState finger_state = STATE_IDLE;
char msg[100];
char buf[32];
uint8_t i = 0;
uint8_t enroll;
bool flag = false;
volatile uint8_t uart_busy = 0;
uint32_t time_start = 0;
uint8_t time_ready = 0;
uint8_t temp;
uint8_t result = 0xFF;
char key ;
uint32_t	Timeout_ms =0;
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c2;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
		UNUSED(huart);
    if (huart->Instance == USART1) {
						//reset_database();
					if (data_rx == 10){ // \r \n
						buf_rx[rx_index] = '\0';  // end chu?i
            rx_index = 0;	
						//cmd
						time_ready =1;
            // ?? X�A buffer sau khi hi?n th?
						//memset(buf_rx, 0, sizeof(buf_rx));
								
					}
					else if (data_rx != 10){
						buf_rx[rx_index++]= data_rx;

					}					
			
   
		HAL_UART_Receive_IT(&huart1, (uint8_t*)&data_rx, 1);

		
		
				}	
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	 HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
}

//------------------------------------------------------------
uint8_t deleteFingerprint(uint8_t id) { // ham xoa van tay
  uint8_t p = -1;

  p = delete_model(id);
	SSD1306_Clear();
  if (p == FINGERPRINT_OK) {
		sprintf(buf, "Da xoa ID:%d ", id);
    display_message(buf, 25,25, &Font_7x10);
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    display_message("Loi giao tiep", 0, 25, &Font_7x10);
  } else if (p == FINGERPRINT_BADLOCATION) {
    display_message("Xoa that bai",0 ,25, &Font_7x10);
  } else if (p == FINGERPRINT_FLASHERR) {
    display_message("Loi ghi flash", 0,25, &Font_7x10);
  } else {
    display_message("Loi ko xac dinh", 0, 25, &Font_7x10);
  }

  return p; // rpacket.data[0]
}
//--------------------------------------------------------
uint8_t downloadFingerprintTemplate(uint16_t id) // ham load van tay
{
  uint8_t p = load_model(id);
	SSD1306_Clear();
  switch (p) {
    case FINGERPRINT_OK:
			sprintf(buf, "ID:%d ", id);
			SSD1306_GotoXY(0, 0);
        SSD1306_Puts(buf, &Font_7x10, 1);
        SSD1306_UpdateScreen();
			
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      display_message("Loi giao tiep",0 ,25, &Font_7x10);
      return p;
    default:
      display_message("Loi ko xac dinh",0 ,25, &Font_7x10);
      return p;
  }
	return p; // rpacket.data[0]
}
//--------------------------------------------------------
void run_check_fingerprint(){
				switch (finger_state)
			{
				 case STATE_IDLE:			
					display_message(buf_rx, 20, 20, &Font_16x26);
					result = check_fingerprint();
					time_start = HAL_GetTick();
					// Neu c� v�n tay dat l�n th� chay kiem tra
					if (rpacket.data[0] == FINGERPRINT_OK || result == 0xFF ) {
							finger_state = STATE_DISPLAY_RESULT;
					}

					break; 
					case STATE_DISPLAY_RESULT:

							if (result != 0xFF) { // match th�nh c�ng
									if(!uart_busy){
										uart_busy = 1;
										sprintf(buf_tx, "%d", result);
										HAL_UART_Transmit_IT(&huart1,(uint8_t* )buf_tx , strlen(buf_tx));
									}
									sprintf(buf, "Thanh cong ID: %d", result); // gia tri result tra ve ID tu 0 - 127 
									display_message(buf, 0, 25, &Font_7x10);

							} else {

									display_message("Khong tim thay", 0, 20, &Font_7x10);
									sprintf(buf_tx, "ban chua dang ky van tay");
									HAL_UART_Transmit_IT(&huart1,(uint8_t* )buf_tx , strlen(buf_tx));
							}
						
							// Delay
							if (HAL_GetTick() - time_start > 1500) {
									uart_busy = 0;
									clear_last_message();
									finger_state = STATE_IDLE;
								
							}
							break;
							}
}
//--------------------------------------------------------
void run_enroll_fingerprint(){
				static uint32_t last_check_time = 0;
				switch (finger_state)
			{
				 case STATE_IDLE:
					display_message("Dang ky van tay", 10, 25, &Font_7x10);
					result = check_fingerprint();
					time_start = HAL_GetTick();
					// Neu c� v�n tay dat l�n th� chay kiem tra
					if (rpacket.data[0] == FINGERPRINT_OK || result == 0xFF ) {
							finger_state = STATE_DISPLAY_RESULT;
					}
					break;
					case STATE_DISPLAY_RESULT:
							if (result != 0xFF) { // dieu kien neu trung van tay
									sprintf(buf, "Trung ID%d", result); 
									display_message(buf,0,25, &Font_7x10);
							} else {
									uint16_t id = get_template_number();
									enroll_fingerprint(id+1);  // Luu neu chua c�
							}
						
							// Delay
							if (HAL_GetTick() - time_start > 1500) {
									clear_last_message();
									finger_state = STATE_IDLE;
								
							}
							break;
							}
}
//--------------------------------------------------------
char inputBuffer[24];   
uint8_t cursor = 0;   
uint8_t HandleKeyPress(const char key)
{
		SSD1306_Fill(0);
    if (key >= '0' && key <= '9' || (key >= 'A' && key <= 'D')) {
        if (cursor < sizeof(inputBuffer) - 1)
        {
            inputBuffer[cursor++] = key;
            inputBuffer[cursor] = '\0';
						
        }
    }
    else if (key == '*') // Backspace
    {
        if (cursor > 0)
        {
            cursor--;
            inputBuffer[cursor] = '\0';
											//	SSD1306_Clear();
					
				}

        }
    
    else if (key == '#') // Enter
    {
        
        // X�a buffer sau khi x�c nh?n
				
			return (uint8_t)atoi(inputBuffer);
    }
    else if (key == 'D') // Clear All
    {
        cursor = 0;
        inputBuffer[0] = '\0';
				SSD1306_Clear();
    }
		    else if (key == 'A') 
    {
    }
    else if (key == 'B') 
    {
    }
    else if (key == 'C') 
    {
    }		
      
			if ( cursor <= 12){
				SSD1306_DrawFilledRectangle(0, 0, 128, 40, 0);  // X�a d�ng 1
				SSD1306_GotoXY(0, 0);
				SSD1306_Puts(inputBuffer, &Font_7x10, 1);
			
			}
			else if (cursor <= 24){
				SSD1306_DrawFilledRectangle(0, 20, 128, 20, 0);  // X�a d�ng 2
				SSD1306_GotoXY(0, 20);
				SSD1306_Puts(inputBuffer + 12, &Font_7x10, 1);
			}
		
		
			SSD1306_UpdateScreen();
		return 0xff;
}
	
// Menu
//--------------------------------------------------------
typedef void (*MenuFunction)(); // Ki?u con tr? h�m

typedef struct {
    char *name;           // T�n menu
    MenuFunction action;  // H�m th?c hi?n khi nh?n Enter
} MenuItem;
void  task1(){
	run_enroll_fingerprint();

}
void  task2(){
	SSD1306_Clear();
	SSD1306_GotoXY(0, 0);
	SSD1306_Puts("xoa van tay", &Font_11x18, 1);
	SSD1306_UpdateScreen();

}
void  task3(){
	uint8_t id=0xff;
	SSD1306_Clear();
	SSD1306_GotoXY(0, 0);
	SSD1306_Puts("Doi mat khau", &Font_11x18, 1);
	SSD1306_UpdateScreen();

	char x = KeyPad_WaitForKeyGetChar();
	id = HandleKeyPress(x);
	sprintf(msg, "%d", id);
	display_message(msg,0,0, &Font_7x10);
	
}
void  task4(){

	SSD1306_Clear();
	SSD1306_GotoXY(0, 0);
	SSD1306_Puts("Nhap mat khau", &Font_11x18, 1);
	SSD1306_UpdateScreen();
						

}
MenuItem  menuItems[] = {
    {"Dang ky van tay", task1},
    {"Xoa van tay", task2},
    {"Doi mat khau", task3},
    {"Xoa tat ca du lieu", task4},
};
int menuSize = sizeof(menuItems) / sizeof(menuItems[0]);
int selected_index =0;
int inMenu = 0; // 0 = m�n h�nh ch�nh, 1 = dang ? menu
uint8_t inAction = 0;
void displayMenu() {

	SSD1306_Fill(0);
    for (int i = 0; i < menuSize; i++) {
				int y = i*15;
				if (i == selected_index) {
            SSD1306_DrawRectangle(0, y, 128, 12, 1); // V? � vu�ng
            SSD1306_GotoXY(4, y + 2);
        } else {
            SSD1306_GotoXY(4, y + 2);
        }
        SSD1306_Puts(menuItems[i].name, &Font_7x10, 1);
    }
    
    SSD1306_UpdateScreen();
}		

//--------------------------------------------------------
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_I2C2_Init();
  /* USER CODE BEGIN 2 */
	SSD1306_Init();       // Kh?i t?o OLED
  init_fingerprint();   // Kh?i t?o c?m bi?n v�n tay
	KeyPad_Init();
	// In ch? "Loading" m?t l?n
	SSD1306_GotoXY(10, 15);
	SSD1306_Puts("Loading", &Font_11x18, 1);
	SSD1306_UpdateScreen();

	uint32_t start= HAL_GetTick();
	while (HAL_GetTick() - start < 2000){}


// Hi?n th? dang dang k� v�n tay
  
  HAL_UART_Receive_IT(&huart1, (uint8_t*)&data_rx, 1);
	SSD1306_Clear();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1)
	{

			key = KeyPad_WaitForKeyGetChar();

			if (inMenu == 0) {
					
							
					run_check_fingerprint();
					if (key == 'D') {
							inMenu = 1;		
							selected_index = 0;
							displayMenu();
					}
			} 
			else if (inMenu == 1 && inAction == 0) {

						if (key == 'A') { // UP
								selected_index = (selected_index - 1 + menuSize) % menuSize;
								displayMenu();
						}
						else if (key == 'B') { // DOWN
								selected_index = (selected_index + 1 + menuSize) % menuSize;
								displayMenu();
						}
						else if (key == 'C') { // ENTER
								
								inAction =1;
						}
					
					if (key == 'D') { // BACK
							inAction =0;
							inMenu = 0;
							clear_last_message();
					}				
			}

			if (inAction){
				 switch (selected_index) {
            case 0: task1(); break;
            case 1: task2(); break;
            case 2: task3(); break;
            case 3: task4(); break;
        }
 				  if (key == 'D') { // BACK
						inAction =0;
						inMenu = 0;
						clear_last_message();
					}
			}


	}



		
	
		
	
						

				
			
        
    
	
   
		
		
		

    
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
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
}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.ClockSpeed = 400000;
  hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

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
  huart2.Init.BaudRate = 57600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
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
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA11 PA12 PA15 */
  GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB3 */
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB4 PB5 PB6 PB7 */
  GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
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
