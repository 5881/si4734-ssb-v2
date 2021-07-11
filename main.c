/* 9 мая 2021 работает настройка, переключение ам/fm
 * начата разработка интерфейса
 * 10 мая 2021 SI4734 поддерживает SSB с тем же патчем что и SI4735!!! 
 * В качестве контроллера использован stm32f030
 */



/**********************************************************************
 * Секция include и defines
**********************************************************************/
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/cm3/scb.h>
#include "oledi2c.h"
#include "oled_printf.h"
#include "si4734.h"
#include "patch_init.h"


#define AM_MODE 0
#define FM_MODE 1
#define SSB_MODE 2
#define SYNC_MODE 3
#define led_toggle() gpio_toggle(GPIOA,GPIO11)
#define led_on() gpio_set(GPIOA,GPIO11)
#define led_off() gpio_clear(GPIOA,GPIO11)

uint16_t MIN_LIMIT=200;
uint16_t  MAX_LIMIT=30000;
//#define IF_FEQ 455

uint16_t encoder=15200;
uint16_t pwm1=750;
uint16_t coef=5;
uint8_t encoder_mode=2;
int16_t bfo=0;
int16_t vol=0x1a;

/*
11 метров, 25.60 — 26.10 МГц (11,72 — 11,49 метра).
13 метров, 21.45 — 21.85 МГц (13,99 — 13,73 метра).
15 метров, 18.90 — 19.02 МГц (15,87 — 15,77 метра).
16 метров, 17.55 — 18.05 МГц (17,16 — 16,76 метра).
19 метров, 15.10 — 15.60 МГц (19,87 — 18,87 метра).
22 метра, 13.50 — 13.87 МГц (22,22 — 21,63 метра).
25 метров 11.60 — 12.10 МГц (25,86 — 24,79 метра).
31 метр, 9.40 — 9.99 МГц (31,91 — 30,03 метра).
41 метр, 7.20 — 7.50 МГц (41,67 — 39,47 метра).
49 метров, 5.85 — 6.35 МГц (52,36 — 47,66 метра).
60 метров, 4.75 — 5.06 МГц (63,16 — 59,29 метра).
75 метров, 3.90 — 4.00 МГц (76,92 — 75 метров).
90 метров, 3.20 — 3.40 МГц (93,75 — 88,24 метров).
120 метров (средние волны), 2.30 — 2.495 МГц (130,43 — 120,24 метра).
*/
uint16_t bands[]={200,1000,3100,3600,5800,7200,9300,11200,13500,14200,15100,17450,21500,27000};
uint8_t steps[]={1,5,10,50};
uint8_t reciver_mode=0;
//0 - am, 1 -fm, 2 - ssb

/*
 * encoder pa1 -кнопка, pa2,pa3 exti2_3_isr()
 * i2c pb6,pb7
 * pcf8574 pa0 - прерывание exti0_1_isr()
 * 
 * 
 */ 

void stop(void){
	 PWR_CR &= ~PWR_CR_PDDS;
	 SCB_SCR|=SCB_SCR_SLEEPDEEP;
	 __asm__("WFI");
	}



/**********************************************************************
 * keyboard section
 * клавиатура сидит на шине i2c и вызывает прерывание по PA0
 **********************************************************************/
#define PCF_ADDRES 0x20
void pcf_write(uint8_t data){
	i2c_transfer7(I2C1,PCF_ADDRES,&data,1,0,0);
}
uint8_t pcf_read(void){
	uint8_t temp=0;
	i2c_transfer7(I2C1,PCF_ADDRES,0,0,&temp,1);
	return temp;
}

/*
 * 0xfe 0xfd 0xfb 0xf7
 * 0xef 0xdf 0xbf 0x7f
 */ 
