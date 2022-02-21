/*
 * gameCtrl.c
 *
 *  Created on: 09 Apr 2021
 *      Author: jherr
 */

#include "stdint.h"
#include "stm32f1xx_hal.h"
#include "main.h"


volatile GPIO_PinState dotmatrix[8][8]={0};
volatile uint8_t rowToDisplay = 0;
volatile int ball_Xpos = 0;
volatile int ball_Ypos = 0;

volatile uint8_t reset_complete = 1;
volatile uint8_t gamestate_has_changed = 0;

//tennis variables
volatile int bat_Xpos = 0;
volatile int bat_Ypos = 4;
volatile int ball_Xvel = -1;
volatile int ball_Yvel = 0;

uint8_t tennisball_movement = 0;

uint8_t goal_Xpos = 7;
uint8_t goal_Ypos = 7;
uint8_t place_holder = 0;
uint8_t move_made = 0;
uint8_t ADC_previous_move = 0;

char i2c_direction = 'N';

uint8_t move_made_up = 0;
uint8_t move_made_down = 0;
uint8_t move_made_left = 0;
uint8_t move_made_right = 0;
uint8_t move_made_middle = 0;

volatile uint16_t tennisball_movement_timer = 0;
extern volatile uint16_t tennisball_movement_reload_time;
uint8_t bat_hits = 0;
uint8_t num_of_hits = 1;
uint8_t tennisball_velocity = 1;

enum gameStates {state_menu, state_Calibration, state_Game_Maze,state_Game_Maze_Select,state_Game_Tennis};
enum gameStates gameState = state_menu;

extern uint8_t updatetimer;
extern uint16_t canMove;
extern uint16_t canMoveADC;
extern uint16_t canTransmitUART;
extern uint16_t ball_canDisplaytimer;
extern uint16_t goal_canDisplaytimer;


extern UART_HandleTypeDef huart2;
extern ADC_HandleTypeDef hadc2;
extern I2C_HandleTypeDef hi2c1;


//Button Flags
volatile uint16_t start_Time_Up = 0;
volatile uint16_t start_Time_Down = 0;
volatile uint16_t start_Time_Right = 0;
volatile uint16_t start_Time_Left = 0;
volatile uint16_t start_Time_Middle = 0;

uint8_t mazechoice = 1;
uint8_t i2cdata[6];
int16_t ax;
int16_t az;
int16_t ay;

int movement_changeX = 0;
int movement_changeY = 0;
uint8_t movement_changeM = 0;

uint8_t YposNew = 4;

#define	POS_MIN  0
#define POS_MAX  7
uint16_t ballDisplayReloadTime =  300;
uint16_t goalDisplayReloadTime =  100;

//Function Prototypes
void loadscrn();
void game_reset();
void calibration();
void checkMovementInputs_Maze();
void uart_Position_Transmission_Maze();
void updateScrn();
void updateColumns(uint8_t rowToDisplay);
void toggleBallDisplay();
void toggleGoalDisplay();
void maze_Select(int maze_choice);
void maze_number_Display(int maze_choice);
void uart_Position_Transmission_Tennis();


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{

  switch(GPIO_Pin){
  	  case GPIO_PIN_0:
  		  start_Time_Down = HAL_GetTick();
  		  break;
  	  case GPIO_PIN_1:
  		  start_Time_Middle = HAL_GetTick();
  		  break;
  	  case GPIO_PIN_2:
  		  start_Time_Right = HAL_GetTick();
  		  break;
  	  case GPIO_PIN_3:
  		  start_Time_Up = HAL_GetTick();
  		  break;
  	  case GPIO_PIN_4:
  		  start_Time_Left = HAL_GetTick();
  		  break;
  	  default:
  		  break;
  }
}

