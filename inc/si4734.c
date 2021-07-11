/**********************************************************************
 *Простая библиотека для работы с si4734
 *4 мая 2021г
 *10 мая 2021 SI4734 поддерживает SSB с тем же патчем что и SI4735!!!
 *16 мая 2021 добавлен автопоиск станций в ам.
 **********************************************************************/

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include "si4734.h"


#define SI4734_RST_CLR() gpio_clear(SI4734D60_RSTPORT, SI4734D60_RSTPIN)
#define SI4734_RST_SET() gpio_set(SI4734D60_RSTPORT, SI4734D60_RSTPIN)


void delay(uint16_t ms){
	uint64_t temp;
	temp=ms<<10;
	while(temp--)__asm__("nop");
	}

void si4734_reset(){
	SI4734_RST_CLR();
	delay(10);
	SI4734_RST_SET();
	delay(10);
	}

void si4734_volume(int8_t dv){
	int16_t vol=si4734_get_prop(0x4000);//AN332 p170
	vol+=dv;
	if(vol<0)vol=0;
	if(vol>0x3f)vol=0x3f;
	si4734_set_prop(0x4000,vol);
	}


uint8_t si4734_am_mode(){
	//ARG1 (1<<4)|1 AN322 p130
	//ARG2 00000101
	uint8_t cmd[3]={POWER_UP,0x11,0x05};
	uint8_t status, tray=0;
	i2c_transfer7(SI4734I2C,SI4734ADR,cmd,3,0,0);
	delay(1000);
	do{	i2c_transfer7(SI4734I2C,SI4734ADR,0,0,&status,1);
		tray++;
		if(tray==255) return 0xff;
		delay(50);
		}while(status!=0x80);
	return status;
}






uint8_t si4734_powerdown(){
	uint8_t cmd=POWER_DOWN,status;
	i2c_transfer7(SI4734I2C,SI4734ADR,&cmd,1,0,0);
	delay(200);
	i2c_transfer7(SI4734I2C,SI4734ADR,0,0,&status,1);
	return status;
}

uint16_t si4734_get_prop(uint16_t prop){
	uint8_t cmd[4]={GET_PROPERTY,0,(prop>>8),(prop&0xff)};
	uint8_t answer[4];
	i2c_transfer7(SI4734I2C,SI4734ADR,cmd,4,0,0);
	delay(100);
	i2c_transfer7(SI4734I2C,SI4734ADR,0,0,answer,4);
	return (answer[2]<<8)|answer[3];
}

uint8_t si4734_set_prop(uint16_t prop, uint16_t val){
	uint8_t cmd[6]={SET_PROPERTY,0,(prop>>8),
									(prop&0xff),(val>>8),(val&0xff)};
	uint8_t status;
	i2c_transfer7(SI4734I2C,SI4734ADR,cmd,6,0,0);
	delay(100);
	i2c_transfer7(SI4734I2C,SI4734ADR,0,0,&status,1);
	return status;
}

uint8_t si4734_get_int_status(){
	uint8_t cmd[1]={GET_INT_STATUS};
	uint8_t status, tray=0;
	i2c_transfer7(SI4734I2C,SI4734ADR,cmd,1,0,0);
	delay(10);
	status=0;
	while(status==0){
		i2c_transfer7(SI4734I2C,SI4734ADR,0,0,&status,1);
		tray++;
		if(tray==255) return 0xff;
		delay(10);
		}
	
	return status; 
}

void si4734_am_seek(uint16_t freq, uint8_t up){
	uint8_t cmd[6]={AM_SEEK_START,(1<<3),0,0,0,1}; //AN332 p138
	uint16_t top,bottom;
	//чтобы поиск на долго не вешал приёмник, ограничим диапазон до 500Кгц
	top=freq+200;
	if(top>30000)top=30000;
	if(freq<400) bottom=200;else bottom=freq-200;
	si4734_set_prop(AM_SEEK_BAND_TOP,top);
	si4734_set_prop(AM_SEEK_BAND_BOTTOM,bottom);
	si4734_set_prop(AM_SEEK_FREQ_SPACING,5);
	si4734_set_prop(AM_SEEK_SNR_THRESHOLD,1);
	//uint16_t freq;
	//uint8_t rssi,snr;
	if(!up) cmd[1]&=~(1<<3);
	i2c_transfer7(SI4734I2C,SI4734ADR,cmd,6,0,0);
	//delay(500);
	while (si4734_get_int_status()!=0x81)delay(100);
	}


