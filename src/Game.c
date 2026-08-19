// Project 2048
// Youtube video: https://www.youtube.com/watch?v=NrkG_aW3MAA

#include "timerISR.h"
#include "helper.h"
#include "periph.h"
#include "spiAVR.h"

#include "serialATmega.h"

#include <util/delay.h>                                     // Helps with delays
#include <stdlib.h>                                         // For generating random number for random
                                                            //   tile creation

// Variables, for functions //

    int nums[17] = {0b00111111, // 0
                0b00000110, // 1
                0b01011011, // 2
                0b01010111, // 3 
                0b01100110, // 4
                0b01110101, // 5
                0b01111101, // 6
                0b00000111, // 7
                0b01111111, // 8
                0b01100111, // 9
                0b01101111, // a
                0b01111100, // b
                0b00111001, // c 
                0b01011110, // d
                0b01111001, // e
                0b01101001, // f 
                }; 

    int board[4][4] =  {14, 14, 14, 14,         // Board currently
                        14, 14, 14, 14,
                        14, 14, 14, 14,
                        14, 14, 14, 14}; 

    int prevBoard[4][4] ={  0, 0, 0, 0,     // Previous board saved
                            0, 0, 0, 0,
                            0, 0, 0, 0,
                            0, 0, 0, 0};

    int prevMax;                            // Previous max, for uno

    int blockControl = 11;                  // Controls the color of each block

    int elapsedTime = 0;                    // Time of round played
    int holdCounter = 0;                    //

    bool staySlide = 0;

    bool change = true;
    bool boardChange = true;                // Detects if the board has changed
    bool redraw = false;                    // Forces game to update screen without updating tile
    bool canUndo = false;                   // If you can undo
    bool start = false;                     // If game has started

    bool win = false;


    unsigned int LR = 550;                  // Center
    unsigned int UD = 550;                  // Center
    unsigned int pressed = 0;

    int max = 0;                            // For RSCORE
    int maxArr[14] = {0, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096};

    int valueDigit = nums[2];               // Current value
    int valueNum = 0b0000;                  // which digit is on

    int currNote = 0;


//Colors of the rainbow
unsigned char redArr[15] = {
    31,  31,  31,  25,  15,   0,   0,   0,  10,  20,  25,  25,  20,  30, 0
};

unsigned char greenArr[15] = {
    63,   0,  15,  20,  63,  63,  63,   0,   0,   0,   10,  20,  30, 40, 0
};

unsigned char blueArr[15] = {
    31,   0,   0,   0,   0,   0,  31,  31,  31,  31,  31,  25,  10,  20, 0
};

unsigned int songOne[1] = {

};

// End of Variables, for functions //


// Functions //



void copyBoardNScore()                                          // Used in creating the previous state for the undo button
{

    for(int row = 0; row < 4; row++){                           // Old board
        for(int col = 0; col < 4; col++){

            prevBoard[row][col] = board[row][col];

        }
    }

    prevMax = max;                                              // Old score

}

void ScoreUpdate(int score)                                     // Updates the score to the 4d7s screen
{

    int d0 = 0;
    int d1 = 0;
    int d2 = 0;
    int d3 = 0;

    if(start){
        d0 = (score / 1000) % 10;                               // Scores to display
        d1 = (score / 100) % 10;  
        d2 = (score / 10) % 10;   
        d3 = score % 10;
    }         

    int numbers[4] = {nums[d3], nums[d2], nums[d1], nums[d0]};  // Array makes it easier
    
    int digit[4] = {0b1110, 0b1101, 0b1011, 0b0111};            // Which digit is on at that time

    for (int d = 0; d < 4; d++)
    {
        int valueDigit = numbers[d];
        int valueNum   = digit[d];

        for (int i = 0; i < 8; i++)                             // Putting the current digit
        {

            if (valueNum & (1 << (7 - i))) 
                { PORTD = PORTD | 0b00100000; } 
            else{ PORTD = PORTD & 0b11011111; }

            PORTD = PORTD | 0b01000000;  
            PORTD = PORTD & 0b10111111;  
        }

        PORTD = PORTD | 0b10000000;  
        PORTD = PORTD & 0b01111111;  

        for (int i = 0; i < 8; i++)                             // Puts the number inside that digit
        {

            if (valueDigit & (1 << (7 - i)))                    
                { PORTD |= 0b00000100; }  
            else{ PORTD &= 0b11111011; }  

            PORTD = PORTD | 0b00001000; 
            PORTD = PORTD & 0b11110111; 

        }

        PORTD = PORTD | 0b00010000;  
        PORTD = PORTD & 0b11101111; 

        SetBit(PORTD, 1, 1);

    if (win) {                                                  // If you won, LED turns on
        for(int d = 0; d < 4; d++) {
            digit[d] = digit[d] | (1 << 4);  
        }
    } else {                                                    // Any time you haven't won, LED is off
        for(int d = 0; d < 4; d++) {
            digit[d] = digit[d] & ~(1 << 4);
        }
    }

        _delay_ms(2);                                           // Keeps numbers on screen, impossible without it
    }
}