void gameLoop(){


	calibration();
	loadscrn();


	while(1){

			//Update Cycle
			if((updatetimer) && (reset_complete) && !gamestate_has_changed){
				updateScrn();
				updatetimer = 0;

			}

			//Transmitting UART of Maze Game
			if((canTransmitUART >= 300) && (gameState == state_Game_Maze)  & reset_complete){
				uart_Position_Transmission_Maze();
			}


			//I2C Movement Input & Processing
			if(canMove>=100 && (gameState == state_Game_Maze || gameState == state_Game_Tennis)){

				i2c_direction = 'N';
				i2cdata[0] = 0xA8; // 0x28 + 0x80  (auto increment) starting register X_low
				HAL_I2C_Master_Transmit(&hi2c1, 0x30, i2cdata, 1, 10);
				HAL_I2C_Master_Receive(&hi2c1, 0x30, i2cdata, 6, 10);
				ax = *(int16_t*)(i2cdata);
				ay = *(int16_t*)(i2cdata+2);
				az = *(int16_t*)(i2cdata+4);

				  		if(ax>= (8192) && az<=(14188.96)){
				  			i2c_direction = 'L';


							if((ball_Xpos-1 >= POS_MIN) && (gameState ==state_Game_Maze) && canMove>=300){
								movement_changeX=movement_changeX-1;

							}
							else if(gameState == state_Game_Tennis){
								if(bat_Xpos-1>=POS_MIN){
									if(!(bat_Ypos==ball_Ypos && ball_Xpos==bat_Xpos-1) && !(bat_Ypos+1==ball_Ypos && ball_Xpos==bat_Xpos-1)){
										movement_changeX-=1;
									}
								}
							}
				  		}


				  		if(ax<= (-8912) && az<=(14188.96)){
				  			i2c_direction = 'R';

				  			if(ball_Xpos+1 <= POS_MAX && gameState == state_Game_Maze && canMove>=300){
				  				movement_changeX=movement_changeX+1;
				  			}
				  			else if(gameState == state_Game_Tennis){
				  				if(bat_Xpos+1<=POS_MAX){
				  					if(!(bat_Ypos==ball_Ypos && ball_Xpos==bat_Xpos+1) &&!(bat_Ypos+1==ball_Ypos && ball_Xpos==bat_Xpos+1) ){
				  						movement_changeX+=1;
				  					}
				  				}
				  			}
				  		}


				  		if(ay>= (8192) && az<=(14188.96)){
				  			i2c_direction = 'D';

				  			if(ball_Ypos+1 <= POS_MAX && gameState == state_Game_Maze && canMove>=300){
				  				movement_changeY=movement_changeY+1;
				  			}
				  			else if(gameState == state_Game_Tennis){
				  				if(bat_Ypos+1<=POS_MAX-1){
				  					if(!(bat_Ypos+1==ball_Ypos && ball_Xpos==bat_Xpos)){
				  						movement_changeY+=1;
				  					}
				  				}
				  			}
				  		}


				  		if(ay <= (-8192) && az<=(14188.96)){
				  		 	i2c_direction = 'U';

				  		 	if(ball_Ypos-1 >= POS_MIN && gameState == state_Game_Maze && canMove>=300){
				  		 		movement_changeY-=1;
				  		 	}
				  		 	else if(gameState == state_Game_Tennis){
				  		 		if(bat_Ypos-1>=POS_MIN){
				  		 			if(!(bat_Ypos-1==ball_Ypos && ball_Xpos==bat_Xpos)){
				  		 				movement_changeY-=1;
				  		 			}
				  		 		}
				  		 	}
				  		}
			}



			//Checking Valid Button Presses

			if((start_Time_Up!=0) && (gameState != state_menu)){
				if((HAL_GetTick()-start_Time_Up>=30) && (!HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_3)) && (!move_made_up)){
					if(ball_Ypos-1 >= POS_MIN && gameState == state_Game_Maze){
						movement_changeY-=1;

						move_made_up = 1;
					}
					else if(gameState == state_Game_Maze_Select){
						movement_changeY=movement_changeY-1;
						move_made_up = 1;
					}
					else if(gameState == state_Game_Tennis){
						if(bat_Ypos-1>=POS_MIN){
							if(!(bat_Ypos-1==ball_Ypos && ball_Xpos==bat_Xpos)){
								movement_changeY-=1;
								move_made_up = 1;
							}
						}
					}
					start_Time_Up=0;
				}
			}

			if((start_Time_Down!=0) && (gameState != state_menu)){
				if((HAL_GetTick()-start_Time_Down>=30) && (!HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0))&& (!move_made_down)){
					if(ball_Ypos+1 <= POS_MAX && gameState == state_Game_Maze){
						movement_changeY=movement_changeY+1;

						move_made_down = 1;
					}
					else if(gameState == state_Game_Maze_Select){
						movement_changeY=movement_changeY+1;
						move_made_down = 1;
					}
					else if(gameState == state_Game_Tennis){
						if(bat_Ypos+1<=POS_MAX-1){
							if(!(bat_Ypos+1==ball_Ypos && ball_Xpos==bat_Xpos)){
								movement_changeY+=1;
								move_made_down = 1;
							}
						}
					}
					start_Time_Down=0;
				}
			}

			if((start_Time_Right!=0) && (gameState == state_Game_Maze || gameState == state_Game_Tennis)){
				if((HAL_GetTick()-start_Time_Right>=30)  && (!HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_2)) && (!move_made_right)){
					if(ball_Xpos+1 <= POS_MAX && gameState == state_Game_Maze){
						movement_changeX=movement_changeX+1;

						move_made_right = 1;
					}
					else if(gameState == state_Game_Tennis){
						if(bat_Xpos+1<=POS_MAX){
							if(!(bat_Ypos==ball_Ypos && ball_Xpos==bat_Xpos+1) &&!(bat_Ypos+1==ball_Ypos && ball_Xpos==bat_Xpos+1) ){
								movement_changeX+=1;
								move_made_up = 1;
							}
						}
					}
					start_Time_Right=0;
				}
			}

			if(start_Time_Left!=0){
				if((HAL_GetTick()-start_Time_Left>=30) && (!HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4))){

					if((ball_Xpos-1 >= POS_MIN) && (gameState ==state_Game_Maze) && (!move_made_left)){
						movement_changeX=movement_changeX-1;
						move_made_left = 1;

					}
					else if(gameState==state_menu){
						movement_changeX=-1;
						move_made_left = 1;
					}
					else if(gameState == state_Game_Tennis){
						if(bat_Xpos-1>=POS_MIN){
							if(!(bat_Ypos==ball_Ypos && ball_Xpos==bat_Xpos-1) && !(bat_Ypos+1==ball_Ypos && ball_Xpos==bat_Xpos-1)){
								movement_changeX-=1;
								move_made_up = 1;
							}
						}
					}
					start_Time_Left=0;
				}
			}

			if(start_Time_Middle!=0){
				if((HAL_GetTick()-start_Time_Middle>=30)  && (!HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_1)) && !move_made_middle){
						movement_changeM = 1;


						move_made_middle = 1;
						start_Time_Middle = 0;
				}
			}

			//Ensuring button cannot cause double movement
			if(movement_changeX !=0){
				if(movement_changeX > 1){
					movement_changeX = 1;
				}
				else if(movement_changeX <-1){
					movement_changeX = -1;
				}
			}
			if(movement_changeY !=0){
				if(movement_changeY > 1){
					movement_changeY = 1;
				}
				else if(movement_changeY < -1){
					movement_changeY = -1;
				}
			}


			//Movement Processing
			if(canMove>=100){

				//Maze Button Movement Processing
				if((gameState == state_Game_Maze) && (canMove>=300)){
					if(((ball_Ypos+movement_changeY) >=POS_MIN) && ((ball_Ypos+movement_changeY) <=POS_MAX )){
						if((!dotmatrix[ball_Ypos+movement_changeY][ball_Xpos] || (ball_Ypos+movement_changeY == goal_Ypos  && ball_Xpos == goal_Xpos ))){
							place_holder = dotmatrix[ball_Ypos][ball_Xpos];
							dotmatrix[ball_Ypos][ball_Xpos] = 0;
							ball_Ypos+=movement_changeY;
							dotmatrix[ball_Ypos][ball_Xpos]=place_holder;
							move_made++;

							move_made_up = 0;
							move_made_down = 0;
						}
					}
					if ((ball_Xpos + movement_changeX >= POS_MIN) && (ball_Xpos+movement_changeX<=POS_MAX)){
						if((!dotmatrix[ball_Ypos][ball_Xpos+movement_changeX])){
							place_holder = dotmatrix[ball_Ypos][ball_Xpos];
							dotmatrix[ball_Ypos][ball_Xpos] = 0;
							ball_Xpos += movement_changeX;
							dotmatrix[ball_Ypos][ball_Xpos] = place_holder;
							move_made++;
							move_made_left = 0;
							move_made_right = 0;
						}
					}

					movement_changeY = 0;
					movement_changeX = 0;
				}

				//Checking Maze Game is Complete
				if(gameState == state_Game_Maze && (canMove>=300)){
					if ((ball_Xpos == goal_Xpos) && (ball_Ypos == goal_Ypos)){
						reset_complete = 0;
						canMove = 0;
						gamestate_has_changed = 1;
						gameState = state_menu;
					}
				}


				//Displaying Numbers
				if(gameState == state_Game_Maze_Select && movement_changeY != 0){

					if((mazechoice+(-1*movement_changeY) >= 1) && (mazechoice+(-1*movement_changeY) <= 4) ){
						mazechoice+=(-1*movement_changeY);
						//movement_changeY = 0;
						move_made++;
						move_made_up = 0;
						move_made_down = 0;
						maze_number_Display(mazechoice);
						HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, 0);
						HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, 0);
						HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, 0);
						HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, 0);
						switch(mazechoice){
						case 1:
							HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, 1);
							break;
						case 2:
							HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, 1);
							break;
						case 3:
							HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, 1);
							break;
						case 4:
							HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, 1);
							break;
						}
					}
					movement_changeY= 0;
				}

				//Tennis Bat Button Processing
				if(gameState==state_Game_Tennis && (movement_changeX!=0 || movement_changeY!=0)){
						move_made_left = 0;
						move_made_right = 0;
						move_made_up = 0;
						move_made_down = 0;
					dotmatrix[bat_Ypos][bat_Xpos] = 0;
					dotmatrix[bat_Ypos+1][bat_Xpos] = 0;
					bat_Ypos += movement_changeY;
					bat_Xpos += movement_changeX;
					dotmatrix[bat_Ypos][bat_Xpos] = 1;
					dotmatrix[bat_Ypos+1][bat_Xpos] = 1;
					movement_changeX = 0;
					movement_changeY = 0;
					move_made++;
				}



				//Checking Left Button to enter maze select screen
				if((gameState == state_menu) && (movement_changeX != 0)){
					gameState = state_Game_Maze_Select;
					maze_number_Display(mazechoice);
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, 1);
					//gamestate_has_changed = 1;
					movement_changeX = 0;
					//reset_complete = 0;
					move_made++;
				}

				//Function to enter a Maze
				else if((gameState == state_Game_Maze_Select) && (movement_changeM != 0)){
					gameState = state_Game_Maze;
					gamestate_has_changed = 1;
					movement_changeM = 0;
					reset_complete = 0;
					move_made_middle = 0;
					move_made++;
				}

				if(gamestate_has_changed){
					game_reset();
					gamestate_has_changed = 0;
				}

				if((movement_changeM && (move_made_middle))){

					//Entering Tennis Game
					if(gameState == state_menu){
						gameState = state_Game_Tennis;
						move_made++;
						move_made_middle = 0;
						reset_complete = 0;
						gamestate_has_changed = 1;
						movement_changeM = 0;
					}

					//Exitting Maze, Maze Select & Tennis
					else if (gameState == state_Game_Maze || gameState == state_Game_Tennis|| gameState == state_Game_Maze_Select){
						gameState = state_menu;
						move_made++;
						move_made_middle = 0;
						reset_complete = 0;
						gamestate_has_changed = 1;
						movement_changeM = 0;
						mazechoice = 1;
					}

					//Resetting Game
					if(gamestate_has_changed){
						game_reset();
						gamestate_has_changed = 0;
					}
				}
				if(move_made){
				canMove = 0;
				move_made=0;
				}

			}




			//Tennis Specific Functions//

			if((tennisball_movement_timer >= tennisball_movement_reload_time) && (gameState == state_Game_Tennis)){

				//Wall Collision Check
				if(ball_Xpos == POS_MAX){
					switch(tennisball_movement){
					case 1:
						tennisball_movement = 0;
						break;
					case 2:
						tennisball_movement = 5;
						break;
					case 4:
						tennisball_movement = 3;
						break;
					}
				}

				if(ball_Ypos == POS_MIN){
					switch(tennisball_movement){
						case 3:
							tennisball_movement = 5;
							break;
						case 4:
							tennisball_movement = 2;
							break;
					}
				}

				else if (ball_Ypos >= POS_MAX){
					switch(tennisball_movement){
						case 2:
							tennisball_movement = 4;
							break;
						case 5:
							tennisball_movement = 3;
							break;
					}
				}

				//Bat Collision Check

				if((!tennisball_movement) && ((ball_Ypos==bat_Ypos)||(ball_Ypos==bat_Ypos+1)) && (ball_Xpos==bat_Xpos+1)){
					if(bat_Ypos==ball_Ypos){
						tennisball_movement = 4;
						bat_hits++;
					}
					else{
						tennisball_movement = 2;
						bat_hits++;
					}
				}

				if((ball_Ypos==bat_Ypos-1)  && (ball_Xpos-1==bat_Xpos) && (tennisball_movement)){
					if(tennisball_movement==5){
						if(bat_Ypos==1){
							tennisball_movement = 2;
						}
						else{
							tennisball_movement = 4;
						}
						bat_hits++;
					}

				}

				if((ball_Ypos==bat_Ypos)  && (ball_Xpos-1==bat_Xpos) && (tennisball_movement)){
					switch(tennisball_movement){
						case 3:
							tennisball_movement = 4;
							bat_hits++;
							break;
						case 5:
							tennisball_movement = 2;
							bat_hits++;
							break;
					}
				}

				if((ball_Ypos==bat_Ypos+1)  && (ball_Xpos-1==bat_Xpos) && (tennisball_movement)){
					switch(tennisball_movement){
						case 3:
							tennisball_movement = 4;
							bat_hits++;
							break;
						case 5:
							tennisball_movement = 2;
							bat_hits++;
							break;
					}
				}


				if((ball_Ypos==bat_Ypos+2)  && (ball_Xpos-1==bat_Xpos) && (tennisball_movement)){
					if(tennisball_movement==3){
						if(bat_Ypos==6){
							tennisball_movement = 4;
						}
						else{
							tennisball_movement = 2;
						}
						bat_hits++;
					}
				}

				//Movement Update
				dotmatrix[ball_Ypos][ball_Xpos] = 0;

				switch(tennisball_movement){
					case 0:
						ball_Xpos -= 1;
						break;
					case 1:
						ball_Xpos += 1;
						break;
					case 2:
						ball_Xpos += 1;
						ball_Ypos += 1;
						break;
					case 3:
						ball_Xpos -= 1;
						ball_Ypos -= 1;
						break;
					case 4:
						ball_Xpos += 1;
						ball_Ypos -= 1;
						break;
					case 5:
						ball_Xpos -= 1;
						ball_Ypos += 1;
						break;

				}

				//Check if Tennis Game is done

				if(ball_Xpos<0){
					gameState = state_menu;
					gamestate_has_changed = 1;
					reset_complete = 0;
					ball_Ypos = 4;
					ball_Xpos = 4;
					dotmatrix[bat_Ypos][bat_Xpos]=0;
					dotmatrix[bat_Ypos+1][bat_Xpos]=0;
				}
				dotmatrix[ball_Ypos][ball_Xpos] = 1;



				// Checking if Speed needs to be increased

				if(((int8_t)(bat_hits/3) == tennisball_velocity) && (tennisball_movement_reload_time>=250)){
					tennisball_movement_reload_time -= 50;
					tennisball_velocity++;
				}

				tennisball_movement_timer=0;
			}

			//Tennis Specific Functions

			//Transmitting UART of Tennis Game
			if((canTransmitUART >= 100) && (gameState == state_Game_Tennis) && reset_complete){
				uart_Position_Transmission_Tennis();
			}

			//ADC Inputs
			if((canMoveADC >= 100) && (gameState == state_Game_Tennis)){
				HAL_ADC_Start(&hadc2);
				HAL_ADC_PollForConversion(&hadc2, 10);

				YposNew = (6-6*((float)HAL_ADC_GetValue(&hadc2)/0xFFF));
				HAL_ADC_Stop(&hadc2);

				//Ensuring only new ADC movement is processed
				if((YposNew != bat_Ypos) && (YposNew <= (POS_MAX)) && (YposNew >= POS_MIN) && ADC_previous_move!=YposNew){
					dotmatrix[bat_Ypos][bat_Xpos] = 0;
					dotmatrix[bat_Ypos+1][bat_Xpos] = 0;
					bat_Ypos = YposNew;
					canMoveADC = 0;
					dotmatrix[bat_Ypos][bat_Xpos] = 1;
					dotmatrix[bat_Ypos+1][bat_Xpos] = 1;
					ADC_previous_move = YposNew;
				}
			}

			//Resetting Game
			if(gamestate_has_changed){
				game_reset();
				gamestate_has_changed = 0;
			}
	}
}


