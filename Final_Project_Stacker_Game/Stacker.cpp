/*
===============================================================================
 Name        : main.c
 Author      : $(Justin Huebner @ justin.hueb@ou.edu)
 Version     :
 Copyright   : $(MIT)
 Description : Stacker arcade game
===============================================================================
*/

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include <cr_section_macros.h>
#include <stdio.h>
#include <string.h>

#define FIO0DIR (*(volatile unsigned int *)0x2009c000)
#define FIO0SET (*(volatile unsigned int *)0x2009c018)
#define FIO0CLR (*(volatile unsigned int *)0x2009c01c)
#define FIO0PIN (*(volatile unsigned int *)0x2009c014)
#define FIO1DIR (*(volatile unsigned int *)0x2009c020)
#define FIO1PIN (*(volatile unsigned int *)0x2009c034)
#define FIO2DIR (*(volatile unsigned int *)0x2009c040)
#define FIO2PIN (*(volatile unsigned int *)0x2009c054)
#define PINMODE4 (*(volatile unsigned int *)0x4002c050)
#define PINMODE3 (*(volatile unsigned int *)0x4002c04c)
#define PINMODE2 (*(volatile unsigned int *)0x4002c048)
#define PINSEL0 (*(volatile unsigned int *)0x4002c000)
#define PINSEL1 (*(volatile unsigned int *)0x4002c004)
#define PINSEL2 (*(volatile unsigned int *)0x4002c008)
#define PINSEL3 (*(volatile unsigned int *)0x4002c00c)
#define PINSEL4 (*(volatile unsigned int *)0x4002c010)
#define PINSEL7 (*(volatile unsigned int *)0x4002c01c)
#define PINSEL8 (*(volatile unsigned int *)0x4002c020)
#define PINSEL9 (*(volatile unsigned int *)0x4002c024)
#define PINSEL10 (*(volatile unsigned int *)0x4002c028)
#define PINMODE0 (*(volatile unsigned int *)0X4002C040)
#define PINMODE1 (*(volatile unsigned int *)0X4002C044)
#define I2CPADCFG (*(volatile unsigned int *)0X4002C07C)
#define I2C0CONSET (*(volatile unsigned int *)0X4001C000)
#define I2C0CONCLR (*(volatile unsigned int *)0X4001C018)
#define PCLKSEL0 (*(volatile unsigned int *)0X400FC1A8)
#define PCLKSEL1 (*(volatile unsigned int *)0X400FC1AC)
#define I2C0STAT (*(volatile unsigned int *)0X4001C028)
#define I2C0DAT (*(volatile unsigned int *)0X4001C004)
#define PINMODE_OD0 (*(volatile unsigned int *)0X4002C068)
#define PINMODE_OD1 (*(volatile unsigned int *)0X4002C06C)
#define PINMODE_OD2 (*(volatile unsigned int *)0X4002C070)
#define PINMODE_OD3 (*(volatile unsigned int *)0X4002C074)
#define PINMODE_OD4 (*(volatile unsigned int *)0X4002C078)
#define I2SCLL (*(volatile unsigned int *)0x4001C014)
#define I2SCLH (*(volatile unsigned int *)0x4001C000)
#define T2EMR (*(volatile unsigned int *)0x4009003C)
#define T2MCR (*(volatile unsigned int *)0x40090014)
#define T2MR1 (*(volatile unsigned int *)0x4009001C)
#define PCONP (*(volatile unsigned int *)0x400FC0C4)
#define T2TCR (*(volatile unsigned int *)0x40090004)
#define T2TC (*(volatile unsigned int *)0x40090008)
#define DACR (*(volatile unsigned int *)0x4008C000)

//RW=p17
//RS=p18
//E=p8
//D7-D0=p16-p9
	int bricks[4] = {1,1,1,0};
	int oldBricks[4] = {1,1,1,1};
	int LorR = -1; //-1 for left, 1 for right
	int lvl=1;
	int cursorPosition = 1;
	int speed = 0;
	char butt = '0';

