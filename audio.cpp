#include "audio.h"

#include <algorithm>
#include <cmath>

#include <fmod.hpp>
#include <fmod_errors.h>

namespace {
	FMOD::System* fmodSystem;
	std::vector<Synth*> synths;
}

static void cfmod(FMOD_RESULT result) {
	if (result != FMOD_OK) {
		fprintf(stderr, "FMOD error! (%d) %s\n", result, FMOD_ErrorString(result));
		std::exit(1);
	}
}

template<class F1, class F2, class F3>
F1 clamp(F1 v, F2 mn, F3 mx) {
	return std::max(std::min(v, (F1)mx), (F1)mn);
}

FMOD_RESULT F_CALLBACK DSPCallback(FMOD_DSP_STATE*, f32*, f32*, u32, s32, s32*);

Synth* CreateSynth() {
	auto s = new Synth{};
	s->playing = false;

	FMOD::DSP* dsp;

	{	FMOD_DSP_DESCRIPTION desc;
		memset(&desc, 0, sizeof(desc));

		desc.numinputbuffers = 0;
		desc.numoutputbuffers = 1;
		desc.read = DSPCallback;
		desc.userdata = s;

		cfmod(fmodSystem->createDSP(&desc, &dsp));
		cfmod(dsp->setChannelFormat(FMOD_CHANNELMASK_MONO,1,FMOD_SPEAKERMODE_MONO));
	}

	cfmod(dsp->setBypass(false));

	FMOD::ChannelGroup* mastergroup;
	FMOD::Channel* channel;

	cfmod(fmodSystem->getMasterChannelGroup(&mastergroup));
	cfmod(fmodSystem->playDSP(dsp, mastergroup, false, &channel));
	cfmod(channel->setMode(FMOD_2D));
	cfmod(channel->setVolume(0.7f));

	synths.push_back(s);
	return s;
}

Synth* GetSynth(u32 id) {
	if(id >= synths.size())
		return nullptr;

	return synths[id];
}

template<class... Args>
u32 CreateNode(Synth* syn, NodeType type, Args&&... vargs) {
	SynthParam args[] {vargs...};

	SynthNode node;
	node.type = type;
	node.inputTypes = 0;
	for(u32 i = 0; i < sizeof...(vargs); i++) {
		node.inputTypes |= args[i].isNode?(1u<<i):0;
		node.inputs[i] = args[i].node;
	}

	syn->nodes.push_back(node);
	return syn->nodes.size()-1u;
}

u32 NewSinOscillator(Synth* syn, SynthParam freq, SynthParam phaseOffset) {
	return CreateNode(syn, NodeType::SourceSin, freq, phaseOffset);
}
u32 NewTriOscillator(Synth* syn, SynthParam freq, SynthParam phaseOffset) {
	return CreateNode(syn, NodeType::SourceTri, freq, phaseOffset);
}
u32 NewSqrOscillator(Synth* syn, SynthParam freq, SynthParam phaseOffset, SynthParam duty) {
	return CreateNode(syn, NodeType::SourceSqr, freq, phaseOffset, duty);
}
u32 NewSawOscillator(Synth* syn, SynthParam freq, SynthParam phaseOffset) {
	return CreateNode(syn, NodeType::SourceSaw, freq, phaseOffset);
}
u32 NewNoiseSource(Synth* syn) {
	return CreateNode(syn, NodeType::SourceNoise);
}
u32 NewTimeSource(Synth* syn) {
	return CreateNode(syn, NodeType::SourceTime);
}

u32 NewFadeEnvelope(Synth* syn, SynthParam duration, u32 trigger) {
	return CreateNode(syn, NodeType::EnvelopeFade, duration, trigger);
}
u32 NewADSREnvelope(Synth* syn, SynthParam attack, SynthParam decay, SynthParam sustain, SynthParam sustainlvl, 
	SynthParam release, u32 trigger) {
	return CreateNode(syn, NodeType::EnvelopeADSR, attack, decay, sustain, sustainlvl, release, trigger);
}

