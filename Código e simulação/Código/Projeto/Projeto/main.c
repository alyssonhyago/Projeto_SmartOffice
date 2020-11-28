/*
 * USART.c
 *
 * Created: 19/10/2020 19:49:34
 * Author : Alysson Hyago Pereira de Oliveira e Pedro Henrique Agra Alexandre
 *
 *	Projeto: SmartOffice
 */ 

#define  F_CPU 16000000UL 
#define BAUD 9600 //bps
#define MYUBRR F_CPU/16/BAUD-1 // modo normal assincrono formula slider 

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "nokia5110.h"
#include <avr/eeprom.h>

// definições para o display 
#define tam_vetor 5
unsigned char leitura_ADC_string[tam_vetor];

// variavels de controle para a temperatura e ADC
uint16_t leitura_ADC = 0 ;
float Celsius;

// Prototipo função
void int_to_str(uint16_t s, unsigned char *d);

// -- Função de vetores de interrupção externa
ISR(INT0_vect); //  INT EXT 0
ISR(USART_RX_vect);// para o protocolo USART

//Interrupção ADC
ISR(ADC_vect)
{
	leitura_ADC = ADC;
}

// Função para inicialização da USART
void USART_Init(unsigned int ubrr)
{
	UBRR0H = (unsigned char)(ubrr>>8); // taxa de transmissao 
	UBRR0L = (unsigned char)ubrr;
	UCSR0B = 0b10011000;// habilita a transmissao e recepção  e as interrupção de recepção
	UCSR0C = 0b00001110; //ajusta a forma do frame: 8 bits e 2 paradas 
}

//TRansmissao de dados de 5 a 8 bits
void USART_transmit(unsigned char data)
{
	while(!(UCSR0A = 0b00100000));// so envia se chegar algo 
	 UDR0 = data; // coloca da do no reg. e o envia 
}
unsigned char USART_Receive(void)
{
	while(!(UCSR0A = 0b00100000));//espera o dado ser recebido
	return UDR0;// lê o dado e retorna 
}

// variaveis de controle dos if e PWM
int i = 0;
int test = 1;


int main(void)
{
	
	//Portas GPIO
	DDRD = 0b01000001; // todos os pinos da porta D como entrada, somente o 6 e o 0 como saida
	DDRC = 0b01100001; // pinos 6 , 5 e 0 como saida
	PORTD= 0b00000100;// GPIO para interrupção Pino D2
	PORTC = 0x00; //todos com nivel baixo 
	
	//configuração das interrupções
	EICRA = 0b00000010;	// configurando como borda de descida
	EIMSK = 0b00000001; //habilita a interrupção extrena INT0 
	
	
	//modo PWM
	TCCR0A = 0b10100011; // habilita o Modo PWM rapido e modo nao invertido  nos pinos OC0A e OC0B
	TCCR0B = 0b00000101;	//liga TC0, prescaler = 1024, fpwm =  16MHZ/(256*1024) = 61Hz
	OCR0A = 0; // controle do ciclo ativo do PWM OC0A(PD6)Duty = 200/256 = 78%
	
	//configurando o ADC
	ADMUX = 0b01000000;// tensao interna de ref(5.0v)
	ADCSRA = 0b11101111;//habilita o AD, interrupção, conversao, prescaler 128
	ADCSRB = 0x00;// conversão continua
	DIDR0 = 0b00111110;// habilita o PC0 como entrada do ADC0
	
	sei();//Habilita interrupção Global 
	
	USART_Init(MYUBRR);// Inicializa o protocolo USART
   
   
   
   
    while (1) 
    {
		// LImpar o display apos as interrupções e atualiza
		if (test == 1)
		{
			nokia_lcd_init();
			nokia_lcd_clear();
			nokia_lcd_render();
			test = 0;
		}
		
		//Calculo do sensor LM35 Vout = 0.01 * T 10mV -> 1ºC
		
		Celsius = (((leitura_ADC/1023.0)*5))/0.01; 
		
		//Gravando na memoria EEPROM
		if (Celsius >= 80)
		{
			int critico;

			critico = Celsius ;
			eeprom_write_float (( float *) 20, critico);// escreve na EEPROM no endereço 20
			critico =eeprom_read_float(( float *) 20);// Lê na EEPROM no endereço 20
			
			//Mostra no display o valor da variavel critica a ser analisada 
			nokia_lcd_clear();
			int_to_str(critico, leitura_ADC_string);
			nokia_lcd_write_string(leitura_ADC_string, 1);
			nokia_lcd_write_string(" C",1);
			nokia_lcd_set_cursor(0, 10);
			nokia_lcd_write_string("Variavel Critica armaz. ",1);
			nokia_lcd_set_cursor(0, 20);
			nokia_lcd_render();
			_delay_ms(3000);
		}
		
		
		// monitoramento da temperatura em tempo real 
		nokia_lcd_clear();
		int_to_str(Celsius, leitura_ADC_string);
		nokia_lcd_write_string("Monitoramento ",1);
		nokia_lcd_set_cursor(0, 10);
		nokia_lcd_write_string("*------------*", 1);
		nokia_lcd_set_cursor(0, 20);
		nokia_lcd_write_string("Temperatura: ",1);
		nokia_lcd_set_cursor(0, 30);
		nokia_lcd_write_string(leitura_ADC_string, 1);
		nokia_lcd_write_string(" C",1);
		nokia_lcd_set_cursor(0, 30);
		nokia_lcd_render();
		_delay_ms(1000);
		
		//condição de acionamento do motor 
		if (Celsius <=50)
		{
			PORTC = 0b00000000; // desliga o motor 
			
		}
		
		else
		{
			PORTC = 0b00100000; //  liga o motor e monstra no display uma flag de alerta para o motor em movimento 
			  nokia_lcd_set_cursor(0, 40);
			  nokia_lcd_write_string("              ",1);
			  nokia_lcd_set_cursor(0, 40);
			  nokia_lcd_write_string("motor ativo ",1);
			  nokia_lcd_render();
			  _delay_ms(5000);
		}	
		
    }
}

