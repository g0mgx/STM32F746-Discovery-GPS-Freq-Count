/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma2d.h"
#include "ltdc.h"
#include "rtc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "fmc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32746g_discovery_lcd.h"
#include "stm32746g_discovery_ts.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>

#include "GPS.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define TRUE 	1
#define FALSE 	0
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
char UsrString[30];
int GPSData = FALSE;
int RTCSet = FALSE;
extern GPS_t GPS;
const uint8_t TimeLine = 3;
const uint8_t TitleLine = 1;
const uint8_t GateCountLine = 10;
const uint8_t ExternalCountLine = 5;
const uint8_t ButtonLine = 7;
TS_StateTypeDef  ts;
RTC_TimeTypeDef sTime = {0};
RTC_DateTypeDef sDate = {0};
RTC_TimeTypeDef sTimePrevious = {0};
char xTouchStr[10];
uint32_t TouchTime = 0;
uint32_t PreviousTouchTime = 0;
const uint32_t TouchUpdateFrequency = 50;
volatile uint32_t GateCount = 0;
uint32_t PreviousGateCount = 0;
volatile uint32_t ExternalCountTimer2 = 0;
volatile uint32_t ExternalCountTimer4 = 0;
volatile float ExternalMHZ = 0.0;
volatile uint32_t LastExternalCountTimer2 = 0;
volatile uint32_t LastExternalCountTimer4 = 0;
uint32_t Timer2Count = 0;
uint32_t Timer4Count = 0;
uint64_t TotalCount = 0;
uint64_t AverageCount = 0;


uint64_t SampleArray[32] = {0};
uint32_t SampleArrayIndex = 0;
uint32_t TimeSinceGPSUpdate = 0;

uint8_t GateTimeValues[] = {2, 3, 5, 10};
uint8_t GateTimeValuesIndex = 0;
uint8_t GateTime = 2;

uint8_t SampleValues[] = {2, 3, 5, 10};
uint8_t SampleValuesIndex = 0;

