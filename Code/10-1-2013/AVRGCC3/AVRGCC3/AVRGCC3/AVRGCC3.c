/*
 * FIRST PROJECT
 *
 * Created: 11/28/2012 9:34:10 PM
 *  Author: Lam Ngoc Chien
 */
/*------------------------------------------------------------------------------------------------*/  
#define F_CPU 8000000L
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <avr/eeprom.h>
#include "myLCD.h"// Thu vien LCD
#include "myRTC.h"// Thu vien Real Time

/*================================================================================================*/  
/*--------------------------------------Phan Khai Bao---------------------------------------------*/  
/*================================================================================================*/  

#define AVCC_MODE	(1<<REFS0)
#define ADC_VREF_TYPE AVCC_MODE //chon dien ap tham chieu chan AREF
#define setbit(port,pin) port |= 1<<pin 
#define clrbit(port,pin) port &= ~(1<<pin)

/*------------------------------------------------------------------------------------------------*/
#define PHONE_NUM "0973471359"
#define  BUFF_SMS 320
char control_device[160];
volatile char temp_sms[BUFF_SMS];
//uint8_t cntr_sms=0;
uint8_t control_counter=0;
uint8_t dem_kt=0;
uint8_t option=0,opt_device=0, opt_time=0, state_device=0, time=0, ok_begin=0, begin_check_sms=0;;
/*------------------------------------------------------------------------------------------------*/
unsigned char u_data, bao_dong=0, ngung_bao_dong=0,hanche_sms=0;
/*------------------------------------------------------------------------------------------------*/  
volatile uint8_t	Second=45, Minute=20, Hour=2, Day=5, Date=10, Month=1, Year=13, Mode=0;
volatile uint8_t AP=1; // mac dinh = 1 de giam so truong hop trong hen gio
volatile  uint8_t tData[7], time_count=0, adc_count=0; //dinh nghia cac bien tam
volatile  uint8_t countdown_min[5], state_count[5], must_edit=0; // So phut hen gio toi da 240phut nap vao stick_time
char dis[5];		//bien dis dung de luu tru string hien thi len LCD
/*------------------------------------------------------------------------------------------------*/ 		
char choise_device=-1, firt_dis=1;

/*------------------------------------------------------------------------------------------------*/  
/*--------------------------------------Cac Ham Con-----------------------------------------------*/ 
/*------------------------------------------------------------------------------------------------*/ 

