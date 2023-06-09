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
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include "config.h"
#include "ov7670.h"
#include "motor.h"

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

/* USER CODE BEGIN PV */
#ifndef M_PI
	#define M_PI 3.14159265358979323846
#endif
#define PREAMBLE "\r\n!START!\r\n"
#define DELTA_PREAMBLE "\r\n!DELTA!\r\n"
#define SUFFIX "!END!\r\n"
#define LANE_SUFFIX "!END_LANE!\r\n"
#define FULL_FRAME_N 5 // change to 1 for no temporal compression

#define LINE_START_ROW 120
#define LINE_MID_ROW4 115
#define LINE_MID_ROW3 110
#define LINE_MID_ROW2 105
#define LINE_MID_ROW1 100
#define LINE_END_ROW 95
//#define LINE_START_ROW 135
//#define LINE_END_ROW 110
//#define DYNAMIC_THRESHOLD 
#define THRESHOLD 0x70

uint16_t snapshot_buff[IMG_ROWS * IMG_COLS];
uint8_t old_snapshot_buff[IMG_ROWS * IMG_COLS];
uint8_t delta_snapshot_buff[IMG_ROWS * IMG_COLS];
uint8_t tx_buff[sizeof(PREAMBLE) + 3000 + sizeof(SUFFIX) // magik 
	 + 5 + sizeof(LANE_SUFFIX)]; // lane stuffs
size_t tx_buff_len = 0;
uint8_t uart_rx_buffer[10] = {0};
bool drive_flag = false;

int length = IMG_ROWS * IMG_COLS;

uint8_t dcmi_flag = 0;
uint8_t tx_half_clpt = 0;
uint8_t tx_full_clpt = 1;
int timer_val;

// lane stuff
int line_row1_i=-1, line_row2_i=-1,line_row2_offset=0, line_row1_offset=0;
int mid_row1_i=-1, mid_row1_offset=0,
   mid_row2_i=-1,mid_row2_offset=0,
   mid_row3_i=-1,mid_row3_offset=0,
   mid_row4_i=-1,mid_row4_offset=0;
int topleft=-1, topright=-1, botleft=-1, botright=-1;
double angle = 0;
int stop = 1;

// Your function definitions here
void uart_dma(void);
void trunc_rle(void);
void send_delta(void);
int fill_lane_data(int iter);
bool find_black_cols(int i, int first_count, int* left, int* right);
bool find_lanes(int* botleft, int* botright, int*topleft, int*topright);
bool compute_angles(int* botleft, int* botright, int*topleft, int*topright,
        double* angle);