void LCD_PinSetup(){
	FIO1DIR |= (1<<31); //P20 TIME
	FIO0DIR |= (1<<26); //RS
	FIO0DIR |= (1<<25); //RW
	FIO0DIR |= (1<<6); //E
	FIO0DIR |= (1<<24); //D7
	FIO0DIR |= (1<<23); //D6
	FIO0DIR |= (1<<16); //D5
	FIO0DIR |= (1<<15); //D4
	FIO0DIR |= (1<<17); //D3
	FIO0DIR |= (1<<18); //D2
	FIO0DIR |= (1<<1); //D1
	FIO0DIR |= (1<<0); //D0
}

void Keypad_PinSetup(){
	//set p2.0-p2.7 to no pulls
	PINMODE4 |= (1<<1);
	PINMODE4 |= (1<<3);
	PINMODE4 |= (1<<5);
	PINMODE4 |= (1<<7);
	PINMODE4 |= (1<<9);
	PINMODE4 |= (1<<11);
	PINMODE4 |= (1<<13);
	PINMODE4 |= (1<<15);
	//set ^ to inputs (by default)
	//set 2.0-3 to outputs.
	FIO2DIR |= (1<<0);
	FIO2DIR |= (1<<1);
	FIO2DIR |= (1<<2);
	FIO2DIR |= (1<<3);
}

void I2C_PinSetup(){
    PINSEL1 |= (1<<22);
    PINSEL1 &= ~(1<<23);
    PINSEL1 |= (1<<24);
    PINSEL1 &= ~(1<<25);

    //Freq stuff
    I2SCLL = 5;
    I2SCLH = 5;

    I2C0CONSET = (1<<6);

}
void Sleep(int n){
	//1 tick is 10.62us
	//1 tick is 43.94khz

	//10 tick is 35.11us

	for (int i=0;i<n;++i){}
}

void BusOutWrite(int value) {
	//From week2pt2 slides
	//portbit[] = {db7-db0}
//	const int portbit[] = {24,23,16,15,17,18,1,0};
	const int portbit[] = {0,1,18,17,15,16,23,24};
	for (int b=0; b<sizeof(portbit)/sizeof(int);b++){
		if ((value>>b) & 1){ // extract bit b
			FIO0PIN |= (1<<portbit[b]); // set port bit
		}else{
			FIO0PIN &= ~(1<<portbit[b]); // clear port bit
		}
	}
}

void LCDwriteCommand(int value){
	//● Update DB0-DB7 to match command code – See Week 2 part 2 Lecture for ideas on how to code this
	BusOutWrite(value);
	//● Drive R/W low if it’s not already grounded
	FIO0PIN &= ~(1<<25);
	//● Drive RS low to indicate this is a command
	FIO0PIN &= ~(1<<26);
	//● Drive E high then low to generate the pulse – If CPU is faster than 4 MHz, you might need a short delay after E is high to meet minimum pulse width requirement
	FIO0PIN |= (1<<6);
	Sleep(1);
	FIO0PIN &= ~(1<<6);
	//● Wait 100 microseconds for command to complete – The HD44780 datasheet suggests 37 µs, but this assumes an LCD oscillator of 270 kHz and many modules use a slower (but unspecified!) clock
	Sleep(40);
}

void LCDwriteData(int value){
	//● Update DB0-DB7 to match data (ASCII) code – See Week 2 part 2 Lecture for ideas on how to code this
	BusOutWrite(value);
	//● Drive R/W low if it’s not already grounded
	FIO0PIN &= ~(1<<25);
	//● Drive RS high to indicate this is data
	FIO0PIN |= (1<<26);
	//● Drive E high then low to generate the pulse – If CPU is faster than 4 MHz, you might need a short delay after E is high to meet minimum pulse width requirement
	FIO0PIN |= (1<<6);
	Sleep(1);
	FIO0PIN &= ~(1<<6);
	//● Wait 100 microseconds for data to be processed – Entire display can be updated in 8 ms (125 frames/second) with this delay, so even though this delay could be reduced there is no point
	Sleep(40);
}

