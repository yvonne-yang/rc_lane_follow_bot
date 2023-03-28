/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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


/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <string.h>
#include <stdio.h>
#include "config.h"
#include "ov7670.h"

/* USER CODE END Includes */
/* USER CODE BEGIN PV */
#define PREAMBLE "\r\n!START!\r\n"
#define DELTA_PREAMBLE "\r\n!DELTA!\r\n"
#define SUFFIX "!END!\r\n"
#define FULL_FRAME_N 5 // change to 1 for no temporal compression

uint16_t snapshot_buff[IMG_ROWS * IMG_COLS];
uint8_t old_snapshot_buff[IMG_ROWS * IMG_COLS];
uint8_t delta_snapshot_buff[IMG_ROWS * IMG_COLS];
//uint8_t tx_buff[sizeof(PREAMBLE) + 2 * IMG_ROWS * IMG_COLS + sizeof(SUFFIX)];
//uint8_t tx_buff[sizeof(PREAMBLE) + IMG_ROWS * IMG_COLS + sizeof(SUFFIX)]; // dma
uint8_t tx_buff[sizeof(PREAMBLE) + IMG_ROWS * IMG_COLS/2 + sizeof(SUFFIX)]; // trunc
size_t tx_buff_len = 0;

int length = IMG_ROWS * IMG_COLS;

uint8_t dcmi_flag = 0;
uint8_t tx_half_clpt = 0;
uint8_t tx_full_clpt = 1;
int timer_val;

// Your function definitions here
void uart_dma(void);
void trunc_rle(void);
void send_delta(void);


int main(void)
{
  /* Reset of all peripherals */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_DCMI_Init();
  MX_USART3_UART_Init();
  MX_USB_OTG_FS_PCD_Init();
  MX_I2C2_Init();
  MX_TIM1_Init();
  MX_TIM6_Init();
  
  char msg[100];
  
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  
  ov7670_init();
  HAL_Delay(100);
	dcmi_flag = 0;
  //HAL_TIM_Base_Start(&htim6);
	//timer_val = __HAL_TIM_GET_COUNTER(&htim6);
  ov7670_capture(snapshot_buff);
  
  // Your startup code here
	uint8_t *buf = (uint8_t*)snapshot_buff;
  memcpy(tx_buff, &PREAMBLE, sizeof(PREAMBLE));
	int frame = 0;

	/* timing code
	while(!dcmi_flag){HAL_Delay(0);}
  print_buf();*/
			
  while (1)
  {
    // Your code here
		/* time dcmi 
		if (dcmi_flag){
			int new_val = __HAL_TIM_GET_COUNTER(&htim6);
			sprintf(msg, "Cycles taken for dcmi: %d\n", new_val - timer_val);
			timer_val = new_val;
			print_msg(msg);
			dcmi_flag=0;
			HAL_DCMI_Resume(&hdcmi);
		}*/
		
		if (HAL_GPIO_ReadPin(USER_Btn_GPIO_Port, USER_Btn_Pin)) {
      HAL_Delay(100);  // debounce
			print_msg("Snap!\n");
		}
		
		/* 1, 2, 3 */
		if(dcmi_flag && HAL_UART_GetState(&huart3)== HAL_UART_STATE_READY){
			//uart_dma();
			if (frame%FULL_FRAME_N==0){
				trunc_rle();
			}
			else{
				send_delta();
			}
			
			dcmi_flag = 0;
			frame++;
		}
		/* 2. UART DMA half frames 
		if(dcmi_flag && tx_full_clpt){ 
			for(int i = 1; i < length * 2; i+=2){
				tx_buff[tx_buff_len++] = buf[i];
			}
			tx_full_clpt = 0;
		}
		if(dcmi_flag && tx_half_clpt){ 
			HAL_DCMI_Resume(&hdcmi);
			tx_half_clpt = 0;
		}
		*/
  }
}