int main(void)
{
  /* Reset of all peripherals */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
	TIM2->CCR4 = 100;
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_DCMI_Init();
  MX_USART3_UART_Init();
  MX_USB_OTG_FS_PCD_Init();
  MX_I2C2_Init();
  MX_TIM1_Init();
  MX_TIM6_Init();
  MX_USART2_UART_Init();
  MX_TIM2_Init();
  
  char msg[100];
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  
  ov7670_init();
  HAL_Delay(100);
	dcmi_flag = 0;
  ov7670_capture(snapshot_buff);
  
  // Your startup code here
	uint8_t *buf = (uint8_t*)snapshot_buff;
  memcpy(tx_buff, &PREAMBLE, sizeof(PREAMBLE));
	int frame = 0;
	
	motor_stop();
	
	/*hardcoded path
	go_straight();
	HAL_Delay(530);
	go_left();
	HAL_Delay(70);
	go_straight();
	HAL_Delay(420);
	motor_stop();
	HAL_Delay(1000);//before first right turn
	go_left();
	HAL_Delay(30);
	go_right();
  HAL_Delay(120);
	go_straight();
	HAL_Delay(330);
	go_right();
  HAL_Delay(280);
	go_straight();
	HAL_Delay(360);
	go_left();
	HAL_Delay(50);
	motor_stop();
	HAL_Delay(1000);//exit first turn
	go_straight();
	HAL_Delay(940);
	go_left();
	HAL_Delay(20);
	go_right();
  HAL_Delay(270);
	go_straight();
	HAL_Delay(480);
	go_right();
  HAL_Delay(160);
	go_straight();
	HAL_Delay(700);
	go_straight();
	HAL_Delay(200);
	motor_stop();*/
	HAL_UART_Receive_IT(&huart2, uart_rx_buffer, 1); 
	HAL_UART_Receive_IT(&huart3, uart_rx_buffer, 1); 
	// main loop
  while (1)
  {

		if (HAL_GPIO_ReadPin(USER_Btn_GPIO_Port, USER_Btn_Pin)) {
      HAL_Delay(100);  // debounce
		
			print_msg("Snap!\n");
		}
		
		if(dcmi_flag && HAL_UART_GetState(&huart2) != HAL_UART_STATE_BUSY_TX){
			topleft=-1, topright=-1, botleft=-1, botright=-1;
			angle = 0;
			trunc_rle();
			dcmi_flag = 0;
			frame++;
		}
		if(drive_flag){
			char direction = uart_rx_buffer[0];
			int delay = 500;
			if(direction == 'w'){
				go_straight();
				HAL_Delay(delay);
				motor_stop();
			}else 
			if(direction == 'a'){
				go_left();
				HAL_Delay(150);
				motor_stop();
			}else 
			if(direction == 's'){
				go_backward();
				HAL_Delay(delay);
				motor_stop();
			}else 
			if(direction == 'd'){
				go_right();
				HAL_Delay(150);
				motor_stop();
			}
			drive_flag=false;
		}
  }
}


void trunc_rle() {	
  int iter = 0;
	uint8_t *buf = (uint8_t*)snapshot_buff;
  memcpy(tx_buff, &PREAMBLE, sizeof(PREAMBLE));
	iter += sizeof(PREAMBLE);
	
    // dynamic threshold
#ifdef DYNAMIC_THRESHOLD
    int maxp=0,minp= 0xff;
    for (int i = 1; i < length*2;i+=4){
			maxp = (buf[i]>maxp)?buf[i]:maxp; 
			minp = (buf[i]<minp)?buf[i]:minp; 
    }
    int threshold = (maxp-minp)*0.7; // min(THRESHOLD, )
#else
  int threshold=0x70;
#endif

	// threshold
	for (int i = 1; i < length*2; i+=2){        
		if(i/2%IMG_COLS<=4){ // get rid of left black column artifact
       buf[i] = 0x80;
    }
    else{
			buf[i] = (buf[i]>= threshold) ? 0x80 : 0;
		}
	}
	
	// truncate and RLE
	line_row1_i=-1,line_row2_i=-1,mid_row1_i=-1,mid_row2_i=-1,mid_row3_i=-1,mid_row4_i=-1;
	for (int i = 1; i < length*2; i+=2){
    int count = 1;
		while(1){ // find repeating
			if (i < length*2-2 && buf[i] == buf[i+2]){
			  count++;
			  i+=2;
      }
      else { break; }
		}

		while(count > 0x7F){ // count > 0x7F, need another byte
			tx_buff[iter++] = buf[i] + 0x7F;
			count -= 0x7F;
		}
		
		// record rle index for lane finding
        if (line_row2_i == -1 && i/2 >= LINE_END_ROW*IMG_COLS){
            line_row2_i = iter;
            line_row2_offset = i/2 - LINE_END_ROW*IMG_COLS+1;
        }
        else if (mid_row1_i == -1 && i/2 >= LINE_MID_ROW1*IMG_COLS){
            mid_row1_i = iter;
            mid_row1_offset = i/2 - LINE_MID_ROW1*IMG_COLS+1;
        }
        else if (mid_row2_i == -1 && i/2 >= LINE_MID_ROW2*IMG_COLS){
            mid_row2_i = iter;
            mid_row2_offset = i/2 - LINE_MID_ROW2*IMG_COLS+1;
        }
        else if (mid_row3_i == -1 && i/2 >= LINE_MID_ROW3*IMG_COLS){
            mid_row3_i = iter;
            mid_row3_offset = i/2 - LINE_MID_ROW3*IMG_COLS+1;
        }
        else if (mid_row4_i == -1 && i/2 >= LINE_MID_ROW4*IMG_COLS){
            mid_row4_i = iter;
            mid_row4_offset = i/2 - LINE_MID_ROW4*IMG_COLS+1;
        }
        else if (line_row1_i == -1 && i/2 >= LINE_START_ROW*IMG_COLS){
            line_row1_i = iter;
            line_row1_offset = i/2 - LINE_START_ROW*IMG_COLS +1;
        }
 		
 		tx_buff[iter++] = buf[i] + count;
   }
	
	/* for calculating deltas
	for (int i = 1; i < length*2; i+=2){
		old_snapshot_buff[i/2] = buf[i];
	}*/
	
  memcpy(&tx_buff[iter], &SUFFIX, sizeof(SUFFIX));
	HAL_DCMI_Resume(&hdcmi);
	
	int size = fill_lane_data(iter+sizeof(SUFFIX));
	uart_send_bin(tx_buff,size);
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) 
{
  HAL_UART_Receive_IT(&huart2, uart_rx_buffer, 1); 
	HAL_UART_Receive_IT(&huart3, uart_rx_buffer, 1); 
	drive_flag = true;
}

