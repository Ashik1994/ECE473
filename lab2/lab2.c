// lab2.c 
// Mitchell Baertlein
// October 3, 2016

//  HARDWARE SETUP:
//  PORTA is connected to the segments the LED display and to the pushbuttons.
//  PORTA.7 corresponds to segment a, PORTA.6 corresponds to segement b, etc.
//  PORTB bits 4-6 go to a,b,c inputs of the 74HC138.
//  PORTB bit 7 goes to the PWM transistor base.
//  Connect y7 on the digit display to com_en on pushbutton board
//  PWM Resistor: 2.2k
//  SEL Resistor: 2.2k
//  SEG Resistor: 470 

#define F_CPU 16000000 // cpu speed in hertz 
#define TRUE 1
#define FALSE 0
#include <avr/io.h>
#include <util/delay.h>

//holds data to be sent to the segments. logic zero turns segment on
uint8_t segment_data[5]; 

//decimal to 7-segment LED display encodings, logic "0" turns on segment
uint8_t dec_to_seg[12]; 


//************************************************************************
//				Decimal to Seven Seg
//Takes the four digits held in segment_data and encodes it to work 
//with the seven seg board. These encodings are stored in dec_to_seg.

void dec_to_seven_seg() {
	// |segA|segB|segC|segD|segE|segF|segG|DP|
	int i = 0;  //used for the for loop
	for(i = 0; i < 5; i++) {  //Loop through all of the five digits
		if(segment_data[i] == 0x00) {dec_to_seg[i] = 0x03;}      //0
		else if(segment_data[i] == 0x01) {dec_to_seg[i] = 0x9F;} //1
        	else if(segment_data[i] == 0x02) {dec_to_seg[i] = 0x25;} //2
		else if(segment_data[i] == 0x03) {dec_to_seg[i] = 0x0D;} //3
		else if(segment_data[i] == 0x04) {dec_to_seg[i] = 0x99;} //4
		else if(segment_data[i] == 0x05) {dec_to_seg[i] = 0x49;} //5
		else if(segment_data[i] == 0x06) {dec_to_seg[i] = 0x41;} //6
		else if(segment_data[i] == 0x07) {dec_to_seg[i] = 0x1F;} //7
		else if(segment_data[i] == 0x08) {dec_to_seg[i] = 0x01;} //8
		else if(segment_data[i] == 0x09) {dec_to_seg[i] = 0x19;} //9
		else {dec_to_seg[i] = 0xFF;} //disply nothing.           //-
	}//for 
}

//************************************************************************

//************************************************************************
//                            chk_buttons                                      
//Checks the state of the button number passed to it. It shifts in 
//ones till the button is pushed. Function returns a 1 only once per 
//debounced button push so a debounce and toggle function can be 
//implemented at the same time. Adapted to check buttons from Ganssel's 
//"Guide to Debouncing" Expects active low pushbuttons on PINA port.  
//Debounce time is determined by external loop delay times 12. 

uint8_t chk_buttons(uint8_t b) {
	static uint16_t state[9]; //holds present state
  	state[b] = (state[b] << 1) | (! bit_is_clear(PINA, b)) | 0xE000;
  	if (state[b] == 0xF000) return 1;
  	return 0;
}
//************************************************************************

//************************************************************************
//                                   segsum                                   
//takes a 16-bit binary input value and places the appropriate equivalent
//4 digit BCD segment code in the array segment_data for display.                       
//array is loaded at exit as:  |digit3|digit2|colon|digit1|digit0|

void segsum(uint16_t sum) {
  	//break up decimal sum into 4 digit-segments
  	segment_data[0] = sum%10;        //ones place
  	segment_data[1] = (sum/10)%10;   //tens place
  	segment_data[2] = 0xff;          //colon
  	segment_data[3] = (sum/100)%10;  //hundreds place
  	segment_data[4] = (sum/1000)%10; //thousands place
        
  	//blank out leading zero digits
  	int x = 4;
  	for (x = 4; x>0; x--) {
		//loop through until we find a nonzero val; 
    		if(segment_data[x] == 0 | segment_data[x]==0xFF) {
			segment_data[x] = 0xFF; //display nothing
		}//if
    		else {x = 0;}	//if we find a non-zero we can exit loop
  	}//for   
}

//*************************************************************************
//* * * * * * * * * * * * * * * * M A I N * * * * * * * * * * * * * * * * *
//*************************************************************************

int main()
{
  	DDRB = 0xFF;         //set PORT B a output
	PORTB = 0x07; 	     //Disable the tristate buffer 
  	uint16_t count = 0;  //number to be displayed
  	uint8_t counter = 0; //which digit to be displayed
  	while(1){
    		_delay_ms(2);  		//loop delay for debounce
    		
		//Setup Pushbuttons
		DDRA = 0x00;   		//Make Port A an input
    		PORTA = 0xFF;  		//Enable pullups
		PORTB |= (0b11100000);  //Enable Tristate buffer (set y7 to 0)
		
		//now check each button and increment the count as needed
    		if(chk_buttons(8)){count++;}               //Give time for buff
    		else if (chk_buttons(7)){count=count+1;}   //check bit 7.
		else if (chk_buttons(6)){count=count+2;}   //check bit 6.
    		else if (chk_buttons(5)){count=count+4;}   //check bit 5.
    		else if (chk_buttons(4)){count=count+8;}   //check bit 4.
    		else if (chk_buttons(3)){count=count+16;}  //check bit 3.
    		else if (chk_buttons(2)){count=count+32;}  //check bit 2.
    		else if (chk_buttons(1)){count=count+64;}  //check bit 1.
    		else if (chk_buttons(0)){count=count+128;} //check bit 0.
		
		//Setup Segment Outputs	
		PORTB = 0x07; 		//Disable Tristate buffer
    		DDRA = 0xFF;  		//Make Port A output	  
		
		//Bind counts (number from 1-1023 and counter 0-4) 
    		if(count > 1023) {count=1;}      //Next in sequence is 1
    		segsum(count);  	         //Break up digits to 4 decimals
    		if (counter == 5) {counter = 0;} //Bound counter 0-4
    
    		//send 7 segment code to LED segments
    		dec_to_seven_seg();          //input data into dec_to_seg array
    		PORTB = counter<<5; 	     //top 4 bits of B control SEL0-2 
    		PORTA = dec_to_seg[counter]; //display digit
    		counter++;                   //increment digit to display
  	}//while
  	return 0;
}