void LCDwrite(int value){
	LCDwriteData(value);	//convenience :)
}

//bool IsBusy(){
////	RS=0;
////	RW=1;
////	while(D7=1){}
////	return false;
//}

void LCD_Init(){
	//● Configure control signals (RS, R/W, and E) as outputs to LCD and drive low
	FIO0PIN &= ~(1<<25); //RW
	FIO0PIN &= ~(1<<26); //RS
	FIO0PIN &= ~(1<<6); //E
	//● Configure data signals (DB0-DB7) as outputs to LCD

	//● Wait 4 ms
	Sleep(2000); //wait >4.1ms
	//● Write LCD command 0x38 (see sequence described earlier for writing commands)
	LCDwriteCommand(0x38);
	//● Write LCD command 0x06
	LCDwriteCommand(0x06);
	//● Write LCD command 0x0c, 0x0e, or 0x0f (see details on upcoming slide)
//	LCDwriteCommand(0x0e);	//cursor visible, not blinking
//	LCDwriteCommand(0x0f);	//cursor visible, blinking
	LCDwriteCommand(0x0c);	//no cursor
	//● Write LCD command 0x01
	LCDwriteCommand(0x01);
	//● Wait 4 m
	Sleep(2000); //wait >4.1ms
}

void LCD_Write(char str[]){
	for (int i=0; i<strlen(str) ; ++i){
		LCDwriteData(str[i]);
	}
}

char Pressed(){
	FIO2PIN &= ~(1<<0);	//set row 1 low.
	if ((FIO2PIN>>5 & 1) == 0) return '2';
	FIO2PIN |= (1<<0);	//set row 1 high.

	FIO2PIN &= ~(1<<1);	//set row 2 low.
	if ((FIO2PIN>>4 & 1) == 0) return '4';
	if ((FIO2PIN>>5 & 1) == 0) return '5';
	if ((FIO2PIN>>6 & 1) == 0) return '6';
	FIO2PIN |= (1<<1);	//set row 2 high.

	FIO2PIN &= ~(1<<2);	//set row 3 low.
	if ((FIO2PIN>>5 & 1) == 0) return '8';
	FIO2PIN |= (1<<2);	//set row 3 high.
	return '0';
}

void LCD_Clear(){
	FIO0PIN &= ~(1<<25); //RW
	FIO0PIN &= ~(1<<26); //RS
	FIO0PIN &= ~(1<<6); //E

	Sleep(1000);

	LCDwriteCommand(0x01);
}

void LCD_Home(){
	LCDwriteCommand(0x80);
}

void LCD_DefineBrick(){
	// Defining bricks
	LCDwriteCommand(0x40);
	LCDwriteData(0b11111); //Byte 1
	LCDwriteData(0b11111); //Byte 2
	LCDwriteData(0b11011);
	LCDwriteData(0b11011);
	LCDwriteData(0b11011);
	LCDwriteData(0b11011);
	LCDwriteData(0b11111);
	LCDwriteData(0b11111); //Byte 8
	LCDwriteCommand(0x80);
}

void LCD_DefineDown(){
	// Defining bricks
	LCDwriteCommand(0x48);
	LCDwriteData(0b00000); //Byte 1
	LCDwriteData(0b00000); //Byte 2
	LCDwriteData(0b00100);
	LCDwriteData(0b00100);
	LCDwriteData(0b00100);
	LCDwriteData(0b10101);
	LCDwriteData(0b01110);
	LCDwriteData(0b00100); //Byte 8
	LCDwriteCommand(0x80);
}