void uart_Position_Transmission_Maze(){
	canTransmitUART = 0;
	char currentpos[10]={ 36, 51, ball_Xpos+48, ball_Ypos+48 , dotmatrix[ball_Ypos][ball_Xpos] + 48, dotmatrix[goal_Ypos][goal_Xpos] + 48, i2c_direction, 95, 95,10};
	HAL_UART_Transmit(&huart2, (uint8_t*)currentpos, 10, 10);
}
void uart_Position_Transmission_Tennis(){
	canTransmitUART = 0;
	char currentpos[10]={ 36, 50, ball_Xpos+48,ball_Ypos+48 , tennisball_velocity + 48, tennisball_movement + 48, bat_Xpos + 48, bat_Ypos + 48, i2c_direction, 10};
	HAL_UART_Transmit(&huart2, (uint8_t*)currentpos, 10, 10);
}

void updateColumns(uint8_t rowToDisplay){
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, dotmatrix[rowToDisplay][0]);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, dotmatrix[rowToDisplay][1]);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, dotmatrix[rowToDisplay][2]);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, dotmatrix[rowToDisplay][3]);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, dotmatrix[rowToDisplay][4]);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, dotmatrix[rowToDisplay][5]);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, dotmatrix[rowToDisplay][6]);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, dotmatrix[rowToDisplay][7]);
}
void toggleBallDisplay(){
		if(dotmatrix[ball_Ypos][ball_Xpos]==0){
			dotmatrix[ball_Ypos][ball_Xpos] = 1;
		}
		else{
			dotmatrix[ball_Ypos][ball_Xpos] = 0;
		}
		ball_canDisplaytimer = 0;
}
void toggleGoalDisplay(){
		if(dotmatrix[goal_Ypos][goal_Xpos]==0){
			dotmatrix[goal_Ypos][goal_Xpos] = 1;
		}
		else{
			dotmatrix[goal_Ypos][goal_Xpos] = 0;
		}
		goal_canDisplaytimer = 0;
}
void updateScrn(){
	if( (gameState == state_Game_Maze) && (ball_canDisplaytimer >= ballDisplayReloadTime)){
		toggleBallDisplay();
	}
	if( (gameState == state_Game_Maze) && (goal_canDisplaytimer >= goalDisplayReloadTime)){
		toggleGoalDisplay();
	}
	switch(rowToDisplay){
	case 0:
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);

		updateColumns(rowToDisplay);

		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);
		rowToDisplay ++;
		break;

	case 1:
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);

		updateColumns(rowToDisplay);

		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
		rowToDisplay ++;
		break;

	case 2:
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);

		updateColumns(rowToDisplay);

		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
		rowToDisplay ++;
		break;

	case 3:
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);

		updateColumns(rowToDisplay);

		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);

		rowToDisplay ++;
		break;

	case 4:
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);

		updateColumns(rowToDisplay);


		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);
		rowToDisplay ++;
		break;

	case 5:

		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET);

		updateColumns(rowToDisplay);

		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
		rowToDisplay ++;
		break;

	case 6:
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);

		updateColumns(rowToDisplay);

		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
		rowToDisplay ++;
		break;

	case 7:
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);

		updateColumns(rowToDisplay);

		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
		rowToDisplay = 0;
		break;

	}

}

