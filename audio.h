#ifndef AUDIO_H
#define AUDIO_H

#include "common.h"

#include <fmod.hpp>
#include <fmod_errors.h>

template<class F1, class F2, class F3>
F1 clamp(F1 v, F2 mn, F3 mx) {
	return std::max(std::min(v, (F1)mx), (F1)mn);
}

template<class F1, class F2, class F3>
decltype(F1{}+F2{}) mix(F1 v1, F2 v2, F3 a) {
	using RetType = decltype(F1{}+F2{});

	return (RetType)(v1 * (1-a) + v2 * a);
}

// Returns frequency of note offset from A
constexpr f32 ntof(f32 n){
	return 220.f * std::pow(2.f, n/12.f);
}

namespace Wave {
	f32 sin(f64 phase);
	f32 saw(f64 phase);
	f32 sqr(f64 phase, f64 width = 0.5);
	f32 tri(f64 phase);
	f32 noise(f64 phase);
}

namespace Env {
	f32 linear(f32 phase);
	f32 exp(f32 phase);
	f32 ar(f32 phase, f32 turn = 0.5f);
	f32 ear(f32 phase, f32 turn = 0.5f);
}


void InitAudio();
void DeinitAudio();

void UpdateAudio();

enum {
	SYNVALUE,
	
	SYNSINE,
	SYNTRIANGLE,
	SYNSAW,
	SYNSQUARE,
	SYNNOISE,

	SYNLINEARENV,
	SYNARENV,

	SYNTRIGGER,
	SYNCYCLE,

	SYNNEG,

	SYNADD,
	SYNSUB,
	SYNMUL,
	SYNDIV,
	SYNPOW,

	SYNMIDIKEY,
	SYNMIDIKEYVEL,
	SYNMIDIKEYTRIGGER,
	SYNMIDICONTROL,

	SYNOUTPUT,

	SYNCOUNT
};

struct SynthNode {
	u32 id;
	u8 type;

	union {
		// Wave
		struct {
			s32 frequency;
			s32 phaseOffset;
			s32 duty;
		};
	
		// Value
		f32 value;

		// Binary Op
		struct {
			s32 left;
			s32 right;
		};

		// Unary
		s32 operand;

		// Env
		struct {
			s32 trigger;
			s32 attack;
			s32 release;
		};

		// Cycle
		struct {
			s32 seqrate;
			u32 seqlength;
			s32* sequence;
		};

		u8 midiCtl;
		struct {
			s8 midiKey;
			u8 midiKeyMode;
		};

		u16 triggerID;
	};

	SynthNode() {}
	~SynthNode() {}
};

SynthNode* NewSynthNode(u8);
u32 NewValue(f32 value);
SynthNode* GetSynthNode(u32);

void SetOutputNode(u32);

void DumpSynthNodes();
void CompileSynth();

void SetTrigger(u16, bool);
void SetMidiControl(u8, f32);
// void SetMidiKey(f32 freq, f32 vel);
void NotifyMidiKey(u8 key, f32 vel);

#endif