u32 NewAddOperation(Synth* syn, SynthParam left, SynthParam right) {
	return CreateNode(syn, NodeType::MathAdd, left, right);
}
u32 NewSubtractOperation(Synth* syn, SynthParam left, SynthParam right) {
	return CreateNode(syn, NodeType::MathSubtract, left, right);
}
u32 NewMultiplyOperation(Synth* syn, SynthParam left, SynthParam right) {
	return CreateNode(syn, NodeType::MathMultiply, left, right);
}
u32 NewDivideOperation(Synth* syn, SynthParam left, SynthParam right) {
	return CreateNode(syn, NodeType::MathDivide, left, right);
}
u32 NewPowOperation(Synth* syn, SynthParam left, SynthParam right) {
	return CreateNode(syn, NodeType::MathPow, left, right);
}
u32 NewNegateOperation(Synth* syn, SynthParam arg) {
	return CreateNode(syn, NodeType::MathNegate, arg);
}

u32 NewSynthControl(Synth* syn, const char* name, f32 initialValue) {
	std::lock_guard<std::mutex>(syn->mutex);
	syn->controls.push_back({strdup(name), initialValue, initialValue, initialValue, 0.f});
	return CreateNode(syn, NodeType::InteractionValue, syn->controls.size()-1u);
}

u32 NewSynthTrigger(Synth* syn, const char* name) {
	std::lock_guard<std::mutex>(syn->mutex);
	syn->triggers.push_back({strdup(name), 0});
	return syn->triggers.size()-1u;
}

void SetSynthControl(Synth* syn, const char* name, f32 val, f32 lerpTime) {
	auto it = std::find_if(syn->controls.begin(), syn->controls.end(), [name](const SynthControl& ctl){
		return !strcmp(ctl.name, name);
	});
	
	if(it != syn->controls.end()) {
		std::lock_guard<std::mutex>(syn->mutex);
		it->begin = it->value;
		it->target = val;
		it->lerpTime = lerpTime;
	}
}
void TripSynthTrigger(Synth* syn, const char* name) {
	auto it = std::find_if(syn->triggers.begin(), syn->triggers.end(), [name](const SynthTrigger& ctl){
		return !strcmp(ctl.name, name);
	});

	if(it != syn->triggers.end()) {
		std::lock_guard<std::mutex>(syn->mutex);
		it->state = 1;
	}
}

void UpdateSynthNode(Synth* syn, u32 nodeID);

f32 EvaluateSynthNodeInput(Synth* syn, SynthNode* node, u8 input) {
	if(node->inputTypes&(1<<input)) {
		u32 nodeID = node->inputs[input].node;
		UpdateSynthNode(syn, nodeID);
		return syn->nodes[nodeID].foutput;
	}

	return node->inputs[input].value;
}

u32 EvaluateTrigger(Synth* syn, SynthNode* node, u8 input) {
	u32 nodeID = node->inputs[input].node;
	if(nodeID == ~0u) return 0;
	return syn->triggers[nodeID].state;
}

