/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
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
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "string.h"
#include "MFS.h"

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
RNG_HandleTypeDef hrng;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = { .name = "defaultTask",
		.stack_size = 128 * 4, .priority = (osPriority_t) osPriorityNormal, };
/* Definitions for Teller1 */
osThreadId_t Teller1Handle;
const osThreadAttr_t Teller1_attributes = { .name = "Teller1", .stack_size = 128
		* 4, .priority = (osPriority_t) osPriorityLow, };
/* Definitions for Teller2 */
osThreadId_t Teller2Handle;
const osThreadAttr_t Teller2_attributes = { .name = "Teller2", .stack_size = 128
		* 4, .priority = (osPriority_t) osPriorityLow, };
/* Definitions for Teller3 */
osThreadId_t Teller3Handle;
const osThreadAttr_t Teller3_attributes = { .name = "Teller3", .stack_size = 128
		* 4, .priority = (osPriority_t) osPriorityLow, };
/* Definitions for myTask05 */
osThreadId_t myTask05Handle;
const osThreadAttr_t myTask05_attributes = { .name = "myTask05", .stack_size =
		128 * 4, .priority = (osPriority_t) osPriorityLow, };
/* Definitions for mfs */
osThreadId_t mfsHandle;
const osThreadAttr_t mfs_attributes = { .name = "mfs", .stack_size = 128 * 4,
		.priority = (osPriority_t) osPriorityLow, };
/* Definitions for Customer_Queue */
osMessageQueueId_t Customer_QueueHandle;
const osMessageQueueAttr_t Customer_Queue_attributes = { .name =
		"Customer_Queue" };
/* Definitions for myMutex01 */
osMutexId_t myMutex01Handle;
const osMutexAttr_t myMutex01_attributes = { .name = "myMutex01" };
/* Definitions for Teller_1 */
osMutexId_t Teller_1Handle;
const osMutexAttr_t Teller_1_attributes = { .name = "Teller_1" };
/* Definitions for Teller_2 */
osMutexId_t Teller_2Handle;
const osMutexAttr_t Teller_2_attributes = { .name = "Teller_2" };
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_RNG_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);
void StartDefaultTask(void *argument);
void StartTask02_Teller1(void *argument);
void StartTask03_Teller2(void *argument);
void StartTask04_Teller3(void *argument);
void StartTask05(void *argument);
void StartTask06(void *argument);

/* USER CODE BEGIN PFP */
void println(UART_HandleTypeDef *huart, char out[]);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

enum Teller_Status Teller1_status = Free;
enum Teller_Status Teller2_status = Free;
enum Teller_Status Teller3_status = Free;
int Customersqueue = 0, bankopen = 1, teller1idle = 1, teller2idle = 1,
		teller3idle = 1, max_depth = 0;

uint8_t buffer[1000];
Teller teller1, teller2, teller3;
Customer customer;

