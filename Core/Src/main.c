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
#include "usb_host.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "dwt_delay.h"
#include "amiga.h"
#include "rtc_msm6242.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
HID_USBDevicesTypeDef *usb;
HID_KEYBD_Info_TypeDef *k_pinfo;

volatile int8_t joy0datH;
volatile int8_t joy0datL;
volatile int8_t joy1datH;
volatile int8_t joy1datL;

volatile int8_t POTGORH;
volatile int8_t POTGORL;

volatile int8_t CIAAPRA;

uint8_t extraBtn;

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

RTC_HandleTypeDef hrtc;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_RTC_Init(void);
void MX_USB_HOST_Process(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static void usb_keyboard_led(USBH_HandleTypeDef *usbhost, keyboard_led_t ld) {
	keyboard_led_t led = ld;
	USBH_StatusTypeDef status;
	int retrygood = 1;
	if (usbhost != NULL) {
		for (;;) {
			status = USBH_HID_SetReport(usbhost, 0x02, 0x00, &led, 1);
			if (status == USBH_OK)
				retrygood--;
			if (retrygood <= 0)
				break;
		}
	}
}

static void usb_keyboard_led_init(USBH_HandleTypeDef *usbhost) {
	keyboard_led_t led = CAPS_LOCK_LED | NUM_LOCK_LED | SCROLL_LOCK_LED;
	int n;
	if (usbhost != NULL) {
		for (n = 0; n < 2; n++) {
			usb_keyboard_led(usbhost, led);
			HAL_Delay(250);
			usb_keyboard_led(usbhost, 0);
			HAL_Delay(125);
		}
		/* Default leds off */
		usb_keyboard_led(usbhost, 0);
	}
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
	/* USER CODE BEGIN 1 */
	led_status_t stat;

	uint8_t do_led = 0;
	uint8_t KeyboardLedInit = 0;
	static keyboard_led_t keyboard_led = 0;

	joy0datH = 0;
	joy0datL = 0;
	joy1datH = 0;
	joy1datL = 0;

	POTGORH = 0x55;   //0x55; //default not pressed buttons
	POTGORL = 0x01;

	CIAAPRA = 0xC0;

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
	MX_USB_HOST_Init();
	MX_RTC_Init();
	/* USER CODE BEGIN 2 */
	DWT_Init();

	// amikb_startup();
	//amikb_ready(0);

	HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(KBD_CLOCK_GPIO_Port, KBD_CLOCK_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(KBD_DATA_GPIO_Port, KBD_DATA_Pin, GPIO_PIN_SET);
	keyboard_code_t keycode = { 0 };

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {

		/* USER CODE END WHILE */
		MX_USB_HOST_Process();

		/* USER CODE BEGIN 3 */

		usb = (HID_USBDevicesTypeDef*) USBH_HID_GetUSBDev();

		if (usb != NULL && usb->keyboardusbhost != NULL) {
			if (KeyboardLedInit == 0) {
				usb_keyboard_led_init(usb->keyboardusbhost);
				//restore LED status on new keyboard.
				usb_keyboard_led(usb->keyboardusbhost, keyboard_led);
				KeyboardLedInit++;
			}

			k_pinfo = usb->keyboard;
			int i = 0;

			if (k_pinfo != NULL) {
				keycode.lctrl = k_pinfo->lctrl;
				keycode.lshift = k_pinfo->lshift;
				keycode.lalt = k_pinfo->lalt;
				keycode.lgui = k_pinfo->lgui;
				keycode.rctrl = k_pinfo->rctrl;
				keycode.rshift = k_pinfo->rshift;
				keycode.ralt = k_pinfo->ralt;
				keycode.rgui = k_pinfo->rgui;
				for (i = 0; i < KEY_PRESSED_MAX; i++) {
					keycode.keys[i] = k_pinfo->keys[i];
				}
				HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
				stat = amikb_process(&keycode);

				switch (stat) {
				case LED_CAPS_LOCK_OFF:
					keyboard_led &= ~CAPS_LOCK_LED;
					do_led = 1;
					break;
				case LED_CAPS_LOCK_ON:
					keyboard_led |= CAPS_LOCK_LED;
					do_led = 1;
					break;
				case LED_NUM_LOCK_OFF:
					keyboard_led &= ~NUM_LOCK_LED;
					do_led = 1;
					break;
				case LED_NUM_LOCK_ON:
					keyboard_led |= NUM_LOCK_LED;
					do_led = 1;
					break;
				case LED_SCROLL_LOCK_OFF:
					keyboard_led &= ~SCROLL_LOCK_LED;
					do_led = 1;
					break;
				case LED_SCROLL_LOCK_ON:
					keyboard_led |= SCROLL_LOCK_LED;
					do_led = 1;
					break;
				case LED_RESET_BLINK:
					usb_keyboard_led_init(usb->keyboardusbhost);
					do_led = 0;
					break;
				default:
				case NO_LED:
					do_led = 0;
					break;
				}
				// ...and let the led management to be done!
				if (do_led) {
					usb_keyboard_led(usb->keyboardusbhost, keyboard_led);
				}

			}
		} else {
			//Keyboard is deinit.

			KeyboardLedInit = 0;

		}
		//clear usbDevHost
		usb->keyboardusbhost = NULL;

		//check if override is required
		if (usb->overridePorts == 1) {
			HAL_GPIO_WritePin(INTSIG6_GPIO_Port, INTSIG6_Pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(INTSIG4_GPIO_Port, INTSIG4_Pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
		} else {
			HAL_GPIO_WritePin(INTSIG6_GPIO_Port, INTSIG6_Pin, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(INTSIG4_GPIO_Port, INTSIG4_Pin, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
		}

		if (usb->mouse != NULL) {
			__disable_irq(); //delay interrupt
			joy0datH = joy0datH + ((usb->mouse->y) / 2);
			joy0datL = joy0datL + ((usb->mouse->x) / 2);

			if (usb->mouse->buttons[1] == 1) {
				POTGORH &= ~(1UL << 2);
			} else {
				POTGORH |= 1UL << 2;
			}

			if (usb->mouse->buttons[2] == 1) {
				POTGORH &= ~(1UL << 0);
			} else {
				POTGORH |= 1UL << 0;
			}

			//set up first button.
			if (usb->mouse->buttons[0] == 1) {
				CIAAPRA ^= (0 ^ CIAAPRA) & (1UL << 6);

			} else {
				CIAAPRA ^= (-1 ^ CIAAPRA) & (1UL << 6);

			}

			__enable_irq();

		}

		//gamepad_data
		//RIGHT = (*joymap&0x1);
		//LEFT = (*joymap>>1&0x1);
		//UP = (*joymap>>3&0x1);
		//DOWN = (*joymap>>2&0x1);

		//BTN1 =  (*joymap>>6&0x1);
		//BTN2 =  (*joymap>>5&0x1);
		//BTN3 =  (*joymap>>4&0x1);
		//BTN4 =  (*joymap>>7&0x1);

		//gamepad_extraBtn
		// Play/Start - 0x20 32
		//right front - 0x02 2
		//left front - 0x01 1

		if (usb->gamepad1 != NULL) {

			__disable_irq();
			joy1datH = 0;
			joy1datL = 0;

			//usb->gamepad1->gamepad_extraBtn;

			if (usb->gamepad1->gamepad_data >> 1 & 0x1)
				joy1datH = 0x3;
			if (usb->gamepad1->gamepad_data >> 3 & 0x1)
				joy1datH = 0x1;
			if (usb->gamepad1->gamepad_data >> 1 & 0x1
					&& usb->gamepad1->gamepad_data >> 3 & 0x1)
				joy1datH = 0x2;

			if (usb->gamepad1->gamepad_data & 0x1)
				joy1datL = 0x3;
			if (usb->gamepad1->gamepad_data >> 2 & 0x1)
				joy1datL = 0x1;
			if (usb->gamepad1->gamepad_data & 0x1
					&& usb->gamepad1->gamepad_data >> 2 & 0x1)
				joy1datL = 0x2;

			if (usb->gamepad1->gamepad_data >> 5 & 0x1) {
				POTGORH &= ~(1UL << 4);
			} else {
				POTGORH |= 1UL << 4;
			}

			if (usb->gamepad1->gamepad_data >> 4 & 0x1) {
				POTGORH &= ~(1UL << 6);
			} else {
				POTGORH |= 1UL << 6;
			}

			if (usb->gamepad1->gamepad_data >> 6 & 0x1) {
				CIAAPRA ^= (0 ^ CIAAPRA) & (1UL << 7);

			} else {
				CIAAPRA ^= (-1 ^ CIAAPRA) & (1UL << 7);

			}

			__enable_irq();

		}

		if (usb->gamepad2 != NULL && usb->mouseDetected != 1) //make sure mouse doesn't colide with controller
				{
			__disable_irq();
			joy0datH = 0;
			joy0datL = 0;

			if (usb->gamepad2->gamepad_data >> 1 & 0x1)
				joy0datH = 0xFF;
			if (usb->gamepad2->gamepad_data >> 3 & 0x1)
				joy0datH = 0x01;
			if (usb->gamepad2->gamepad_data >> 1 & 0x1
					&& usb->gamepad2->gamepad_data >> 3 & 0x1)
				joy1datH = 0x02;

			if (usb->gamepad2->gamepad_data & 0x1)
				joy0datL = 0xFF;
			if (usb->gamepad2->gamepad_data >> 2 & 0x1)
				joy0datL = 0x01;
			if (usb->gamepad2->gamepad_data & 0x1
					&& usb->gamepad2->gamepad_data >> 2 & 0x1)
				joy0datL = 0x02;

			if (usb->gamepad2->gamepad_data >> 5 & 0x1) {
				POTGORH &= ~(1UL << 0);
			} else {
				POTGORH |= 1UL << 0;
			}

			if (usb->gamepad2->gamepad_data >> 4 & 0x1) {
				POTGORH &= ~(1UL << 2);
			} else {
				POTGORH |= 1UL << 2;
			}

			if (usb->gamepad2->gamepad_data >> 6 & 0x1) {
				CIAAPRA ^= (0 ^ CIAAPRA) & (1UL << 6);

			} else {
				CIAAPRA ^= (-1 ^ CIAAPRA) & (1UL << 6);
			}
			__enable_irq();
		}

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
	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = { 0 };

	/** Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI
			| RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.LSIState = RCC_LSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 25;
	RCC_OscInitStruct.PLL.PLLN = 432;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 9;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}
	/** Activate the Over-Drive mode
	 */
	if (HAL_PWREx_EnableOverDrive() != HAL_OK) {
		Error_Handler();
	}
	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK) {
		Error_Handler();
	}
	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC
			| RCC_PERIPHCLK_CLK48;
	PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
	PeriphClkInitStruct.Clk48ClockSelection = RCC_CLK48SOURCE_PLL;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief RTC Initialization Function
 * @param None
 * @retval None
 */
static void MX_RTC_Init(void) {

	/* USER CODE BEGIN RTC_Init 0 */

	/* USER CODE END RTC_Init 0 */

	RTC_TimeTypeDef sTime = { 0 };
	RTC_DateTypeDef sDate = { 0 };

	/* USER CODE BEGIN RTC_Init 1 */

	/* USER CODE END RTC_Init 1 */
	/** Initialize RTC Only
	 */
	hrtc.Instance = RTC;
	hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
	hrtc.Init.AsynchPrediv = 127;
	hrtc.Init.SynchPrediv = 255;
	hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
	hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
	hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
	if (HAL_RTC_Init(&hrtc) != HAL_OK) {
		Error_Handler();
	}

	/* USER CODE BEGIN Check_RTC_BKUP */

	/* USER CODE END Check_RTC_BKUP */

	/** Initialize RTC and set the Time and Date
	 */
	sTime.Hours = 6;
	sTime.Minutes = 25;
	sTime.Seconds = 55;
	sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
	sTime.StoreOperation = RTC_STOREOPERATION_RESET;
	if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK) {
		Error_Handler();
	}
	sDate.WeekDay = RTC_WEEKDAY_MONDAY;
	sDate.Month = RTC_MONTH_JANUARY;
	sDate.Date = 15;
	sDate.Year = 20;

	if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN RTC_Init 2 */

	/* USER CODE END RTC_Init 2 */

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOC, RW_Pin | FIRE1_Pin | KBD_DATA_Pin, GPIO_PIN_SET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOA, FIRE0_Pin | INTSIG3_Pin, GPIO_PIN_SET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB, KBD_CLOCK_Pin | LED2_Pin, GPIO_PIN_SET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOC, INTSIG4_Pin | INTSIG6_Pin | INTSIG7_Pin,
			GPIO_PIN_RESET);

	/*Configure GPIO pin : INTSIG8_Pin */
	GPIO_InitStruct.Pin = INTSIG8_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(INTSIG8_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pins : RW_Pin FIRE1_Pin KBD_DATA_Pin */
	GPIO_InitStruct.Pin = RW_Pin | FIRE1_Pin | KBD_DATA_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/*Configure GPIO pins : FIRE0_Pin INTSIG3_Pin */
	GPIO_InitStruct.Pin = FIRE0_Pin | INTSIG3_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO pins : A0_Pin A1_Pin A2_Pin INTSIG5_Pin */
	GPIO_InitStruct.Pin = A0_Pin | A1_Pin | A2_Pin | INTSIG5_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/*Configure GPIO pins : D0_Pin D1_Pin D2_Pin D3_Pin
	 D4_Pin D5_Pin D6_Pin D7_Pin */
	GPIO_InitStruct.Pin = D0_Pin | D1_Pin | D2_Pin | D3_Pin | D4_Pin | D5_Pin
			| D6_Pin | D7_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO pin : KBD_CLOCK_Pin */
	GPIO_InitStruct.Pin = KBD_CLOCK_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(KBD_CLOCK_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pins : INTSIG2_Pin INTSIG1_Pin */
	GPIO_InitStruct.Pin = INTSIG2_Pin | INTSIG1_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO pin : LED2_Pin */
	GPIO_InitStruct.Pin = LED2_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(LED2_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pins : INTSIG4_Pin INTSIG6_Pin */
	GPIO_InitStruct.Pin = INTSIG4_Pin | INTSIG6_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/*Configure GPIO pin : A4_Pin */
	GPIO_InitStruct.Pin = A4_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(A4_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : INTSIG7_Pin */
	GPIO_InitStruct.Pin = INTSIG7_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(INTSIG7_GPIO_Port, &GPIO_InitStruct);

	/* EXTI interrupt init*/
	HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

	HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

}

/* USER CODE BEGIN 4 */

static inline uint8_t ReadRTCAddress() {
	uint8_t address = 0;
	address = ((GPIOC->IDR >> 9) & 1U); //PC9 - INTSIG5
	address = (address << 1) | ((GPIOA->IDR >> 10) & 1U); //PA10 - A4
	address = (address << 1) | ((GPIOA->IDR >> 15) & 1U); //PA15 - INTSIG3
	address = (address << 1) | ((GPIOC->IDR >> 7) & 1U); //PC7 - A2
	return address;
}

static inline uint8_t ReadAddress() {
	uint8_t address = 0;
	address = ((GPIOC->IDR >> 9) & 1U); //PC9 - INTSIG5
	address = (address << 1) | ((GPIOA->IDR >> 10) & 1U); //PA10 - A4
	address = (address << 1) | ((GPIOA->IDR >> 15) & 1U); //PA15 - INTSIG3
	address = (address << 2) | ((GPIOC->IDR >> 6) & 3U); //PC6 - A1 & PC7 - A2
	address = (address << 1) | ((GPIOC->IDR >> 4) & 1U); //PC4 - A0
	return address;
}

static inline void WriteData(uint8_t data) {
	uint32_t pupdr = GPIOB->PUPDR;
	GPIOB->PUPDR = pupdr & 0xFFFF0000; // no pull on bits 0 to 7;
	GPIOB->OTYPER &= ~(0xFF);     //Set up push-pull
	GPIOB->MODER = (GPIOB->MODER & 0xFFFF0000) | 0x5555; // set to output
	GPIOB->ODR = (GPIOB->ODR & 0xFF00) | data;

	GPIOB->OTYPER |= 0xFF; // set to open drain
	GPIOB->MODER = (GPIOB->MODER & 0xFFFF0000); // set to input
	GPIOB->PUPDR = pupdr; // restore
}

static inline uint8_t ReadData() {
	uint8_t data = 0;
	data = GPIOB->IDR;
	//GPIOC->BSRR = GPIO_PIN_11 << 16; // set
	//GPIOC->BSRR = GPIO_PIN_11; // reset
	return data;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	uint8_t address = 0;
	address = ReadAddress();
	//Read data 8 bit GPIOB->IDR

	if (GPIO_Pin == INTSIG1_Pin
			&& HAL_GPIO_ReadPin(INTSIG1_GPIO_Port, INTSIG1_Pin)) //Process RTC
					{
		uint8_t rtcaddress = ReadRTCAddress();
		if (HAL_GPIO_ReadPin(RW_GPIO_Port, RW_Pin)) {

			WriteData(RTC_Read(rtcaddress, &hrtc));
		} else {
			RTC_Write(rtcaddress, ReadData(), &hrtc);
		}

	}

	if ((GPIO_Pin == INTSIG8_Pin
			&& HAL_GPIO_ReadPin(INTSIG8_GPIO_Port, INTSIG8_Pin))) //Process  Mouse/Joystick moves
	{
		if (HAL_GPIO_ReadPin(RW_GPIO_Port, RW_Pin)) {
			if (address == 0xA) {
				WriteData(joy0datH);
			}

			if (address == 0xB) {
				WriteData(joy0datL);
			}

			if (address == 0xC) {
				WriteData(joy1datH);
			}

			if (address == 0xD || address == 0xF) {
				WriteData(joy1datL);
			}
		}

	}

	if (GPIO_Pin == INTSIG2_Pin
			&& HAL_GPIO_ReadPin(INTSIG2_GPIO_Port, INTSIG2_Pin)) //Process buttons
					{

		if (HAL_GPIO_ReadPin(RW_GPIO_Port, RW_Pin)) {
			if (address == 0x01) {
				WriteData(CIAAPRA);
			}

			if (address == 0x16) {
				WriteData(POTGORH);
			}

			if (address == 0x17) {
				WriteData(POTGORL);
			}
		} else {
			if (address == 0x01) {
				uint8_t data = 0;
				data = ReadData();
				CIAAPRA = data;
			}

			if (address == 0x34) {
				//	data = ReadData();
				//	POTGORH = data;
			}

			if (address == 0x35) {
				//	data = ReadData();
				//	POTGORL = data;
			}
		}

	}

	// do this inline to avoid function call overhead
	GPIOC->BSRR = GPIO_PIN_11 << 16; // set //HAL_GPIO_WritePin(INTSIG7_GPIO_Port, INTSIG7_Pin, GPIO_PIN_SET);
	GPIOC->BSRR = GPIO_PIN_11; // reset //HAL_GPIO_WritePin(INTSIG7_GPIO_Port, INTSIG7_Pin, GPIO_PIN_RESET);
}

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */

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
