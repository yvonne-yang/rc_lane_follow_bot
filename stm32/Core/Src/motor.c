#include "main.h"
#include "motor.h"
void motor_stop(){
	HAL_GPIO_WritePin(H_Bridge_IN1_GPIO_Port, H_Bridge_IN1_Pin, 0);
	HAL_GPIO_WritePin(H_Bridge_IN2_GPIO_Port, H_Bridge_IN2_Pin, 0);
	HAL_GPIO_WritePin(H_Bridge_IN3_GPIO_Port, H_Bridge_IN3_Pin, 0); 
	HAL_GPIO_WritePin(H_Bridge_IN4_GPIO_Port, H_Bridge_IN4_Pin, 0);
}

void go_left(double angle){
	HAL_GPIO_WritePin(H_Bridge_IN1_GPIO_Port, H_Bridge_IN1_Pin, 1);
	HAL_GPIO_WritePin(H_Bridge_IN2_GPIO_Port, H_Bridge_IN2_Pin, 0);
	HAL_GPIO_WritePin(H_Bridge_IN3_GPIO_Port, H_Bridge_IN3_Pin, 0); // do 1 to turn sharply
	HAL_GPIO_WritePin(H_Bridge_IN4_GPIO_Port, H_Bridge_IN4_Pin, 0);
}

void go_right(double angle){
	HAL_GPIO_WritePin(H_Bridge_IN1_GPIO_Port, H_Bridge_IN1_Pin, 0); // do 1 to turn sharply
	HAL_GPIO_WritePin(H_Bridge_IN2_GPIO_Port, H_Bridge_IN2_Pin, 0);
	HAL_GPIO_WritePin(H_Bridge_IN3_GPIO_Port, H_Bridge_IN3_Pin, 0);
	HAL_GPIO_WritePin(H_Bridge_IN4_GPIO_Port, H_Bridge_IN4_Pin, 1);
}

void go_straight(){
	HAL_GPIO_WritePin(H_Bridge_IN1_GPIO_Port, H_Bridge_IN1_Pin, 1); 
	HAL_GPIO_WritePin(H_Bridge_IN2_GPIO_Port, H_Bridge_IN2_Pin, 0);
	HAL_GPIO_WritePin(H_Bridge_IN3_GPIO_Port, H_Bridge_IN3_Pin, 0);
	HAL_GPIO_WritePin(H_Bridge_IN4_GPIO_Port, H_Bridge_IN4_Pin, 1);
}

void go_backward(){
	HAL_GPIO_WritePin(H_Bridge_IN1_GPIO_Port, H_Bridge_IN1_Pin, 1); 
	HAL_GPIO_WritePin(H_Bridge_IN2_GPIO_Port, H_Bridge_IN2_Pin, 0);
	HAL_GPIO_WritePin(H_Bridge_IN3_GPIO_Port, H_Bridge_IN3_Pin, 1);
	HAL_GPIO_WritePin(H_Bridge_IN4_GPIO_Port, H_Bridge_IN4_Pin, 0);
}


void motor(double angle){
  if (angle < -10){
		go_left(0-angle);
	}
	else if (angle > 10){
		go_right(angle);
	}
	else{
		go_straight();
	}
}
