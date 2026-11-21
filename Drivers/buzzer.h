#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>

void Buzzer_On(void);
void Buzzer_Off(void);
uint8_t Buzzer_Is_Echo(void);
//间歇鸣叫，第一个参数为间隔时长，第二个参数为鸣叫时长，单位为ms
void Buzzer_Flash(uint32_t gap, uint32_t echo);
//车祸发生通知音
void Buzzer_Alarm(void);


#endif