void send_delta(){
  int iter = 0;
	uint8_t *buf = (uint8_t*)snapshot_buff;
  memcpy(tx_buff, &DELTA_PREAMBLE, sizeof(DELTA_PREAMBLE));
	iter += sizeof(DELTA_PREAMBLE);
	
	// threshold
	for (int i = 1; i < length*2; i+=2){
		buf[i] = ((buf[i]&0xF0)>= THRESHOLD) ? 0x80 : 0;
	}
	
	// difference
	for (int i = 1; i < length*2; i+=2){
		int val = buf[i] - old_snapshot_buff[i/2];
		old_snapshot_buff[i/2] = buf[i];
		buf[i/2] = val;
	}

	
	// truncate and RLE
	for (int i = 0; i < length; i++){
				
    int count = 1;
		while(1){ // find repeating
			if (i < length-1 && buf[i] == buf[i+1]){
			  count++;
			  i++;
      }
      else { break; }
		}

		while(count >= 0x7F){ // count > 0x75, need another byte
			tx_buff[iter++] = buf[i] + 0x7F;
			count -= 0x7F;
		}
 		tx_buff[iter++] = buf[i] + count;
   }
	
	memcpy(&tx_buff[iter], &SUFFIX, sizeof(SUFFIX));
	HAL_DCMI_Resume(&hdcmi);
	
	int size = fill_lane_data(iter+sizeof(SUFFIX));
	uart_send_bin(tx_buff,size);
	
}


int fill_lane_data(int iter){
	int ret = find_lanes(&botleft, &botright, &topleft, &topright);
	if (ret){
		ret = compute_angles(&botleft, &botright, &topleft, &topright,
   &angle);
	}
	if (!ret) {stop = 1; angle = 0; botleft=topleft=topright=botright=-1;} 
  else {stop = 0;}
	
	tx_buff[iter++] = topleft;
	tx_buff[iter++] = 110;
	tx_buff[iter++] = botleft;
	tx_buff[iter++] = 135;
	tx_buff[iter++] = topright;
	tx_buff[iter++] = 110;
	tx_buff[iter++] = botright;
	tx_buff[iter++] = 135;
	tx_buff[iter++] = stop;
	tx_buff[iter++] = (int8_t)angle;
	memcpy(&tx_buff[iter], &LANE_SUFFIX, sizeof(LANE_SUFFIX));
	return iter + sizeof(LANE_SUFFIX);
}

