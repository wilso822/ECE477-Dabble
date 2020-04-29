/*
 * cfah2004.h
 *
 *  Created on: Feb 25, 2020
 *      Author: comic
 */

#ifndef CFAH2004_H_
#define CFAH2004_H_

#include "mfrc522.h"

#define Cword 0x14  //20
#define LINE_ONE    0x80  // DD RAM Address The starting position of the first line is 0x00
                          // set the DD RAM address to 0x80 + 0x00 = 0x80
#define LINE_TWO    0xc0  // DD RAM Address The starting position of the second line is 0x40
                          // set the DD RAM address to 0x80 + 0x40 = 0xc0
#define LINE_THREE  0x94  // DD RAM Address The starting position of the second line is 0x14
                          // set the DD RAM address to 0x80 + 0x14 = 0x94
#define LINE_FOUR   0xD4  // DD RAM Address The starting position of the second line is 0x54
                          // set the DD RAM address to 0x80 + 0x54 = 0xD4

void writeCommand(uint8_t command);
void writeData(uint8_t data);
void writeString(uint8_t count, uint8_t *MSG);
void Initialize_CFAH2004AP(void);
void Initialize_CGRAM(void);
void dummy_write(void);
void hex_to_char_id(uint8_t* hex_id, char* char_id);

#endif /* CFAH2004_H_ */