void myprintf(UART_HandleTypeDef *huart, uint32_t out) {

	HAL_UART_Transmit(huart, buffer, out, 1000);
	char spacer[3] = "\r\n";
	HAL_UART_Transmit(huart, (uint8_t*) spacer, 3, 10);
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */
	MFS_init();
	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_RNG_Init();
	MX_USART2_UART_Init();
	MX_TIM2_Init();
	/* USER CODE BEGIN 2 */
	uint32_t ddata = sprintf((char*) buffer,
			" Welcome to Project 3 \n\r Press Enter to continue");
	myprintf(&huart2, ddata);
	while (USART2->RDR != '\r')
		;

	/* USER CODE END 2 */

	/* Init scheduler */
	osKernelInitialize();
	/* Create the mutex(es) */
	/* creation of myMutex01 */
	myMutex01Handle = osMutexNew(&myMutex01_attributes);

	/* creation of Teller_1 */
	Teller_1Handle = osMutexNew(&Teller_1_attributes);

	/* creation of Teller_2 */
	Teller_2Handle = osMutexNew(&Teller_2_attributes);

	/* USER CODE BEGIN RTOS_MUTEX */
	/* add mutexes, ... */
	/* USER CODE END RTOS_MUTEX */

	/* USER CODE BEGIN RTOS_SEMAPHORES */
	/* add semaphores, ... */
	/* USER CODE END RTOS_SEMAPHORES */

	/* USER CODE BEGIN RTOS_TIMERS */
	/* start timers, add new ones, ... */
	HAL_TIM_Base_Start(&htim2);
	/* USER CODE END RTOS_TIMERS */

	/* Create the queue(s) */
	/* creation of Customer_Queue */
	Customer_QueueHandle = osMessageQueueNew(16, sizeof(uint32_t),
			&Customer_Queue_attributes);

	/* USER CODE BEGIN RTOS_QUEUES */
	/* add queues, ... */
	/* USER CODE END RTOS_QUEUES */

	/* Create the thread(s) */
	/* creation of defaultTask */
	defaultTaskHandle = osThreadNew(StartDefaultTask, NULL,
			&defaultTask_attributes);

	/* creation of Teller1 */
	Teller1Handle = osThreadNew(StartTask02_Teller1, NULL, &Teller1_attributes);

	/* creation of Teller2 */
	Teller2Handle = osThreadNew(StartTask03_Teller2, NULL, &Teller2_attributes);

	/* creation of Teller3 */
	Teller3Handle = osThreadNew(StartTask04_Teller3, NULL, &Teller3_attributes);

	/* creation of myTask05 */
	myTask05Handle = osThreadNew(StartTask05, NULL, &myTask05_attributes);

	/* creation of mfs */
	mfsHandle = osThreadNew(StartTask06, NULL, &mfs_attributes);

	/* USER CODE BEGIN RTOS_THREADS */
	/* add threads, ... */
	/* USER CODE END RTOS_THREADS */

	/* USER CODE BEGIN RTOS_EVENTS */
	/* add events, ... */
	/* USER CODE END RTOS_EVENTS */

	/* Start scheduler */
	osKernelStart();

	/* We should never get here as control is now taken by the scheduler */
	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Configure the main internal regulator output voltage
	 */
	if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1)
			!= HAL_OK) {
		Error_Handler();
	}
	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
	RCC_OscInitStruct.MSIState = RCC_MSI_ON;
	RCC_OscInitStruct.MSICalibrationValue = 0;
	RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
	RCC_OscInitStruct.PLL.PLLM = 1;
	RCC_OscInitStruct.PLL.PLLN = 40;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
	RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
	RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}
	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief RNG Initialization Function
 * @param None
 * @retval None
 */
static void MX_RNG_Init(void) {

	/* USER CODE BEGIN RNG_Init 0 */

	/* USER CODE END RNG_Init 0 */

	/* USER CODE BEGIN RNG_Init 1 */

	/* USER CODE END RNG_Init 1 */
	hrng.Instance = RNG;
	if (HAL_RNG_Init(&hrng) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN RNG_Init 2 */

	/* USER CODE END RNG_Init 2 */

}

/**
 * @brief TIM2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM2_Init(void) {

	/* USER CODE BEGIN TIM2_Init 0 */

	/* USER CODE END TIM2_Init 0 */

	TIM_ClockConfigTypeDef sClockSourceConfig = { 0 };
	TIM_MasterConfigTypeDef sMasterConfig = { 0 };

	/* USER CODE BEGIN TIM2_Init 1 */

	/* USER CODE END TIM2_Init 1 */
	htim2.Instance = TIM2;
	htim2.Init.Prescaler = 7900;
	htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim2.Init.Period = 4294967295;
	htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim2) != HAL_OK) {
		Error_Handler();
	}
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK) {
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig)
			!= HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN TIM2_Init 2 */

	/* USER CODE END TIM2_Init 2 */

}