/*Lane detection*/
// find 2 black regions (lanes) and return middle pixel for each region
// return 0 if no lanes found
// left and right are set to -1 if not found
bool find_black_cols(int i, int first_count, int* left, int* right){
    *left = -1, *right = -1;
    int col = 0,black_count=0;
	  const int old_first_val = tx_buff[i], old_i = i;
    tx_buff[i] = (tx_buff[i]&0x80) + first_count;
    while (col<IMG_COLS){
        if ((tx_buff[i]&0x80) == 0x0){ // black
            int count = 0;
            if (*left==-1){ //leftmost
                int start_col = col;
                count=0;
                while(col+0x7F<IMG_COLS &&
                      (tx_buff[i+1]&0x80) == 0x0){
                    //printf("%d %d\n", tx_buff[i]&0x80, tx_buff[i]&0x7F);
                    count+=tx_buff[i]&0x7F;
                    col+=tx_buff[i++]&0x7F; // 0x7F
                }
                if (col+(tx_buff[i]&0x7f)>=IMG_COLS){
                    count+=IMG_COLS-col;
                }
                else{
                    count+=tx_buff[i]&0x7F;
                }
            *left=start_col + (count)/2; // middle
            }
            else{ //rightmost
                int start_col = col;
                count=0;
                while(col+0x7f<IMG_COLS &&
                      (tx_buff[i+1]&0x80) == 0x0){
                    //printf("%d %d\n", tx_buff[i]&0x80, tx_buff[i]&0x7F);
                    count+=tx_buff[i]&0x7F;
                    col+=tx_buff[i++]&0x7F; // 0x7F
                }
                if (col+(tx_buff[i]&0x7f)>=IMG_COLS){
                    count+=IMG_COLS-col;
                }
                else{
                count+=tx_buff[i]&0x7F;
                }
                *right=start_col + (count)/2; // middle
            }
            black_count+=count;
        }
        //printf("%d %d\n", tx_buff[i]&0x80, tx_buff[i]&0x7F);
        col+=tx_buff[i++]&0x7F;
    }
	  tx_buff[old_i] = old_first_val;
    //printf("black_count=%d\n",black_count);
    if (black_count > 50 || black_count < 5)
    {
        //printf("No lanes found\n");
        return 0;
    }
    // if only found one col, determine left or right lane
    if (*right == -1){
        if (*left > IMG_COLS/2){
            *right = *left;
            *left = -1;
        }
    }
    return 1;
}

