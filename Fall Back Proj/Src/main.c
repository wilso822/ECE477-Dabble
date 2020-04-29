/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2020 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "mfrc522.h"
#include "cfah2004.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LED_GREEN	0x0
#define LED_RED 	0x1
#define LED_BLUE 	0x2
#define LED_ORANGE 	0x3
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define CLR_RS HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);
#define SET_RS HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);
#define CLR_CS HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_RESET);
#define SET_CS HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart5;

/* USER CODE BEGIN PV */
enum game_state{start, scan, place_row, place_col, confirm, board_check, end, state_error, display_msg};
enum button{up, down, enter};
enum board_status{init, valid, invalid, error};
enum tile_type{norm, dl, tl, dw, tw};
uint8_t CardID[5];
int count = 0;
uint8_t MSG1[20] = "                    ";
uint8_t player_scores[4] = {0};
uint8_t reset_board[83] = "---------------------------------------------------------------------------------\r\n";
uint8_t curr_board[83] = "000000000000000000000000000000000000000000000000000000000000000000000000000000000\r\n";
uint8_t prev_board[83] = "000000000000000000000000000000000000000000000000000000000000000000000000000000000\r\n";
char string_1[20];
char string_2[20];
char string_3[20];
char string_4[20];
uint8_t in;
uint8_t check = init;

struct letter_info{
	char letter;
	uint8_t row;
	uint8_t col;
};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM3_Init(void);
static void MX_UART5_Init(void);
/* USER CODE BEGIN PFP */
void LED_On(uint8_t);
void LED_Off(uint8_t);
void LED_Toggle(uint8_t);
uint8_t check_but(uint8_t);
char decode_ID(uint8_t ID[5]);
uint8_t get_score(char letter);
uint8_t get_tile(uint8_t row, uint8_t col);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void display_two_lines(char string_1[20], char string_2[20]){
	  writeCommand(0x1);
	  HAL_Delay(1);
	  writeCommand(LINE_TWO);
	  writeString(20, string_1);
	  writeCommand(LINE_THREE);
	  writeString(20, string_2);
}

void display_four_lines(char string_1[20], char string_2[20], char string_3[20], char string_4[20]){
	  writeCommand(0x1);
	  HAL_Delay(10);
	  writeString(20, string_1);
	  writeCommand(LINE_TWO);
	  writeString(20, string_2);
	  writeCommand(LINE_THREE);
	  writeString(20, string_3);
	  writeCommand(LINE_FOUR);
	  writeString(20, string_4);

}

uint8_t check_but(uint8_t option){
	if(option == up && HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_11) == 0){
		return 1;
	}
	if(option == down && HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_10) == 0){
		return 1;
	}
	if(option == enter && HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_12) == 0){
		return 1;
	}
	return 0;
}

int verify_placed_word(struct letter_info placed_word[9], int word_len){
	check = init;
	for(int i = 0; i < 83; i++){
		prev_board[i] = curr_board[i];
	}

	for(int i = 0; i < word_len; i++){
		curr_board[(placed_word[i].row - 1) * 9 + (placed_word[i].col - 1)] = placed_word[i].letter;
	}

	HAL_UART_Transmit(&huart5, curr_board, 83, 1000);
	HAL_UART_Receive_IT(&huart5, &in, 1);

	while(1){
		if(check != init){
			break;
		}
		sprintf(string_1, "   Verifying the    ");
		sprintf(string_2, "   current board    ");
		display_two_lines(string_1, string_2);
		HAL_Delay(100);
	}

	if(check == invalid || check == error){
		for(int i = 0; i < 83; i++){
			curr_board[i] = prev_board[i];
		}
	}
	HAL_UART_Receive_IT(&huart5, &in, 1);

	return check;
}

