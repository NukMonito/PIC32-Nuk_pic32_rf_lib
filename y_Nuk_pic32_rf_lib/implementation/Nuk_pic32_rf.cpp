// ****************************************************************************
// File:  Rf.cpp
// Autor: Lluis
// Descripcio
//      Implemantacio de la clase Rf
//




// ****************************************************************************
// Espai
//
using namespace std;




// ****************************************************************************
// Includes
//
#include "Nuk_pic32_rf.h"




// ****************************************************************************
// Implementacio de la clase Rf

// ----------------------------------------------------------------------------
Rf::~Rf() {
	/*
	DESTRUCTOR

	*/

	}


// ----------------------------------------------------------------------------
Rf::Rf(
	char pch,
	const char* prxdir,
	const char* ptxdir,
	int ppayload_len,
	Spi_mstr& pspi,
	Digital& pce,
	Digital& pcsn,
	Digital& pirqrf ) :

Fifo() {
	/*
	CONSTRUCTOR
		Contrueix l'objecte RF
 
	PARMETRES:
		char* prxdir
			5 chars Identificacio del dispositiu

		char ptxdir
			5 chars Identificacio del destinatari

		int ppayload_len
			Tamany dels payload

		unsigned int& pce
			Port i bit que s'ha de posar a down per activar CE del modul

		unsigned int& pcsn
			Port i bit que s'ha de posar a down per activar CSN del modul

		unsigned int& pirqf
			Port i bit per on s'espera la IRQ del modul

	RETORN:
		void
	*/

	// 'i' Contador de proposit general
	// 'tspi' Transmisio via SPI
	// 'rspi' Resposta   via SPI
	int   i;
	char  tspi[6];
	char* rspi;
	char  status;
	Timer timer(TIMER1);

	// Registre SPI
	spi   = &pspi;
	ce    = &pce;
	csn   = &pcsn;
	irqrf = &pirqrf;

	// Fem que spi estigui deseleccionat
	// Fem que el modul estigui deseleccionat
	ce->off();
	csn->off();

	// Chanel
	// Destinaatari
	// Receptor
	// Tamamayn del msg
	ch = pch;
	for(i=0;i<5;i++) rxdir[i] = prxdir[i];
	for(i=0;i<5;i++) txdir[i] = ptxdir[i];
	payload_len = ppayload_len;
	if( payload_len>RF_MAXTAM ) payload_len = RF_MAXTAM;

	// Down
	tspi[0] = RF_W_REGISTER|RF_CONFIG;
	config &= ~(0x0002);
	tspi[1] = config;
	spi->transmit(*csn,tspi,2);
	timer.delay(16000);

	// Get default config from the module
	tspi[0] = RF_R_REGISTER|RF_CONFIG;
	rspi = spi->transmit(*csn,tspi,2);
	config = rspi[1];

	// Up
	tspi[0] = RF_W_REGISTER|RF_CONFIG;
	config |= 0x02;
	tspi[1] = config;
	spi->transmit(*csn,tspi,2);
	timer.delay(16000);

	// Dissable transmit interrupts
	tspi[0] = RF_W_REGISTER|RF_CONFIG;
	config |= 0x30;
	tspi[1] = config;
	spi->transmit(*csn,tspi,2);

	// Transmit address
	tspi[0] = RF_W_REGISTER|RF_TX_ADDR;
	for(i=0;i<5;i++) tspi[i+1] = txdir[i];
	spi->transmit(*csn,tspi,6);

	// Payload 0 address
	tspi[0] = RF_W_REGISTER|RF_RX_ADDR_P0;
	for(i=0;i<5;i++) tspi[i+1] = txdir[i];
	spi->transmit(*csn,tspi,6);

	// Payload 1 address
	tspi[0] = RF_W_REGISTER|RF_RX_ADDR_P1;
	for(i=0;i<5;i++) tspi[i+1] = rxdir[i];
	spi->transmit(*csn,tspi,6);

	// Bytes in payload 1
	// Bytes del msg, demanats per l'usuari, mes 1 de longitud
	tspi[0] = RF_W_REGISTER|RF_RX_PW_P1;
	tspi[1] = payload_len+1;
	spi->transmit(*csn,tspi,2);

	// RF Channel
	tspi[0] = RF_W_REGISTER|RF_CH;
	tspi[1] = ch;
	spi->transmit(*csn,tspi,2);

	// Automatic Retransmission 250 us 15 times
	tspi[0] = RF_W_REGISTER|RF_SETUP_RETR;
	tspi[1] = 0x0F;
	spi->transmit(*csn,tspi,2);

	// Enable interrupt
	irqrf->onchange(this,0);

	// Rx mode
	tspi[0] = RF_W_REGISTER|RF_CONFIG;
	config |= 1;
	tspi[1] = config;
	spi->transmit(*csn,tspi,2);
	ce->on();

	return;
	}