//Chuong trinh truyen du lieu qua UART
void uart_char_tx(unsigned char chr)
{
	if(chr == '\n')
		uart_char_tx('\r');
	while(bit_is_clear(UCSRA,UDRE)) {};
	UDR = chr;
}
static FILE lcdstd = FDEV_SETUP_STREAM(putChar_LCD,NULL,_FDEV_SETUP_WRITE);
static FILE uartstd = FDEV_SETUP_STREAM(uart_char_tx,NULL,_FDEV_SETUP_WRITE);
/*------------------------------------------------------------------------------------------------*/  
void off_device (uint8_t val)
{
	switch (val)
		{
			case 1:		setbit(PORTC,PORTC7); break;
			case 2: 	setbit(PORTC,PORTC6); break;
			case 3: 	setbit(PORTC,PORTC5); break;
			case 4: 	setbit(PORTC,PORTC4); break;
			default: break;
		}
	if(choise_device>0&&choise_device<5)	check_on_off(choise_device);
}
void on_device (uint8_t val)
{
	switch (val)
		{
			case 1:		clrbit(PORTC,PORTC7); break;
			case 2: 	clrbit(PORTC,PORTC6); break;
			case 3: 	clrbit(PORTC,PORTC5); break;
			case 4: 	clrbit(PORTC,PORTC4); break;
			default: break;
		}
	if(choise_device>0&&choise_device<5)	check_on_off(choise_device);
}
/*------------------------------------------------------------------------------------------------*/
/*----------------------------------------SMS Last------------------------------------------------*/ 
void init_arry_sms()
{
	for(int i=0;i<160;i++)
		temp_sms[i]=32;
}
void sms_sync()
{
	stdout=&uartstd;
	printf("AT\r");
	_delay_ms(50);
	//printf("AT\r");
	//_delay_ms(5);
	//printf("AT\r");
	//_delay_ms(5);
}
void send_sms()
{
	stdout=&uartstd;
	sms_sync();
	printf("AT+CMGS=\""PHONE_NUM"\"\r");
	_delay_ms(100);
	printf("SIM548C Ready!!!");
	putchar(0x1A);
	_delay_ms(10);	
}
void send_alarm_sms()
{
	stdout=&uartstd;
	sms_sync();
	printf("AT+CMGS=\""PHONE_NUM"\"\r");
	_delay_ms(100);
	printf("CANH BAO NHIET DO QUA CAO");
	putchar(0x1A);
	_delay_ms(10);
	hanche_sms=5;
}
void sms_recv()
{
	sms_sync();
	printf("AT+CMGR=1\r");
	_delay_ms(100);
}
void sms_del()
{
	stdout=&uartstd;
	printf("AT+CMGD=1\r");
	_delay_ms(10);
}
void state_sms()
{
	uint8_t state_dev[5],state_set[5],time_dev[5];
	for(uint8_t i=1;i<=4;i++)
		{
		if(state_count[i]==240)
			state_set[i]=1;
		else
			state_set[i]=0;	
		if(countdown_min[i]<=240)
			time_dev[i]=countdown_min[i];
		else
			time_dev[i]=0;
		}					
	if(bit_is_clear(PORTC,PINC7))	
		state_dev[1]=1; //ON
	else	
		state_dev[1]=0; //OFF
	if(bit_is_clear(PORTC,PINC6))	
		state_dev[2]=1; //ON
	else	
		state_dev[2]=0; //OFF	
	if(bit_is_clear(PORTC,PINC5))	
		state_dev[3]=1; //ON
	else	
		state_dev[3]=0; //OFF	
	if(bit_is_clear(PORTC,PINC4))	
		state_dev[4]=1; //ON
	else	
		state_dev[4]=0; //OFF
	stdout=&uartstd;
	sms_sync();
	printf("AT+CMGS=\""PHONE_NUM"\"\r");
	_delay_ms(100);
	//Demo: TB1: 1 15 0 =>TB1 Dang Bat Hen gio Tat sau 15p
	printf("TB%d: %d %d %d\rTB%d: %d %d %d\rTB%d: %d %d %d\rTB%d: %d %d %d\r",1,state_dev[1],time_dev[1],state_set[1],2,state_dev[2],time_dev[2],state_set[2],3,state_dev[3],time_dev[3],state_set[3],4,state_dev[4],time_dev[4],state_set[4]);
	putchar(0x1A);
	_delay_ms(10);			
}
void ok_sms()
{
	clrbit(PORTC,PORTC3);	
	_delay_ms(100);
	setbit(PORTC,PORTC3);
}
void process_symbol_sms()
{
	int cn=0;
	stdout=&lcdstd;
	char symbol;
	for(int i=0;i<dem_kt-4;i++)
		{
			symbol=temp_sms[i];
			if(symbol==34) cn++;
			if((cn==8) && (symbol!=34) && (symbol>=32))
				{
					control_device[++control_counter]=symbol;
					_delay_ms(30);
				}
		}
	if(control_counter==5&&control_device[1]=='c'&&control_device[2]=='h'&&control_device[3]=='e'&&control_device[4]=='c'&&control_device[5]=='k')
		{
		state_sms();
		ok_sms();
		}		
	if(control_counter>=4)
		{	
		if(control_device[2]=='n')
			state_device=240;
		else
			if(control_device[2]='f')
				state_device=15;
		for(uint8_t i=3;i<=control_counter;i++)
			if(control_device[i]==42)
				{
				option++;
				if(option==1)
					opt_device=i;
				else
					if(option==2)
						{
							opt_time=i;						
							break;
						}
				}
		if(option==2)
			{
			for(uint8_t i=control_counter;i>opt_time;i--)
					time += (control_device[i]-48)*pow(10,control_counter-i);	
			for(uint8_t i=opt_device+1;i<opt_time;i++)		
				set_time_count(control_device[i]-48,state_device,time);
			ok_sms();
			}
		else
		if(option==1)
			{
			for(uint8_t i=opt_device+1;i<=control_counter;i++)
				if(state_device==240)		
					on_device(control_device[i]-48);	
				else
					if(state_device==15)		
						off_device(control_device[i]-48);	
			ok_sms();					
			}							
		}
	control_counter=0;
	dem_kt=0;
	option=0;
	opt_device=0;
	opt_time=0;
	state_device=0;
	time=0;
}
void process_sms()
{
	init_arry_sms();
	_delay_ms(5);
	sms_recv();
	_delay_ms(1000);
	process_symbol_sms();
	_delay_ms(5);	
	sms_del();
	_delay_ms(5);
	stdout=&lcdstd;
}
/*------------------------------------------------------------------------------------------------*/
// doi BCD sang thap phan va nguoc lai
uint8_t BCD2Dec(uint8_t BCD){
	uint8_t L, H;
	L=BCD & 0x0F;
	H=(BCD>>4)*10;
	return (H+L);
}
uint8_t Dec2BCD(uint8_t Dec){
	uint8_t L, H;
	L=Dec % 10;
	H=(Dec/10)<<4;
	return (H+L);
}
/*------------------------------------------------------------------------------------------------*/  
//chuong trinh con  hien thi thoi gian doc tu DS1307
void Display(void)
{ 
	Second 	= BCD2Dec(tData[0] & 0x7F);
	Minute 	= BCD2Dec(tData[1]);
	if (Mode !=0) 	Hour = BCD2Dec(tData[2] & 0x1F); //mode 12h
	else 		  	Hour = BCD2Dec(tData[2] & 0x3F); //mode 24h	
	Day		= BCD2Dec(tData[3]);
	Date   	= BCD2Dec(tData[4]);
	Month	= BCD2Dec(tData[5]);
	Year	= BCD2Dec(tData[6]);
	
	/****************************************************************************************/
	clr_LCD();		//xoa LCD
	//Xuat Hour
	sprintf(dis, "%i",Hour);
	move_LCD(1,1);  
	if (Hour<10) putChar_LCD(' ');print_LCD(dis); 
	move_LCD(1,3); putChar_LCD(':');
	//Xuat Minute
	sprintf(dis, "%i",Minute); 
	move_LCD(1,4); if (Minute<10) putChar_LCD('0');		print_LCD(dis); 
	move_LCD(1,6);	putChar_LCD(':');
	//Xuat Second
	sprintf(dis, "%i",Second); 
	move_LCD(1,7); if (Second<10) putChar_LCD('0');		print_LCD(dis); 
	if (Mode !=0){ //mode 12h
		move_LCD(1,1);
		if (bit_is_set(tData[2],5))  putChar_LCD('P'); //kiem tra bit AP, if AP=1
		else putChar_LCD('A');
	}
	/****************************************************************************************/
	if(choise_device==0||firt_dis==1)
	{
		move_LCD(2,1);
		switch (Day)
		 {
			case 1:		print_LCD("Sun");	break;
		    case 2:		print_LCD("Mon");	break;
			case 3:		print_LCD("Tue");	break;
		    case 4:		print_LCD("Wed");	break;
			case 5:		print_LCD("Thu");	break;
		    case 6:		print_LCD("Fri");	break;
			case 7:		print_LCD("Sat");	break;
		 }
		sprintf(dis, "%i",Date);  
		move_LCD(2,5); 
		if (Date<10)	putChar_LCD(' '); 
		print_LCD(dis);
		move_LCD(2,7);	putChar_LCD('/');//dau cach 1
		sprintf(dis, "%i",Month);  
		if (Month<10)	putChar_LCD('0'); 
		print_LCD(dis); 
		move_LCD(2,10); putChar_LCD('/');//dau cach 2
		//putChar_LCD('2'); putChar_LCD('0');//xuat so '20'
		sprintf(dis, "%i",Year);	
		move_LCD(2,11);
		if (Year<10) putChar_LCD('0'); // neu nam <10, in them so 0 ben trai, vi du 09 
		print_LCD(dis);		
	}
}
//Chuong trinh lay gia tri ADC
unsigned char read_adc(unsigned char adc_channel)
{
	ADMUX=adc_channel|ADC_VREF_TYPE;
	ADCSRA|=(1<<ADSC);
	loop_until_bit_is_set(ADCSRA,ADIF);
	return ADCW;
}
/*------------------------------------------------------------------------------------------------*/  
//chuong trinh xuat nhiet do ra LCD
void adc_out(unsigned char val)
{
	unsigned char chuc, dvi, temp_val;
	//Tinh toan gia tri nhiet do
	//temp_val=(int)(val/2.046);
	temp_val=(int)(val/2.726);
	chuc=(int)(temp_val/10);
	dvi=(int)(temp_val-(10*chuc));
	//Put ra LCD
	move_LCD(1,13);
	putChar_LCD(chuc+48);
	putChar_LCD(dvi+48);
	putChar_LCD(0xdf);
	print_LCD("C");
}
/*------------------------------------------------------------------------------------------------*/  
//Chuong trinh bao dong qua loa khi qua nhiet: >=80
void alarm()
{	
		if((unsigned char)(read_adc(0)/2.726)>=80)
			{
			bao_dong++;
			ngung_bao_dong=0;	
			}
		else
			{
			ngung_bao_dong++;
			bao_dong=0;	
			}
		if(bao_dong==3)	
			{
				clrbit(PORTC,PORTC3);	
				bao_dong=0;
				if(hanche_sms==0)
					send_alarm_sms();	
			}
		if(ngung_bao_dong>=10)	{setbit(PORTC,PORTC3);	ngung_bao_dong=0;}			
}
/*------------------------------------------------------------------------------------------------*/  
//Khoi dong gia tri ghi vao DS1307
void set_clock()
{
	unsigned char temp, flag=1;// mac dinh la cho phep ghi gia tri thoi gian vao ds1307
	while(!eeprom_is_ready());
	temp=eeprom_read_byte(0); // gia tri mac dinh ban dau cua byte 0 trong eeprom la 0xff = 255
	if(temp!=240)
		{
			while(!eeprom_is_ready());
			eeprom_write_byte(0,240); // danh dau lan ghi dau tien 0xf0
			_delay_ms(1);
		}
	else
			flag=0;
	if(flag||must_edit)
	{
		must_edit=0; // Su dung de tinh chinh thoi gian sau nay
		tData[0]=Dec2BCD(Second); 
		tData[1]=Dec2BCD(Minute); 
		if (Mode!=0) 
			tData[2]=Dec2BCD(Hour)|(Mode<<6)|(AP<<5); //mode 12h
		else 
			tData[2]=Dec2BCD(Hour);
		tData[3]=Dec2BCD(Day);
		tData[4]=Dec2BCD(Date);
		tData[5]=Dec2BCD(Month); 
		tData[6]=Dec2BCD(Year); 		
		TWI_Init(); //khoi dong TWI		
		TWI_DS1307_wblock(0x00, tData, 7); //ghi lien tiep cac bien thoi gian vao DS1307
		_delay_ms(1);	//cho DS1307 xu li 
	}
}
/*------------------------------------------------------------------------------------------------*/ 
void countdown_out(uint8_t device)
{
	unsigned char tram, chuc, dvi, val;
	val=countdown_min[device];
	if(val<=240&&val>0)
		{
		tram = val/100;
		val-=tram*100;
		dvi	= val%10;
		chuc = (val-dvi)/10;
		move_LCD(2,13);
		if(state_count[device]==240)	putChar_LCD(0x2a);
		if(state_count[device]==15)		putChar_LCD(0x2b);
		move_LCD(2,14);
		if(tram==0)		move_LCD(2,15);		else	putChar_LCD(tram+48);
		if(chuc==0&&tram==0)	move_LCD(2,16);		else	putChar_LCD(chuc+48);
		putChar_LCD(dvi+48);			
		}
}
/*------------------------------------------------------------------------------------------------*/  
/*------------------------------------------------------------------------------------------------*/ 
unsigned char max_date(unsigned char val_month, unsigned char val_year)
{
	val_year+=2000;
	if(val_month==2)	{	if(val_year%4==0)	return 29;	else	return 28;	}
	else	{	if(val_month==4||val_month==6||val_month==9||val_month==11)		return 30;	else	return 31;	}	
}
uint16_t conver2date(unsigned int val_date,unsigned int val_month,unsigned int val_year)
{
	unsigned int kqua;
	kqua=(val_year-13)*365;	// Tinh so ngay tu nam 2013
	kqua+=val_date;
	if(val_month>1)
		for (uint8_t i=1;i<val_month;i++)
		{
			kqua+=max_date(val_month,val_year);
		}
	if((val_year+2000)%4==0)
		return kqua++;
	return kqua;
}
uint16_t convert2minute(uint8_t val_min, uint8_t val_hour)
{
	return (val_hour*60+val_min);
}
void reset_eeprom()
{
	cli();
	for (unsigned char i=1;i<=48;i++)
		{
			while(!eeprom_is_ready());	
			eeprom_write_byte(i,255);
			_delay_ms(1);//Cho 1ms
		}
	sei();
}
void set_time_count(uint8_t device, uint8_t state, uint8_t time)
{
	cli();
	uint8_t new_eeprom[15], temp;
	countdown_min[device] = time;
	state_count[device] = state;
	new_eeprom[device+6]=time;
	new_eeprom[device+10]=state;
	new_eeprom[2] = BCD2Dec(tData[2] & 0x3F);
	for(uint8_t i=1;i<=6;i++)
		if(i!=2)
			new_eeprom[i] = BCD2Dec(tData[i]);
	for(uint8_t i=1;i<=4;i++)
		if(i!=device)
			if(countdown_min[i]!=0)
				{
					new_eeprom[i+6] = countdown_min[i];
					new_eeprom[i+10] = state_count[i];						
				}
			else
				{
					new_eeprom[i+6] = 255;
					new_eeprom[i+10] = 255;						
				}
	for(uint8_t i=1;i<=14;i++)
		{
			while(!eeprom_is_ready());
			eeprom_write_byte(i,new_eeprom[i]);
			_delay_ms(1);
		}		
	sei();
}
void update_count()
{
	cli();
	uint8_t copy_eeprom[15],copy_tData[7];
	uint16_t temp_date, temp_min, temp_val, flag=0;
	for(uint8_t i=1;i<=14;i++)
	{
		while(!eeprom_is_ready());		
		copy_eeprom[i]=eeprom_read_byte(i);	
		_delay_ms(1);
	}
	for(uint8_t i=1;i<=4;i++)
		if(copy_eeprom[i+6]!=255)
			{
				flag=1;					
				break;
			}
	if(flag)
		{
		copy_tData[2]=BCD2Dec(tData[2] & 0x3F);
		for(uint8_t i=1;i<=6;i++)
			if(i!=2)
				copy_tData[i]=BCD2Dec(tData[i]);
		temp_date = conver2date(copy_tData[4],copy_tData[5],copy_tData[6])-conver2date(copy_eeprom[4],copy_eeprom[5],copy_eeprom[6]);
		temp_min = convert2minute(copy_tData[1],copy_tData[2])-convert2minute(copy_eeprom[1],copy_eeprom[2]);
		temp_val = temp_date*1440+temp_min;
		for(uint8_t i=1;i<=6;i++)
			copy_eeprom[i]=copy_tData[i];
		if(temp_val>0)
			for(uint8_t i=1;i<=4;i++)
				if(copy_eeprom[i+6]>temp_val)
					copy_eeprom[i+6]-=temp_val;
				else
					{
						if(copy_eeprom[i+6]=temp_val)
							if(copy_eeprom[i+10]==0xf0)
								on_device(i);
							else
								off_device(i);
						copy_eeprom[i+6]=0xff;
						copy_eeprom[i+10]=0xff;
					}
				
		//Tao du lieu countdown_min va state_count moi
		for(uint8_t i=1;i<=4;i++)
			{
				if(copy_eeprom[i+6]!=255)
					{
					countdown_min[i]=copy_eeprom[i+6];
					state_count[i]=copy_eeprom[i+10];						
					}
			}
		
		//Luu gia tri moi vao eeprom
		for(uint8_t i=1;i<15;i++)
			{
			while(!eeprom_is_ready());
			eeprom_write_byte(i,copy_eeprom[i]);
			_delay_ms(1);
			}
		}
	else
		reset_eeprom();		
	sei();
}
void now_device (uint8_t device)
{
	uint8_t flag=0;
	if(state_count[device]==240)
		on_device(device);
	else
		if(state_count[device]==15)
			off_device(device);
	countdown_min[device]=0;	
	state_count[device]=0;
	for(uint8_t i=1;i<=4;i++)
		if(countdown_min[i]!=255)
			{
			flag=0;
			reset_eeprom();
			break;
			}		
	if(flag)
	{
		cli();
		while(!eeprom_is_ready());
		eeprom_write_byte(device+6,255);
		_delay_ms(1);
		while(!eeprom_is_ready());
		eeprom_write_byte(device+10,255);
		_delay_ms(1);	
		sei();
	}
}
void active_countdown()
{
	for(uint8_t i=1;i<=4;i++)
	{	
		if(countdown_min[i]<=240&&countdown_min[i]!=0)
			{
			if(countdown_min[i]==1)
				now_device(i);	
			else
				countdown_min[i]--;					
			}
	}
}
//Check Device
void check_on_off(uint8_t val)
{
	clr_LCD();
	Display();
	adc_out(read_adc(0));
	move_LCD(2,1);
	print_LCD("TB ");
	putChar_LCD(val+48);
	switch (val)
		{
			case 1: 	if(bit_is_clear(PORTC,PINC7))	print_LCD(": On");	else	print_LCD(": Off"); break;
			case 2: 	if(bit_is_clear(PORTC,PINC6))	print_LCD(": On");	else	print_LCD(": Off"); break;
			case 3: 	if(bit_is_clear(PORTC,PINC5))	print_LCD(": On");	else	print_LCD(": Off"); break;
			case 4: 	if(bit_is_clear(PORTC,PINC4))	print_LCD(": On");	else	print_LCD(": Off"); break;
			default: break;
		}
	countdown_out(val);	
}

