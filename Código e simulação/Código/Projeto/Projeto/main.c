/*
 * USART.c
 *
 * Created: 19/10/2020 19:49:34
 * Author : Alysson Hyago Pereira de Oliveira
 */ 

#define  F_CPU 16000000UL
#define BAUD 9600 //bps
#define MYUBRR F_CPU/16/BAUD-1 // modo normal assincrono formula slider 

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "nokia5110.h"

// definições para o display 
#define tam_vetor 5
unsigned char leitura_ADC_string[tam_vetor];

// variavels de controle para a temperatura e ADC
uint16_t leitura_ADC= 0 ;
float Celsius;

// Prototipo função
void int_to_str(uint16_t s, unsigned char *d);

// -- Função de vetores de interrupção externa
ISR(INT0_vect); //  INT EXT 0
ISR(USART_RX_vect);

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
	while(!(UCSR0A = 0b00100000));
	 UDR0 = data; // coloca da do no reg. e o envia 
}
unsigned char USART_Receive(void)
{
	while(!(UCSR0A = 0b00100000));//espera o dado ser recebido
	return UDR0;// lê o dado e retorna 
}

int i = 0;
int test = 1;


int main(void)
{
	
	//Portas GPIO
	DDRD = 0b01000000; // todos os pinos da porta D como entrada, somente o 6 como saida
	DDRC = 0b00100001; // como modo de saida 
	PORTD= 0b00000100;
	PORTC = 0x00; // so C0 em baixo nivel
	
	//configuração das interrupções
	EICRA = 0b00000010;	// configurando como borda de descida
	EIMSK = 0b00000001; //habilita a interrupção extrena INT0 
	sei(); //habilita a chave de interrupção global
	
	//modo PWM
	TCCR0A = 0b10100011; // habilita o Modo PWM rapido e modo nao invertido  nos pinos OC0A e OC0B
	TCCR0B = 0b00000101;	//liga TC0, prescaler = 1024, fpwm =  16MHZ/(256*1024) = 61Hz
	OCR0A = 0; // controle do ciclo ativo do PWM OC0A(PD6)Duty = 200/256 = 78%
	
	//configurando o ADC
	ADMUX = 0b11000000; // tensao interna de ref(1.1v)
	ADCSRA = 0b11101111;//habilita o AD, interrupção, conversao, prescaler 128
	ADCSRB = 0x00;// conversão continua
	DIDR0 = 0b00111110;// habilita o PC0 como entrada do ADC0
	
	sei();
	// USART
	USART_Init(MYUBRR);
   
    while (1) 
    {
		
		if (test == 1)
		{

			nokia_lcd_init();
			nokia_lcd_clear();
			nokia_lcd_write_string("Meu Escritorio",1);
			nokia_lcd_set_cursor(0, 10);
			nokia_lcd_write_string("*------------*", 1);
			nokia_lcd_set_cursor(0, 20);
			nokia_lcd_write_string("-L. Escritorio", 1);
			nokia_lcd_set_cursor(0, 30);
			nokia_lcd_write_string("-Sala de maquinas", 1);
			nokia_lcd_set_cursor(0,40);
			nokia_lcd_render();
			test = 0;
		}
		
		//Calculo do sensor LM35 Vout = 0.01 * T ;
		
		Celsius = (leitura_ADC/1023.0)*100;

		nokia_lcd_clear();
		int_to_str(Celsius, leitura_ADC_string);
		nokia_lcd_write_string("Monitoramento ",1);
		nokia_lcd_set_cursor(0, 10);
		nokia_lcd_write_string("*------------*", 1);
		nokia_lcd_set_cursor(0, 20);
		nokia_lcd_write_string("Temperatura: ",1);
		nokia_lcd_set_cursor(0, 30);
		nokia_lcd_write_string(leitura_ADC_string, 1);
		nokia_lcd_write_string(" Celsius ",1);
		nokia_lcd_set_cursor(0, 40);
		nokia_lcd_render();
		_delay_ms(1000);
		
		if (Celsius <=70)
		{
			PORTC = 0b00000000;
			
		}
		else
		{
			PORTC = 0b00100000;
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
	
	nokia_lcd_init();
	nokia_lcd_clear();
	nokia_lcd_write_string("Seja bem vindo",1);
	nokia_lcd_set_cursor(0, 20);
	nokia_lcd_render();
	_delay_ms(2000);
	test = 1;
}

ISR(USART_RX_vect)
{
		char recebido;
		recebido = UDR0;
		
		
		if(recebido == '0')
		{
			 i = 0;
			 OCR0A = i *60;
			 nokia_lcd_init();
			 nokia_lcd_clear();
			 nokia_lcd_write_string("Lamp.Escritorio:desligada",1);
			 nokia_lcd_set_cursor(0, 10);
			 nokia_lcd_write_string("Porcentagem:0%",1);
			 nokia_lcd_set_cursor(0, 10);
			 nokia_lcd_render();
			 _delay_ms(2000);
			 test=1;
			
		}
		
		if(recebido == '1')
		{
			 i = 1;
			 OCR0A = i *60;
			 nokia_lcd_init();
			 nokia_lcd_clear();
			 nokia_lcd_write_string("Lamp. Escritorio:ligada",1);
			 nokia_lcd_set_cursor(0, 10);
			 nokia_lcd_write_string("Porcentagem:25%",1);
			 nokia_lcd_set_cursor(0, 10);
			 nokia_lcd_render();
			 _delay_ms(2000);
			 test=1;
		}
		
		if(recebido == '2')
		{
			 i = 2;
			 OCR0A = i *60;
			 nokia_lcd_init();
			 nokia_lcd_clear();
			 nokia_lcd_write_string("Lamp. Escritorio:ligada",1);
			 nokia_lcd_set_cursor(0, 10);
			 nokia_lcd_write_string("Porcentagem:50%",1);
			 nokia_lcd_set_cursor(0, 10);
			 nokia_lcd_render();
			 _delay_ms(2000);
			 test=1;
		}
		
		if(recebido == '3')
		{
			 i = 3;
			 OCR0A = i *60;
			 nokia_lcd_init();
			 nokia_lcd_clear();
			 nokia_lcd_write_string("Lamp. Escritorio:ligada",1);
			 nokia_lcd_set_cursor(0, 10);
			 nokia_lcd_write_string("Porcentagem:75%",1);
			 nokia_lcd_set_cursor(0, 10);
			 nokia_lcd_render();
			 _delay_ms(2000);
			 test=1;
		}
		
		if(recebido == '4')
		{
			 i = 4;
			 OCR0A = i *60;
			 nokia_lcd_init();
			 nokia_lcd_clear();
			 nokia_lcd_write_string("Lamp. Escritorio:ligada",1);
			 nokia_lcd_set_cursor(0, 10);
			 nokia_lcd_write_string("Porcentagem:100%",1);
			 nokia_lcd_set_cursor(0, 10);
			 nokia_lcd_render();
			 _delay_ms(2000);
			 test=1;

		}
		
		USART_transmit(recebido);
		
}