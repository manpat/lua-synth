#ifndef RECORDING_H
#define RECORDING_H

#include "common.h"

bool InitRecording(const char* fname);
void RecordBuffer(const f32* buf, u32 len);
void FinishRecording();

#endif