/*
 * MotorContol.c
 *
 * Created: 2024-08-16 오후 11:47:04
 * Author : MC
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

#define DEBOUNCE_DELAY 50  // 디바운스 딜레이 시간 (단위: ms)


void timer0_init(void);
uint8_t read_input(void);
uint32_t millis() ;
void InputSwitch_PD3();
void InputSwitch_PD3_PD4();
volatile uint16_t timer_overflow_count = 0;
volatile uint8_t T_AC_Cnt = 0;
volatile uint8_t T_AC_Cnt1 = 0;


volatile uint32_t timer_millis = 0;

uint8_t switchBuffer[5] = {0};
uint8_t switchBuf[2][5] = {0};

uint8_t PrePD3 =1 ;
uint8_t PrePD4 =1 ;
uint32_t lastDebounceTime = 0;

uint8_t F_MoterBi =0;

volatile uint8_t FSpeed =0;
volatile uint8_t time_delay =0;

uint8_t SpeedInput = 0;
uint8_t MoterEn = 0;
uint8_t eeprom_en =0 ;
uint16_t save_1 = 0x0001;

int main (void)
{
	/* Insert system clock initialization code here (sysclk_init()). */

	DDRB &= ~(1 <<PB0);
	DDRC &= ~(1 <<PD2);
	DDRD &= ~((1 << PD3) | (1 << PD4));
	DDRD &= ~((1 <<PD5)|(1 <<PD6) |(1 <<PD7));
	DDRC |= (1 << PC3);
	
	DDRC |= (1 << PB1);
	DDRC |= (1 << PB2);
	
	
	while(1)
	{
		F_MoterBi = eeprom_read_byte((uint8_t*)save_1); // read the byte in location 23
		
		F_MoterBi = F_MoterBi & 0x03;
		
		if(F_MoterBi >= 0x03)
		{
			F_MoterBi= 0;
			eeprom_write_byte( (uint8_t *)save_1, F_MoterBi); //  write the byte 64 to location 23 of the EEPROM
		}
		if(F_MoterBi == 0)
			break;
		else if(F_MoterBi == 1)
			break;
	}
	
	

	
	// 외부 인터럽트 설정
	// INT0 (PD2) low level에서 인터럽트 발생
	//MCUCR &= ~((1 << ISC01) | (1 << ISC00)); // ISC01=0, ISC00=0 -> Low level
	MCUCR |= (1 << ISC01) | (1 << ISC00);    // ISC01=1, ISC00=1 -> 상승 엣지
	// 인터럽트 활성화
	GICR |=  (1 << INT0);
	
	timer0_init();
	
	//PORTB |=(1 << PB1);
	PORTB &= ~(1 << PB2);
	PORTC |= (1 << PC3);

	time_delay = 1;

	while (1)
	{
		if(time_delay == 0)
		{
			if(MoterEn)
			{
				switch(SpeedInput)
				{
					case 0x00:
					FSpeed = 0;
					PORTC |= (1 << PC3);
					break;
					case 0x01:
					FSpeed =1;
					if(timer_overflow_count > 999)
					{
						timer_overflow_count = 0;
						PORTC ^= (1 << PC3);
						
					}
					
					break;
					case 0x02:
					FSpeed =2;
					if(timer_overflow_count > 1999)
					{
						timer_overflow_count = 0;
						PORTC ^= (1 << PC3);
						
					}
					break;
					case 0x03:
					FSpeed =1;
					if(timer_overflow_count > 999)
					{
						timer_overflow_count = 0;
						PORTC ^= (1 << PC3);
						
					}
					break;

					default:
					FSpeed = 0;
					PORTC |= (1 << PC3);
					break;
				}
			}
			else
			{
				PORTC |= (1 << PC3);
			}

			switch(read_input())
			{
				

				case 0x01:    // in case one Sensor ..
				// F_MoterBi = 0;
				MoterEn = 1;
				InputSwitch_PD3();
				break;
				
				case 0x02:
				MoterEn = 1;
				InputSwitch_PD3_PD4();
				
				break;
				case 0x03:
					F_MoterBi = 0;
					MoterEn = 1;
				break;

				default:
				FSpeed = 0;
				MoterEn = 0;
				break;
				
			}
			
			
			if(T_AC_Cnt1 >= 1)
			{
				T_AC_Cnt1 = 0;
				if(FSpeed==1)
				T_AC_Cnt = 7;
				else if(FSpeed==2)
				T_AC_Cnt = 6;
				else
				T_AC_Cnt= 0;
				//PORTC ^= (1 << PC3);
			}
			
			
		
			if(T_AC_Cnt)
			{
				if(F_MoterBi==1)
				PORTB |=(1 << PB1);
				else
				PORTB |=(1 << PB2);
				
			}else
			{
				PORTB &= ~(1 << PB1);
				PORTB &= ~(1 << PB2);
				
			}
		}
		
				

		if(eeprom_en)
		{
			
			{
				eeprom_write_byte( (uint8_t *)save_1, F_MoterBi); //  write the byte 64 to location 23 of the EEPROM
				eeprom_en = 0;
			}
		
		}
		
	}
	/* Insert application code here, after the board has been initialized. */
}