/**
 * @brief USART2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART2_UART_Init(void) {

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
	if (HAL_UART_Init(&huart2) != HAL_OK) {
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
static void MX_GPIO_Init(void) {

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOA_CLK_ENABLE();

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument) {
	/* USER CODE BEGIN 5 */
	int mins = 0, hrs = 9, dayover = 0, teller1breakcheck, teller2breakcheck,
			teller3breakcheck;
	char T1stat[10], T2stat[10], T3stat[10], busy[10] = "Busy", free[10] =
			"Free", breaak[10] = "Break";
	uint32_t data, teller1_break, teller2_break, teller3_break;
	/* Infinite loop */
	teller1.Customers_served = 0;
	teller2.Customers_served = 0;
	teller3.Customers_served = 0;
	teller1.ready_for_break = 1;
	teller2.ready_for_break = 1;
	teller3.ready_for_break = 1;
	teller1.customer_service_time = 4;
	teller2.customer_service_time = 4;
	teller3.customer_service_time = 4;
	teller1.breaks = 0;
	teller2.breaks = 0;
	teller3.breaks = 0;
	teller1.avg_breaktime = 2;
	teller2.avg_breaktime = 2;
	teller3.avg_breaktime = 2;
	teller1.lbreak = 0;
	teller2.lbreak = 0;
	teller3.lbreak = 0;
	teller1.sbreak = 9999;
	teller2.sbreak = 9999;
	teller3.sbreak = 9999;
	teller1.avg_wait = 1;
	teller2.avg_wait = 1;
	teller3.avg_wait = 1;
	teller1.max_wait = 0;
	teller2.max_wait = 0;
	teller3.max_wait = 0;
	customer.Customer_avg_wait = 0;
	customer.Customer_max_wait = 0;

	for (;;) {

		if (mins == 60) {
			if (hrs == 12) {
				hrs = 0;
			}
			hrs++;
			mins = 0;
			if (hrs == 4) {

				bankopen = 0;
				dayover = 1;
			}
		}
		if (bankopen || Teller1_status == Busy || Teller2_status == Busy
				|| Teller3_status == Busy) {
			if (Teller1_status == Busy) {
				strcpy(T1stat, busy);
			} else if (Teller1_status == Free) {
				strcpy(T1stat, free);
			} else if (Teller1_status == Break) {
				strcpy(T1stat, breaak);
			}
			if (Teller2_status == Busy) {
				strcpy(T2stat, busy);
			} else if (Teller2_status == Free) {
				strcpy(T2stat, free);
			} else if (Teller2_status == Break) {
				strcpy(T2stat, breaak);
			}
			if (Teller3_status == Busy) {
				strcpy(T3stat, busy);
			} else if (Teller3_status == Free) {
				strcpy(T3stat, free);
			} else if (Teller3_status == Break) {
				strcpy(T3stat, breaak);
			}

			data =
					sprintf((char*) buffer,
							"Time = %d::%d queue:%d:: Teller1 %s has served %d :: Teller2 %s has served %d :: Teller3 %s has served %d",
							hrs, mins,
							osMessageQueueGetCount(Customer_QueueHandle),
							T1stat, teller1.Customers_served, T2stat,
							teller2.Customers_served, T3stat,
							teller3.Customers_served);

			myprintf(&huart2, data);

		} else if (dayover) {
			data = sprintf((char*) buffer,
					" Bank has Closed \n\r End of day Report\n\r");
			myprintf(&huart2, data);
			data =
					sprintf((char*) buffer,
							" Teller1 served %d Customers\n\r Teller2 served %d Customers \n\r Teller3 served %d Customers",
							teller1.Customers_served, teller2.Customers_served,
							teller3.Customers_served);
			myprintf(&huart2, data);
			data = sprintf((char*) buffer, " Total Customers Served %d",
					teller1.Customers_served + teller2.Customers_served
							+ teller3.Customers_served);
			myprintf(&huart2, data);
			data = sprintf((char*) buffer,
					"Teller1-Customer Transaction time  %d mins\n\r"
							" Teller2-Customer Transaction time  %d mins\n\r"
							" Teller3-Customer Transaction time  %d mins\n\r",
					teller1.customer_service_time,
					teller2.customer_service_time,
					teller3.customer_service_time);
			myprintf(&huart2, data);
			data = sprintf((char*) buffer, " Teller1-Breaks  %d \n\r"
					" Teller2-Breaks  %d \n\r"
					" Teller3-Breaks  %d \n\r", teller1.breaks, teller2.breaks,
					teller3.breaks);
			myprintf(&huart2, data);
			data =
					sprintf((char*) buffer,
							" Teller1-Longest break  %d     Shortest break  %d mins\n\r"
									" Teller2-Longest Breaks %d     Shortest break   %d mins\n\r"
									" Teller3-Longest Breaks %d     Shortest break   %d mins\n\r",
							teller1.lbreak, teller1.sbreak, teller2.lbreak,
							teller2.sbreak, teller3.lbreak, teller3.sbreak);
			myprintf(&huart2, data);
			data = sprintf((char*) buffer,
					" Customer MAX wait time in queue  %d seconds \n\r"

					, customer.Customer_max_wait);
			myprintf(&huart2, data);
			data = sprintf((char*) buffer,
					" Customer AVG wait time in queue  %d seconds \n\r"

					, customer.Customer_avg_wait);
			myprintf(&huart2, data);

			data = sprintf((char*) buffer, " Teller1-Avg break time  %d mins\n\r"
					" Teller2-Avg Break time  %d mins\n\r"
					" Teller3-Avg Break time  %d mins\n\r",
					teller1.avg_breaktime, teller2.avg_breaktime,
					teller3.avg_breaktime);
			myprintf(&huart2, data);
			data = sprintf((char*) buffer, " Teller1-Avg wait time  %d mins\n\r"
					" Teller2-Avg wait time  %d mins\n\r"
					" Teller3-Avg wait time  %d mins\n\r", teller1.avg_wait,
					teller2.avg_wait, teller3.avg_wait);
			myprintf(&huart2, data);
			data = sprintf((char*) buffer, " Teller1-Max wait time  %d mins\n\r"
					" Teller2-Max wait time  %d mins\n\r"
					" Teller3-Max wait time  %d mins\n\r", teller1.max_wait,
					teller2.max_wait, teller3.max_wait);
			myprintf(&huart2, data);
			data = sprintf((char*) buffer,
					" Teller1-Max Transaction time  %d mins\n\r"
							" Teller2-Max Transaction time  %d mins\n\r"
							" Teller3-Max Transaction time  %d mins\n\r",
					teller1.max_trans, teller2.max_trans, teller3.max_trans);
			myprintf(&huart2, data);
			data = sprintf((char*) buffer, " Max Queue Depth %d \n\r",

			max_depth);
			myprintf(&huart2, data);
			data = sprintf((char*) buffer, " Idle %d", vApplicationIdleHook());
			myprintf(&huart2, data);

			dayover--;
		}

		mins++;
		if (osMessageQueueGetCount(Customer_QueueHandle) == 0
				&& Teller1_status == Free) {
			teller1idle++;
		}
		if (Teller1_status == Busy) {
			if (teller1.max_wait < teller1idle) {
				teller1.max_wait = teller1idle;
			}
			teller1.avg_wait = (teller1.avg_wait + teller1idle) / 2;
			teller1idle = 1;
		}
		if (osMessageQueueGetCount(Customer_QueueHandle) == 0
				&& Teller2_status == Free) {
			teller2idle++;
		}
		if (Teller2_status == Busy) {
			if (teller2.max_wait < teller2idle) {
				teller2.max_wait = teller2idle;
			}
			teller2.avg_wait = (teller2.avg_wait + teller2idle) / 2;
			teller2idle = 1;
		}
		if (osMessageQueueGetCount(Customer_QueueHandle) == 0
				&& Teller3_status == Free) {
			teller3idle++;
		}
		if (Teller3_status == Busy) {
			if (teller3.max_wait < teller3idle) {
				teller3.max_wait = teller3idle;
			}
			teller3.avg_wait = (teller3.avg_wait + teller3idle) / 2;
			teller3idle = 1;
		}

		if (Teller1_status != Break && teller1.ready_for_break == 1) {
			teller1_break = HAL_RNG_GetRandomNumber(&hrng);
			teller1breakcheck = (teller1_break % 30) + 30;
			teller1.ready_for_break = 0;
		}
		if (Teller1_status != Break) {
			teller1breakcheck--;
		}
		if (Teller1_status != Busy && teller1breakcheck <= 0) {
			Teller1_status = Break;

		}
		if (Teller2_status != Break && teller2.ready_for_break == 1) {
			teller2_break = HAL_RNG_GetRandomNumber(&hrng);
			teller2breakcheck = (teller2_break % 30) + 30;
			teller2.ready_for_break = 0;
		}
		if (Teller2_status != Break) {
			teller2breakcheck--;
		}
		if (Teller2_status != Busy && teller2breakcheck <= 0) {
			Teller2_status = Break;

		}
		if (Teller3_status != Break && teller3.ready_for_break == 1) {
			teller3_break = HAL_RNG_GetRandomNumber(&hrng);
			teller3breakcheck = (teller3_break % 30) + 30;
			teller3.ready_for_break = 0;
		}
		if (Teller3_status != Break) {
			teller3breakcheck--;
		}
		if (Teller3_status != Busy && teller3breakcheck <= 0) {
			Teller3_status = Break;

		}

		//MFS_print_int(osMessageQueueGetCount(Customer_QueueHandle));

		osDelay(100);
//		MFS_7seg_refresh();

	}
	/* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartTask02_Teller1 */