void keyboard(void){
	uint8_t key;
	if(!gpio_get(GPIOA,GPIO1)){
		while(!gpio_get(GPIOA,GPIO1));
			encoder_mode=0;
			if(reciver_mode==SSB_MODE){
				encoder=encoder-bfo/1000;
				bfo=bfo%1000;
				}
			}
	key=pcf_read();
	if(key!=0xff){
		//реагируем на клавиатуру
		led_toggle();
		o_printf_at(0,7,1,0,"Key code 0x%x",key);
		switch(key){
			case 0xfe:
				//reciver_set_mode(AM_MODE);
				reciver_next_mode();
				break;
			case 0xfd:
					if(reciver_mode==AM_MODE){
					si4734_am_seek(encoder,0);
					si4734_get_freq_v2(&encoder);}
				//stop();
				break;
			case 0xfb:
				//reciver_set_mode(SSB_MODE);
				if(reciver_mode==AM_MODE){
					si4734_am_seek(encoder,1);
					si4734_get_freq_v2(&encoder);}
				break;
			case 0xef:
				encoder_mode=1;
				break;
			case 0xdf:
				//encoder_mode=2;
				next_step();
				break;
			case 0xbf:
				if(reciver_mode==2){
					encoder_mode=3;//BFO только в режиме SSB
					encoder-=bfo/1000;
					bfo=bfo%1000;
				}
				break;
			case 0x7f:
				//si4734_volume(-7);//тише
				encoder_mode=4;
				break;
			case 0xf7:
				//si4734_volume(7);//громче
				reciver_am_ssb_mode();
				break;
		}
	}
	while(pcf_read()!=0xff);
	}


void pcf_init(void){
	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_INPUT,
					GPIO_PUPD_NONE, GPIO0);
	//exti_select_source(EXTI0,GPIOA);
	//exti_set_trigger(EXTI0,EXTI_TRIGGER_FALLING);
	//nvic_enable_irq(NVIC_EXTI0_1_IRQ);
	//exti_enable_request(EXTI0);
	pcf_write(0xff);
	
}

void reciver_next_mode(){
	static uint8_t rm=1;
	rm+=1;
	if(rm>2)rm=0;
	reciver_set_mode(rm);
}
void reciver_am_ssb_mode(void){
	switch (reciver_mode){
		case AM_MODE:
			reciver_set_mode(SSB_MODE);
			break;
		case SSB_MODE:
			reciver_set_mode(AM_MODE);
			break;
		}
	}

void reciver_set_mode(uint8_t rec_mod){
static uint16_t amfreq=15200,fmfreq=8910;//запоминаем старое значение 
si4734_powerdown();							//частоты
if(reciver_mode==FM_MODE)fmfreq=encoder; else amfreq=encoder;
if(rec_mod==AM_MODE){
	//o_printf("AM mode\n");
	reciver_mode=AM_MODE;
	si4734_am_mode();
	si4734_set_prop(AM_CHANNEL_FILTER, 0x0100);
	si4734_set_prop(AM_SOFT_MUTE_MAX_ATTENUATION, 0);//soft mute off
	si4734_set_prop(AM_AUTOMATIC_VOLUME_CONTROL_MAX_GAIN, 0x5000); //60дб
	si4734_set_prop(RX_VOLUME, vol);
	//si4734_set_prop(AM_SEEK_BAND_TOP, 30000);
	MIN_LIMIT=200;
	MAX_LIMIT=30000;
	//encoder=15200;
	encoder=amfreq-bfo/1000;//поправка на bfo
	bfo=bfo%1000;
	si4734_am_set_freq(encoder);
	coef=1;
	encoder_mode=0;
	} else if(rec_mod==FM_MODE){
	//oled_clear();
	//o_printf("FM mode\n");
	reciver_mode=FM_MODE;
	si4734_fm_mode();
	si4734_set_prop(FM_DEEMPHASIS,0x0001);//01 = 50 µs. Used in Europe, Australia, Japan
	si4734_set_prop(RX_VOLUME, vol);
	MIN_LIMIT=6000;
	MAX_LIMIT=11100;
	coef=1;
	//encoder=8910;
	encoder=fmfreq;
	si4734_fm_set_freq(encoder);
	encoder_mode=0;
	}else{
	reciver_mode=SSB_MODE;
	//bfo=0;
	si4734_ssb_patch_mode(ssb_patch_content);
	si4734_set_prop(0x0101,((1<<15)|(1<<12)|(1<<4)|2));//ssb man page 24
	si4734_set_prop(SSB_BFO, bfo);
	si4734_set_prop(AM_SOFT_MUTE_MAX_ATTENUATION, 0);//soft mute off
	si4734_set_prop(AM_AUTOMATIC_VOLUME_CONTROL_MAX_GAIN, 0x7000); //84дб
	si4734_set_prop(RX_VOLUME, vol);
	MIN_LIMIT=200;
	MAX_LIMIT=30000;
	//encoder=7100;
	encoder=amfreq;
	si4734_ssb_set_freq(encoder);
	coef=1;
	encoder_mode=0;
	}
	
}