void SlideLeft(int row[4])                                      // Slides the blocks to the left
{

    int rowBefore[4];
    for(int i = 0; i < 4; i++){rowBefore[i] = row[i];}

    int rowI[4] = {0, 0, 0, 0};                                 // Row used to get rid of the 0's
    int rowIndex = 0;                                           // Outide iterator

    for(int i = 0; i < 4; i++)                                  // Eliminates 0's
    {
        if(row[i] != 0)
        {
            rowI[rowIndex] = row[i];
            rowIndex++;
        }
    }

    for(int i = 0; i < 3; i++)                                  // Combines to make the new colors
    {
        if(rowI[i] == rowI[i+1] && rowI[i] != 0)
        {
            rowI[i] = rowI[i] + 1;
            rowI[i + 1] = 0;
        }

    }

    int rowF[4] = {0, 0, 0, 0};                                 // Row to eliminate the 0's after merging
    rowIndex = 0;                                               // Reinitialize the index

    for(int i = 0; i < 4; i++){                                 // Fills the row to return with 0's eliminated

        if(rowI[i] != 0)
        { 
            rowF[rowIndex] = rowI[i]; 
            rowIndex++;
        }
    }

    for(int i = 0; i < 4; i++){                                 // Return completed shift

        row[i] = rowF[i];

    }

    for(int i = 0; i < 4; i++)                                  // If the row changed then allow for an undo and update the screen
    {
        if(row[i] != rowBefore[i])
        boardChange = true;
    }

}

void SlideRight(int row[4])                                     // Slides the blocks to the right
{
    int rowBefore[4];
    for(int i = 0; i < 4; i++){rowBefore[i] = row[i];}

    int rowI[4] = {0, 0, 0, 0};                                 // Row used to get rid of the 0's
    int rowIndex = 3;                                           // Outide iterator

    for(int i = 3; i >= 0; i--)                                 // Eliminates 0's
    {
        if(row[i] != 0)
        {
            rowI[rowIndex] = row[i];
            rowIndex--;
        }
    }

    for(int i = 3; i > 0; i--)                                  // Combines to make the new colors
    {
        if(rowI[i] == rowI[i-1] && rowI[i] != 0)
        {
            rowI[i] = rowI[i] + 1;
            rowI[i - 1] = 0;
        }

    }

    int rowF[4] = {0, 0, 0, 0};                                 // Row to eliminate the 0's after merging
    rowIndex = 3;                                               // Reinitialize the index

    for(int i = 3; i >= 0; i--){                                // Fills the row to return with 0's eliminated

        if(rowI[i] != 0)
        { 
            rowF[rowIndex] = rowI[i]; 
            rowIndex--;
        }
    }

    for(int i = 0; i < 4; i++){                                 // Return completed shift

        row[i] = rowF[i];

    }

    for(int i = 0; i < 4; i++)                                  // If the row changed then allow for an undo and update the screen
    {
        if(row[i] != rowBefore[i])
        boardChange = true;
    }

}

