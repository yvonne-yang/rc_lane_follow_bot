#include "main.h"
#include "motor.h"
void motor_stop(){
	HAL_GPIO_WritePin(H_Bridge_IN1_GPIO_Port, H_Bridge_IN1_Pin, 0);
	HAL_GPIO_WritePin(H_Bridge_IN2_GPIO_Port, H_Bridge_IN2_Pin, 0);
	HAL_GPIO_WritePin(H_Bridge_IN3_GPIO_Port, H_Bridge_IN3_Pin, 0); 
	HAL_GPIO_WritePin(H_Bridge_IN4_GPIO_Port, H_Bridge_IN4_Pin, 0);
}

void go_left(){
	HAL_GPIO_WritePin(H_Bridge_IN1_GPIO_Port, H_Bridge_IN1_Pin, 0); // do 1 to turn sharply
	HAL_GPIO_WritePin(H_Bridge_IN2_GPIO_Port, H_Bridge_IN2_Pin, 0);
	HAL_GPIO_WritePin(H_Bridge_IN3_GPIO_Port, H_Bridge_IN3_Pin, 0);
	HAL_GPIO_WritePin(H_Bridge_IN4_GPIO_Port, H_Bridge_IN4_Pin, 1);
}

void go_right(){
	HAL_GPIO_WritePin(H_Bridge_IN1_GPIO_Port, H_Bridge_IN1_Pin, 1); 
	HAL_GPIO_WritePin(H_Bridge_IN2_GPIO_Port, H_Bridge_IN2_Pin, 0);
	HAL_GPIO_WritePin(H_Bridge_IN3_GPIO_Port, H_Bridge_IN3_Pin, 0); // do 1 to turn sharply
	HAL_GPIO_WritePin(H_Bridge_IN4_GPIO_Port, H_Bridge_IN4_Pin, 0);
}

void go_straight(){
	HAL_GPIO_WritePin(H_Bridge_IN1_GPIO_Port, H_Bridge_IN1_Pin, 1); 
	HAL_GPIO_WritePin(H_Bridge_IN2_GPIO_Port, H_Bridge_IN2_Pin, 0);
	HAL_GPIO_WritePin(H_Bridge_IN3_GPIO_Port, H_Bridge_IN3_Pin, 0);
	HAL_GPIO_WritePin(H_Bridge_IN4_GPIO_Port, H_Bridge_IN4_Pin, 1);
}

void go_backward(){
	HAL_GPIO_WritePin(H_Bridge_IN1_GPIO_Port, H_Bridge_IN1_Pin, 0); 
	HAL_GPIO_WritePin(H_Bridge_IN2_GPIO_Port, H_Bridge_IN2_Pin, 1);
	HAL_GPIO_WritePin(H_Bridge_IN3_GPIO_Port, H_Bridge_IN3_Pin, 1);
	HAL_GPIO_WritePin(H_Bridge_IN4_GPIO_Port, H_Bridge_IN4_Pin, 0);
}

void go_left_or_right(double angle){
	if (angle<0){go_left();} else{go_right();}
}

void motor(double angle){
	double val = (angle>=0)?angle:0-angle;
  if (val<10){ // straight
		go_straight();
		HAL_Delay(100);
	}
	else if (val>45-2 && val<45+2){ //45 degree
	  go_straight();
	  HAL_Delay(150);
		go_left_or_right(angle);
		HAL_Delay(320);
	}
	else if (val>90-2 && val<90+2){ //90 degree
	  go_straight();
	  HAL_Delay(220);
		go_left_or_right(angle);
		HAL_Delay(580);
	}
	else{	  
		go_straight();
	  HAL_Delay(100);
		go_left_or_right(angle);
		HAL_Delay(5.778*val+60);
	}
}