void check_time()
{
	TWI_DS1307_wadr(0x00); 				//set dia chi ve 0
	_delay_ms(1);		   				//cho DS1307 xu li 
	TWI_DS1307_rblock(tData,7); 	//doc ca khoi thoi gian (7 bytes)			
	//hien thi ket qua len LCD
	if(BCD2Dec(tData[0]) !=Second)	//chi hien thi ket qua khi da qua 1s
		{ 			
			Second=BCD2Dec(tData[0] & 0x7F);
			sprintf(dis, "%i",Second); 
			move_LCD(1,7); 
			if (Second<10) 
				putChar_LCD('0');	
			print_LCD(dis);
			if (Second==0) //moi phut cap nhat 1 lan	
				{
					active_countdown();
					Display();
					adc_out(read_adc(0));
					if(choise_device>0&&choise_device<5)	
						check_on_off(choise_device);
					if(hanche_sms>0)
						hanche_sms--;
				}
/*------------------------------------------------------------------------------------------------*/ 				
			if(Second!=1&&begin_check_sms!=111)
				begin_check_sms++;		
			if(begin_check_sms==25)
				{
					begin_check_sms=111;
					ok_begin=0;
					ok_sms();
				}
			if(Second%10==0&&ok_begin==0)	
				process_sms();			
			if(Second%2)				
				{
					alarm();
					adc_out(read_adc(0));
				}
/*------------------------------------------------------------------------------------------------*/ 			
		}	
}