// return 0 if 0 lanes found
bool find_lanes(int* botleft, int* botright, int*topleft, int*topright){
    int i, col;
    int black_count=0;
    if (line_row1_i == -1 || line_row2_i == -1){
        //printf("dunno what index line should be\n");
        return 0;
    }

    // TODO: virtual lines, line extensions
    bool ret1 = find_black_cols(line_row1_i,line_row1_offset,botleft,botright); // bottom
    bool ret2 = find_black_cols(line_row2_i,line_row2_offset,topleft,topright); // top

    // if lane straddles center
    if (*botright!=-1 && *topleft!=-1 && *topright==-1 && *botleft==-1){
        int tmpleft, tmpright;
        // check if the mid rows have a black col within the bounds of botright topleft
        if (find_black_cols(mid_row1_i,mid_row1_offset,&tmpleft,&tmpright) &&
            ((tmpleft!=-1 && tmpleft<*botright+5 && tmpleft>*topleft-5) || 
             (tmpright!=-1 && tmpright<*botright+5 && tmpright>*topleft-5))){
        if (find_black_cols(mid_row2_i,mid_row2_offset,&tmpleft,&tmpright) &&
            ((tmpleft!=-1 && tmpleft<*botright+5 && tmpleft>*topleft-5) || 
             (tmpright!=-1 && tmpright<*botright+5 && tmpright>*topleft-5))){
        if (find_black_cols(mid_row3_i,mid_row3_offset,&tmpleft,&tmpright) &&
            ((tmpleft!=-1 && tmpleft<*botright+5 && tmpleft>*topleft-5) || 
             (tmpright!=-1 && tmpright<*botright+5 && tmpright>*topleft-5))){
        if (find_black_cols(mid_row4_i,mid_row4_offset,&tmpleft,&tmpright) &&
            ((tmpleft!=-1 && tmpleft<*botright+5 && tmpleft>*topleft-5) || 
             (tmpright!=-1 && tmpright<*botright+5 && tmpright>*topleft-5))){
        *topright = *topleft;
        *topleft = -1;
        }}}}
    } 
    if (*botleft!=-1 && *topright!=-1 && *topleft==-1 && *botright==-1){
        int tmpleft, tmpright;
        // check if the mid rows have a black col within the bounds of botleft topright
        if (find_black_cols(mid_row1_i,mid_row1_offset,&tmpleft,&tmpright) &&
            ((tmpleft!=-1 && tmpleft<*topright+5 && tmpleft>*botleft-5) || 
             (tmpright!=-1 && tmpright<*topright+5 && tmpright>*botleft-5))){
        if (find_black_cols(mid_row2_i,mid_row2_offset,&tmpleft,&tmpright) &&
            ((tmpleft!=-1 && tmpleft<*topright+5 && tmpleft>*botleft-5) || 
             (tmpright!=-1 && tmpright<*topright+5 && tmpright>*botleft-5))){
        if (find_black_cols(mid_row3_i,mid_row3_offset,&tmpleft,&tmpright) &&
            ((tmpleft!=-1 && tmpleft<*topright+5 && tmpleft>*botleft-5) || 
             (tmpright!=-1 && tmpright<*topright+5 && tmpright>*botleft-5))){
        if (find_black_cols(mid_row4_i,mid_row4_offset,&tmpleft,&tmpright) &&
            ((tmpleft!=-1 && tmpleft<*topright+5 && tmpleft>*botleft-5) || 
             (tmpright!=-1 && tmpright<*topright+5 && tmpright>*botleft-5))){
        *botleft = *topright;
        *topright = -1;
        }}}}
    } 

    // return 0 if neither lane found
    if (!ret1 && !ret2){
			return 0;
		}
		else{
        //printf("line1=(%d,%d), (%d,%d)\n",*botleft,LINE_START_ROW,*topleft,LINE_END_ROW);
        //printf("line2=(%d,%d), (%d,%d)\n",*botright,LINE_START_ROW,*topright,LINE_END_ROW);
        return 1;
    }
}

bool compute_angles(int* botleft, int* botright, int*topleft, int*topright,
        double* angle){
    if ((*botleft == -1 || *topleft==-1) && (*botright == -1 || *topright==-1)){ // no lanes
			return 0;
		}
		else if (*botleft == -1 || *topleft==-1){ // only see right lane
        *angle = atan2(*topright-*botright,LINE_START_ROW-LINE_END_ROW)*180/M_PI;
		}
    else if (*botright == -1 || *topright==-1){ // only see left lane
        *angle = atan2(*topleft-*botleft,LINE_START_ROW-LINE_END_ROW)*180/M_PI;
		}
    else {
        float right_ang = atan2(*topright-*botright,LINE_START_ROW-LINE_END_ROW)*180/M_PI;
        //printf("right=%f\n",right_ang);
        float left_ang = atan2(*topleft-*botleft,LINE_START_ROW-LINE_END_ROW)*180/M_PI;
        //printf("left=%f\n",left_ang);
        *angle = (right_ang + left_ang)/2;
		}
    //printf("angle=%f\n",*angle);
    return 1;
}



/* lab 6 code
void trunc_rle() {	
  int iter = 0;
	uint8_t *buf = (uint8_t*)snapshot_buff;
  memcpy(tx_buff, &PREAMBLE, sizeof(PREAMBLE));
	iter += sizeof(PREAMBLE);
	
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
	
	// timing code 
  while (HAL_UART_GetState(&huart3)!= HAL_UART_STATE_READY){HAL_Delay(0);}
	int new_val = __HAL_TIM_GET_COUNTER(&htim6);
	sprintf(msg, "Cycles taken for uart send: %d", new_val - timer_val);
	print_msg(msg);

}
*/

/* USER CODE END PV */
