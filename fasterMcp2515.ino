#include <SPI.h>
#include <mcp_can.h>

int const LED = D0;
#define CAN0_INT D5   // Set INT to pin 5
#define CAN0_CS  D6
MCP_CAN Can0(CAN0_CS);     // Set CS to pin 6

hw_timer_t * timer = NULL;

#define USE_CANWISER_ADDON
#define USE_CANWISER_HWIO

#ifdef USE_CANWISER_ADDON
  #ifdef USE_CANWISER_HWIO
    #define TX0RTS   D7
    #define TX1RTS   D1
    #define TX2RTS   D2
    #define RX0INT   D3
    #define RX1INT   D4
  #endif
  #include "CanwiserLite.hpp"
  CanWiserLite canwiser(CAN0_CS); // Set CS to pin 6
#endif

uint8_t timer1ms;
volatile uint8_t timer1msTriggered;

void ARDUINO_ISR_ATTR onTimer( void )
{
  // ----------------------------------------------------
  // 1 millisecond timer count
  // ----------------------------------------------------
  timer1ms++;
  if( timer1ms >= 10 )
  {
   timer1ms = 0;
   timer1msTriggered = true;
  }
 
}

void setup() {
  // put your setup code here, to run once:

  pinMode( TX0RTS, OUTPUT );
  pinMode( TX1RTS, OUTPUT );
  pinMode( TX2RTS, OUTPUT );
  pinMode( RX0INT, INPUT );
  pinMode( RX1INT, INPUT );
  digitalWrite( TX0RTS, HIGH );
  digitalWrite( TX1RTS, HIGH );
  digitalWrite( TX2RTS, HIGH );
  
  delay( 100 );
  Serial.begin(115200);
  while(!Serial) { }
  Serial.println( "CanWiser Lite - Faster MCP2515 SPI Interface" );
  Serial.println( "Version 3/14/2023 (emicros.com)" );  
  /* Setup SPI access */
  SPI.begin();
  SPI.setClockDivider( SPI_CLOCK_DIV2 );
  
  pinMode(CAN0_INT, INPUT);
  // Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
  if(Can0.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) Serial.println("MCP2515 Initialized Successfully!");
  else Serial.println("Error Initializing MCP2515...");

  Can0.setMode(MCP_NORMAL);  

  // timerInterrupt to run every 20 milliseconds
  // Use 1st timer of 4 (counted from zero).
  // Set 80 divider for prescaler (see ESP32 Technical Reference Manual for more
  // info).
  timer = timerBegin(0, 80, true);

  // Attach onTimer function to our timer.
  timerAttachInterrupt(timer, &onTimer, true); 

  // Set alarm to call onTimer function every .1 milliseconds (value in microseconds).
  // Repeat the alarm (third )
  timerAlarmWrite(timer, 100, true);

  timerAlarmEnable( timer );   
}

long unsigned int rxId;
unsigned char len = 0;
unsigned char rxBuf[8];
char msgString[128];                        // Array to store serial string

uint32_t canId;
uint8_t canExt;
uint8_t buffer[30];
uint32_t messageTime;

void checkForMessage( void )
{
  if(!digitalRead(CAN0_INT))                         // If CAN0_INT pin is low, read receive buffer
  {    
    #ifdef USE_CANWISER_ADDON 
      messageTime = micros();
      digitalWrite( LED, HIGH );
      canwiser.readCanData( &buffer[0], 14 );
      digitalWrite( LED, LOW );
  
      canExt = 0;
      canId = (buffer[0]<<3) + (buffer[1]>>5);
      if ( (buffer[1] & MCP_TXB_EXIDE_M) ==  MCP_TXB_EXIDE_M ) 
      {
                                                                          /* extended id                  */
          canId = (canId<<2) + (buffer[1] & 0x03);
          canId = (canId<<8) + buffer[2];
          canId = (canId<<8) + buffer[3];
          canExt = 1;
      }
      Serial.print( messageTime );
      Serial.print( "," );
      Serial.print( canId, HEX );
      Serial.print( "," );
      for( byte x=5; x<5+buffer[4]; x++ )
      {
        Serial.print( buffer[x], HEX );
        Serial.print( "," );
      }
      Serial.println();
    
    #else
    
      digitalWrite( LED, HIGH );
      Can0.readMsgBuf(&rxId, &len, rxBuf);      // Read data: len = data length, buf = data byte(s)
      digitalWrite( LED, LOW );   
      if((rxId & 0x80000000) == 0x80000000)     // Determine if ID is standard (11 bits) or extended (29 bits)
        sprintf(msgString, "Extended ID: 0x%.8lX  DLC: %1d  Data:", (rxId & 0x1FFFFFFF), len);
      else
        sprintf(msgString, "Standard ID: 0x%.3lX       DLC: %1d  Data:", rxId, len);
    
      Serial.print(msgString);
    
      if((rxId & 0x40000000) == 0x40000000){    // Determine if message is a remote request frame.
        sprintf(msgString, " REMOTE REQUEST FRAME");
        Serial.print(msgString);
      } else {
        for(byte i = 0; i<len; i++){
          sprintf(msgString, " 0x%.2X", rxBuf[i]);
          Serial.print(msgString);
        }
      }
          
      Serial.println();
    #endif    
  }  
}


bool onLine = true;
char inByte;

void loop() {
  // put your main code here, to run repeatedly:
  pinMode( LED, OUTPUT );
  for(;; )
  {
    if( Serial.available() > 0 )
    {
      inByte = Serial.read();
      if( inByte == '0' )
      {
        onLine = false;
      }
    }
    if( onLine == true )
    {
       checkForMessage();
    }
        
    if( timer1msTriggered == true )
    {
      timer1msTriggered = false;
      uint8_t data[] = {1,2,3,4,5,6,7,8};
      //digitalWrite( LED, HIGH );
      #ifndef USE_CANWISER_ADDON
        byte sndStat = Can0.sendMsgBuf(0x25, 0, 8, data);
      #else
        canwiser.sendCanMessage( 0, 0, 0x25, &data[0], 8 );
        canwiser.sendCanMessage( 1, 0, 0x35, &data[0], 4 );
        canwiser.sendCanMessage( 2, 0, 0x45, &data[0], 0 );
      #endif
      //digitalWrite( LED, LOW );
    }   
  }
}