// ----------------------------------------------------------------------------
bool
Rf::put(const char* cadena,bool bdiscard) {
	/*
	FUNCIO:
		Transmiteix per RF una cadena de caracters acabada amb NULL

	PARAMETRES:
		char* cadena
			Cadena a transmitir

		bool bdiscard (opcional)
			false == retransmeteix idefinidament
			true == si la comunicacio falla, deixa de transmitir

	RETORN:
		bool
			true == la transmisio ha tingut exit
			false == la transmisio no ha tingut exit
	*/

	// Preparem el retorn
	bool r = true;

	// Transmitim la cadena
	// Fem tans paquets com sigui necessari per transmitir tota la cadena
	int j = 0;
	int i = 0;
	while( (cadena[j+i]!=0) && r ) {
		if( i==payload_len) {
			r = transmit(((char*)cadena)+j,payload_len,bdiscard);
			i = 0;
			j += payload_len;
			}
		else i++;
		}

	// Darrer paquet, si queda una resta
	if( (i>0) && r ) r = transmit(((char*)cadena)+j,i,bdiscard);

	return r;
	}


// ----------------------------------------------------------------------------
bool
Rf::put(char* cadena,int size,bool bdiscard) {
	/*
	FUNCIO:
		Transmiteix per RF 'size' caracters de la cadena 'cadena'

	PARAMETRES:
		char* cadena
			Caracters a transmitir

		unsigned int size
			Nombre de caracters a transmitir

		bool bdiscard (opcional)
			false == retransmeteix idefinidament
			true == si la comunicacio falla, deixa de transmitir

	RETORN:
		bool
			true == la transmisio ha tingut exit
			false == la transmisio no ha tingut exit
	*/

	// Preparem el retorn
	bool r = true;

	// Transmitim la cadena
	// Fem tans paquets com sigui necessari per transmitir tota la cadena
	unsigned int i = 0;
	while( (size>payload_len) && r) {
		r = transmit(cadena+i,payload_len,bdiscard);
		size -= payload_len;
		i += payload_len;
		}

	// Darrer paquet, si queda una resta
	if( size>0 && r) r = transmit(cadena+i,size,bdiscard);

	return r;
	}


// ----------------------------------------------------------------------------
bool
Rf::put_char(char caracter,bool bdiscard) {
	/*
	FUNCIO:
		Transmiteix per RF un caracter

	PARAMETRES:
		char caracter
		Caracter a transmitir

		bool bdiscard (opcional)
			false == retransmeteix idefinidament
			true == si la comunicacio falla, deixa de transmitir

	RETORN:
		bool
			true == la transmisio ha tingut exit
			false == la transmisio no ha tingut exit
	*/

	return transmit(&caracter,1,bdiscard);
	}


// ----------------------------------------------------------------------------
bool
Rf::put_line(const char* cadena,bool bdiscard) {
	/*
	FUNCIO:
		Transmiteix per RF una cadena de caracters acabada amb NULL
		Finalment transmeteix un EOL

	PARAMETRES:
		char* cadena
		Cadena a transmitir

		bool bdiscard (opcional)
			false == retransmeteix idefinidament
			true == si la comunicacio falla, deixa de transmitir

	RETORN:
		bool
			true == la transmisio ha tingut exit
			false == la transmisio no ha tingut exit
	*/

	// Preparem el retorn
	bool r;

	r = put(cadena,bdiscard);
	if( r )  r = put_char(EOL,bdiscard);

	return r ;
	}