void SlideDown(int board[4][4])                                 // Slides the blocks down
{

    for(int col = 0; col < 4; col++)                            // For every column do this
    {

        int colBefore[4];
        for(int row = 0; row < 4; row++){colBefore[row] = board[row][col];}

        int colI[4] = {0, 0, 0, 0};
        int colIndex = 3;

        for(int row = 3; row >= 0; row--)                       // Eliminate the 0's
        {

            if(board[row][col] != 0)
            {

                colI[colIndex] = board[row][col];
                colIndex--;

            }

        }

        for(int row = 3; row > 0; row--)                        // Combines to make new colors
        {

            if(colI[row] == colI[row-1] && colI[row] != 0)
            {

                colI[row] = colI[row] + 1;
                colI[row - 1] = 0;

            }

        }

        int colF[4] = {0, 0, 0, 0};                             // Eliminate 0's again after merging
        colIndex = 3;                                           // Reinitialize Variable

        for(int row = 3; row >= 0; row--)
        {

            if(colI[row] != 0)
            {

                colF[colIndex] = colI[row];
                colIndex--;

            }

        }

        for(int row = 0; row < 4; row++)                        // Put this column back, now merged correctly
        {

            board[row][col] = colF[row];

        }

        for(int row = 0; row < 4; row++)
            if( board[row][col] != colBefore[row] ){ boardChange = true; }

    }

}

void SlideUp(int board[4][4])                                   // Slides the blocks up
{

    for(int col = 0; col < 4; col++)                            // For every column do this
    {

        int colBefore[4];
        for(int row = 0; row < 4; row++){colBefore[row] = board[row][col];}


        int colI[4] = {0, 0, 0, 0};
        int colIndex = 0;

        for(int row = 0; row < 4; row++)                        // Eliminate the 0's
        {

            if(board[row][col] != 0)
            {

                colI[colIndex] = board[row][col];
                colIndex++;

            }

        }

        for(int row = 0; row < 3; row++)                        // Combines to make new colors
        {

            if(colI[row] == colI[row+1] && colI[row] != 0)
            {

                colI[row] = colI[row] + 1;
                colI[row + 1] = 0;

            }

        }

        int colF[4] = {0, 0, 0, 0};                             // Eliminate 0's again after merging
        colIndex = 0;                                           // Reinitialize Variable

        for(int row = 0; row < 4; row++)
        {

            if(colI[row] != 0)
            {

                colF[colIndex] = colI[row];
                colIndex++;

            }

        }

        for(int row = 0; row < 4; row++)                        // Put this column back, now merged correctly
        {

            board[row][col] = colF[row];

        }
                
        for(int row = 0; row < 4; row++)
        if( board[row][col] != colBefore[row] ){ boardChange = true; }

    }

}

void AddBlock(int board[4][4]){                                 // Adds block randomly to board

    int randomNumber = rand();                                  // The random part
    int emptyCount = 0;
    int newNumber = 0; 

    if(randomNumber % 10 > 7){newNumber = 2;}                   // Whether block is 2 or 4
    else{newNumber = 1;}

    for(int row = 0; row < 4; row++)                            // Checks how many empty blocks there are
    {
        for(int col = 0; col < 4; col++)
        {

            if(board[row][col] == 0){emptyCount++;}

        }
    }

    int correctIndex = 0;


    if(emptyCount > 0)                                          // Assigns which empty block gets it
        { randomNumber = randomNumber % emptyCount; }


    for(int row = 0; row < 4; row++)                            // Puts new value inside empty block
    {
        for(int col = 0; col < 4; col++)
        {

            if(board[row][col] == 0)
            {

                if(correctIndex == randomNumber)
                {

                    board[row][col] = newNumber;

                }

                correctIndex++;       
            }
        }
    }
}

void GameReset()
{
        for(int row = 0; row < 4; row++)
        {
            for(int col = 0; col < 4; col++)
            {

                board[row][col] = 0;
                prevBoard[row][col] = 0;

            }

        }

        max = 0;
        prevMax = 0;

        canUndo = false;

        AddBlock(board);

        redraw = true;
        boardChange = false;

}

void HardwareReset(){

    PORTB = SetBit(PORTB, 1, 0);
    _delay_ms(200);
    PORTB = SetBit(PORTB, 1, 1);
    _delay_ms(200);

}

void Send_Command(char command){        // A0 is now 0

    PORTC = SetBit(PORTC, 0, 0);
    PORTB = SetBit(PORTB, 2, 0);

    SPI_SEND(command);

    PORTB = SetBit(PORTB, 2, 1);


}