void select_band(int8_t direction){
	static int8_t band=5;
	band+=direction;
	if(band<0)band=0;
	if(band>13) band=13;
	encoder=bands[band];
	}
void select_step(int8_t direction){
	static int8_t step=1;
	step+=direction;
	if(step<0)step=0;
	if(step>3)step=3;
	coef=steps[step];
	}

void next_step(void){
	static int8_t step=1;
	step++;
	if(step<0)step=3;
	if(step>3)step=0;
	coef=steps[step];
	}



void exti_encoder_init(){
		/*	энкодер с подтяжкой к питанию.
	 * PA2 энкодер 1
	 * PA3 энклдер 2
	 * PA1 кнопка энкодера
	 */
	//Enable GPIOA clock.
	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_INPUT,
					GPIO_PUPD_NONE, GPIO1|GPIO2|GPIO3);
	exti_select_source(EXTI2,GPIOA);
	//Прерывания по ноге PA2
	exti_set_trigger(EXTI2,EXTI_TRIGGER_FALLING);
	exti_enable_request(EXTI2);
	nvic_enable_irq(NVIC_EXTI2_3_IRQ);
	}

void exti0_1_isr(void){
	
	led_toggle();
	//if(sleep_mode)wake();
	//Флаг прерывания надо сбросить вручную
	uint8_t key=pcf_read();
	exti_reset_request(EXTI0);
	}
	
void exti2_3_isr(void){
	//led_toggle();
	//indicate(2);
	//o_printf_at(0,6,1,0,"ISR_8_pin9=%x",((gpio_get(GPIOA,GPIO7))>>7));
	int8_t encoder_direction;
	if(gpio_get(GPIOA,GPIO3))encoder_direction=1;else encoder_direction=-1;
	//o_printf_at(0,6,1,0,"ISR_8_pin9=%x",((gpio_get(GPIOA,GPIO7))>>7));
	if(encoder_mode==0){
		encoder+=coef*encoder_direction;
		if(encoder<MIN_LIMIT)encoder=MIN_LIMIT;
		if(encoder>MAX_LIMIT)encoder=MAX_LIMIT;
		}
	//if(encoder_mode==1){
	//	pwm1+=10*encoder_direction;
	//	if(pwm1<100)pwm1=100;
	//	if(pwm1>3300)pwm1=3300;
	//	timer_set_oc_value(TIM3, TIM_OC4, pwm1);
	//	}
	if(encoder_mode==1)select_band(encoder_direction);
	if(encoder_mode==2)select_step(encoder_direction);
	if(encoder_mode==3)bfo-=100*encoder_direction;
	if(encoder_mode==4){vol+=7*encoder_direction;
						if(vol<0)vol=0;
						if(vol>0x3f)vol=0x3f;};
	exti_reset_request(EXTI2);
	}	


static void i2c_setup(void){
	/* Enable clocks for I2C1 and AFIO. */
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_I2C1);
	/* Set alternate functions for the SCL and SDA pins of I2C1.
	 * SDA Pb7
	 * SCL Pb6
	 *  */
	gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO6|GPIO7);
	gpio_set_output_options(GPIOB, GPIO_OTYPE_OD,
                    GPIO_OSPEED_2MHZ, GPIO6|GPIO7);
    gpio_set_af(GPIOB,GPIO_AF1,GPIO6|GPIO7);//ремапинг на i2c
	/* Disable the I2C before changing any configuration. */
	i2c_peripheral_disable(I2C1);
	i2c_set_speed(I2C1,i2c_speed_fm_400k,8);
	i2c_peripheral_enable(I2C1);
}	