uint8_t read_input(void)
{
	uint8_t input_value = 0;

	// PB0 상태 읽기
	if (!(PIND & (1 << PD5))) {
		input_value |= (1 << 1);
	}else
	{
		input_value &= ~(1 << 1);
	}

	// PD3 상태 읽기
	if (!(PIND & (1 << PD6))) {
		input_value |= (1 << 0);
	}else
	{
		input_value &= ~(1 << 0);
	}

	// PB1 상태 읽기
	if (!(PIND & (1 << PD7))) {
		SpeedInput |= (1 << 1);
	}else
	{
		SpeedInput &= ~(1 << 1);
	}
	// PB0 상태 읽기
	if (!(PINB & (1 << PB0))) {
		SpeedInput|= (1 << 0);
	}else
	{
		SpeedInput &= ~(1 << 0);
	}

	return input_value & 0x03;
}

uint8_t F_Switch_PD3 = 0;
uint8_t F_Switch_PD4 = 0;

void InputSwitch_PD3()
{
	uint8_t cnt = 0;

		switchBuffer[cnt] = !(PIND & (1 << PD3));
		cnt++;
		if(cnt > 4)
			cnt = 0;

		if((switchBuffer[0] == 0) && (switchBuffer[1] == 0) && (switchBuffer[2] == 0) && (switchBuffer[3] == 0)&& (switchBuffer[4] == 0) )
			F_Switch_PD3 = 1;
		else 
			F_Switch_PD3 = 0;

		if(F_Switch_PD3 != PrePD3)
		{
			if(F_Switch_PD3)
			{
				// F_MoterBi = !F_MoterBi;    // F_MoterBi 값을 토글
				eeprom_en = 1;             // EEPROM 동작 설정
				time_delay = 1;            // 지연 시간 설정
			
				
			}else 
			{
				F_MoterBi = !F_MoterBi;    // F_MoterBi 값을 토글
			}
			PrePD3 = F_Switch_PD3;
		}

		
				
}
void InputSwitch_PD3_PD4()
{
	uint8_t cnt = 0;

		switchBuf[0][cnt] = !(PIND & (1 << PD3));
		switchBuf[1][cnt] = !(PIND & (1 << PD4));
		cnt++;
		if(cnt > 4)
			cnt = 0;

		if((switchBuf[0][0] == 0) && (switchBuf[0][1] == 0) && (switchBuf[0][2] == 0) && (switchBuf[0][3] == 0)&& (switchBuf[0][4] == 0) )
			F_Switch_PD3 = 1;
		else 
			F_Switch_PD3 = 0;

		if((switchBuf[1][0] == 0) && (switchBuf[1][1] == 0) && (switchBuf[1][2] == 0) && (switchBuf[1][3] == 0)&& (switchBuf[1][4] == 0) )
			F_Switch_PD4 = 1;
		else 
			F_Switch_PD4 = 0;


	
		if(F_Switch_PD3 != PrePD3)
		{
			if(F_Switch_PD3)
			{
				
				eeprom_en = 1;
				time_delay=1;
				
			}
			else
			{
				F_MoterBi = 0;
				
			}
			PrePD3 = F_Switch_PD3;
		}

		if(F_Switch_PD4 != PrePD4)
		{
			if(F_Switch_PD4)
			{
				
				eeprom_en = 1;
				time_delay=1;
				
			}
			else
			{
				F_MoterBi = 1;
				
			}
			PrePD4 = F_Switch_PD4;
		}
				
}
	
			


uint32_t millis() {
	uint32_t millis_copy;
	
	cli(); // 인터럽트 비활성화 (원자적 접근을 위해)
	millis_copy = timer_millis;
	sei(); // 인터럽트 활성화
	
	return millis_copy;
}


// 타이머0 초기화 함수
void timer0_init(void)
{
	// 타이머0 클럭을 설정 (분주율 64)
	TCCR0 = (1 << CS01) | (1 << CS00);

	// 타이머0 오버플로우 인터럽트 활성화
	TIMSK |= (1 << TOIE0);

	// 전역 인터럽트 활성화
	sei();

	// 타이머0 초기값 설정 (195)
	TCNT0 = 239;//239;
}

// 타이머0 오버플로우 인터럽트 서비스 루틴
uint8_t time_1ms = 0;
uint8_t time_10ms = 0;
uint8_t time_100ms = 0;

ISR(TIMER0_OVF_vect)
{
	// 타이머0 초기값 설정 (195)
	TCNT0 = 239;
	if(T_AC_Cnt)
	{
		T_AC_Cnt--;
		
	}
	// 타이머 오버플로우 카운터 증가
	timer_millis++;
	
	timer_overflow_count++;
	time_1ms++;
	if(time_1ms > 10)
	{
		time_1ms =0 ;
		time_10ms++;
	}
	if(time_10ms > 10)
	{
		time_10ms = 0;
		time_100ms++;

	}
	if(time_100ms > 10)
	{
		time_100ms= 0;
		if(time_delay)
			time_delay--;
	}

	
}


// INT0 (PD2) 인터럽트 서비스 루틴
ISR(INT0_vect)
{
	// PD4 핀에서 low level 인터럽트가 발생했을 때 실행될 코드
	// 예: PC3 핀 토글
	//PORTC ^= (1 << PC3);
	
	T_AC_Cnt1++;

	
}