// ----------------------------------------------------------------------------
bool
Rf::put_var(const char* cp, void* p, ...) {
	/*
	FUNCIO:
		Transmiteix per RF una llista variable de parametres
		Fa una cadena de la llista i la transmeteix

	PARMETRES:
		char* cp
			Cadena que informa dels tipus del parametres variables
				'c' char
				's' cadena
				'e' cadena (incloura un EOL
				'i' integer
				'l' long
				'd' double
		...
			Llista de parametres variable

	RETORN:
		void

	NOTA:
		Es responsabilitat del usuari que la cadena reflexi fidelment
		el contingut de la llista de variables.
	*/

	// Parametres variables
	va_list ar;
	va_start(ar,p);

	// Auxiliars per captar els bytes de cada tipus
	// dels parametres variables
	char* s;
	int i;
	long l;
	double d;

	// 'msg' Encadenat dels bytes dels paramametres variables
	// 'bytes' Nombre de bytes del parametre
	// 'count' bytes totals de la cadena
	char msg[BUFF_SIZE];
	int bytes = 0;
	int count = 0;

	// Construim el missatge
	// Per cada caracter de cp ...
	for(int ii=0;cp[ii]!=0;ii++) {
		switch( cp[ii] ) {

			case 'c':
				if( ii==0 ) memcpy(msg+count,(char*)p,1);
				else memcpy(msg+count,va_arg(ar,char*),1);
				bytes++;
				break;

			case 's':
				if( ii==0 ) s = (char*)p;
				else s = va_arg(ar,char*);
				memcpy(msg+count,s,(bytes = strlen(s)));
				break;

			case 'e':
				if( ii==0 ) s = (char*)p;
				else s = va_arg(ar,char*);
				memcpy(msg+count,s,(bytes = strlen(s)));
				msg[count+1] = EOL;
				bytes++;
				break;

			case 'i':
				if( ii==0 ) i = *((int*)p);
				else i = *(va_arg(ar,int*));
				memcpy(msg+count,&i,(bytes = sizeof(int)));
				break;

			case 'l':
				if( ii==0 ) l = *((long*)p);
				else l = *(va_arg(ar,long*));
				memcpy(msg+count,&l,(bytes = sizeof(long)));
				break;

			case 'd':
				if( ii==0 ) d = *((double*)p);
				else d = *(va_arg(ar,double*));
				memcpy(msg+count,&d,(bytes = sizeof(double)));
				break;

			default:
				break;
			}
		count += bytes;
		}

	// Transmitim
	bool r = false;
	if( count ) r = put(msg,count);

	return r;
	}


// ----------------------------------------------------------------------------
bool
Rf::transmit(char* msg,char size,bool bdiscard) {
	/*
	FUNCIO:
		Transmisio per el modul RF

	PARMETRES:
		char* msg
			Missatge a transmitir

		char size
			Nombre de caracters a transmitir

		bool bdiscard
			flase == retransmeteix idefinidament si la transmisio falla
			true == si la comunicacio falla, la cancela

	RETORN:
		bool
			true == la transmisio ha tingut exit
			false == la transmisio no ha tingut exit
	*/

	// Preparem el retorn
	bool r = true;

	// Mirem que 'size' no sigui major a 'payload_len'
	if( size>payload_len ) size = payload_len;

	// 'i' Contador de proposit general
	// 'tspi' Transmisio via SPI
	// 'rspi' Resposta   via SPI
	int   i;
	char  tspi[33];
	char* rspi;

	// Standby mode
	ce->off();

	// Load missage on module
	// 'tspi' format:
	//		[0]   = Cmd for rf module
	//    [1]   = Message length
	//		[2..] = Message
	tspi[0] = RF_W_TX_PAYLOAD;
	tspi[1] = size;
	for(i=0;i<size;i++) tspi[i+2] = msg[i];
	spi->transmit(*csn,tspi,payload_len+2);

	// Tx mode
	tspi[0] = RF_W_REGISTER|RF_CONFIG;
	config &= 0xFE;
	tspi[1] = config;
	spi->transmit(*csn,tspi, 2);
	ce->on();

	// Wait for transmision until no data on FIFO
	// or for a maximun of number of TX retransmits
	// 	flag if the user wants no retry
	char status;
	do {

		// If a maximun of number of TX retransmits flag is set
		tspi[0] = RF_NOP;
		rspi = spi->transmit(*csn,tspi,1);
		status = rspi[0];
		if( (status&0x10)!=0 ) {
			ce->off();

			// If user wants discartad the paquet
			if( bdiscard ) {

				// Flush (discard payload)
				tspi[0] = RF_FLUSH_TX;
				spi->transmit(*csn,tspi,1);
				r = false;
				}

			// User wants retry forever
			else {

				// Clear counters	and retransmit
				tspi[0] = RF_W_REGISTER|RF_CH;
				tspi[1] = ch;
				spi->transmit(*csn,tspi,2);

				// Clear interrupts flag of module
				tspi[0] = RF_W_REGISTER|RF_STATUS;
				tspi[1] = 0x70;
				spi->transmit(*csn,tspi,2);

				ce->on();
				}
			}

		} while( ((status&0x20)==0) && r );

	// Clear interrupts flag of module
	tspi[0] = RF_W_REGISTER|RF_STATUS;
	tspi[1] = 0x70;
	spi->transmit(*csn,tspi,2);

	// Rx mode
	tspi[0] = RF_W_REGISTER|RF_CONFIG;
	config |= 1;
	tspi[1] = config;
	spi->transmit(*csn,tspi,2);
	ce->on();

	return r;
	}