/**
 * @brief Function implementing the Teller1 thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartTask02_Teller1 */
void StartTask02_Teller1(void *argument) {
	/* USER CODE BEGIN StartTask02_Teller1 */
	uint32_t e = 0, z, b;
	int a;
	/* Infinite loop */
	for (;;) {
		if (osMessageQueueGetCount(Customer_QueueHandle) > 0
				&& Teller1_status == Free) {
			osMessageQueueGet(Customer_QueueHandle, &e, 0, 0xFFFFFFFF);
			int time1 = ((__HAL_TIM_GET_COUNTER(&htim2) - e) % 100);
			if (time1 > customer.Customer_max_wait) {
				customer.Customer_max_wait = time1;
			}
			customer.Customer_avg_wait = (customer.Customer_avg_wait + (time1))
					/ 2;

			z = HAL_RNG_GetRandomNumber(&hrng);
			a = (z % 450) + 30;
			if (teller1.max_trans < (a / 60)) {
				teller1.max_trans = a / 60;
			}
			teller1.customer_service_time = (teller1.customer_service_time
					+ (a / 60)) / 2;
			//m--;
			Teller1_status = Busy;
			teller1.Customers_served += 1;
			osDelay(a * 1.66);
			Teller1_status = Free;

		} else if (Teller1_status == Break) {
			z = HAL_RNG_GetRandomNumber(&hrng);
			a = (z % 3) + 1;
			osDelay(a * 100);
			Teller1_status = Free;
			teller1.avg_breaktime = (teller1.avg_breaktime + a) / 2;
			if (a > teller1.lbreak) {
				teller1.lbreak = a;
			}
			if (a < teller1.sbreak) {
				teller1.sbreak = a;
			}
			teller1.breaks++;
			teller1.ready_for_break = 1;
		}

	}
	/* USER CODE END StartTask02_Teller1 */
}

