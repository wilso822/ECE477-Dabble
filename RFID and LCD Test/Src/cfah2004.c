/*
 * cfah2004.c
 *
 *  Created on: Feb 25, 2020
 *      Author: Alex Wilson
 */

#include "main.h"
#include "cfah2004.h"

#define CLR_RS HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);
#define SET_RS HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);
#define CLR_CS HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_RESET);
#define SET_CS HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);

uint8_t CGRAM[8][8] = {
  {21, 42, 21, 42, 21, 42, 21, 42},
  {42, 21, 42, 21, 42, 21, 42, 21},
  {63, 31, 15,  7,  3,  1,  0,  0},
  { 0,  0, 32, 48, 56, 60, 62, 63},
  { 1,  2,  4,  8,  8,  4,  2,  1},
  {32, 16,  8,  4,  4,  8, 16, 32},
  {62, 61, 59, 55, 47, 31, 47, 55},
  {59, 61, 62, 61, 59, 55, 47, 31},
};

void writeCommand(uint8_t command){
	//HAL_Delay(1);
	txbuff[0] = command;
	CLR_RS;
	CLR_CS;

	if(HAL_SPI_Transmit(&hspi1, txbuff, 1, 5000) != HAL_OK){
	  Error_Handler();
	}

	SET_CS;
}

void writeData(uint8_t data){
	//HAL_Delay(1);
	txbuff[0] = data;
	SET_RS;
	CLR_CS;

	if(HAL_SPI_Transmit(&hspi1, txbuff, 1, 5000) != HAL_OK){
	  Error_Handler();
	}

	SET_CS;
}

void Initialize_CFAH2004AP(void){
	writeCommand(0x38); // Function set
	writeCommand(0x0C); // Display ON/OFF
	writeCommand(0x01); // Clear display
	writeCommand(0x06); // Entry mode set
}

void Initialize_CGRAM(void){
	unsigned char i, j;
	// The first graph of the CGRAM Address start address
	//  is 000000 (0x00) CGRAM address is set to 0x40 + 0x00 = 0x40
	// The second graph's CGRAM Address starts with address 001000 (0x08)
	// etc.

	for (i = 0; i < 8; i ++){
		writeCommand(0x40 + (0x08 * i));
		for (j = 0; j < 8; j ++){
			writeData(CGRAM[i][j]);
		}
	}
}

void writeString(uint8_t count, uint8_t *MSG){
	  for(uint8_t i = 0; i<count;i++)
	  {
	    writeData(MSG[i]);
	  }
}

void dummy_write(){
	writeData(' ');
	writeData(' ');
	writeData(' ');
}

void hex_to_char_id(uint8_t* hex_id, char* char_id){
	int top , bot;
	for(int i = 0; i < 5; i++){
		top = (hex_id[i] & 0xF0) >> 4;
		bot = hex_id[i] & 0x0F;
		if(top > 9){
			char_id[i*2] = 'W' + top;
		}
		else{
			char_id[i*2] = '0' + top;
		}
		if(bot > 9){
			char_id[i*2+1] = 'W' + bot;
		}
		else{
			char_id[i*2+1] = '0' + bot;
		}
	}
}