// ----------------------------------------------------------------------------
void
Rf::callback(int) {

	/*
	FUNCIO INTERNA:
		El modul ha enviat una Irq per el pin
		El modul ha rebut, al manco, un paquet
		Baixem tots els paquets del modul i l'insertem a la nostra FIFO

	PARMETRES:
		int
			dummy

	RETORN:
		void
	*/

	// 'i' Contador de proposit general
	// 'tspi' Trasmisio via SPI
	// 'rspi' Resposta  via SPI
	int i;
	char  tspi[33];
	char* rspi;

	// Si 'status' == Data Ready RX FIFO interrupt
	tspi[0] = RF_NOP;
	rspi = spi->transmit(*csn,tspi,1);
	if( rspi[0]&0x40 ) {

		// Mentre hagin missatges
		do {

			// Baixem el missatge del modul i l'insertem a la nostra FIFO
			// 'rspi' te el format:
			// 	[0]  = status
			//		[1]  = tamany
			//		[2-] = misatge
			tspi[0] = RF_R_RX_PAYLOAD;
			rspi = spi->transmit(*csn,tspi,payload_len+2);
			insert(rspi+2,rspi[1]);

			// Mirem si hi han mes missatges a la FIFO del modul
			tspi[0] = RF_R_REGISTER|RF_FIFO_STATUS,
			rspi = spi->transmit(*csn,tspi,2);
			} while( (rspi[1]&0x01)==0 );
		}

	// Clear de la interrupcio en el modul
	tspi[0] = RF_W_REGISTER|RF_STATUS;
	tspi[1] = 0x70;
	spi->transmit(*csn,tspi,2);

	// Continuem Rx
	ce->on();

	return;
	}


// ----------------------------------------------------------------------------
void
Rf::carrier(void) {
	/*
	FUNCIO:
		Transmisio de portadora RF nRF24L01+

	PARMETRES:
		void

	RETORN:
		void
	*/

	// 'i' Contador de proposit general
	// 'tspi' Transmisio via SPI
	// 'rspi' Resposta   via SPI
	int   i;
	char  tspi[33];
	char* rspi;

	// Dissable interrupt
	irqrf->onchange(NULL);

	// Standby
	ce->off();

	// TX mode
	tspi[0] = RF_W_REGISTER|RF_CONFIG;
	config &= 0xFE;
	tspi[1] = config;
	spi->transmit(*csn,tspi, 2);

	// Continuous carrier transmit
	tspi[0] = RF_R_REGISTER|RF_SETUP;
	rspi = spi->transmit(*csn,tspi, 2);

	tspi[0] = RF_W_REGISTER|RF_SETUP;
	tspi[1] = rspi[1] |= 0x90;
	spi->transmit(*csn,tspi, 2);

	ce->on();

	while(true);

	return;
	}