/* USER CODE BEGIN Header_StartTask03_Teller2 */
/**
 * @brief Function implementing the Teller2 thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartTask03_Teller2 */
void StartTask03_Teller2(void *argument) {
	/* USER CODE BEGIN StartTask03_Teller2 */
	uint32_t w = 0, zz;
	int b;
	/* Infinite loop */
	for (;;) {
		if (osMessageQueueGetCount(Customer_QueueHandle) > 0
				&& Teller2_status == Free) {

			osMessageQueueGet(Customer_QueueHandle, &w, 0, 0xFFFFFFFF);
			int time1 = ((__HAL_TIM_GET_COUNTER(&htim2) - w) % 100);
			;
			if (time1 > customer.Customer_max_wait) {
				customer.Customer_max_wait = time1;
			}
			customer.Customer_avg_wait = (customer.Customer_avg_wait + (time1))
					/ 2;
			zz = HAL_RNG_GetRandomNumber(&hrng);
			b = (zz % 450) + 30;
			if (teller2.max_trans < (b / 60)) {
				teller2.max_trans = b / 60;
			}
			teller2.customer_service_time = (teller2.customer_service_time
					+ (b / 60)) / 2;
			Teller2_status = Busy;
			teller2.Customers_served += 1;
			osDelay(b * 1.66);

			Teller2_status = Free;
		} else if (Teller2_status == Break) {
			zz = HAL_RNG_GetRandomNumber(&hrng);
			b = (zz % 3) + 1;
			osDelay(b * 100);
			Teller2_status = Free;
			teller2.avg_breaktime = (teller2.avg_breaktime + b) / 2;
			if (b > teller2.lbreak) {
				teller2.lbreak = b;
			}
			if (b < teller2.sbreak) {
				teller2.sbreak = b;
			}
			teller2.breaks++;
			teller2.ready_for_break = 1;
		}

	}
	/* USER CODE END StartTask03_Teller2 */
}