uint32_t LastParameterChangeTime = 0;
uint32_t ParameterChangeFrequency = 300;
uint8_t NumberOfSamples = 2;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void LCDConfig(void);
void TSConfig(void);
uint32_t GetSystemTime(void);
void PrintSerial(char *format,...);
void SetRTC(void);
int DayofWeek(int y, int m, int d);
void WaitforGPS(void);
void UpdateTimeDisplay(void);
void DisplayScreenTitles(void);
void ProcessTouch(void);
uint32_t GetSystemTime(void);
void CalculateCount(void);
void DisplayButtons(void);
void DrawTriangle (uint32_t XPos, uint32_t Line);
void DrawUpseideDownTriangle (uint32_t XPos, uint32_t Line);
void DrawGateTimeAndSamples(void);
void ToggleGateBlob(void);
void ToggleSampleBlob(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	/* Enable the CPU Cache */
	/* Enable I-Cache */
	SCB_EnableICache();
	/* Enable D-Cache */
	SCB_EnableDCache();
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
  MX_FMC_Init();
  MX_LTDC_Init();
  MX_RTC_Init();
  MX_USART6_UART_Init();
  MX_DMA2D_Init();
  MX_TIM2_Init();
  MX_TIM8_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */
  LCDConfig();

  TSConfig();
  GPS_Init();
  // Start Timer to count PPS
  HAL_TIM_Base_Start_IT(&htim8);
  // Start Timer to count external signal
  // Timer 2 is the Master and is 32 bits
  HAL_TIM_Base_Start(&htim2);
  // Timer 4 counts 1 every time timer2 overflows
  HAL_TIM_Base_Start(&htim4);

  GateTimeValuesIndex = 0;
  GateTime = GateTimeValues[GateTimeValuesIndex];
  SampleValuesIndex = 0;
  NumberOfSamples = SampleValues[SampleValuesIndex];
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

	// if we havent yet received GPS data and the RTC isnt set
	if ((GPSData == FALSE) && (RTCSet == FALSE))
	{
		WaitforGPS();
	}

	// process any incoming GPS data
	GPS_Process();
	ProcessTouch();


	// get RTC latest date and time
	HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
	HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);

	if ((GPSData == TRUE) && (RTCSet == FALSE))
	{
		// we have valid GPS data so lets assume the PPS is working and we have a valid date/time
		SetRTC();
		RTCSet = TRUE;
		DisplayScreenTitles();
		DisplayButtons();
	}

	if (RTCSet == TRUE)
	{
		UpdateTimeDisplay();
	}

	if (GateCount != PreviousGateCount)
	{
		// we have new sample data
		CalculateCount();
		SampleArray[SampleArrayIndex++] = TotalCount;

		if (SampleArrayIndex == NumberOfSamples)
		{
			// we have all the samples we need to calculate the result
			// reset the index to 0 for next time
			SampleArrayIndex = 0;
			AverageCount = 0;
			for (int CountIndex = 0; CountIndex < NumberOfSamples; CountIndex++)
			{
				AverageCount = AverageCount + SampleArray[CountIndex];
			}
			ExternalMHZ = ((AverageCount / NumberOfSamples) / GateTime) / 1000000.0;
			memset(&UsrString, 0, sizeof(UsrString));

			sprintf(UsrString," Frequency : %07.4f MHz ",ExternalMHZ);
			BSP_LCD_DisplayStringAtLine(ExternalCountLine, (uint8_t*)&UsrString);
			ToggleSampleBlob();
		}
		PreviousGateCount = GateCount;

	}

	// if its 2am UTC lets force an update of the RTC from the GPS
	TimeSinceGPSUpdate = GetSystemTime() - GPS.LastTime;
	if ((sTime.Hours == 2) && (sTime.Minutes == 0) && (sTime.Seconds == 0) && (TimeSinceGPSUpdate < 500))
	{
		RTCSet = FALSE;
		GPSData = FALSE;
	}
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
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
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Configure LSE Drive Capability 
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);
  /** Configure the main internal regulator output voltage 
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 400;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Activate the Over-Drive mode 
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_6) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC|RCC_PERIPHCLK_RTC
                              |RCC_PERIPHCLK_USART6;
  PeriphClkInitStruct.PLLSAI.PLLSAIN = 384;
  PeriphClkInitStruct.PLLSAI.PLLSAIR = 5;
  PeriphClkInitStruct.PLLSAI.PLLSAIQ = 2;
  PeriphClkInitStruct.PLLSAI.PLLSAIP = RCC_PLLSAIP_DIV8;
  PeriphClkInitStruct.PLLSAIDivQ = 1;
  PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_8;
  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  PeriphClkInitStruct.Usart6ClockSelection = RCC_USART6CLKSOURCE_PCLK2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void CalculateCount(void)
{

	  Timer2Count = ExternalCountTimer2 - LastExternalCountTimer2;
	  Timer4Count = ExternalCountTimer4 - LastExternalCountTimer4;

	  TotalCount = (uint64_t)Timer4Count << 32;
	  TotalCount = TotalCount + Timer2Count;

}
void LCDConfig(void)
{

	BSP_LCD_Init();
	BSP_LCD_LayerDefaultInit(LTDC_LAYER_1, LCD_FB_START_ADDRESS);
	BSP_LCD_SelectLayer(LTDC_LAYER_1);
	BSP_LCD_Clear(LCD_COLOR_BLUE);
	BSP_LCD_LayerDefaultInit(LTDC_LAYER_2, LCD_FB_START_ADDRESS);
	BSP_LCD_SelectLayer(LTDC_LAYER_2);

	BSP_LCD_SetTransparency(LTDC_LAYER_1, 0);
	BSP_LCD_SetTransparency(LTDC_LAYER_2, 255);

	BSP_LCD_DisplayOn();

	BSP_LCD_SetBackColor(LCD_COLOR_BLUE);
	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
	DisplayScreenTitles();

}

void TSConfig()
{
	BSP_TS_Init(480, 272);
}
void WaitforGPS(void)
{
	memset(&UsrString, 0, sizeof(UsrString));
	sprintf(UsrString,"Waiting for GPS");
	BSP_LCD_DisplayStringAtLine(TimeLine, (uint8_t*)&UsrString);
}

void SetRTC(void)
{
	RTC_TimeTypeDef sTime = {0};
	RTC_DateTypeDef sDate = {0};

	sTime.Hours = GPS.GPRMC.UTC_Hour;
	sTime.Minutes = GPS.GPRMC.UTC_Min;
	sTime.Seconds = GPS.GPRMC.UTC_Sec;
	sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
	sTime.StoreOperation = RTC_STOREOPERATION_RESET;
	if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
	{
		Error_Handler();
	}
	sDate.WeekDay =  DayofWeek(GPS.GPRMC.Date_Year, GPS.GPRMC.Date_Month, GPS.GPRMC.Date_Day);
	sDate.Month = GPS.GPRMC.Date_Month;
	sDate.Date = GPS.GPRMC.Date_Day;
	sDate.Year = GPS.GPRMC.Date_Year;
	if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
	{
		Error_Handler();
	}
}

int DayofWeek(int y, int m, int d)
{
	//gives day of week for a given date Sunday=0, Saturday=6

	static int ShiftedMonths[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};

	// https://en.wikipedia.org/wiki/Determination_of_the_day_of_the_week#Sakamoto.27s_algorithm

	y -= m < 3;
	return (y + y/4 - y/100 + y/400 + ShiftedMonths[m-1] + d) % 7;

}
char* strtok_fr (char* s, char delim, char **save_ptr)
{
    char *tail;
    char c;

    if (s == NULL) {
        s = *save_ptr;
    }
    tail = s;
    if ((c = *tail) == '\0') {
        s = NULL;
    }
    else {
        do {
            if (c == delim) {
                *tail++ = '\0';
                break;
           }
        }while ((c = *++tail) != '\0');
    }
    *save_ptr = tail;
    return s;
}

char* strtok_f (char* s, char delim)
{
    static char* save_ptr;

    return strtok_fr (s, delim, &save_ptr);
}

void UpdateTimeDisplay(void)
{
	if (sTime.Seconds != sTimePrevious.Seconds)
	{
		memset(&UsrString, 0, sizeof(UsrString));
		sprintf(UsrString,"%02d/%02d/%02d %02d:%02d:%02d UTC", sDate.Date, sDate.Month, sDate.Year, sTime.Hours,sTime.Minutes,sTime.Seconds);
		BSP_LCD_DisplayStringAtLine(TimeLine, (uint8_t*)&UsrString);
		DrawGateTimeAndSamples();

		sTimePrevious = sTime;
	}
}
void DisplayScreenTitles(void)
{
	BSP_LCD_Clear(LCD_COLOR_BLUE);
	memset(&UsrString, 0, sizeof(UsrString));
	sprintf(UsrString,"G0MGX STM32 Freq Counter");
	BSP_LCD_DisplayStringAtLine(TitleLine, (uint8_t*)&UsrString);
}

void DrawGateTimeAndSamples(void)
{
	memset(&UsrString, 0, sizeof(UsrString));
	sprintf(UsrString," Gate : %d, Samples : %d ", GateTime, NumberOfSamples);
	BSP_LCD_DisplayStringAtLine(GateCountLine, (uint8_t*)&UsrString);
}
void DisplayButtons(void)
{
	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
	// screen is 480 wide, buttons are 50
	BSP_LCD_FillRect(100, ButtonLine * 24, 50, 50);
	BSP_LCD_FillRect(160, ButtonLine * 24, 50, 50);
	BSP_LCD_FillRect(270, ButtonLine * 24, 50, 50);
	BSP_LCD_FillRect(330, ButtonLine * 24, 50, 50);

	BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
	BSP_LCD_FillRect(101, (ButtonLine * 24) + 1, 48, 48);
	BSP_LCD_FillRect(161, (ButtonLine * 24) + 1, 48, 48);
	BSP_LCD_FillRect(271, (ButtonLine * 24) + 1, 48, 48);
	BSP_LCD_FillRect(331, (ButtonLine * 24) + 1, 48, 48);

	BSP_LCD_SetTextColor(LCD_COLOR_RED);
	DrawTriangle(100, ButtonLine);
	DrawTriangle(270, ButtonLine);
	DrawUpseideDownTriangle(160, ButtonLine);
	DrawUpseideDownTriangle(330, ButtonLine);

	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
}

void DrawTriangle (uint32_t XPos, uint32_t Line)
{
	uint32_t XPosIndex = XPos + 5;
	uint32_t YPosIndex = (Line * 24) + 44;
	uint32_t LineLength = 40;
	do
	{
		BSP_LCD_DrawHLine(XPosIndex, YPosIndex, LineLength);
		YPosIndex--;
		BSP_LCD_DrawHLine(XPosIndex, YPosIndex, LineLength);
		YPosIndex--;
		XPosIndex++;
		LineLength = LineLength -2;
	}
	while (LineLength >= 2);

}

void DrawUpseideDownTriangle (uint32_t XPos, uint32_t Line)
{
	uint32_t XPosIndex = XPos + 5;
	uint32_t YPosIndex = (Line * 24) + 4;
	uint32_t LineLength = 40;
	do
	{
		BSP_LCD_DrawHLine(XPosIndex, YPosIndex, LineLength);
		YPosIndex++;
		BSP_LCD_DrawHLine(XPosIndex, YPosIndex, LineLength);
		YPosIndex++;
		XPosIndex++;
		LineLength = LineLength - 2;
	}
	while (LineLength <= 40);

}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	GPS_CallBack();
}

void ProcessTouch(void)
{
	uint32_t ElapsedTime = 0;
	uint32_t ParameterChangeElapsedTime = 0;
	uint32_t TouchX = 0;
	uint32_t TouchY = 0;
	uint8_t MaxGateTimeIndex = 0;
	uint8_t MaxSamplesIndex = 0;

	TouchTime = GetSystemTime();

	if (TouchTime > PreviousTouchTime)
	{
		ElapsedTime = TouchTime - PreviousTouchTime;
	}
	else
	{
		ElapsedTime = 0xFFFFFFFF - PreviousTouchTime + TouchTime;
	}

	if (TouchTime > LastParameterChangeTime)
	{
		ParameterChangeElapsedTime = TouchTime - LastParameterChangeTime;
	}
	else
	{
		ParameterChangeElapsedTime = 0xFFFFFFFF - LastParameterChangeTime + TouchTime;
	}

	if (ElapsedTime > TouchUpdateFrequency)
	{
	  BSP_TS_GetState(&ts);
	  PreviousTouchTime = TouchTime;

	  if (ts.touchDetected)
	  {
		  TouchX = ts.touchX[0];
		  TouchY = ts.touchY[0];
		  MaxGateTimeIndex = sizeof(GateTimeValues)-1;
		  MaxSamplesIndex = sizeof(SampleValues)-1;

		  // this is the Gate Up Button
		  if ((TouchX >= 110) && (TouchX <= 150) && (TouchY >=170) && (TouchY <=210) && (ParameterChangeElapsedTime > ParameterChangeFrequency))
		  {
			  if ((GateTimeValuesIndex + 1) > MaxGateTimeIndex)
			  {
				  // if adding 1 to the index takes us out of range
				  // set back to the 1st value in the list
				  GateTimeValuesIndex = 0;
			  }
			  else
			  {
				  // otherwise increment the index to the next value
				  GateTimeValuesIndex++;
			  }
			  LastParameterChangeTime = TouchTime;
			  GateTime = GateTimeValues[GateTimeValuesIndex];
		  }
		  // this is the Gate Down Button
		  if ((TouchX >= 160) && (TouchX <= 200) && (TouchY >=170) && (TouchY <=210) && (ParameterChangeElapsedTime > ParameterChangeFrequency))
		  {
			  if (GateTimeValuesIndex == 0)
			  {
				  GateTimeValuesIndex = MaxGateTimeIndex;
			  }
			  else
			  {
				  GateTimeValuesIndex--;
			  }
			  LastParameterChangeTime = TouchTime;
			  GateTime = GateTimeValues[GateTimeValuesIndex];
		  }

		  if ((TouchX >= 270) && (TouchX <=320) && (TouchY >= 170) && (TouchY <=210) && (ParameterChangeElapsedTime > ParameterChangeFrequency))
		  {
			  // this is the sample up button
			  if ((SampleValuesIndex + 1) > MaxSamplesIndex)
			  {
				  SampleValuesIndex = 0;
			  }
			  else
			  {
				  SampleValuesIndex++;
			  }
			  LastParameterChangeTime = TouchTime;
			  NumberOfSamples = SampleValues[SampleValuesIndex];
			  SampleArrayIndex = 0;
		  }

		  if ((TouchX >= 340) && (TouchX <=380) && (TouchY >= 170) && (TouchY <=210) && (ParameterChangeElapsedTime > ParameterChangeFrequency))
		  {
			  // this is the sample down button
			  if (SampleValuesIndex == 0)
			  {
				SampleValuesIndex = MaxSamplesIndex;
			  }
			  else
			  {
				  SampleValuesIndex--;
			  }
			  LastParameterChangeTime = TouchTime;
			  NumberOfSamples = SampleValues[SampleValuesIndex];
			  SampleArrayIndex = 0;
		  }

		  htim8.Init.Period = GateTime - 1;
		  if (HAL_TIM_Base_Init(&htim8) != HAL_OK)
		  {
		    Error_Handler();
		  }
		  DrawGateTimeAndSamples();
	  }
	}
}

void ToggleGateBlob(void)
{
	static int GateBlobOn = FALSE;
	int GateBlobX = 50;
	int GateBlobY = 192;
	int GateBlobRadius = 10;

	if (GateBlobOn)
	{
		BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
		BSP_LCD_FillCircle(GateBlobX, GateBlobY, GateBlobRadius);
		GateBlobOn = FALSE;
	}
	else
	{
		BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
		BSP_LCD_FillCircle(GateBlobX, GateBlobY, GateBlobRadius);
		GateBlobOn = TRUE;
	}
	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
}

void ToggleSampleBlob(void)
{
	static int SampleBlobOn = FALSE;
	int SampleBlobX = 430;
	int SampleBlobY = 192;
	int SampleBlobRadius = 10;

	if (SampleBlobOn)
	{
		BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
		BSP_LCD_FillCircle(SampleBlobX, SampleBlobY, SampleBlobRadius);
		SampleBlobOn = FALSE;
	}
	else
	{
		BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
		BSP_LCD_FillCircle(SampleBlobX, SampleBlobY, SampleBlobRadius);
		SampleBlobOn = TRUE;
	}
	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
}

uint32_t GetSystemTime(void)
{
	return HAL_GetTick();
}
/* USER CODE END 4 */

 /**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */
  if (htim->Instance == TIM8)
  {
	  // this is the PPS interrupt
	  GateCount++;
	  LastExternalCountTimer2 = ExternalCountTimer2;
	  LastExternalCountTimer4 = ExternalCountTimer4;
	  ExternalCountTimer2 = __HAL_TIM_GET_COUNTER(&htim2);
	  ExternalCountTimer4 = __HAL_TIM_GET_COUNTER(&htim4);
	  HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
	  ToggleGateBlob();

  }
  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
	while (1)
	{
		HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
		HAL_Delay(150);
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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