void Send_Data(char data){              // A0 is now 1
    
    PORTC = SetBit(PORTC, 0, 1);
    PORTB = SetBit(PORTB, 2, 0);

    SPI_SEND(data);

    PORTB = SetBit(PORTB, 2, 1);


}

void ColorChoice(unsigned char red, unsigned char green, unsigned char blue) // 31, 63, 31. Max for each
{                                                                            // Chooses color to display
    unsigned short color = (red << 11) | (green << 5) | blue;

    Send_Data(color >> 8); // high
    Send_Data(color & 0xFF); // low
}

void AreaChoice(unsigned char xStart, unsigned char xEnd,                   // Chooses location of pixel you send
                unsigned char yStart, unsigned char yEnd)                   //    max X is 127, max Y is 159. (Different lcd)
{                                                                           // They need to find a way to abbreviate unsigned

    Send_Command(0x2A);                                                     // CASET

    Send_Data(0x00);                                                        //  x-coordinates       
    Send_Data(xStart);
                                                            
    Send_Data(0x00); 
    Send_Data(xEnd);                                                        

    Send_Command(0x2B);                                                     // RASET

    Send_Data(0x00);                                                        //  y-coordinates
    Send_Data(yStart);  

    Send_Data(0x00); 
    Send_Data(yEnd);                                                        

    Send_Command(0x2C); // RAMWR

}

void CreateTile(unsigned int xStart, unsigned int yStart,                   // For AreaChoice
                unsigned char red, unsigned char green, unsigned char blue) // For ColorChoice
{

    AreaChoice(xStart * 32              + 4, 
               (xStart * 32) + 27       + 2,
               yStart * 40              + 4,
               (yStart * 40) + 35       + 2);

    for (int i = 0; i < 32 * 40; i++) {
        ColorChoice(red, green, blue);
    }


}

void ST7735_init(){                                                         // Initializes the LCD

    HardwareReset();
    
    PORTB = SetBit(PORTB, 2, 0);

    Send_Command(0x01);                                                     // SWRESET
    _delay_ms(150);

    Send_Command(0x11);                                                     // SLPOUT
    _delay_ms(200);

    Send_Command(0x3A);                                                     // COLMOD
    Send_Data(0x05);
    _delay_ms(10);

    Send_Command(0x29);                                                     // DISPON
    _delay_ms(200);

}

bool IsGameOver()                                                           // Checks if there are any available moves left
{

    bool over = true;

    for(int row = 0; row < 4; row++)
    {

        for(int col = 0; col < 4; col++)
        {

            if(board[row][col] == 0){over = false;}

            if(col < 3 && board[row][col] == board[row][col+1]){ over = false; }

            if(row < 3 && board[row][col] == board[row+1][col]){ over = false; }

        }

    }

    return over;

}

// End of Functions // 


unsigned int i = 0;

const unsigned long periodJOYStates = 100;
const unsigned long periodRSCOREStates = 10; // Reads the top score from the screen
const unsigned long periodSCOREStates = 10; // Displays the top score from the screen
const unsigned long periodSLIStates = 100;
const unsigned long periodSCREENStates = 100;
const unsigned long periodUNDOStates = 100;
const unsigned long periodSTARTStates = 50;
const unsigned long periodWINStates = 100;

const unsigned long GCD_PERIOD = 10; 

//Task struct for concurrent synchSMs implmentations
typedef struct _task{
	signed 	 char state; 		//Task's current state
	unsigned long period; 		//Task period
	unsigned long elapsedTime; 	//Time elapsed since last task tick
	int (*TickFct)(int); 		//Task tick function
} task;

#define NUM_TASKS 8

task tasks[NUM_TASKS];


enum SCOREStates{SCORE_Start, SCORE_DISPLAY};
enum RSCOREStates{RSCORE_Start, RSCORE_READ};

enum JOYStates{JOY_Start, JOY_READ};

enum SLIStates{SLI_Start, SLI_WAIT, SLI_UP, SLI_DOWN, SLI_LEFT, SLI_RIGHT};

enum SCREENStates{SCREEN_Start, SCREEN_UPDATE};

