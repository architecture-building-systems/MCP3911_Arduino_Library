/*
  MCP3911.cpp - Library for the MCP3911.
  Version 1.0
  Created by Ruben J. Stadler, May 10, 2018.
  <rstadler@ethz.ch>
  
  This library is licensed under the MIT license.
*/

#include "Arduino.h"
#include "SPI.h"
#include "MCP3911.h"

MCP3911::MCP3911(){
//Constructor
}

//Initializes SPI and saves pin variables
void MCP3911::begin(int _CLOCK_PIN, int _CS_PIN)
{
	CLOCK_PIN = _CLOCK_PIN;
	CS_PIN = _CS_PIN;
	
	pinMode(CS_PIN, OUTPUT);  	//Define CS-Pin as output
	digitalWrite(CS_PIN, HIGH);
  
	SPI.begin();
	//Use the maximum serial clock frequency of 20MHz, MSB first mode and
	//- SPI_MODE0 for low clock idle, output edge falling, data capture rising
	//- SPI_MODE3 for high clock idle, output edge falling, data capture rising
	SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
}

//Generates 4MHZ-Clock at pin 0C1A. 
void MCP3911::generate_CLK(void)
{
	pinMode(CLOCK_PIN, OUTPUT);             //We can only toggle Output Compare Pin OC1A
	//Set CS1[2:0] in Timer1ControlRegisterB to '001' to count without a prescaler.
	//Set WGM1[2:0] in Timer1ControlRegisterB to '010' to use CTS operation mode.
	//(This means we only count up to to the value in the OCR1A-Register.)
	TCCR1B = (1 << CS10) | (1 << WGM12);
	TCCR1A = (1 << COM1A0);                 //Set COM1A[1:0] to '01' to toggle the OC1A-Register on Compare Match with OCR1A.
	TIMSK1 = 0;                             //Disable all Interrupts for Timer1
	OCR1A = 0;                              //Set Output Compare Register A to 0. This means we divide our internal clock by factor 2, generating 4 MHZ.
	delay(200);                             //Necessary delay, since it takes some time until the clock starts.
}

//Reset and configure the MCP3911 with user-defined settings
void MCP3911::configure(REGISTER_SETTINGS _settings)
{
	settings = _settings;							//Load settings into private variable
	
	//Put both ADC's into reset mode to enable a configuration in one write-cycle.
	digitalWrite(CS_PIN,LOW);
	SPI.transfer(ADDR_BITS | (REG_CONFIG2 << 1));  //Control Byte: Choose CONFIG2-Register
	SPI.transfer(0b11000000);                      //Write RESET<1:0> to '11' to reset both ADC's
	digitalWrite(CS_PIN,HIGH);

	//Start write-cycle and configure all necessary registers
	digitalWrite(CS_PIN,LOW);
	SPI.transfer(ADDR_BITS | (REG_PHASE << 1));     //Control Byte: Choose phase register to start cycling through all registers.
	SPI.transfer16(settings.PHASE);                 //Write PHASE-Register
	SPI.transfer((settings.BOOST << 6)|             //Write GAIN-Register
                 (settings.PGA_CH1 << 3)|
                  settings.PGA_CH0);       
	SPI.transfer16((settings.MODOUT << 14)|         //Write STATUSCOM-Register
				   (settings.DR_HIZ << 12)|
                   (settings.DRMODE << 10)|
                   (settings.READ << 6)|
                   (settings.WRITE << 5)|
                   (settings.WIDTH << 3)|
                   (settings.EN_OFFCAL << 2)|
                   (settings.EN_GAINCAL << 1));   
	SPI.transfer16((settings.PRE << 14)|            //Write CONFIG-Register
                   (settings.OSR << 11)|
                   (settings.DITHER << 9)|
                   (settings.AZ_FREQ << 8)|
                   (settings.RESET << 6)|
                   (settings.SHUTDOWN << 4)|
                   (settings.VREFEXT << 2)|
                   (settings.CLKEXT << 1));  
	digitalWrite(CS_PIN,HIGH);
}