void calibration(){
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);

	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);

	char snum[10]={36, 50, 50, 55, 56, 50, 53, 48, 56, 10};
	char col[10]={ 36, 49, 48, 95, 95, 95, 95, 95, 95,10};

	//Column 0
	HAL_UART_Transmit(&huart2, (uint8_t*)snum, 10, 10);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);
	HAL_UART_Transmit(&huart2, (uint8_t*)(col), 10, 10);
	HAL_Delay(1000);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);
	col[2]+=1;

	//Column 1
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_SET);
	HAL_UART_Transmit(&huart2, (uint8_t*)(col), 10, 10);
	HAL_Delay(1000);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET);
	col[2]+=1;

	//Column 2
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET);
	HAL_UART_Transmit(&huart2, (uint8_t*)(col), 10, 10);
	HAL_Delay(1000);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);
	col[2]+=1;

	//Column 3
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_SET);
	HAL_UART_Transmit(&huart2, (uint8_t*)(col), 10, 10);
	HAL_Delay(1000);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);
	col[2]+=1;


	//Column 4
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_SET);
	HAL_UART_Transmit(&huart2, (uint8_t*)(col), 10, 10);
	HAL_Delay(1000);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);
	col[2]+=1;

	//Column 5
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
	HAL_UART_Transmit(&huart2, (uint8_t*)(col), 10, 10);
	HAL_Delay(1000);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
	col[2]+=1;

	//Column 6
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_SET);
	HAL_UART_Transmit(&huart2, (uint8_t*)(col), 10, 10);
	HAL_Delay(1000);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);
	col[2]+=1;

	//Column 7
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);
	HAL_UART_Transmit(&huart2, (uint8_t*)(col), 10, 10);
	HAL_Delay(1000);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);

	//switch off
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);

	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);

	loadscrn();
}

