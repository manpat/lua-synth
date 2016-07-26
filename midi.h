#ifndef MIDI_H
#define MIDI_H

#include "common.h"

struct MidiMessage {
	enum {
		None,
		Note,
		Control,
		Bend,
	};

	u8 type, b0, b1;

	operator bool() { return type != None; }
};

void InitMidi();
void DeinitMidi();

MidiMessage GetMidiMessage();

#endif