//Read chosen channel, convert it to a readable form and return it. 
//Function only usable in 24-bit mode.
float MCP3911::read_chX(uint8_t channel)
{
	digitalWrite(CS_PIN, LOW);
	SPI.transfer(ADDR_BITS | (channel << 1) | 1); //Control Byte
	
	byte upper = SPI.transfer(0x00);			   //Read the three bytes
	byte middle = SPI.transfer(0x00);
	byte lower = SPI.transfer(0x00);
	
	digitalWrite(CS_PIN, HIGH);
	
	//Concat the three bytes.
	//When I cast the three bytes to long, they will be shifted with respect to their sign. 
	//The back-shift by 8 bit at the end makes sure that the sign is at bit 32 instead of bit 24.
	long combined_value = ((((long)upper << 24) | ((long)middle << 16) | (long)lower<<8) >> 8); 
 
    //Conversion to readable form according to datasheet
	float voltage = data_to_voltage(combined_value, channel);
	
	return voltage;
}


//Read chosen channel and return raw data. 
//Function only usable in 24-bit mode.
long MCP3911::read_raw_data(uint8_t channel)
{
	digitalWrite(CS_PIN, LOW);
	SPI.transfer(ADDR_BITS | (channel << 1) | 1); //Control Byte
	
	byte upper = SPI.transfer(0x00);			   //Read the three bytes
	byte middle = SPI.transfer(0x00);
	byte lower = SPI.transfer(0x00);
	
	digitalWrite(CS_PIN, HIGH);
	
	//Concat the three bytes.
	//When I cast the three bytes to long, they will be shifted with respect to their sign. 
	//The back-shift by 8 bit at the end makes sure that the sign is at bit 32 instead of bit 24.
	long combined_value = ((((long)upper << 24) | ((long)middle << 16) | (long)lower<<8) >> 8); 
	
	return combined_value;
}


//Takes a 24-bit value and calculates voltage from it.
float MCP3911::data_to_voltage(long data, uint8_t channel)
{
	uint8_t gain = 0;
	
	if(channel == REG_CHANNEL0)
		gain = settings.PGA_CH0;
	else if(channel == REG_CHANNEL1)
		gain = settings.PGA_CH1;
	else{
		Serial.println("Entered wrong register at data_to_voltage-function");
		return 0;
	}
	
	//Depending on which gain was chosen for the channel, the conversion is different
	switch(gain){
		case 0b111:	gain = 1;
					break;
		case 0b110: gain = 1;
					break;
		case 0b101: gain = 32;
					break;
		case 0b100: gain = 16;
					break;
		case 0b011: gain = 8;
					break;
		case 0b010: gain = 4;
					break;
		case 0b001: gain = 2;
					break;
		case 0b000: gain = 1;
					break;
		default:	gain = 0;
	}
	float voltage = (data * 1.2)/(8388608*1.5*gain);  	//Conversion according to datasheet.
	return voltage;
}



//Enter reset mode on both Channels
void MCP3911::enter_reset_mode(void)
{
	uint8_t value_conf2 = read_register(REG_CONFIG2); //Read configuration of CONFIG2-Register, so nothing gets lost
	//Put both ADC's into reset mode
    digitalWrite(CS_PIN,LOW);
    SPI.transfer(ADDR_BITS | (REG_CONFIG2 << 1));  //Control Byte: Choose CONFIG2-Register
    SPI.transfer(0b11000000 | value_conf2);        //Write RESET<1:0> to '11' to reset both ADC's
    digitalWrite(CS_PIN,HIGH);
}

//Exit reset mode on both Channels
void MCP3911::exit_reset_mode(void)
{
	uint8_t value_conf2 = read_register(REG_CONFIG2); //Read configuration of CONFIG2-Register, so nothing gets lost	
	//Exit both ADC's from reset mode
    digitalWrite(CS_PIN,LOW);
    SPI.transfer(ADDR_BITS | (REG_CONFIG2 << 1));  //Control Byte: Choose CONFIG2-Register
    SPI.transfer(0b00111111 & value_conf2);        //Write RESET<1:0> to '00' to exit both ADC's reset mode
    digitalWrite(CS_PIN,HIGH);
}

//Write given offset to offset register of either CH0 or CH1
void MCP3911::write_offset(long offset, uint8_t channel)
{
	digitalWrite(CS_PIN, LOW);
	SPI.transfer(ADDR_BITS | (channel << 1)); //Control Byte
	SPI.transfer(offset >> 16);
	SPI.transfer(offset >> 8);
	SPI.transfer(offset >> 0);
	digitalWrite(CS_PIN, HIGH);
}	

//Read a register and return its value
uint8_t MCP3911::read_register(uint8_t reg)
{
	digitalWrite(CS_PIN, LOW);
	SPI.transfer(ADDR_BITS | (reg << 1) | 1); //Control Byte
	byte result = SPI.transfer(0x00);			   
	digitalWrite(CS_PIN, HIGH);
	
	return result;
}