void trunc_rle() {	
  int iter = 0;
	uint8_t *buf = (uint8_t*)snapshot_buff;
  memcpy(tx_buff, &PREAMBLE, sizeof(PREAMBLE));
	iter += sizeof(PREAMBLE);
	
	/* only truncate
	for(int i = 1; i < length*2; i+=2){
		if (i % 4 == 1){
			tx_buff[iter] = buf[i] & 0xF0;
		}
		else if(i % 4 == 3){
			tx_buff[iter] += (buf[i] & 0xF0)>>4;
			iter += 1;
		}
	}*/
	
	// truncate and RLE
	for (int i = 1; i < length*2; i+=2){
    int count = 1;
		while(1){ // find repeating
			if (i < length*2-2 && (buf[i]&0xF0) == (buf[i+2]&0xF0)){
			  count++;
			  i+=2;
      }
      else { break; }
		}

		while(count > 0xF){ // count > 15, need another byte
			tx_buff[iter++] = (buf[i]&0xF0) + 0xF;
			count -= 0xF;
		}
 		tx_buff[iter++] = (buf[i]&0xF0) + count;
   }
	
	 /* sanity check
	 int sum = 0;
	 for (int j = sizeof(PREAMBLE); j < iter; j++){
	   sum += tx_buff[j]&0x0F;
	 }
	 char msg[20];
	 sprintf(msg, "%d\n", sum);
	 print_msg(msg);
	 */
	 
	// for calculating deltas
	for (int i = 1; i < length*2; i+=2){
		old_snapshot_buff[i/2] = buf[i];
	}
	
  memcpy(&tx_buff[iter], &SUFFIX, sizeof(SUFFIX));
	HAL_DCMI_Resume(&hdcmi);
	
	uart_send_bin(tx_buff,iter+sizeof(SUFFIX));
}


void send_delta(){
  int iter = 0;
	uint8_t *buf = (uint8_t*)snapshot_buff;
  memcpy(tx_buff, &DELTA_PREAMBLE, sizeof(DELTA_PREAMBLE));
	iter += sizeof(DELTA_PREAMBLE);
	
	// difference
	for (int i = 1; i < length*2; i+=2){
		int val = (0xF0 & buf[i]) - (0xF0 &old_snapshot_buff[i/2]);
		old_snapshot_buff[i/2] = buf[i];
		buf[i/2] = val;
	}

	
	// truncate and RLE
	for (int i = 0; i < length; i++){
    int count = 1;
		while(1){ // find repeating
			if (i < length-1 && (buf[i]&0xF0) == (buf[i+1]&0xF0)){
			  count++;
			  i++;
      }
      else { break; }
		}

		while(count >= 0xF){ // count > 15, need another byte
			tx_buff[iter++] = (buf[i]&0xF0) + 0xF;
			count -= 0xF;
		}
 		tx_buff[iter++] = (buf[i]&0xF0) + count;
   }
	
	memcpy(&tx_buff[iter], &SUFFIX, sizeof(SUFFIX));
	HAL_DCMI_Resume(&hdcmi);
	
	uart_send_bin(tx_buff,iter+sizeof(SUFFIX));
	
}

void uart_dma() {
  // Send image data through serial port.
  // Your code here
	//timer_val = __HAL_TIM_GET_COUNTER(&htim6);
  tx_buff_len = 0;
	uint8_t *buf = (uint8_t*)snapshot_buff;
  memcpy(tx_buff, &PREAMBLE, sizeof(PREAMBLE));
	tx_buff_len += sizeof(PREAMBLE);
	
	for(int i = 1; i < length*2; i+=2){
		tx_buff[tx_buff_len++] = buf[i];
	}
  //memcpy(&tx_buff[tx_buff_len], &SUFFIX, sizeof(SUFFIX));
	HAL_DCMI_Resume(&hdcmi);
	
	uart_send_bin(tx_buff,sizeof(tx_buff));
	
	/* timing code 
  while (HAL_UART_GetState(&huart3)!= HAL_UART_STATE_READY){HAL_Delay(0);}
	int new_val = __HAL_TIM_GET_COUNTER(&htim6);
	sprintf(msg, "Cycles taken for uart send: %d", new_val - timer_val);
	print_msg(msg);
	*/
}


/*
void HAL_UART_TxHalfCpltCallback(UART_HandleTypeDef *huart){
	tx_half_clpt = 1;
}


void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart){
	tx_full_clpt = 1;
}
*/

/* USER CODE END PV */
