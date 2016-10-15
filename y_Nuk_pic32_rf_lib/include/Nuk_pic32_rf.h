// ****************************************************************************
// File:  Rf.h
// Autor: Lluis
// Descripcio
//      Declaracio de la clase Rf
//


// ****************************************************************************
// Includes
//
#include "Nuk_pic32_essentials.h"

#ifndef NUK_PIC_RF
#define NUK_PIC_RF




// ****************************************************************************
// Definicions
//

// Comandaments del modul RF
#define RF_R_REGISTER         0b00000000
#define RF_W_REGISTER         0b00100000
#define RF_R_RX_PAYLOAD       0b01100001
#define RF_W_TX_PAYLOAD       0b10100000
#define RF_FLUSH_TX           0b11100001      
#define RF_FLUSH_RX           0b11100010
#define RF_REUSE_TX_PL        0b11100011
#define RF_ACTIVATE           0b01010000
#define RF_0X73               0x73
#define RF_R_RX_PL_WID        0b01100000
#define RF_W_ACT_PAYLOAD      0b10101000
#define RF_W_TX_PAYLOAD_NOACK 0b10110000
#define RF_NOP                0b11111111

//Registres del modul RF
#define RF_CONFIG      0x00
#define RF_EN_AA       0x01
#define RF_EN_RXADDR   0x02
#define RF_SETUP_AW    0x03
#define RF_SETUP_RETR  0x04
#define RF_CH          0x05
#define RF_SETUP       0x06
#define RF_STATUS      0x07
#define RF_OBSERVE_TX  0x08
#define RF_CD          0x09
#define RF_RX_ADDR_P0  0x0A
#define RF_RX_ADDR_P1  0x0B
#define RF_RX_ADDR_P2  0x0C
#define RF_RX_ADDR_P3  0x0D
#define RF_RX_ADDR_P4  0x0E
#define RF_RX_ADDR_P5  0x0F
#define RF_TX_ADDR     0x10
#define RF_RX_PW_P0    0x11
#define RF_RX_PW_P1    0x12
#define RF_RX_PW_P2    0x13
#define RF_RX_PW_P3    0x14
#define RF_RX_PW_P4    0x15
#define RF_RX_PW_P5    0x16
#define RF_FIFO_STATUS 0x17
#define RF_DYNPD       0x1C
#define RF_FEATURE     0x1D

// Maxim tamany d'un missatge per el modul RF
#define RF_MAXTAM 30




// ****************************************************************************
// Declaracio de la clase Rf
//
class Rf : public Callable, public Fifo {

	public:

	// Atributs
        
	// 'rxdir' Identificacio del dispositiu
	// 'txdir' Identificacio del destinatari
	// 'payload_len' tamany del payload
	char rxdir[5];
	char txdir[5];
	int  payload_len;

	// 'spi_obj'    Objecte SPI
	// 'port_ce'    Port CE
	// 'port_csn'   Port CSN
	// 'port_irq'   Port IRQ
	Spi_mstr* spi;
	Digital* ce;
	Digital* csn;
	Digital* irqrf;

	//Registres del modul
	char config;
	char ch;

	// Destructor
	~Rf();

	// Constructors
	Rf(char,const char*,const char*,int,Spi_mstr&,Digital&,Digital&,Digital&);

	// Funcions
	bool put(const char*,bool=true);
	bool put(char*,int,bool=true);
	bool put_char(char,bool=true);
	bool put_line(const char*,bool=true);
	bool put_var(const char* cp, void* p, ...);
	bool transmit(char*,char,bool);
	void callback(int);
	void carrier(void);
	};



 
#endif // NUK_PIC_RF



