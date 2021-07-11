# Всеволновый AM/FM/SSB приёмник на si4734 V2
Приёмник выполнен на микросхеме si4734 и микроконтроллере
stm32f030k6t6.
Не смотря на то что в даташитах и в сети говорят о том что SSB патч
работает только с si4735 и si4732, он ра,отает и с si4734 которая в 
5 раз дешевле.
В коде использован патч [Вадима Афонкина](https://youtu.be/fgjPGnTAVgM)
за который ему большое спасибо.
Код написан на C с использованием библиотеки LibopenCM3.
Контроллер клавиатуры выполнен на PCF8574

# Instructions
 
 1. $sudo pacman -S openocd arm-none-eabi-binutils arm-none-eabi-gcc arm-none-eabi-newlib arm-none-eabi-gdb
 2. $git clone https://github.com/5881/si4734-ssb-v2.git
 3. $cd si4734-ssb-v2
 4. $git submodule update --init # (Only needed once)
 5. $TARGETS=stm32/f0 make -C libopencm3 # (Only needed once)
 6. $make 
 7. $make flash

Александр Белый 2021