uint8_t si4734_fm_mode(){
	//ARG1 (1<<4)|1 AN322 p130
	//ARG2 00000101
	uint8_t cmd[3]={POWER_UP,0x10,0x05};
	uint8_t status, tray=0;
	i2c_transfer7(SI4734I2C,SI4734ADR,cmd,3,0,0);
	delay(1000);
	do{	i2c_transfer7(SI4734I2C,SI4734ADR,0,0,&status,1);
		tray++;
		if(tray==255) return 0xff;
		delay(50);
		}while(status!=0x80);
	return status;
}

uint8_t si4734_ssb_patch_mode(uint8_t *patch){
	//ARG1 (1<<4)|1 AN322 p130
	//ARG2 00000101
	uint8_t cmd[3]={POWER_UP,0x31,0x05};
	uint8_t status, tray=0;
	uint16_t count,iterate=0;
	i2c_transfer7(SI4734I2C,SI4734ADR,cmd,3,0,0);
	delay(1000);
	do{	i2c_transfer7(SI4734I2C,SI4734ADR,0,0,&status,1);
		tray++;
		if(tray==255) return 0xff;
		delay(50);
		}while(status!=0x80);
	if(status!=0x80) return 0x1;
	//count=(sizeof patch)/8;
	count=0x451;
	while(count--){
		tray=0;
		i2c_transfer7(SI4734I2C,SI4734ADR,patch+iterate,8,0,0);
		iterate+=8;
		delay(2);
		do{	i2c_transfer7(SI4734I2C,SI4734ADR,0,0,&status,1);
		tray++;
		if(tray==255) return 0x02;
		delay(1);
		}while(status!=0x80);
	}
	return status;
}
	
uint8_t si4734_fm_set_freq(uint16_t freq_10khz){
	uint8_t fast,freq_h,freq_l,status,tray=0;
	fast=0;
	freq_h=freq_10khz>>8;
	freq_l=freq_10khz&0xff;
	uint8_t cmd[6]={FM_TUNE_FREQ,fast,freq_h,freq_l,0,0};
	i2c_transfer7(SI4734I2C,SI4734ADR,cmd,6,0,0);
	delay(20);
	//do {status=si4734_get_int_status();
		//delay(50);
		//} while(!status || status&1);
	do{	i2c_transfer7(SI4734I2C,SI4734ADR,0,0,&status,1);
		tray++;
		if(tray==255) return 0xff;
		delay(20);
		}while(status!=0x80);
	return status;
}




uint8_t si4734_get_freq(uint16_t *freq,uint8_t *snr, uint8_t *rssi){
	uint8_t cmd[2]={AM_TUNE_STATUS,0x0};
	uint8_t tray=0;
	uint8_t answer[8];
	i2c_transfer7(SI4734I2C,SI4734ADR,cmd,2,0,0);
	delay(50);
	answer[0]=0;
	while(answer[0]==0){
		i2c_transfer7(SI4734I2C,SI4734ADR,0,0,answer,8);
		tray++;
		if(tray==255) return 0xff;
		delay(50);
		}
	*freq=((answer[2]<<8)|answer[3]); 
	*rssi=answer[4];
	*snr=answer[5];
	return answer[0];
	}

uint8_t si4734_get_freq_v2(uint16_t *freq){//возвращает только частоту
	uint8_t cmd[2]={AM_TUNE_STATUS,0x0};
	uint8_t tray=0;
	uint8_t answer[8];
	i2c_transfer7(SI4734I2C,SI4734ADR,cmd,2,0,0);
	delay(50);
	answer[0]=0;
	while(answer[0]==0){
		i2c_transfer7(SI4734I2C,SI4734ADR,0,0,answer,8);
		tray++;
		if(tray==255) return 0xff;
		delay(50);
		}
	*freq=((answer[2]<<8)|answer[3]); 
	return answer[0];
	}