void game_reset(){
	movement_changeM = 0;
	movement_changeX = 0;
	movement_changeY = 0;

	if(gameState != state_Game_Maze){
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, 0);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, 0);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, 0);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, 0);
	}
	if (gameState == state_Game_Maze){
		ball_Xpos = 0;
		ball_Ypos = 0;
		rowToDisplay = 0;
		maze_Select(mazechoice);
		dotmatrix[ball_Ypos][ball_Xpos] = 1;
		canTransmitUART = 0;
		mazechoice = 1;
	}

	else if (gameState == state_Game_Tennis){
		ball_Xpos = 7;
		ball_Ypos = 4;
		tennisball_movement_reload_time = 700;
		tennisball_movement_timer = 0;
		tennisball_velocity = 1;
		rowToDisplay = 0;
		canTransmitUART = 0;
		for(int i = 0; i<=POS_MAX;i++){
			for(int j = 0; j<=POS_MAX;j++){
				dotmatrix[i][j]=0;
			}
		}
		dotmatrix[bat_Ypos][bat_Xpos] = 1;
		dotmatrix[bat_Ypos+1][bat_Xpos] = 1;
		dotmatrix[ball_Ypos][ball_Xpos] = 1;
		tennisball_movement = 0;
	}

	else if(gameState == state_menu){
		for(int i = 0; i<=POS_MAX;i++){
			for(int j = 0; j<=POS_MAX;j++){
				dotmatrix[i][j]=0;
			}
		}
		loadscrn();

	}
	reset_complete = 1;
}