void LCD_DefineUp(){
	// Defining bricks
	LCDwriteCommand(0x50);
	LCDwriteData(0b00100); //Byte 1
	LCDwriteData(0b01110); //Byte 2
	LCDwriteData(0b10101);
	LCDwriteData(0b00100);
	LCDwriteData(0b00100);
	LCDwriteData(0b00100);
	LCDwriteData(0b00000);
	LCDwriteData(0b00000); //Byte 8
	LCDwriteCommand(0x80);
}

void LCD_MoveCursor(int row, int col){
	--col;
	if (col>20 || col<0) return;
	if (row>4 || row<=0) return;

	switch(row){
	case 1:
		col+=0x80;
		LCDwriteCommand(col);
		break;

	case 2:
		col+=0x80;
		col+=0x40;
		LCDwriteCommand(col);
		break;

	case 3:
		col+=0x80;
		col+=0x14;
		LCDwriteCommand(col);
		break;

	case 4:
		col+=0x80;
		col+=0x54;
		LCDwriteCommand(col);
		break;

	default:
		break;
	}

	LCDwriteCommand(0x0c);	//no cursor
}

void LCD_ClearRow(int row){
	LCD_MoveCursor(row,1);
	for (int i=1; i<=20; ++i){
		LCDwrite(' ');
	}
}

void LCD_ClearCol(int col){
	for (int i=1; i<=4; ++i){
		LCD_MoveCursor(i,col);
		LCDwrite(' ');
	}
}

void DrawBricks(int col){
	for (int i=1;i<=4;++i){
		LCD_MoveCursor(i,col);
		if (bricks[i-1] == 1) LCDwriteData(0x00);
		if (bricks[i-1] == 0) LCDwriteData(' ');
	}
}

void BrickSlide(int col, int blocks){
//	for (int i=0;i<=4;++i){
//		LCD_MoveCursor(i,col);
//		LCDwriteData(0x00);
//	}

	LCD_MoveCursor(1,col);
	LCDwriteData(0x00);
	LCD_MoveCursor(2,col);
	LCDwriteData(0x00);
	LCD_MoveCursor(3,col);
	LCDwriteData(0x00);
}

void LCD_Test(int sleepTime){
	LCD_MoveCursor(1,1);
	LCDwrite(0);
//	Sleep(100);
	LCD_MoveCursor(1,20);
	LCDwrite(0);
//	Sleep(100);
	LCD_MoveCursor(4,1);
	LCDwrite(0);
//	Sleep(100);
	LCD_MoveCursor(4,20);
	LCDwrite(0);
//	Sleep(100);
	LCD_MoveCursor(2,9);
	LCD_Write("Brick");
//	Sleep(100);
	LCD_MoveCursor(3,8);
	LCD_Write("Stacker!");
	Sleep(sleepTime);
	LCD_MoveCursor(2,9);
	LCD_Write("Press");
	LCD_MoveCursor(3,8);
	LCD_Write(" Enter  ");
}

int Freq(int freq){
	return int((-1.4*freq)+1693);
}

int Notes(char note){
	switch(note){
	case 'C': return int(261.63);
	break;
	case 'c': return int(277.18);
	break;
	case 'D': return int(293.66);
	break;
	case 'd': return int(311.13);
	break;
	case 'E': return int(329.63);
	break;
	case 'F': return int(349.23);
	break;
	case 'f': return int(369.99);
	break;
	case 'G': return int(392);
	break;
	case 'g': return int(415.3);
	break;
	case 'A': return int(440);
	break;
	case 'a': return int(466.16);
	break;
	case 'B': return int(493.88);
	break;
	case 'x':
	case 'X':
	default: return(0);
	break;
	}
	return 0;
}

void HappySong(){
	char song[] = {'C','c','D','d','E','F','f','G','g','A','a','B'}; //{'E','E','F','G','G','F','E','D','C','C','D','E','E','D','D'};

	for (int i=0;i<=11;++i){
		T2MR1=Freq(Notes(song[i]));
		T2TCR = 2;
		T2TCR = 1;
		Sleep(200000/4);
		T2MR1=0;
		Sleep(200000/32);
	}
	T2MR1=0;
	T2TCR = 2;
	T2TCR = 1;
}