void rcc_clock_setup_in_hsi_out_64mhz(void)
 {
         rcc_osc_on(RCC_HSI);
         rcc_wait_for_osc_ready(RCC_HSI);
         rcc_set_sysclk_source(RCC_HSI);
  
         rcc_set_hpre(RCC_CFGR_HPRE_NODIV);
         rcc_set_ppre(RCC_CFGR_PPRE_NODIV);
  
         flash_prefetch_enable();
         flash_set_ws(FLASH_ACR_LATENCY_024_048MHZ);
  
         /* 8MHz * 12 / 2 = 48MHz */
         rcc_set_pll_multiplication_factor(RCC_CFGR_PLLMUL_MUL16);
         rcc_set_pll_source(RCC_CFGR_PLLSRC_HSI_CLK_DIV2);
  
         rcc_osc_on(RCC_PLL);
         rcc_wait_for_osc_ready(RCC_PLL);
         rcc_set_sysclk_source(RCC_PLL);
  
         rcc_apb1_frequency = 64000000;
         rcc_ahb_frequency = 64000000;
 }

void rcc_clock_setup_in_hsi_out_8mhz(void)
 {
         rcc_osc_on(RCC_HSI);
         rcc_wait_for_osc_ready(RCC_HSI);
         rcc_set_sysclk_source(RCC_HSI);
  
         rcc_set_hpre(RCC_CFGR_HPRE_NODIV);
         rcc_set_ppre(RCC_CFGR_PPRE_NODIV);
  
         flash_prefetch_enable();
         flash_set_ws(FLASH_ACR_LATENCY_000_024MHZ);
  
         /* 8MHz * 4 / 2 = 16MHz */
         rcc_set_pll_multiplication_factor(RCC_CFGR_PLLMUL_MUL2);
         rcc_set_pll_source(RCC_CFGR_PLLSRC_HSI_CLK_DIV2);
  
         rcc_osc_on(RCC_PLL);
         rcc_wait_for_osc_ready(RCC_PLL);
         rcc_set_sysclk_source(RCC_PLL);
  
         rcc_apb1_frequency = 8000000;
         rcc_ahb_frequency = 8000000;
 }

void led_setup(){
	//отладочный светодиод PA11
	//SI4730 RST pin PA7
	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT,
					GPIO_PUPD_NONE, GPIO11|GPIO7);
	gpio_set_output_options(GPIOA, GPIO_OTYPE_PP,
                    GPIO_OSPEED_2MHZ, GPIO11|GPIO7);
                    gpio_set(GPIOA, GPIO11|GPIO7);
    }


void gpio_setup(){
	//PA7 si rst pin
	rcc_periph_clock_enable(SI4734D60_RSTPORT);
	gpio_mode_setup(SI4734D60_RSTPORT, GPIO_MODE_OUTPUT,
					GPIO_PUPD_NONE, SI4734D60_RSTPIN);
	gpio_set_output_options(SI4734D60_RSTPORT, GPIO_OTYPE_PP,
                    GPIO_OSPEED_2MHZ, SI4734D60_RSTPIN);
    gpio_clear(SI4734D60_RSTPORT, SI4734D60_RSTPIN);
      
    }
	
	

void print_prop(uint16_t prop){
	o_printf("prop %x is 0x%x\n",prop, si4734_get_prop(prop));
	
}

uint8_t get_recivier_signal_status(uint8_t *snr,uint8_t *rssi,uint8_t *freq_of){
	uint8_t status,resp1,resp2;
	switch(reciver_mode){
		case AM_MODE: status=si4734_am_signal_status(&resp1,&resp2,rssi,snr);
			break;
		case FM_MODE: status=si4734_fm_signal_status(rssi,snr,freq_of);
			break;
		case SSB_MODE: status=si4734_am_signal_status(&resp1,&resp2,rssi,snr);
			break;
		}
	return status;
	}
/********************************************************************
 * Секция функций оформления дисплея
 ********************************************************************/
void logo(void){
	o_printf_at(0,0,1,0,"The best AM/FM tuner!");
	}