void UpdateSynthNode(Synth* syn, u32 nodeID) {
	auto node = &syn->nodes[nodeID];
	if(node->frameID == syn->frameID) // Already updated
		return;

	node->frameID = syn->frameID;

	switch(node->type) {
		case NodeType::SourceSin: {
			f32 freq = EvaluateSynthNodeInput(syn, node, 0);
			f32 phaseOffset = EvaluateSynthNodeInput(syn, node, 1);
			node->foutput = std::sin(2.0*M_PI*(node->phase+phaseOffset));
			node->phase += freq * syn->dt;
		}	break;
		case NodeType::SourceTri: {
			f32 freq = EvaluateSynthNodeInput(syn, node, 0);
			f32 phaseOffset = EvaluateSynthNodeInput(syn, node, 1);
			auto nph = std::fmod((node->phase+phaseOffset), 1.f);
			node->foutput = (nph <= 0.5f)
				?(nph-0.25f)*4.f
				:(0.75f-nph)*4.f;
			node->phase += freq * syn->dt;
		}	break;
		case NodeType::SourceSaw: {
			f32 freq = EvaluateSynthNodeInput(syn, node, 0);
			f32 phaseOffset = EvaluateSynthNodeInput(syn, node, 1);
			node->foutput = std::fmod((node->phase+phaseOffset)*2.f, 2.f)-1.f;
			node->phase += freq * syn->dt;
		}	break;
		case NodeType::SourceSqr: {
			f32 freq = EvaluateSynthNodeInput(syn, node, 0);
			f32 phaseOffset = EvaluateSynthNodeInput(syn, node, 1);
			f32 width = EvaluateSynthNodeInput(syn, node, 2);
			width = clamp(width/2.f, 0.f, 1.f);
			auto nph = std::fmod((node->phase+phaseOffset), 1.f);
			node->foutput = (nph < width)? -1.f : 1.f;
			node->phase += freq * syn->dt;
		}	break;
		case NodeType::SourceNoise: {
			node->foutput = (std::rand() %100000) / 50000.f - 0.5f;
		}	break;
		case NodeType::SourceTime: {
			node->foutput = syn->time;
		}	break;


		case NodeType::EnvelopeFade: {
			f32 duration = EvaluateSynthNodeInput(syn, node, 0);

			if(EvaluateTrigger(syn, node, 1))
				node->phase = 0.f;	

			node->foutput = node->phase;
			node->phase = clamp(node->phase + syn->dt/duration, 0.f, 1.f);
		}	break;
		case NodeType::EnvelopeADSR: {
			f32 attack = EvaluateSynthNodeInput(syn, node, 0);
			f32 decay = EvaluateSynthNodeInput(syn, node, 1);
			f32 sustain = EvaluateSynthNodeInput(syn, node, 2);
			f32 sustainlvl = EvaluateSynthNodeInput(syn, node, 3);
			f32 release = EvaluateSynthNodeInput(syn, node, 4);

			if(EvaluateTrigger(syn, node, 5)){
				if(node->phase < (attack+decay+sustain+release))
					node->phase = node->foutput*attack;
				else
					node->phase = 0.f;
			}

			f32 phase = node->phase;
			node->phase += syn->dt;

			if(phase < attack) {
				node->foutput = phase/attack;
				break;
			}
			phase -= attack;
			if(phase < decay) {
				node->foutput = (1.f-phase/decay*(1.f-sustainlvl));
				break;
			}
			phase -= decay;
			if(phase < sustain) {
				node->foutput = sustainlvl;
				break;
			}
			phase -= sustain;
			if(phase < release) {
				node->foutput = (1.f - phase/release)*sustainlvl;
				break;
			}

			node->foutput = 0.f;
		}	break;

		case NodeType::MathAdd: {
			f32 a = EvaluateSynthNodeInput(syn, node, 0);
			f32 b = EvaluateSynthNodeInput(syn, node, 1);
			node->foutput = a+b;
		}	break;
		case NodeType::MathSubtract: {
			f32 a = EvaluateSynthNodeInput(syn, node, 0);
			f32 b = EvaluateSynthNodeInput(syn, node, 1);
			node->foutput = a-b;
		}	break;
		case NodeType::MathMultiply: {
			f32 a = EvaluateSynthNodeInput(syn, node, 0);
			f32 b = EvaluateSynthNodeInput(syn, node, 1);
			node->foutput = a*b;
		}	break;
		case NodeType::MathDivide: {
			f32 a = EvaluateSynthNodeInput(syn, node, 0);
			f32 b = EvaluateSynthNodeInput(syn, node, 1);
			node->foutput = a/b;
		}	break;
		case NodeType::MathPow: {
			f32 a = EvaluateSynthNodeInput(syn, node, 0);
			f32 b = EvaluateSynthNodeInput(syn, node, 1);
			node->foutput = std::pow(a, b);
		}	break;
		case NodeType::MathNegate: {
			f32 a = EvaluateSynthNodeInput(syn, node, 0);
			node->foutput = -a;
		}	break;

		case NodeType::InteractionValue: {
			u32 ctlid = node->inputs[0].node;
			node->foutput = syn->controls[ctlid].value;
		}	break;

		default: break;
	}
}