void SadSong(){
	char song[] = {'C','c','D','d','E','F','f','G','g','A','a','B'}; //{'E','E','F','G','G','F','E','D','C','C','D','E','E','D','D'};

	for (int i=11;i>=0;--i){
		T2MR1=Freq(Notes(song[i]));
		T2TCR = 2;
		T2TCR = 1;
		Sleep(200000/4);
		T2MR1=0;
		Sleep(200000/32);
	}
	T2MR1=0;
	T2TCR = 2;
	T2TCR = 1;
}

void SelectSpeed(){
//	speed*=2;
	LCD_Clear();
	LCD_Test(1);
	LCD_ClearRow(2);
	LCD_ClearRow(3);
	LCD_MoveCursor(1,5);
	LCD_Write("Select Speed!");
	LCD_MoveCursor(2,9);
	LCDwriteData(0x02);
//	LCDwriteData(0x02);
	LCD_MoveCursor(4,9);
	LCDwriteData(0x01);
//	LCDwriteData(0x01);
	LCD_MoveCursor(3,9);
	LCDwrite(0x30+(int(speed/10)));
	LCDwrite(0x30+(int(speed%10)));


	cursorPosition=1;

	while(butt!='5'){
			if (butt=='0'){
				butt=Pressed();
			}else{
				if (cursorPosition==1 && butt=='8'){
					cursorPosition = 2;//move right
					LCD_MoveCursor(2,9);
					LCDwrite(' ');
					LCDwrite(0x02);
					LCD_MoveCursor(4,9);
					LCDwrite(' ');
					LCDwrite(0x01);
				} else if (cursorPosition==2 && butt=='2') {
					cursorPosition = 1;//move left
					LCD_MoveCursor(2,9);
					LCDwrite(0x02);
					LCDwrite(' ');
					LCD_MoveCursor(4,9);
					LCDwrite(0x01);
					LCDwrite(' ');
				} else if ((cursorPosition==1 && butt=='6') && speed<10) {
					speed+=10;
				} else if ((cursorPosition==1 && butt=='4') && speed>=10) {
					speed-=10;
				} else if ((cursorPosition==2 && butt=='6') && (speed%10<9)) {
					speed+=1;
				} else if ((cursorPosition==2 && butt=='4') && (speed%10>0)) {
					speed-=1;
				}

				LCD_MoveCursor(3,9);
				LCDwrite(0x30+(int(speed/10)));
				LCDwrite(0x30+(int(speed%10)));
	//			LCD_MoveCursor(1,5);
	//			LCD_Write("::");
	//			LCDwrite(butt);
	//			LCD_Write("::");
				Sleep(100000);
				butt='0';
			}
				Sleep(1000);
		}
//	speed=speed/2;
	LCD_Clear();
}

void EndGame(int WorL){
	if (WorL==1){ //win
		LCD_Clear();
		Sleep(1000);
		LCD_Home();
		LCD_MoveCursor(2,9);
		LCD_Write("You");
		LCD_MoveCursor(3,8);
		LCD_Write(" WON!  ");
		HappySong();
		Sleep(500000);
		LCD_Home();
		LCD_Clear();

		for (int i=0; i<3; ++i){
			bricks[i]=1;
		}

		bricks[3]=0;

		for (int i=0; i<4; ++i){
			oldBricks[i]=1;
		}

		LorR = -1; //-1 for left, 1 for right
		lvl=1;
	} else { //lose
		LCD_Clear();
		Sleep(1000);
		LCD_Home();
		LCD_MoveCursor(2,9);
		LCD_Write("You");
		LCD_MoveCursor(3,8);
		LCD_Write(" Lost  ");
		SadSong();
		Sleep(500000);
		LCD_Home();
		LCD_Clear();

		for (int i=0; i<3; ++i){
			bricks[i]=1;
		}

		bricks[3]=0;

		for (int i=0; i<4; ++i){
			oldBricks[i]=1;
		}

		LorR = -1; //-1 for left, 1 for right
		lvl=1;
	}
	SelectSpeed();
	LCD_Clear();
	LCD_Home();
	Sleep(200000);
}