/*================================================================================================*/  
/*--------------------------------------Chuong Trinh Main-----------------------------------------*/  
/*================================================================================================*/  

int main(void)
{	
/*------------------------------------------------------------------------------------------------*/ 
	DDRC=0xff;	//out
	PORTC=0xff;
	DDRB=0x00;	//in	
	PORTB=0xff;
	DDRD=0x00;
	PORTD=0xff;
	init_LCD();
	clr_LCD();
/*------------------------------------------------------------------------------------------------*/  
	set_clock();							// Nap gia tri cho RTC
	TWI_DS1307_wadr(0x00);					//set dia chi ve 0
	_delay_ms(1);							//cho DS1307 xu li 
	TWI_DS1307_rblock(tData,7);				//doc ca khoi thoi gian (7 bytes)	
	Display();								// hien thi ket qua len lan dau LCD	
	firt_dis=0;
/*------------------------------------------------------------------------------------------------*/  	
	// Khai bao ngat dk = tay (INT0,1)
	//MCUCSR|=(0<<ISC2);							//INT2 ngat mode canh xuong
	MCUCR|=(1<<ISC11)|(1<<ISC01);				//INT 0,1 la ngat canh xuong MCUCR=0b00001010;
	GICR|=(1<<INT1)|(1<<INT0)|(1<<INT2);		//GICR=0b11100000	
/*------------------------------------------------------------------------------------------------*/  
	UBRRH=0;
	UBRRL=51;
	UCSRA=0x00;
	UCSRC=(1<<URSEL)|(1<<UCSZ1)|(1<<UCSZ0);
	UCSRB=(1<<RXEN)|(1<<TXEN)|(1<<RXCIE); // ca truyen va nhan
	sei();         	//set bit I cho phep ngat toan cuc
/*------------------------------------------------------------------------------------------------*/  
	ADCSRA=(1<<ADEN)|(1<<ADPS2)|(1<<ADPS0);//enable ADC
	ADMUX=ADC_VREF_TYPE;
	adc_out(read_adc(0));// Hien thi nhiet do lan dau
	choise_device=0;
/*------------------------------------------------------------------------------------------------*/ 
	update_count();  //Cap nhat hen gio trong eeprom
	init_arry_sms();
    while(1)
    {
		check_time();
		_delay_ms(320);	
    }
}


