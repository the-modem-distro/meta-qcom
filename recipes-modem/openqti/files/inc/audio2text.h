#include <stdint.h>

#ifndef __AUDIO2TEXT_H__
#define __AUDIO2TEXT_H__

void speech_to_text_stay_running(uint8_t enable);
int callaudio_stt_demo(char *filename);
void *callaudio_stt_continuous();
#endif