void setMatrix(int maze_layout[]){
	uint16_t i = 0;
	for(int n = 0;  n<8; n++){
		for(int m = 0;  m<8; m++){
			dotmatrix[n][m]= maze_layout[i];
			i++;
		}
	}
}
void maze_number_Display(int maze_choice){
	if(maze_choice == 1){
			 int maze_layout[] = {
					 0,0,0,0,0,0,0,0,
					 0,0,0,1,0,0,0,0,
					 0,0,0,1,0,0,0,0,
					 0,0,0,1,0,0,0,0,
					 0,0,0,1,0,0,0,0,
					 0,0,0,1,0,0,0,0,
					 0,0,0,1,0,0,0,0,
					 0,0,0,0,0,0,0,0
			 };
			setMatrix(maze_layout);
		}

		else if(maze_choice==2){
			int maze_layout[] = {
					 0,0,0,0,0,0,0,0,
					 0,0,0,1,1,0,0,0,
					 0,0,1,0,0,1,0,0,
					 0,0,0,0,0,1,0,0,
					 0,0,0,0,1,0,0,0,
					 0,0,0,1,0,0,0,0,
					 0,0,1,1,1,1,0,0,
					 0,0,0,0,0,0,0,0
			};
			setMatrix(maze_layout);
		}

		else if(maze_choice==3){
			int maze_layout[] = {
					 0,0,0,0,0,0,0,0,
					 0,0,1,1,0,0,0,0,
					 0,0,0,0,1,0,0,0,
					 0,0,1,1,0,0,0,0,
					 0,0,0,0,1,0,0,0,
					 0,0,0,0,1,0,0,0,
					 0,0,1,1,0,0,0,0,
					 0,0,0,0,0,0,0,0
			};
			setMatrix(maze_layout);
		}
		else if (maze_choice==4){
			int maze_layout[] = {
					 0,0,0,0,0,0,0,0,
					 0,0,0,0,0,0,1,0,
					 0,0,0,0,0,1,0,0,
					 0,0,0,0,1,0,0,0,
					 0,0,0,1,0,1,0,0,
					 0,0,1,1,1,1,1,0,
					 0,0,0,0,0,1,0,0,
					 0,0,0,0,0,0,0,0
			};
			setMatrix(maze_layout);
		}
}

