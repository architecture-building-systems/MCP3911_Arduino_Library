/*
  MCP3911.h - Library for the MCP3911.
  Version 1.0
  Created by Ruben J. Stadler, May 10, 2018.
  <rstadler@ethz.ch>
  
  This library is licensed under the MIT license.
*/

#ifndef MCP3911_h
#define MCP3911_h

#include "Arduino.h"
#include "SPI.h"

//MCP3911 Register Map
#define ADDR_BITS       0x00            //Hardware adress of ADC.
#define REG_CHANNEL0  	0x00
#define REG_CHANNEL1	0x03
#define REG_PHASE       0x07
#define REG_GAIN        0x09
#define REG_STATCOM1    0x0A
#define REG_STATCOM2    0x0B
#define REG_CONFIG1     0x0C
#define REG_CONFIG2     0x0D
#define REG_OFFCAL_CH0  0x0E
#define REG_OFFCAL_CH1  0x14

struct REGISTER_SETTINGS{
    //PHASE-SETTINGS
    uint16_t PHASE;             
    //GAIN-SETTINGS
    uint8_t BOOST;          
    uint8_t PGA_CH1;         
    uint8_t PGA_CH0;         
    //STATUSCOM-SETTINGS
    uint8_t MODOUT;        
    uint8_t DR_HIZ;        
    uint8_t DRMODE;           
    uint8_t READ;             
    uint8_t WRITE;             
    uint8_t WIDTH;             
    uint8_t EN_OFFCAL;         
    uint8_t EN_GAINCAL;        
    //CONFIG-SETTINGS
    uint8_t PRE;             
    uint8_t OSR;            
    uint8_t DITHER;         
    uint8_t AZ_FREQ;           
    uint8_t RESET;            
    uint8_t SHUTDOWN;        
    uint8_t VREFEXT;           
    uint8_t CLKEXT;          
}; 


class MCP3911
{
	public:
		MCP3911();
	    void begin(int _CLOCK_PIN, int _CS_PIN);
		void generate_CLK(void);
		void configure(REGISTER_SETTINGS setting);
		float read_chX(uint8_t channel);
	    long read_raw_data(uint8_t channel);
		float data_to_voltage(long data, uint8_t channel);
		void enter_reset_mode();
		void exit_reset_mode();
		void write_offset(long offset, uint8_t channel);
		uint8_t read_register(uint8_t reg);
	
	private:
		int CLOCK_PIN;
		int CS_PIN;
		REGISTER_SETTINGS settings;
};

#endif