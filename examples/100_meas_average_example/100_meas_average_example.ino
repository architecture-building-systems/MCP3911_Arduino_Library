#include <SPI.h>
#include <arduino.h>
#include <MCP3911.h>

MCP3911 mcp3911;

int CLOCK_PIN = 9;  //Pin 9 is the OC1A-Pin of the Arduino Pro Mini and goes to OSC1-Pin of the MCP3911
int CS_PIN = 8;     //Pin 8 of the Arduino Pro Mini goes to the CS-Pin of the MCP3911

volatile uint8_t index = 0; //Array-Index
long values_ch0[100] = {};  //Array for the 100 measurements
 
void ch0_data_interrupt(void)   //Interrupt function
{
  values_ch0[index] = mcp3911.read_ch0_raw();
  index++;
  if(index > 99)
    mcp3911.enter_reset_mode(); //Enter reset mode to stop any more interrupts
}

void ch0_calibrate_offset(void)
{
  mcp3911.enter_reset_mode();   //Enter reset mode to prevent any measurements from being taken
  mcp3911.write_offset_ch0(0);  //Write initial offset to zero

  //Start 100 measurements with interrupts
  index = 0;              
  mcp3911.exit_reset_mode();    //Exit reset mode to start interrupts again.
  while(index < 100);           //Do nothing until 100 measurements are completed
  
  long average = 0;             //Calculate average of the 100 measurements  
  for(int i = 0; i<100; i++){            
    average += values_ch0[i];
  }
  long offset = (average/100) * -1;   //We multiply by -1 since the offset gets added,
                                      //but to be correct we want it subtracted

  mcp3911.write_offset_ch0(offset);   //Write offset to register
   
  float offset_volt = mcp3911.data_to_voltage(offset);
  Serial.print("Offset = ");
  Serial.print(offset_volt, 8);
  Serial.print("V\n");

  index = 0;
  mcp3911.exit_reset_mode(); //Exit reset mode to start interrupts again.
}                                     


void setup() {
  Serial.begin(9600);
  
  mcp3911.begin(CLOCK_PIN, CS_PIN);     //Initialize MCP3911
  mcp3911.generate_CLK();               //Generate 4MHZ clock on CLOCK_PIN

  REGISTER_SETTINGS settings = {};
 
  //PHASE-SETTINGS
  settings.PHASE    = 0;           //Phase shift between CH0/CH1 is 0
  //GAIN-SETTINGS
  settings.BOOST     = 0b10;       //Current boost is 1 
  settings.PGA_CH1   = 0b000;      //CH1 gain is 1
  settings.PGA_CH0   = 0b000;      //CH0 gain is 1
  //STATUSCOM-SETTINGS
  settings.MODOUT    = 0b00;       //No modulator output enabled
  settings.DR_HIZ    = 0b1;        //DR pin state is logic high when data is not ready
  settings.DRMODE    = 0b00;       //Data ready pulses from lagging ADC are output on DR-Pin
  settings.READ      = 0b10;       //Adress counter loops register types
  settings.WRITE     = 0b1;        //Adress counter loops entire register map
  settings.WIDTH     = 0b11;       //CH0 and CH1 are in 24bit-mode
  settings.EN_OFFCAL = 0b1;        //Digital offset calibration on both channels enabled
  settings.EN_GAINCAL = 0b0;       //Gain calibration on both channels disabled
  //CONFIG-SETTINGS
  settings.PRE       = 0b00;       //AMCLK = MCLK
  settings.OSR       = 0b011;      //Oversamplingratio is set to 256
  settings.DITHER    = 0b11;       //Dithering on both channels maximal
  settings.AZ_FREQ   = 0b0;        //Auto-zeroing running at lower speed
  settings.RESET     = 0b00;       //Neither ADC in Reset mode
  settings.SHUTDOWN  = 0b00;       //Neither ADC in Shutdown
  settings.VREFEXT   = 0b0;        //Internal voltage reference enabled
  settings.CLKEXT    = 0b1;        //External clock drive on OSC1-Pin enabled

  mcp3911.configure(settings);     //Configure the MCP3911 with the settings above

  attachInterrupt(digitalPinToInterrupt(3), ch0_data_interrupt, FALLING);  //Call "ch0_data_interrupt" whenever an edge on the DR-Pin occurs

  delay(100);                      //Add a delay until the interrupt routine has started

  ch0_calibrate_offset();          //Calibrate MCP3911
}

void loop() {
 //If 100 measurement interrupts have taken place  
 //calculate the average of all the measurements that were taken into "values_ch0[]".
 if(index > 99){              
    double average = 0;              
    for(int i = 0; i<100; i++){             
	    average += values_ch0[i];
    }
	  float voltage = mcp3911.data_to_voltage(average/100);
    Serial.print("Voltage CH0 = ");
    Serial.print(voltage, 8);
    Serial.print("V\n");

    index = 0;
    delay(1000);
    mcp3911.exit_reset_mode(); //Exit reset mode to start interrupts again.
 }
}