uint8_t update_scores(struct letter_info placed_word[9], int word_len){
	uint8_t num_formed = 0;
	int total_score = 0;
	uint8_t curr_tile;
	uint8_t flag = 0;
	int j = 0;

	for(int i = 0; i < word_len; i++) {
		int curr_score = 0;
		uint8_t sel_row_1 = placed_word[i].row - 1;
		uint8_t sel_row_2 = placed_word[i].row - 1;
		uint8_t sel_col_1 = placed_word[i].col - 1;
		uint8_t sel_col_2 = placed_word[i].col - 1;
		uint8_t num_dw = 0;
		uint8_t num_tw = 0;

		//check up
		while(sel_row_1 >= 0 && curr_board[sel_row_1 * 9 + sel_col_1] != '0'){
			for(j = (i - 1); j >= 0; j--){
				if((placed_word[j].row - 1) == sel_row_1 && (placed_word[j].col - 1) == sel_col_1){
					flag = 1;
					break;
				}
			}
			if(flag == 1){
				break;
			}
			if(sel_row_1 == 0){
				break;
			}
			sel_row_1--;
		}
		if(curr_board[sel_row_1 * 9 + sel_col_1] == '0'){
			sel_row_1++;
		}

		//check down
		while(sel_row_2 <= 8 && curr_board[sel_row_2 * 9 + sel_col_1] != '0'){
			for(j = (i-1); j >= 0; j--){
				if((placed_word[j].row - 1) == sel_row_2 && (placed_word[j].col - 1) == sel_col_1){
					flag = 1;
					break;
				}
			}
			if(flag == 1){
				break;
			}
			if(sel_row_2 == 8){
				break;
			}
			sel_row_2++;
		}
		if(curr_board[sel_row_2 * 9 + sel_col_1] == '0'){
			sel_row_2--;
		}
		//check horz score
		if(sel_row_1 == sel_row_2){
			flag = 1;
		}

		if(flag == 0){
			for(j = sel_row_1; j <= sel_row_2; j++){
				if(prev_board[j * 9 + sel_col_1] == '0'){
					curr_tile = get_tile(j, sel_col_1);
					if(curr_tile == dl){
						curr_score += 2 * get_score(curr_board[j * 9 + sel_col_1]);
					}
					else if(curr_tile == tl){
						curr_score += 3 * get_score(curr_board[j * 9 + sel_col_1]);
					}
					else if(curr_tile == dw){
						curr_score += get_score(curr_board[j * 9 + sel_col_1]);
						num_dw++;
					}
					else if(curr_tile == tw){
						curr_score += get_score(curr_board[j * 9 + sel_col_1]);
						num_tw++;
					}
					else{
						curr_score += get_score(curr_board[j * 9 + sel_col_1]);
					}
				}
				else{
					curr_score += get_score(curr_board[j * 9 + sel_col_1]);
				}
			}

			if(num_dw){
				curr_score = curr_score * (2 * num_dw);
			}
			if(num_tw){
				curr_score = curr_score * (3 * num_tw);
			}
			total_score += curr_score;
			num_formed++;
		}

		flag = 0;
		curr_score = 0;
		sel_row_1 = placed_word[i].row - 1;
		sel_row_2 = placed_word[i].row - 1;
		sel_col_1 = placed_word[i].col - 1;
		sel_col_2 = placed_word[i].col - 1;
		num_dw = 0;
		num_tw = 0;

		//check left
		while(sel_col_1 >= 0 && curr_board[sel_row_1 * 9 + sel_col_1] != '0'){
			for(j = (i-1); j >= 0; j--){
				if((placed_word[j].row - 1) == sel_row_1 && (placed_word[j].col - 1) == sel_col_1){
					flag = 1;
					break;
				}
			}
			if(flag == 1){
				break;
			}
			if(sel_col_1 == 0){
				break;
			}
			sel_col_1--;
		}
		if(curr_board[sel_row_1 * 9 + sel_col_1] == '0'){
			sel_col_1++;
		}

		//check right
		while(sel_col_2 <= 8 && curr_board[sel_row_2 * 9 + sel_col_2] != '0'){
			for(j = (i-1); j >= 0; j--){
				if((placed_word[j].row - 1) == sel_row_2 && (placed_word[j].col - 1) == sel_col_2){
					flag = 1;
					break;
				}
			}
			if(flag == 1){
				break;
			}
			if (sel_col_2 == 8){
				break;
			}
			sel_col_2++;
		}
		if(curr_board[sel_row_2 * 9 + sel_col_2] == '0'){
			sel_col_2--;
		}

		//check vert score
		if(sel_col_1 == sel_col_2){
			flag = 1;
		}

		if(flag == 0){
			for(j = sel_col_1; j <= sel_col_2; j++){
				if(prev_board[sel_row_1 * 9 + j] == '0'){
					curr_tile = get_tile(sel_row_1, j);
					if(curr_tile == dl){
						curr_score += 2 * get_score(curr_board[sel_row_1 * 9 + j]);
					}
					else if(curr_tile == tl){
						curr_score += 3 * get_score(curr_board[sel_row_1 * 9 + j]);
					}
					else if(curr_tile == dw){
						curr_score += get_score(curr_board[sel_row_1 * 9 + j]);
						num_dw++;
					}
					else if(curr_tile == tw){
						curr_score += get_score(curr_board[sel_row_1 * 9 + j]);
						num_tw++;
					}
					else{
						curr_score += get_score(curr_board[sel_row_1 * 9 + j]);
					}
				}
				else{
					curr_score += get_score(curr_board[sel_row_1 * 9 + j]);
				}
			}

			if(num_dw > 0){
				curr_score = curr_score * (2 * num_dw);
			}
			if(num_tw > 0){
				curr_score = curr_score * (3 * num_tw);
			}
			total_score += curr_score;

			num_formed++;
		}
	}

	if(word_len >= 7){
		total_score += 50;
	}
	return total_score;
}