void show_freq(uint16_t freq, int16_t offset){
	//NB колонки в пиксилях, а строки по 8 пикселей, отсчёт с нуля из
	//верхнего левого угла
	uint16_t offset_hz;
	uint16_t freq_khz;
	if(reciver_mode==FM_MODE)o_printf_at(18*5,1,1,0,"x10");
		else o_printf_at(18*5,1,1,0,"   ");
	//Вся эта канитель от того что bfo работает не как описанно в 
	//в даташите f=f-bfo а f=f-bfo
	if(reciver_mode==SSB_MODE){
		offset_hz=(1000-offset%1000)%1000;
		freq_khz=freq-offset/1000;
		if(offset%1000>0)freq_khz--;
		o_printf_at(18*5,3,1,0,"%03d",offset_hz);
		o_printf_at(0,1,3,0,"%5d",freq_khz);}
	else{
		o_printf_at(18*5,3,1,0,"   ");
		o_printf_at(0,1,3,0,"%5d",freq);
		}
	o_printf_at(18*5,2,1,0,"KHz");
	
	}
void show_reciver_status(uint8_t snr, uint8_t rssi, uint8_t status){
	uint8_t n=1;
	o_printf_at(1,4,1,0,"SNR:%2ddB SI: %2duVdB",snr,rssi);
	//coef - глобальная переменная
	//n поправочный коэфициэнт шага
	if(reciver_mode==FM_MODE)n=10;
	o_printf_at(1,5,1,0,"status x%x %dKHz   ",status,coef*n);
	}

void show_reciver_full_status(uint16_t freq, int16_t offset,
				uint8_t snr, uint8_t rssi, uint8_t status){
	show_freq(freq, offset);
	show_reciver_status(snr,rssi,status);
	}


/*********************************************************************
 * ssb mode test
 *********************************************************************/





void main(){
	uint32_t old_enc=0;
	int16_t old_bfo=0;
	uint32_t count=0;
	uint8_t status,rev,snr,rssi,resp1,resp2;
	int8_t freq_of;
	uint16_t j=0,freq;
	uint16_t old_encoder=0;
	uint16_t old_vol=0x1A;
	//test();
	//o_printf("si4734 init start");
	rcc_clock_setup_in_hsi_out_48mhz();
	//gpio_setup();
	exti_encoder_init();
	i2c_setup();
	led_setup();
	led_on();
	oled_init();
	oled_clear();
	pcf_init();
	logo();
	//while(1);
	si4734_reset();
	for(uint32_t i=0;i<0x5ff;i++)__asm__("nop");
	reciver_set_mode(FM_MODE);
		
	while(1){
		
		//spi_send8(SPI1,j++);
		//exti_disable_request(EXTI0);
		logo();
		if(old_encoder!=encoder){
			old_encoder=encoder;
			if(reciver_mode==1) status=si4734_fm_set_freq(encoder);
			else if(reciver_mode==2) status=si4734_ssb_set_freq(encoder);
				else status=si4734_am_set_freq(encoder);
				}
		//show_freq(encoder);	
		
		if(old_bfo!=bfo && reciver_mode==2){
						si4734_set_prop(SSB_BFO, bfo);
						o_printf_at(1,6,1,0,"BFO: %d  ",-bfo);
						if(bfo>16000||bfo<-16000){
							encoder=encoder-bfo/1000;
							bfo=bfo%1000;
							}
						//si4734_ssb_set_freq(encoder);
						old_bfo=bfo;
						}
		if(old_vol!=vol){old_vol=vol;
				si4734_set_prop(RX_VOLUME, vol);
				}
		//if(!reciver_mode) status=si4734_am_signal_status(&resp1,&resp2,&rssi,&snr);
		//else if(reciver_mode==1) status=si4734_fm_signal_status(&rssi,&snr,&freq_of);
		//	else if(reciver_mode==2) status=si4734_am_signal_status(&resp1,&resp2,&rssi,&snr);
		get_recivier_signal_status(&snr,&rssi,&freq_of);
		show_reciver_full_status(encoder,bfo,snr,rssi,status);
		//exti_enable_request(EXTI0);
		keyboard();
		for(uint32_t i=0;i<0xffff;i++)__asm__("nop");
		
		
		}
	}