enum UNDOStates{UNDO_Start, UNDO_CANT, UNDO_CAN};

enum STARTStates{START_Start, START_PLAY};

enum WINStates{WIN_Start, WIN_CHECK};

int TickFct_JOYStates(int state){

    switch(state){

        case JOY_Start:
            state = JOY_READ;
            break;

        case JOY_READ:
            state = JOY_READ;
            break;
        
        default: 
            break;

    }

    switch(state){

        case JOY_READ:
            if(start){
                LR = ADC_read(4);
                pressed = ADC_read(5);
                UD = ADC_read(3);
            }

            break;

    }

    return state;

}

int TickFct_SCOREStates(int state){

    switch(state){

        case SCORE_Start:
            state = SCORE_DISPLAY;
            break;

        case SCORE_DISPLAY:
            break;


    }

    switch(state){

        case SCORE_DISPLAY:
            
            ScoreUpdate(maxArr[max]);
            break;

    }

    return state;

}

int TickFct_RSCOREStates(int state){

    switch(state){

        case RSCORE_Start:
            state = RSCORE_READ;
            break;


        case RSCORE_READ:
            
            int curr = 0;

            for(int row = 0; row < 4; row++)
            {

                for(int col = 0; col < 4; col++)
                {

                    curr = board[row][col];

                    if(curr > max){max = curr;}

                }

            }
            break;

    }

    switch(state){

        case RSCORE_READ:

            int curr = 0;

            for(int row = 0; row < 4; row++)
            {

                for(int col = 0; col < 4; col++)
                {

                    curr = board[row][col];

                    if(curr > max){max = curr;}

                }

            }

            break;

    }

    return state;

}

int TickFct_SLIStates(int state){

    switch(state){

        case SLI_Start:
            state = SLI_WAIT;
            break;

        case SLI_WAIT:

            if( ((400 > LR || LR > 600) && (400 > UD || UD > 600)) || // If in a corner don't move
                 ((400 < LR && LR < 600) && (400 < UD && UD < 600)) ) // If in center, don't move
                {state = SLI_WAIT;}
            else if(400 > LR )
                {state = SLI_LEFT;}
            else if(LR > 600)
                {state = SLI_RIGHT;}
            else if(400 > UD)
                {state = SLI_DOWN;}
            else if(UD > 600)
                {state = SLI_UP;}         
            break;

        case SLI_UP:

            if( ((400 > LR || LR > 600) && (400 > UD || UD > 600)) || // If in a corner don't move
                 ((400 < LR && LR < 600) && (400 < UD && UD < 600)) ) // If in center, don't move
                {state = SLI_WAIT;}
            else if(400 > LR )
                {state = SLI_LEFT;}
            else if(LR > 600)
                {state = SLI_RIGHT;}
            else if(400 > UD)
                {state = SLI_DOWN;}
            else if(UD > 600)
                {state = SLI_UP;}            
            break;

        case SLI_DOWN:

            if( ((400 > LR || LR > 600) && (400 > UD || UD > 600)) || // If in a corner don't move
                 ((400 < LR && LR < 600) && (400 < UD && UD < 600)) ) // If in center, don't move
                {state = SLI_WAIT;}
            else if(400 > LR )
                {state = SLI_LEFT;}
            else if(LR > 600)
                {state = SLI_RIGHT;}
            else if(400 > UD)
                {state = SLI_DOWN;}
            else if(UD > 600)
                {state = SLI_UP;}           
            break;

        case SLI_LEFT:

            if( ((400 > LR || LR > 600) && (400 > UD || UD > 600)) || // If in a corner don't move
                 ((400 < LR && LR < 600) && (400 < UD && UD < 600)) ) // If in center, don't move
                {state = SLI_WAIT;}
            else if(400 > LR )
                {state = SLI_LEFT;}
            else if(LR > 600)
                {state = SLI_RIGHT;}
            else if(400 > UD)
                {state = SLI_DOWN;}
            else if(UD > 600)
                {state = SLI_UP;}           
            break;

        case SLI_RIGHT:

            if( ((400 > LR || LR > 600) && (400 > UD || UD > 600)) || // If in a corner don't move
                 ((400 < LR && LR < 600) && (400 < UD && UD < 600)) ) // If in center, don't move
                {state = SLI_WAIT;}
            else if(400 > LR )
                {state = SLI_LEFT;}
            else if(LR > 600)
                {state = SLI_RIGHT;}
            else if(400 > UD)
                {state = SLI_DOWN;}
            else if(UD > 600)
                {state = SLI_UP;}          
            break;

        default:
            break;

    }

    switch(state){

        case SLI_WAIT:
            staySlide = false;
            break;

        case SLI_LEFT:

            if(!staySlide)
            {
                
                copyBoardNScore();
                canUndo = true;

                for(int i = 0; i < 4; i++){
                    SlideLeft(board[i]);
                }

                change = true;
                staySlide = true;
                
            }

            break;

        case SLI_RIGHT:

            if(!staySlide)
            {

                copyBoardNScore();
                canUndo = true;

                for(int i = 0; i < 4; i++){
                    SlideRight(board[i]);
                }

                change = true;
                staySlide = true;

            }
            break;

        case SLI_DOWN:
            
            if(!staySlide)
            {
                copyBoardNScore();
                canUndo = true;
                SlideDown(board);
                change = true;
                staySlide = true;
            }
            break;

        case SLI_UP:
            if(!staySlide)
            {
                copyBoardNScore();
                canUndo = true;
                SlideUp(board);
                change = true;
                staySlide = true;
            }
            break;

        default:
            break; 

    }


    return state;


}