FMOD_RESULT F_CALLBACK DSPCallback(FMOD_DSP_STATE* dsp_state, 
	f32* /*inbuffer*/, f32* outbuffer, u32 length, 
	s32 /*inchannels*/, s32* outchannels){

	assert(*outchannels == 1);

	auto thisdsp = (FMOD::DSP *)dsp_state->instance; 

	Synth* ud = nullptr;
	cfmod(thisdsp->getUserData((void**)&ud));
	if(!ud->playing) {
		for(u32 i = 0; i < length; i++)
			outbuffer[i] = 0.f;

		return FMOD_OK;
	}

	std::lock_guard<std::mutex>(ud->mutex);

	s32 samplerate = 0;
	cfmod(dsp_state->callbacks->getsamplerate(dsp_state, &samplerate));
	f64 inc = 1.0/samplerate;
	ud->dt = inc;

	for(u32 i = 0; i < length; i++){
		ud->frameID++;
		UpdateSynthNode(ud, ud->outputNode);
		outbuffer[i] = ud->nodes[ud->outputNode].foutput;
		ud->time += ud->dt;

		for(auto& t: ud->triggers)
			t.state = 0;

		for(auto& c: ud->controls){
			f32 span = c.target-c.begin;
			if(c.lerpTime < 1e-6 || std::abs(span) < 1e-6) {
				c.value = c.target;
				continue;
			}

			f32 a = (c.value-c.begin)/span;
			if(a < 1.f) {
				c.value += span/c.lerpTime*ud->dt;
			}else if (a > 1.f) {
				c.value = c.target;
			}
		}
	}

	return FMOD_OK;
}

bool InitAudio(){
	cfmod(FMOD::System_Create(&fmodSystem));

	u32 version = 0;
	cfmod(fmodSystem->getVersion(&version));
	if(version < FMOD_VERSION){
		printf("FMOD version of at least %d required. Version used %u\n", FMOD_VERSION, version);
		throw "FMOD Error";
	}

	cfmod(fmodSystem->init(10, FMOD_INIT_NORMAL, nullptr));

	FMOD::DSP* compressor;
	cfmod(fmodSystem->createDSPByType(FMOD_DSP_TYPE_COMPRESSOR, &compressor));
	cfmod(compressor->setParameterFloat(FMOD_DSP_COMPRESSOR_THRESHOLD, -15));
	cfmod(compressor->setParameterFloat(FMOD_DSP_COMPRESSOR_ATTACK, 1));
	cfmod(compressor->setParameterFloat(FMOD_DSP_COMPRESSOR_RELEASE, 200));
	cfmod(compressor->setBypass(false));

	FMOD::ChannelGroup* mastergroup;
	cfmod(fmodSystem->getMasterChannelGroup(&mastergroup));
	cfmod(mastergroup->addDSP(0, compressor));

	// http://www.fmod.org/docs/content/generated/FMOD_REVERB_PROPERTIES.html
	// FMOD_REVERB_PROPERTIES rprops = {
	// 	.DecayTime			= 12000.0, //1500.0, /* Reverberation decay time in ms */
	// 	.EarlyDelay			= 7.0, //7.0, /* Initial reflection delay time */
	// 	.LateDelay			= 11.0, //11.0, /* Late reverberation delay time relative to initial reflection */
	// 	.HFReference		= 5000.0, /* Reference high frequency (hz) */
	// 	.HFDecayRatio		= 50.0, /* High-frequency to mid-frequency decay time ratio */
	// 	.Diffusion			= 100.0, // Value that controls the echo density in the late reverberation decay. 
	// 	.Density			= 100.0, //100.0, /* Value that controls the modal density in the late reverberation decay */
	// 	.LowShelfFrequency	= 250.0, /* Reference low frequency (hz) */
	// 	.LowShelfGain		= 0.0, /* Relative room effect level at low frequencies */
	// 	.HighCut			= 10000.0, /* Relative room effect level at high frequencies */
	// 	.EarlyLateMix		= 50.0, /* Early reflections level relative to room effect */
	// 	.WetLevel			= -4.0, //-6.0, /* Room effect level (at mid frequencies) */
	// };

	// FMOD::Reverb3D* reverb;
	// cfmod(fmodSystem->createReverb3D(&reverb));
	// cfmod(reverb->setProperties(&rprops));

	return true;
}

void DeinitAudio() {
	fmodSystem->release();
}

void UpdateAudio() {
	fmodSystem->update();
}