uint8_t si4734_am_signal_status(uint8_t *resp1,uint8_t *resp2,uint8_t *rssi,uint8_t *snr){
	uint8_t cmd[3]={AM_RSQ_STATUS,0x1};
	uint8_t tray=0;
	uint8_t answer[6];
	i2c_transfer7(SI4734I2C,SI4734ADR,cmd,2,0,0);
	delay(50);
	answer[0]=0;
	while(answer[0]==0){
		i2c_transfer7(SI4734I2C,SI4734ADR,0,0,answer,6);
		tray++;
		if(tray==255) return 0xff;
		delay(50);
		}
	*resp1=answer[1];
	*resp2=answer[2];
	*rssi=answer[4];
	*snr=answer[5];
	return answer[0];
	}

uint8_t si4734_ssb_signal_status(uint8_t *resp1,uint8_t *resp2,uint8_t *rssi,uint8_t *snr){
	uint8_t cmd[3]={AM_RSQ_STATUS,0x1};
	uint8_t tray=0;
	uint8_t answer[6];
	i2c_transfer7(SI4734I2C,SI4734ADR,cmd,2,0,0);
	delay(50);
	answer[0]=0;
	while(answer[0]==0){
		i2c_transfer7(SI4734I2C,SI4734ADR,0,0,answer,6);
		tray++;
		if(tray==255) return 0xff;
		delay(50);
		}
	*resp1=answer[1];
	*resp2=answer[2];
	*rssi=answer[4];
	*snr=answer[5];
	return answer[0];
	}

uint8_t si4734_fm_signal_status(uint8_t *rssi,uint8_t *snr,int8_t *freq_of){
	uint8_t cmd[3]={FM_RSQ_STATUS,0x1};
	uint8_t tray=0;
	uint8_t answer[8];
	i2c_transfer7(SI4734I2C,SI4734ADR,cmd,2,0,0);
	delay(50);
	answer[0]=0;
	while(answer[0]==0){
		i2c_transfer7(SI4734I2C,SI4734ADR,0,0,answer,8);
		tray++;
		if(tray==255) return 0xff;
		delay(50);
		}
	*rssi=answer[4];
	*snr=answer[5];
	*freq_of=answer[7];
	return answer[0];
	
}



uint8_t si4734_get_rev(void){
	uint8_t cmd[3]={GET_REV};
	uint8_t status, tray=0;
	uint8_t answer[9];
	i2c_transfer7(SI4734I2C,SI4734ADR,cmd,1,0,0);
	delay(50);
	answer[0]=0;
	while(answer[0]==0){
		i2c_transfer7(SI4734I2C,SI4734ADR,0,0,answer,9);
		tray++;
		if(tray==255) return 0xff;
		delay(10);
		}
	return answer[1]; 
	//return status;
	}



uint8_t si4734_am_set_freq(uint16_t freq_khz){
	uint8_t fast,freq_h,freq_l,status,tray=0;
	fast=0;
	freq_h=freq_khz>>8;
	freq_l=freq_khz&0xff;
	uint8_t cmd[6]={AM_TUNE_FREQ,fast,freq_h,freq_l,0,1};
	i2c_transfer7(SI4734I2C,SI4734ADR,cmd,6,0,0);
	delay(20);
	//do {status=si4734_get_int_status();
		//delay(50);
		//} while(!status || status&1);
	do{	i2c_transfer7(SI4734I2C,SI4734ADR,0,0,&status,1);
		tray++;
		if(tray==255) return 0xff;
		delay(20);
		}while(status!=0x80);
	return status;
}

uint8_t si4734_ssb_set_freq(uint16_t freq_khz){
	uint8_t mode,freq_h,freq_l,status,tray=0;
	if(freq_khz>10000)mode=0b10000000;else mode=0b01000000;
	freq_h=freq_khz>>8;
	freq_l=freq_khz&0xff;
	uint8_t cmd[6]={AM_TUNE_FREQ,mode,freq_h,freq_l,0,1};
	i2c_transfer7(SI4734I2C,SI4734ADR,cmd,6,0,0);
	delay(20);
	//do {status=si4734_get_int_status();
		//delay(50);
		//} while(!status || status&1);
	do{	i2c_transfer7(SI4734I2C,SI4734ADR,0,0,&status,1);
		tray++;
		if(tray==255) return 0xff;
		delay(20);
		}while(status!=0x80);
	return status;
}