//Função para o display 
void int_to_str(uint16_t s, unsigned char *d)
{
	uint8_t n = tam_vetor - 2;

	for (int8_t i = n; i >= 0; i--)
	{
		d[i]=s%10+48;
		s/=10;
	}
}


ISR(INT0_vect) //Sensor de presença
{
	// mostra no display uma mensagem de saudação apos o sensor detectar presença  
	nokia_lcd_init();
	nokia_lcd_clear();
	nokia_lcd_write_string("Seja bem vindo",1);
	nokia_lcd_set_cursor(0, 10);
	nokia_lcd_render();
	_delay_ms(2000);
	test = 1;
}

//Protocolo Usart para definição de luminosidade da lampada
ISR(USART_RX_vect)
{
		char recebido;
		recebido = UDR0; // variavel q ira receber os comandos do seriam monitor 
		
		if (recebido == 'l')// recebe l para ligar o Led da sala de maquinas
		{
			PORTD |= (1 << PD0);
			
			nokia_lcd_init();
			nokia_lcd_clear();
			nokia_lcd_write_string("Lamp. Maq:",1);
			nokia_lcd_set_cursor(0, 10);
			nokia_lcd_write_string("ligada",1);
			nokia_lcd_set_cursor(0, 20);
			nokia_lcd_render();
			_delay_ms(2000);
			test = 1;
		}
		if (recebido == 'd')//recebe d para desligar o Led da sala de maquinas
		{
			PORTD &= ~(1 << PD0);
			nokia_lcd_init();
			nokia_lcd_clear();
			nokia_lcd_write_string("Lamp. Maq:",1);
			nokia_lcd_set_cursor(0, 10);
			nokia_lcd_write_string("desligada",1);
			nokia_lcd_set_cursor(0, 20);
			nokia_lcd_render();
			_delay_ms(2000);
			test=1;
		}
		
		// para o controle do LED dos escritorio
		if(recebido == '0')// recebe 0 para desligar o LED
		{
			 i = 0;
			 OCR0A = i *60;
			 nokia_lcd_init();
			 nokia_lcd_clear();
			 nokia_lcd_write_string("Lamp. desligada",1);
			 nokia_lcd_set_cursor(0, 10);
			 nokia_lcd_write_string("Porcent:0%",1);
			 nokia_lcd_set_cursor(0, 10);
			 nokia_lcd_render();
			 _delay_ms(2000);
			 test=1;
			
		}
		
		if(recebido == '1')// recebe 1 para ligar o LED com 25% de luminosidade
		{
			 i = 1;
			 OCR0A = i *60;
			 nokia_lcd_init();
			 nokia_lcd_clear();
			 nokia_lcd_write_string("Lamp: ligada",1);
			 nokia_lcd_set_cursor(0, 10);
			 nokia_lcd_write_string("Porcent:25%",1);
			 nokia_lcd_set_cursor(0, 10);
			 nokia_lcd_render();
			 _delay_ms(2000);
			 test=1;
		}
		
		if(recebido == '2')// recebe 2 para ligar o LED com 50% de luminosidade
		{
			 i = 2;
			 OCR0A = i *60;
			 nokia_lcd_init();
			 nokia_lcd_clear();
			 nokia_lcd_write_string("Lamp: ligada",1);
			 nokia_lcd_set_cursor(0, 10);
			 nokia_lcd_write_string("Porcent: 50%",1);
			 nokia_lcd_set_cursor(0, 10);
			 nokia_lcd_render();
			 _delay_ms(2000);
			 test=1;
		}
		
		if(recebido == '3')// recebe3 para ligar o LED com 75% de luminosidade
		{
			 i = 3;
			 OCR0A = i *60;
			 nokia_lcd_init();
			 nokia_lcd_clear();
			 nokia_lcd_write_string("Lamp: ligada",1);
			 nokia_lcd_set_cursor(0, 10);
			 nokia_lcd_write_string("Porcent: 75%",1);
			 nokia_lcd_set_cursor(0, 10);
			 nokia_lcd_render();
			 _delay_ms(2000);
			 test=1;
		}
		
		if(recebido == '4')// recebe 4 para ligar o LED com 100% de luminosidade
		{
			 i = 4;
			 OCR0A = i *60;
			 nokia_lcd_init();
			 nokia_lcd_clear();
			 nokia_lcd_write_string("Lamp: ligada",1);
			 nokia_lcd_set_cursor(0, 10);
			 nokia_lcd_write_string("Porcent: 100%",1);
			 nokia_lcd_set_cursor(0, 10);
			 nokia_lcd_render();
			 _delay_ms(2000);
			 test=1;

		}
		
		USART_transmit(recebido);
		
}