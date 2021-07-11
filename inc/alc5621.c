#include "alc5621.h"
#include <libopencm3/stm32/i2c.h>

void alc5621_write_reg(uint8_t regadr, uint16_t regval){
	uint8_t data[3];
	data[0]=regadr;
	data[1]=regval&0xff;
	data[2]=regval>>8;
	i2c_transfer7(ALC5621PORT,ALC5621ADR,data,3,0,0);
	}
uint16_t alc5621_read_reg(uint8_t regadr){
	uint8_t data[2];
	uint16_t regval;	
	i2c_transfer7(ALC5621PORT,ALC5621ADR,&regadr,1,data,2);
	regval=(data[0]<<8)|data[1];
	return regval;
	}


void alc5621_init(){
	//комутируем linein на lineout и spkout
	//настроим тактирование
	//внешний кварц 4мгц
	//FOUT = (MCLK * (N+2)) / ((M+2) * (K+2))
	//N=240, M=8, K=2
	uint16_t alcregval;
	uint8_t N=240,M=8,K=2;
	alcregval=((N-2)<<8)|(M-2);
	//0x44 - pll reg
	alc5621_write_reg(0x44, alcregval);
	
	//linein to and hpmix
	//0xAH line in vol and mixe control
	alcregval=alc5621_read_reg(0x0A);
	alcregval&=~(1<<15);//unmute hpmixer
	alc5621_write_reg(0x0A, alcregval);
	
	//output mixer
	//0x1c out mixer registr
	alcregval=alc5621_read_reg(0x1C);
	alcregval|=(1<<10)|(1<<9)|(1<<9);
	alc5621_write_reg(0x1C, alcregval);
	//spk volume
	//0x02 spk vol register
	alcregval=alc5621_read_reg(0x02);
	alcregval&=~((1<<15)|(1<<7));
	alc5621_write_reg(0x1C, alcregval);
	
	
}
