

#ifndef _CANWISERLITE_H_
#define _CANWISERLITE_H_

uint32_t baudrates[15] = {
    4096 ,
    5000 ,
    10000 ,
    20000 ,
    31250 ,
    33333 ,
    40000 ,
    50000 ,
    80000 ,
    100000 ,
    125000 ,
    200000 ,
    250000 ,
    500000 ,
    1000000 
};

#define UNSELECT() digitalWrite( CWCS, HIGH )
#define SELECT()   digitalWrite( CWCS, LOW )

typedef struct
{
  uint8_t  id[4];
  uint8_t  dlc;
  uint8_t  d[8];

} cms;

#define MCP_SIDH        0
#define MCP_SIDL        1
#define MCP_EID8        2
#define MCP_EID0        3

#define SPI_RATE 20000000

class CanWiserLite
{
	private:
	
    INT8U   CWCS;  // Chip Select pin number
	const uint8_t ctrl[3] = {MCP_TXB0CTRL, MCP_TXB1CTRL, MCP_TXB2CTRL};

	public:
	
	/*********************************************************************************************************
	** Function name:           CanWiserLite
	** Descriptions:            Public function to declare class and the /CS pin.
	*********************************************************************************************************/
	CanWiserLite(uint8_t _CS)
	{
		CWCS = _CS;
		UNSELECT();
		pinMode(CWCS, OUTPUT);
	}
  /*********************************************************************************************************
  ** Function name:           mcp2515_readStatus
  ** Descriptions:            Reads status register
  *********************************************************************************************************/
  uint8_t readStatus(void)                             
  {
      uint8_t i;
      SPI.beginTransaction(SPISettings(SPI_RATE, MSBFIRST, SPI_MODE0));
      SELECT();
      SPI.transfer(MCP_READ_STATUS);
      i = SPI.transfer(0);
      UNSELECT();
      SPI.endTransaction();
      return i;
  }	
	
	void readCanData( uint8_t *buffer, uint8_t len )
	{
    #ifndef CANWISER_USEHWIO	
    uint8_t stat = readStatus();
    if( stat & MCP_STAT_RX0IF )
    #else
    if( digitalRead( RX0INT ) == LOW )
    #endif
    {
  		SPI.beginTransaction(SPISettings(SPI_RATE, MSBFIRST, SPI_MODE0));
  		SELECT();
  		SPI.transfer(MCP_READ);
  		SPI.transfer(MCP_RXBUF_0);
  		SPI.transferBytes( NULL, buffer, len );
  		UNSELECT();
  		SPI.endTransaction();

      SPI.beginTransaction(SPISettings(SPI_RATE, MSBFIRST, SPI_MODE0));
      SELECT();
      SPI.transfer(MCP_BITMOD);
      SPI.transfer(MCP_CANINTF);
      SPI.transfer(MCP_RX0IF);
      SPI.transfer(0);
      UNSELECT();
      SPI.endTransaction();
    }
    #ifndef CANWISER_USEHWIO
    else if( stat & MCP_STAT_RX1IF )
    #else
    if( digitalRead( RX1INT ) == LOW )
    #endif
    {
      SPI.beginTransaction(SPISettings(SPI_RATE, MSBFIRST, SPI_MODE0));
      SELECT();
      SPI.transfer(MCP_READ);
      SPI.transfer(MCP_RXBUF_1);
      SPI.transferBytes( NULL, buffer, len );
      UNSELECT();
      SPI.endTransaction();

      SPI.beginTransaction(SPISettings(SPI_RATE, MSBFIRST, SPI_MODE0));
      SELECT();
      SPI.transfer(MCP_BITMOD);
      SPI.transfer(MCP_CANINTF);
      SPI.transfer(MCP_RX1IF);
      SPI.transfer(0);
      UNSELECT();
      SPI.endTransaction();
    }	
  }

	void writeCanData( uint8_t addr, uint8_t *buffer, uint8_t len )
	{	
		SPI.beginTransaction(SPISettings(SPI_RATE, MSBFIRST, SPI_MODE0));
		SELECT();
		SPI.transfer(MCP_WRITE);
		SPI.transfer(addr);
		SPI.transferBytes( buffer, NULL, len );
		UNSELECT();
		SPI.endTransaction();
	}
	
	void writeCanRegister( uint8_t addr, uint8_t data )
	{	
		SPI.beginTransaction(SPISettings(SPI_RATE, MSBFIRST, SPI_MODE0));
		SELECT();
		SPI.transfer(MCP_WRITE);
		SPI.transfer(addr);
		SPI.transfer(data);
		UNSELECT();
		SPI.endTransaction();
	}
	
	void sendCanMessage( uint8_t tx, const uint8_t ext, const uint32_t id, uint8_t *buffer, uint8_t len)
	{
		cms canMessage;
		uint16_t canid;
		uint8_t txnum = ctrl[tx];

		canid = (uint16_t)(id & 0x0FFFF);

		if ( ext == 1) 
		{
			canMessage.id[MCP_EID0] = (INT8U) (canid & 0xFF);
			canMessage.id[MCP_EID8] = (INT8U) (canid >> 8);
			canid = (uint16_t)(id >> 16);
			canMessage.id[MCP_SIDL] = (INT8U) (canid & 0x03);
			canMessage.id[MCP_SIDL] += (INT8U) ((canid & 0x1C) << 3);
			canMessage.id[MCP_SIDL] |= MCP_TXB_EXIDE_M;
			canMessage.id[MCP_SIDH] = (INT8U) (canid >> 5 );
		}
		else 
		{
			canMessage.id[MCP_SIDH] = (INT8U) (canid >> 3 );
			canMessage.id[MCP_SIDL] = (INT8U) ((canid & 0x07 ) << 5);
			canMessage.id[MCP_EID0] = 0;
			canMessage.id[MCP_EID8] = 0;
		}
		canMessage.dlc = len;
        for( uint8_t x=0; x<len; x++ )
		{
			canMessage.d[x] = buffer[x];
		}	
		writeCanData( txnum+1, (uint8_t *)&canMessage, len+5 );
		
		// request to send
		//writeCanRegister( txnum, 0x0c );
    if( tx == 0 )
    {
      digitalWrite( TX0RTS, LOW );
      digitalWrite( TX0RTS, LOW );
      digitalWrite( TX0RTS, HIGH );
    }
    else if( tx == 1 )
    {
      digitalWrite( TX1RTS, LOW );
      digitalWrite( TX1RTS, LOW );
      digitalWrite( TX1RTS, HIGH );
    }
    else if( tx == 2 )
    {
      digitalWrite( TX2RTS, LOW );
      digitalWrite( TX2RTS, LOW );
      digitalWrite( TX2RTS, HIGH );
    }

    
	}
};

#endif // #ifndef _CANWISERLITE_H_

// eof