/* USER CODE BEGIN Header_StartTask04_Teller3 */
/**
 * @brief Function implementing the Teller3 thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartTask04_Teller3 */
void StartTask04_Teller3(void *argument) {
	/* USER CODE BEGIN StartTask04_Teller3 */
	uint32_t q = 0, zzz;
	int c;
	/* Infinite loop */
	for (;;) {
		if (osMessageQueueGetCount(Customer_QueueHandle) > 0
				&& Teller3_status == Free) {
			//Customer customer3;
			osMessageQueueGet(Customer_QueueHandle, &q, 0, 0xFFFFFFFF);
			//lll = q;
			int time1 = ((__HAL_TIM_GET_COUNTER(&htim2) - q) % 100);
			if (time1 > customer.Customer_max_wait) {
				customer.Customer_max_wait = time1;
			}
			customer.Customer_avg_wait = (customer.Customer_avg_wait + (time1))
					/ 2;
			zzz = HAL_RNG_GetRandomNumber(&hrng);
			//m--;
			c = (zzz % 450) + 30;
			if (teller3.max_trans < (c / 60)) {
				teller3.max_trans = c / 60;
			}
			teller3.customer_service_time = (teller3.customer_service_time
					+ (c / 60)) / 2;
			Teller3_status = Busy;
			teller3.Customers_served += 1;
			osDelay(c * 1.66);
			Teller3_status = Free;
		} else if (Teller3_status == Break) {
			zzz = HAL_RNG_GetRandomNumber(&hrng);
			c = (zzz % 3) + 1;
			osDelay(c * 100);
			Teller3_status = Free;
			teller3.avg_breaktime = (teller3.avg_breaktime + c) / 2;
			if (c > teller3.lbreak) {
				teller3.lbreak = c;
			}
			if (c < teller3.sbreak) {
				teller3.sbreak = c;
			}
			teller3.breaks++;
			teller3.ready_for_break = 1;
		}

	}
	/* USER CODE END StartTask04_Teller3 */
}

/* USER CODE BEGIN Header_StartTask05 */
/**
 * @brief Function implementing the myTask05 thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartTask05 */
void StartTask05(void *argument) {
	/* USER CODE BEGIN StartTask05 */
	uint32_t qu, l;
	for (;;) {
		if (bankopen) {
			//Customer customer;
			qu = HAL_RNG_GetRandomNumber(&hrng);
			qu = (qu % 3) + 1;
			l = __HAL_TIM_GET_COUNTER(&htim2);
			osMessageQueuePut(Customer_QueueHandle, &l, 0, 0xFFFFFFFF);
			if (osMessageQueueGetCount(Customer_QueueHandle) > max_depth) {
				max_depth = osMessageQueueGetCount(Customer_QueueHandle);

			}
			//MFS_print_int(osMessageQueueGetCount(Customer_QueueHandle)-1);
			osDelay(100 * qu);
			//MFS_7seg_refresh();
		}
	}
	/* USER CODE END StartTask05 */
}

/* USER CODE BEGIN Header_StartTask06 */
/**
 * @brief Function implementing the mfs thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartTask06 */
void StartTask06(void *argument) {
	/* USER CODE BEGIN StartTask06 */
	/* Infinite loop */
	for (;;) {
		MFS_print_int(1);
		osDelay(1);
		MFS_7seg_refresh();
		MFS_print_int(1);
				osDelay(1);
				MFS_7seg_refresh();
	}
	/* USER CODE END StartTask06 */
}

/**
 * @brief  Period elapsed callback in non blocking mode
 * @note   This function is called  when TIM5 interrupt took place, inside
 * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
 * a global variable "uwTick" used as application time base.
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	/* USER CODE BEGIN Callback 0 */

	/* USER CODE END Callback 0 */
	if (htim->Instance == TIM5) {
		HAL_IncTick();
	}
	/* USER CODE BEGIN Callback 1 */

	/* USER CODE END Callback 1 */
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (100) {
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