uint8_t get_score(char letter){
	switch(letter){
		case 'A':
			return 1;
		case 'B':
			return 3;
		case 'C':
			return 3;
		case 'D':
			return 2;
		case 'E':
			return 1;
		case 'F':
			return 4;
		case 'G':
			return 2;
		case 'H':
			return 4;
		case 'I':
			return 1;
		case 'J':
			return 8;
		case 'K':
			return 5;
		case 'L':
			return 1;
		case 'M':
			return 3;
		case 'N':
			return 1;
		case 'O':
			return 1;
		case 'P':
			return 3;
		case 'Q':
			return 10;
		case 'R':
			return 1;
		case 'S':
			return 1;
		case 'T':
			return 1;
		case 'U':
			return 1;
		case 'V':
			return 4;
		case 'W':
			return 4;
		case 'X':
			return 8;
		case 'Y':
			return 4;
		case 'Z':
			return 10;
		default:
			return 0;
	}
}

uint8_t get_tile(uint8_t row, uint8_t col){
	if((row == 4 && col == 0) || (row == 0 && col == 4) || (row == 8 && col == 4) || (row == 4 && col == 8) || (row == 3 && col == 3) || (row == 3 && col == 5) || (row == 5 && col == 3) || (row == 5 && col == 5)){
		return dl;
	}
	if((row == 2 && col == 2) || (row == 6 && col == 2) || (row == 2 && col == 6) || (row == 6 && col == 6)){
		return tl;
	}
	if((row == 4 && col == 4) || (row == 1 && col == 1) || (row == 1 && col == 7) || (row == 7 && col == 1) || (row == 7 && col == 7)){
		return dw;
	}
	if((row == 0 && col == 0) || (row == 0 && col == 8) || (row == 8 && col == 0) || (row == 8 && col == 8)){
		return tw;
	}

	return norm;
}
char decode_ID(uint8_t ID[5]){
	int snippet = ID[2] << 16 | ID[3] << 8 | ID[4];
	switch(snippet){
		case 0x52954b:
		case 0x24953d:
		case 0x109509:
		case 0xd495cd:
		case 0xab95b2:
		case 0x34952d:
		case 0x16950f:
		case 0x84959d:
		case 0x389521:
			return 'A';
		case 0xef95f6:
		case 0xd595cc:
			return 'B';
		case 0x53954a:
		case 0x74956d:
			return 'C';
		case 0x221fb1:
		case 0x051f96:
		case 0x0f1f9c:
		case 0x041f97:
			return 'D';
		case 0x67967d:
		case 0x231fb0:
		case 0x7d9667:
		case 0x609579:
		case 0x119508:
		case 0x26953f:
		case 0x4d9554:
		case 0x02951b:
		case 0x191f8a:
		case 0x489551:
		case 0x5b9542:
			return 'E';
		case 0x67957e:
		case 0x7b9562:
			return 'F';
		case 0xd11f42:
		case 0xbe1f2d:
		case 0xb31f20:
			return 'G';
		case 0xbd1f2e:
		case 0xa01f33:
			return 'H';
		case 0x4e9557:
		case 0x6f9576:
		case 0x5c9545:
		case 0x25953c:
		case 0x3f9526:
		case 0x4c9555:
		case 0x3e9527:
		case 0x3a9523:
		case 0x03951a:
			return 'I';
		case 0x909589:
			return 'J';
		case 0x85969f:
			return 'K';
		case 0xfd95e4:
		case 0xe995f0:
		case 0xdb95c2:
		case 0xc695df:
			return 'L';
		case 0x399520:
		case 0x919588:
			return 'M';
		case 0xee95f7:
		case 0xc795de:
		case 0xe895f1:
		case 0xfc95e5:
		case 0xda95c3:
		case 0xac95b5:
			return 'N';
		case 0x709569:
		case 0x181f8b:
		case 0x2b9532:
		case 0x83959a:
		case 0x17950e:
		case 0x2a9533:
		case 0x619578:
		case 0x62957b:
			return 'O';
		case 0x7c9565:
		case 0x86969c:
			return 'P';
		case 0x8a9593:
			return 'Q';
		case 0x97958e:
		case 0x76956f:
		case 0x6d9677:
		case 0x7c9666:
		case 0x989581:
		case 0x6e9674:
			return 'R';
		case 0x0e1f9d:
		case 0xf01f63:
		case 0xf11f62:
		case 0xfb1f68:
			return 'S';
		case 0x9d9584:
		case 0xb295ab:
		case 0xb395aa:
		case 0xc195d8:
		case 0xc095d9:
		case 0x9e9587:
			return 'T';
		case 0xfa1f69:
		case 0xe71f74:
		case 0xdb1f48:
		case 0xdc1f4f:
			return 'U';
		case 0xc71f54:
		case 0xd21f41:
			return 'V';
		case 0x75956c:
		case 0x66957f:
			return 'W';
		case 0x599643:
			return 'X';
		case 0xc81f5b:
		case 0xe61f75:
			return 'Y';
		case 0x899590:
			return 'Z';
		default:
			return '\0';
	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	/* Transmit one byte with 100 ms timeout */
	if(in == '1'){
		check = valid;
	}
	else if(in == '0'){
		check = invalid;
	}
	else{
		check = error;
	}

	/* Receive one byte in interrupt mode */
}
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
  MX_SPI1_Init();
  MX_TIM3_Init();
  MX_UART5_Init();
  /* USER CODE BEGIN 2 */
  nss_inactive();
  CLR_RS;
  SET_CS;

  MFRC522_Init();
  Initialize_CFAH2004AP();
  Initialize_CGRAM();

  uint8_t curr_state = start;
  uint8_t num_players = 2;
  uint8_t curr_player = 1;
  uint8_t row = 1;
  uint8_t col = 1;
  char letter = '\0';
  char board_letter = '\0';
  struct letter_info placed_word[9] = {0};
  uint8_t word_len = 0;
  uint8_t check_flag;
  int score_gain = 0;

  HAL_Delay(1);
  HAL_UART_Transmit(&huart5, reset_board, 83, 1000);
  HAL_UART_Receive_IT(&huart5, &in, 1);

  while(1){
	if(check != init){
	  break;
	}
	sprintf(string_1, "  Setting up Wi-Fi  ");
	sprintf(string_2, "Connection, plz wait");
	display_two_lines(string_1, string_2);
	HAL_Delay(100);
  }

  if(check != valid){
	  curr_state = state_error;
  }
  HAL_UART_Receive_IT(&huart5, &in, 1);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	if(curr_state == start){
		if(check_but(up) && num_players < 4){
			num_players++;
		}
		else if(check_but(down) && num_players > 2){
			num_players--;
		}
		else if(check_but(enter)){
			curr_state = scan;
			HAL_Delay(200);
		}
		sprintf(string_1, "  select number of  ");
		sprintf(string_2, "  players: %d        ", num_players);
		display_two_lines(string_1, string_2);

		HAL_Delay(50);
	}
	else if(curr_state == scan){
		MFRC522_Status_t status = MFRC522_Check(CardID);
		while(status != MI_OK){
			sprintf(string_1, " P1: %03d    P2: %03d ", player_scores[0], player_scores[1]);
			sprintf(string_2, "  Player %d: please  ", curr_player);
			sprintf(string_3, "  scan a letter     ");
			sprintf(string_4, " P3: %03d    P4: %03d ", player_scores[2], player_scores[3]);
			display_four_lines(string_1, string_2, string_3, string_4);
			status = MFRC522_Check(CardID);
		}
		board_letter = decode_ID(CardID);
		curr_state = place_row;
		HAL_Delay(200);
		row = 1;
		col = 1;
	}
	else if(curr_state == place_row){
		if(check_but(up) && row < 9){
			row++;
		}
		else if(check_but(down) && row > 1){
			row--;
		}
		else if(check_but(enter)){
			curr_state = place_col;
			HAL_Delay(200);
		}
		sprintf(string_1, "Letter %c: select row", board_letter);
		sprintf(string_2, "row: %d, col:%d       ", row, col);
		display_two_lines(string_1, string_2);
		HAL_Delay(50);
	}
	else if(curr_state == place_col){
		if(check_but(up) && col < 9){
			col++;
		}
		else if(check_but(down) && col > 1){
			col--;
		}
		else if(check_but(enter)){
			letter = 'N';
			placed_word[word_len].row = row;
			placed_word[word_len].col = col;
			placed_word[word_len++].letter = board_letter;
			curr_state = confirm;
			HAL_Delay(200);
		}
		sprintf(string_1, "Letter %c: select col", board_letter);
		sprintf(string_2, "row: %d, col:%d       ", row, col);
		display_two_lines(string_1, string_2);
		HAL_Delay(50);
	}
	else if(curr_state == confirm){
		if(check_but(up)){
			letter = 'Y';
		}
		else if(check_but(down) && row > 1){
			letter = 'N';
		}
		else if(check_but(enter)){
			if(letter == 'Y'){
				curr_state = board_check;
				HAL_Delay(200);
			}
			else{
				curr_state = scan;
				HAL_Delay(200);
			}
		}
		sprintf(string_1, "  Player %d: are     ", curr_player);
		sprintf(string_2, "  you done Y/N?  %c   ", letter);
		display_two_lines(string_1, string_2);
		HAL_Delay(50);
	}

	else if(curr_state == board_check){
		sprintf(string_1, " Verifying %d tiles  ", word_len);
		sprintf(string_2, "   %c %c %c %c %c %c %c %c %c", placed_word[0].letter, placed_word[1].letter, placed_word[2].letter, placed_word[3].letter, placed_word[4].letter, placed_word[5].letter, placed_word[6].letter, placed_word[7].letter, placed_word[8].letter);
		sprintf(string_3, "r: %d %d %d %d %d %d %d %d %d", placed_word[0].row, placed_word[1].row, placed_word[2].row, placed_word[3].row, placed_word[4].row, placed_word[5].row, placed_word[6].row, placed_word[7].row, placed_word[8].row);
		sprintf(string_4, "c: %d %d %d %d %d %d %d %d %d", placed_word[0].col, placed_word[1].col, placed_word[2].col, placed_word[3].col, placed_word[4].col, placed_word[5].col, placed_word[6].col, placed_word[7].col, placed_word[8].col);
		display_four_lines(string_1, string_2, string_3, string_4);
		HAL_Delay(2000);
		check_flag = verify_placed_word(placed_word, word_len);
		curr_state = display_msg;
	}
	else if(curr_state == display_msg){
		if(check_flag == valid){
			score_gain = update_scores(placed_word, word_len);
			player_scores[curr_player-1] += score_gain;
			sprintf(string_1, " Player %d gains %03d ", curr_player, score_gain);
			sprintf(string_2, " for %03d total pts  ", player_scores[curr_player-1]);
			curr_player = (curr_player % num_players) + 1;
			curr_state = scan;

		}
		else if(check_flag == invalid){
			sprintf(string_1, " You fucked up kid  ");
			sprintf(string_2, " run that shit back ");
			curr_state = scan;
		}
		else{
			sprintf(string_1, "  Shit is on fire   ");
			sprintf(string_2, "  run while you can ");
			curr_state = state_error;
		}

		for(int i = 0; i < 9; i++){
			placed_word[i].letter = 0;
			placed_word[i].row = 0;
			placed_word[i].col = 0;
		}
		word_len = 0;
		display_two_lines(string_1, string_2);
		HAL_Delay(2000);
	}
	else if(curr_state == state_error){
		sprintf(string_1, "  There's been an   ");
		sprintf(string_2, "  error oh no :(    ");
		display_two_lines(string_1, string_2);
		HAL_Delay(50);
	}
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

  /**Configure the main internal regulator output voltage 
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /**Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 50;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /**Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV8;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV4;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 625-1;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 10-1;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief UART5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_UART5_Init(void)
{

  /* USER CODE BEGIN UART5_Init 0 */

  /* USER CODE END UART5_Init 0 */

  /* USER CODE BEGIN UART5_Init 1 */

  /* USER CODE END UART5_Init 1 */
  huart5.Instance = UART5;
  huart5.Init.BaudRate = 115200;
  huart5.Init.WordLength = UART_WORDLENGTH_8B;
  huart5.Init.StopBits = UART_STOPBITS_1;
  huart5.Init.Parity = UART_PARITY_NONE;
  huart5.Init.Mode = UART_MODE_TX_RX;
  huart5.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart5.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart5) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART5_Init 2 */

  /* USER CODE END UART5_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0|GPIO_PIN_8|GPIO_PIN_9, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET);

  /*Configure GPIO pin : PE3 */
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : PC0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PC3 */
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB2 */
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PE10 PE11 PE12 */
  GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : PB10 */
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PC7 PC10 */
  GPIO_InitStruct.Pin = GPIO_PIN_7|GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PC8 PC9 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA9 */
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PA10 PA11 PA12 */
  GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG_FS;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PD4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : PD5 */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : PE1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */
void LED_On(uint8_t led){
	switch(led){
		case LED_GREEN: 	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);
					break;
		case LED_RED:		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);
					break;
		case LED_BLUE:		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_SET);
					break;
		case LED_ORANGE:	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET);
					break;
	}
}

void LED_Off(uint8_t led){
	switch(led){
		case LED_GREEN:		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);
					break;
		case LED_RED:		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);
					break;
		case LED_BLUE:		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_RESET);
					break;
		case LED_ORANGE:	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_RESET);
					break;
	}
}

void LED_Toggle(uint8_t led){
	switch(led){
		case LED_GREEN:		HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_12);
					break;
		case LED_RED:		HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_14);
					break;
		case LED_BLUE:		HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_15);
					break;
		case LED_ORANGE:	HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_13);
					break;
	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
	count++;
	if(count > 1000000){
		count = 0;
	}
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
	LED_Off(LED_GREEN);
	LED_Off(LED_BLUE);
	LED_Off(LED_ORANGE);
	LED_On(LED_RED);
//  BSP_LED_On(LED5);
  while(1)
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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