void maze_Select(int maze_choice){
	if(maze_choice == 1){
		 int maze_layout[] = {
				 1,0,0,0,0,0,1,0,
				 1,1,0,1,1,0,0,0,
				 0,0,0,0,0,0,1,0,
				 0,1,1,1,1,1,0,1,
				 0,0,0,0,1,0,0,0,
				 1,1,0,1,1,0,1,0,
				 0,0,0,1,0,0,1,0,
				 0,1,0,0,0,1,1,1
		 };
		setMatrix(maze_layout);
		goal_Xpos = 7;
		goal_Ypos = 7;
	}

	else if(maze_choice==2){
		int maze_layout[] = {
				1,0,0,0,0,0,0,0,
				0,1,0,1,0,1,1,0,
				0,0,1,0,1,0,0,0,
				0,1,0,0,0,0,1,1,
				0,0,1,1,1,1,0,0,
				1,0,1,0,0,1,1,0,
				0,0,1,0,0,0,1,0,
				0,0,0,0,1,0,0,0
		};
		goal_Xpos = 3;
		goal_Ypos = 3;
		setMatrix(maze_layout);
	}

	else if(maze_choice==3){
		int maze_layout[] = {
				1,0,0,1,1,0,0,0,
				0,1,0,0,1,0,1,0,
				0,0,1,0,1,0,1,0,
				1,0,1,0,0,0,1,0,
				0,0,1,0,1,1,1,0,
				0,1,0,0,1,0,0,0,
				0,1,0,1,1,0,1,1,
				0,0,0,0,1,0,0,1
		};
		goal_Xpos = 7;
		goal_Ypos = 7;
		setMatrix(maze_layout);
	}
	else if (maze_choice==4){
		int maze_layout[] = {
				1,1,1,1,0,0,0,0,
				0,1,0,0,0,1,1,0,
				0,1,1,1,1,0,0,0,
				0,1,0,0,1,0,1,0,
				0,1,0,1,0,0,1,0,
				0,0,0,0,0,1,0,0,
				1,1,1,1,1,1,0,1,
				0,0,0,0,0,0,0,0
		};
		goal_Xpos = 2;
		goal_Ypos = 0;
		setMatrix(maze_layout);
	}
}

void loadscrn(){
	dotmatrix[0][0]=1;
	dotmatrix[7][0]=1;
	dotmatrix[0][7]=1;
	dotmatrix[7][7]=1;
}