int TickFct_SCREENStates(int state){

    switch(state){

        case SCREEN_Start:
            state = SCREEN_UPDATE;
            break;

        case SCREEN_UPDATE:
            break;

        default:
            break;

    }

    switch(state){

        case SCREEN_UPDATE:

            if(boardChange || redraw){

                if(boardChange){AddBlock(board);}

                Send_Command(0x2A);                             // All pixels are now black
                Send_Data(0x00); 
                Send_Data(0x00);
                Send_Data(0x00); 
                Send_Data(0x7F);

                Send_Command(0x2B);                             
                Send_Data(0x00); 
                Send_Data(0x00);
                Send_Data(0x00); 
                Send_Data(0x9F);

                Send_Command(0x2C);


                for (int i = 0; i < 128 * 160 ; i++)            // Gets everything turned to black
                {
                    Send_Data(0x00); 
                    Send_Data(0x00);
                }

                for (int row = 0; row < 4; row++) 
                {                       
                    for (int col = 0; col < 4; col++) 
                    {

                        CreateTile(3-col, 3-row,
                        redArr[board[row][col]],
                        greenArr[board[row][col]],
                        blueArr[board[row][col]]);

                    }
                }
                change = false;
                boardChange = false;
                redraw = false;
            }

            break;
        
        default:
            break;

    }

    return state;


}

int TickFct_UNDOStates(int state){

    switch(state)
    {

        case UNDO_Start:
            state = UNDO_CANT;
            break;

        case UNDO_CAN:
            if(GetBit(PINB, 0) && canUndo)
            { 
                state = UNDO_CANT; 

                for(int row = 0; row < 4; row++){
                    for(int col = 0; col < 4; col++){

                        board[row][col] = prevBoard[row][col];

                    }
                }
                max = prevMax;
                redraw = true;
                canUndo = false;
            }
            break;

        case UNDO_CANT:

            if(canUndo)
            {

                state = UNDO_CAN;

            }

            break;

        default:
            break;

    }

    switch(state)
    {

        case UNDO_CAN:
            break;

        case UNDO_CANT:
            break;

        default:
            break;

    }


    return state;

}

