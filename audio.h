#ifndef AUDIO_H
#define AUDIO_H

#include "common.h"
#include <vector>
#include <mutex>

enum class NodeType : u8 {
	SourceSin,
	SourceTri,
	SourceSqr,
	SourceSaw,
	SourceNoise,
	SourceSampler, // TODO
	SourceTime,

	MathAdd,
	MathSubtract,
	MathMultiply,
	MathDivide,
	MathPow,
	MathNegate,

	EnvelopeFade,
	EnvelopeADSR,

	EffectsConvolution,
	EffectsLowPass,
	EffectsHighPass,

	InteractionValue,
	// InteractionTrigger,
};

union SynthInput {
	f32 value;
	u32 node;

	SynthInput() : node{0} {}
	SynthInput(f32 x) : value{x} {}
	SynthInput(f64 x) : value{f32(x)} {}
	SynthInput(u32 x) : node{x} {}
	SynthInput(s32 x) : node{u32(x)} {}
};

struct SynthNode {
	u32 frameID;
	// u8 numReferences;
	NodeType type;

	u8 inputTypes;
	SynthInput inputs[8];

	f64 phase;

	union {
		f32 foutput;
		u32 uoutput;
	};

	SynthNode() {memset(this, 0, sizeof(SynthNode));}
};

struct SynthControl {
	const char* name;
	f32 value;
	
	f32 begin;
	f32 target;
	f32 lerpTime;
};

struct SynthTrigger {
	const char* name;
	u32 state;
};

struct Synth;

using AudioPostProcessHook = void(f32* buffer, u32 length);
using SynthPostProcessHook = void(Synth*, f32* buffer, u32 length, f32 stereoCoeffs[2]);

struct Synth {
	u32 id;

	SynthPostProcessHook* chunkPostProcess;

	std::mutex mutex;
	std::vector<SynthNode> nodes;
	std::vector<SynthControl> controls;
	std::vector<SynthTrigger> triggers;

	SynthTrigger globalTrigger;
	u32 outputNode;
	u32 frameID;

	f64 dt;
	f32 time;
	bool playing;
};

struct SynthParam {
	bool isNode;
	union {
		f32 value;
		u32 node;
	};

	SynthParam(bool x, u32 n) : isNode{x}, node{n} {}
	SynthParam(f32 x) : isNode{false}, value{x} {}
	SynthParam(f64 x) : isNode{false}, value{f32(x)} {}
	SynthParam(u32 x) : isNode{true}, node{x} {}
	SynthParam(u64 x) : isNode{true}, node{u32(x)} {}
};

bool InitAudio();
void DeinitAudio();
void SetAudioPostProcessHook(AudioPostProcessHook*);
void SetSynthPostProcessHook(SynthPostProcessHook*);

Synth* CreateSynth();
Synth* GetSynth(u32);

u32 NewSinOscillator(Synth*, SynthParam freq, SynthParam phaseOffset = {0.f});
u32 NewTriOscillator(Synth*, SynthParam freq, SynthParam phaseOffset = {0.f});
u32 NewSqrOscillator(Synth*, SynthParam freq, SynthParam phaseOffset = {0.f}, SynthParam duty = {1.f}); // duty: [0, 1] -> [0%, 50%]
u32 NewSawOscillator(Synth*, SynthParam freq, SynthParam phaseOffset = {0.f});
u32 NewNoiseSource(Synth*);
u32 NewTimeSource(Synth*);

u32 NewFadeEnvelope(Synth*, SynthParam duration, u32 trigger = ~0u);
u32 NewADSREnvelope(Synth*, SynthParam attack, SynthParam decay, SynthParam sustain, SynthParam sustainlvl, SynthParam release, u32 trigger = ~0u);

u32 NewLowPassEffect(Synth*, SynthParam input, SynthParam freq);
u32 NewHighPassEffect(Synth*, SynthParam input, SynthParam freq);
// u32 NewConvolutionEffect(Synth*, SynthParam freq);

u32 NewAddOperation(Synth*, SynthParam left, SynthParam right);
u32 NewSubtractOperation(Synth*, SynthParam left, SynthParam right);
u32 NewMultiplyOperation(Synth*, SynthParam left, SynthParam right);
u32 NewDivideOperation(Synth*, SynthParam left, SynthParam right);
u32 NewPowOperation(Synth*, SynthParam left, SynthParam right);
u32 NewNegateOperation(Synth*, SynthParam arg);

u32 NewSynthControl(Synth*, const char*, f32 initialValue = 0.f);
u32 NewSynthTrigger(Synth*, const char*);

void SetSynthControl(Synth*, const char*, f32, f32 = 0.f);
void TripSynthTrigger(Synth*, const char*);

// Sources
// 	- Oscillators (Sin, saw, sqr, tri)
// 		- input: frequency, phase offset, duty
//		- output: value
// 	- Noise
// 		- output: value
//  - Sampler?
// 		- invariant: filename
//		- input: position/phase, loop, ...
// 		- output: value
//	- Time
// 		- output: value

// Envelopes
// 	- FadeIn, FadeOut
// 	- ADSR
// 		- input: attack, sustain, sustain lvl, release
//		- triggers: restart
// 		- output: value

// Effects
// 	- Convolution
// 	- LowPass, HighPass, BandPass

// Interaction
// 	- Triggers
// 		- invariant: name
// 		- output: trigger
// 	- Values
// 		- invariant: name, lerpable
// 		- output: value

#endif