/*================================================================================================*/    
/*--------------------------------Cac Ham Ngat----------------------------------------------------*/  
/*================================================================================================*/  
//Trinh phuc vu ngat uart sms
ISR(SIG_UART_RECV)
{
	unsigned char kt;
	kt=UDR;
	temp_sms[dem_kt++]=kt;
	if(dem_kt>BUFF_SMS)
		dem_kt=BUFF_SMS-1;
		
} 
/*------------------------------------------------------------------------------------------------*/  
//Trinh phuc vu ngat cua INT0 DK TB1
ISR(INT0_vect)
{
	choise_device++;
	if(choise_device>=5)	
		{
			clr_LCD();
			choise_device=0;
			Display();
			adc_out(read_adc(0));
		}	
	if(choise_device>0&&choise_device<5)	check_on_off(choise_device);
}

/*------------------------------------------------------------------------------------------------*/  
//Trinh phuc vu ngat cua INT1 DK TB2
ISR(INT1_vect)
{
	switch (choise_device)
	{
		case 1: 	if(bit_is_clear(PORTC,PINC7))	setbit(PORTC,PORTC7);	else	clrbit(PORTC,PORTC7); break;
		case 2: 	if(bit_is_clear(PORTC,PINC6))	setbit(PORTC,PORTC6);	else	clrbit(PORTC,PORTC6); break;
		case 3: 	if(bit_is_clear(PORTC,PINC5))	setbit(PORTC,PORTC5);	else	clrbit(PORTC,PORTC5); break;
		case 4: 	if(bit_is_clear(PORTC,PINC4))	setbit(PORTC,PORTC4);	else	clrbit(PORTC,PORTC4); break;
		default: break;
	}
	if(choise_device>0&&choise_device<5)	check_on_off(choise_device);
}
 

/*================================================================================================*/    
/*------------------------------------------------End---------------------------------------------*/  
/*================================================================================================*/  		