int TickFct_STARTStates(int state){

    switch(state){

        case START_Start:
            state = START_PLAY;
            break;

        case START_PLAY:
            break;

        default:
            break;

    }

    switch(state){

        case START_PLAY:
            
            if(!GetBit(PINC, 1))
            {
                holdCounter++;
            }
            else 
            {
                if(holdCounter > 3 && holdCounter < 30)
                {
                    if (start) {GameReset();} 
                    else 
                    {

                        start = true;
                        GameReset();

                    }
                }
                if(holdCounter >= 30)
                {
                    start = false;

                    for(int row = 0; row < 4; row++) 
                    {
                        for(int col = 0; col < 4; col++) 
                        {
                            board[row][col] = 14; 
                        }
                    }

                    redraw = true;
                    start = false; 
                    }
                holdCounter = 0; 
            }

            if(start && IsGameOver() && !win)
            {
                start = false;
                for(int row = 0; row < 4; row++) 
                {
                    for(int col = 0; col < 4; col++) 
                    {
                        board[row][col] = 1; 
                    }
                }
                redraw = true;
            }
            else if(start && IsGameOver() && win)
            {
                start = false;
                for(int row = 0; row < 4; row++) 
                {
                    for(int col = 0; col < 4; col++) 
                    {
                        board[row][col] = 5; 
                    }
                }
                redraw = true;
            }

            break;

        default:
            break;

    }

    return state;

}

int TickFct_WINStates(int state){

    switch(state){

        case WIN_Start:
            state = WIN_CHECK;
            break;

        case WIN_CHECK:
            break;

        default:
            break;

    }

    switch(state){

        case WIN_CHECK:
            if(maxArr[max] >= 2048){ win = true; }
            else{ win = false; }
            break;

        default:
            break;
    }

    return state;

}

void TimerISR() {
	for ( unsigned int i = 0; i < NUM_TASKS; i++ ) {                   // Iterate through each task in the task array
		if ( tasks[i].elapsedTime == tasks[i].period ) {           // Check if the task is ready to tick
			tasks[i].state = tasks[i].TickFct(tasks[i].state); // Tick and set the next state for this task
			tasks[i].elapsedTime = 0;                          // Reset the elapsed time for the next tick
		}
		tasks[i].elapsedTime += GCD_PERIOD;                        // Increment the elapsed time by GCD_PERIOD
	}
}

int main(void) {

  DDRC     = 0b00000001; // Output
  PORTC    = 0b00111110; // Input

  DDRB     = 0b11111111;
  PORTB    = 0b00000000;

  DDRD     = 0b11111111;
  PORTD    = 0b00000000;

  SPI_INIT();
  ADC_init();                                       // initializes ADC
  ST7735_init();

  srand(ADC_read(3));                               // Seeds a random number for the tile creation

  tasks[i].state        = JOY_Start;
  tasks[i].period       = periodJOYStates;
  tasks[i].elapsedTime  = tasks[i].period;
  tasks[i].TickFct      = &TickFct_JOYStates;

  i++;

  tasks[i].state        = SCORE_Start; 
  tasks[i].period       = periodSCOREStates;
  tasks[i].elapsedTime  = tasks[i].period;
  tasks[i].TickFct      = &TickFct_SCOREStates;

  i++;

  tasks[i].state        = RSCORE_Start;
  tasks[i].period       = periodRSCOREStates;
  tasks[i].elapsedTime  = tasks[i].period;
  tasks[i].TickFct      = &TickFct_RSCOREStates;

  i++;

  tasks[i].state        = SLI_Start;
  tasks[i].period       = periodSLIStates;
  tasks[i].elapsedTime  = tasks[i].period;
  tasks[i].TickFct      = &TickFct_SLIStates;

  i++;

  tasks[i].state        = SCREEN_Start;
  tasks[i].period       = periodSCREENStates;
  tasks[i].elapsedTime  = tasks[i].period;
  tasks[i].TickFct      = &TickFct_SCREENStates;

  i++;

  tasks[i].state        = UNDO_Start;
  tasks[i].period       = periodUNDOStates;
  tasks[i].elapsedTime  = tasks[i].period;
  tasks[i].TickFct      = &TickFct_UNDOStates;

  i++;

  tasks[i].state        = START_Start;
  tasks[i].period       = periodSTARTStates;
  tasks[i].elapsedTime  = tasks[i].period;
  tasks[i].TickFct      = &TickFct_STARTStates;

  i++;

  tasks[i].state        = WIN_Start;
  tasks[i].period       = periodWINStates;
  tasks[i].elapsedTime  = tasks[i].period;
  tasks[i].TickFct      = &TickFct_WINStates;

  TimerSet(GCD_PERIOD);
  TimerOn();
serial_init(9600);
  

while(1){}

  return 0;

}