void Tick(){
	int sum = bricks[0] + bricks[1] + bricks[2] + bricks[3];

	if (sum==0){
		EndGame(0);
	}

	if (sum==3){
		if (bricks[0]==1){ //1110
			bricks[0]=0;
			bricks[3]=1;
		}else{ //0111
			bricks[0]=1;
			bricks[3]=0;
		}
	}else if(sum==2){
		if (bricks[0]==1 && bricks[1]==1){ //1100
			bricks[0]=0;
			bricks[1]=1;
			bricks[2]=1;
			bricks[3]=0;
		} else if (bricks[1]==1 && bricks[2]==1){ //0110
			if (LorR<0){ //left
				bricks[0]=1;
				bricks[1]=1;
				bricks[2]=0;
				bricks[3]=0;
				LorR=1;
			} else { //right
				bricks[0]=0;
				bricks[1]=0;
				bricks[2]=1;
				bricks[3]=1;
				LorR=-1;
			}
		} else { //0011
			bricks[0]=0;
			bricks[1]=1;
			bricks[2]=1;
			bricks[3]=0;
		}
	}else if(sum==1){
		if (bricks[0]==1){
			LorR=1;
			bricks[0]=0;
			bricks[1]=1;
		} else if (bricks[1]==1){
			if(LorR<0){ //left
				bricks[1]=0;
				bricks[0]=1;
			}else{
				bricks[1]=0;
				bricks[2]=1;
			}
		} else if (bricks[2]==1){
			if(LorR<0){
				bricks[2]=0;
				bricks[1]=1;
			}else{
				bricks[2]=0;
				bricks[3]=1;
			}
		} else if (bricks[3]==1){
			LorR=-1;
			bricks[3]=0;
			bricks[2]=1;
		}
	}
}

void DropBricks(){
	for (int i=0; i<4;++i){
		if (oldBricks[i]==0){
			bricks[i]=0;	//drop any blocks that are unsupport
		}
	}

	for (int i=0; i<4; ++i){
			oldBricks[i]=bricks[i]; //make current row old row
	}
}



int main(void) {
	butt = Pressed();
	int counter = 0;

	LCD_PinSetup();
	Keypad_PinSetup();
	LCD_Init();
	LCD_DefineBrick();
	LCD_DefineDown();
	LCD_DefineUp();
    PINSEL0 |= (0b11 << 14);

    //en t2
    PCONP |= (1 << 22);
    //Toggle match bit
    T2EMR |= (0b11 << 6);
    T2MCR |= (1 << 4);
    //Start t2
    T2TCR = 1;
    T2TCR = 2;
    //reset
    T2TCR = 2;
    T2TCR = 1;

	//Splash sreen
	LCD_Test(1000000);
	SelectSpeed();

//	butt='0';
	LCD_Clear();
	LCD_Home();
	Sleep(200000);
//	LCD_Write("::");
//	LCDwrite(butt);
//	LCD_Write("::");
	butt='0';
//	Sleep(10000000);
	//loop

    while(1) {
    	Sleep(20000);

    	butt=Pressed();
    	counter++;

    	if(counter >= speed/2){
    		Tick();
    		DrawBricks(lvl); // update brick locations
    		counter=0;
    	}

    	if (butt != '0'){
    		DropBricks();
    		DrawBricks(lvl);

    		Sleep(60000); //2000 for debounce works.
    		butt='0';
    		++lvl;
    		Tick();

    		if(lvl>=21){//end game condition...
    			Sleep(10000);
    			EndGame(1);
    		}
    	}
    }
    return 0;
}
