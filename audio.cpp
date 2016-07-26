#include "audio.h"
#include <limits>
#include <vector>
#include <mutex>
#include <map>

FMOD::System* fmodSystem;
FMOD::Channel* channel;

static void cfmod(FMOD_RESULT result) {
	if (result != FMOD_OK) {
		printf("FMOD error! (%d) %s\n", result, FMOD_ErrorString(result));
		throw "FMOD Error";
	}
}

namespace Wave {
	f32 sin(f64 phase){
		return std::sin(2.0*M_PI*phase);
	}

	f32 saw(f64 phase){
		return std::fmod(phase*2.0, 2.0)-1.0;
	}

	f32 sqr(f64 phase, f64 width){
		width = clamp(width, 0.0, 1.0);
		auto nph = std::fmod(phase, 1.0);
		if(nph < width) return -1.0;

		return 1.0;
	}

	f32 tri(f64 phase){
		auto nph = std::fmod(phase, 1.0);
		if(nph <= 0.5) return (nph-0.25)*4.0;

		return (0.75-nph)*4.0;
	}

	f32 noise(f64) {
		return (std::rand() %100000) / 50000.f - 0.5f;
	}
}

namespace Env {
	f32 linear(f32 phase) {
		return clamp(phase, 0, 1);
	}

	f32 exp(f32 phase) {
		f32 v = linear(phase);
		return v*v;
	}

	f32 ar(f32 phase, f32 turn) {
		if(phase < turn)
			return clamp(phase/turn, 0, 1);

		return clamp(1-(phase-turn)/(1-turn), 0, 1);
	}

	f32 ear(f32 phase, f32 turn) {
		f32 v = Env::ar(phase, turn);
		return v*v;
	}
}

constexpr f32 infinity = std::numeric_limits<f32>::infinity();

SynthNode synthNodes[1024<<4];
u32 nodeCount = 0;
u32 triggerCount = 0;
s32 outputNode = -1;

SynthNode* NewSynthNode(u8 type) {
	synthNodes[nodeCount].id = nodeCount;
	synthNodes[nodeCount].type = type;

	switch(type) {
	case SYNSINE:
	case SYNTRIANGLE:
	case SYNSAW:
	case SYNSQUARE:
		synthNodes[nodeCount].frequency = -1;
		synthNodes[nodeCount].phaseOffset = -1;
		synthNodes[nodeCount].duty = -1;
		break;

	case SYNTRIGGER:
		synthNodes[nodeCount].triggerID = triggerCount++;
		break;


	case SYNCYCLE:
		synthNodes[nodeCount].seqrate = -1;
		synthNodes[nodeCount].seqlength = 0;
		synthNodes[nodeCount].sequence = nullptr;
		break;

	case SYNLINEARENV:
	case SYNARENV:
		synthNodes[nodeCount].trigger = -1;
		break;

	case SYNMIDIKEY:
	case SYNMIDIKEYVEL:
	case SYNMIDIKEYTRIGGER:
		synthNodes[nodeCount].midiKey = -1;
		break;
	}

	return &synthNodes[nodeCount++];
}

u32 NewValue(f32 value) {
	synthNodes[nodeCount].type = SYNVALUE;
	synthNodes[nodeCount].value = value;
	return nodeCount++;
}

SynthNode* GetSynthNode(u32 id) {
	assert(id < nodeCount && id < 1024);
	return &synthNodes[id];
}

void SetOutputNode(u32 id) {
	assert(id < nodeCount && id < 1024);
	outputNode = id;
}

const char* oscTypeNames[] = {
	[SYNVALUE] = "Value",

	[SYNSINE] = "Sine",
	[SYNTRIANGLE] = "Triangle",
	[SYNSAW] = "Saw",
	[SYNSQUARE] = "Square",
	[SYNNOISE] = "Noise",

	[SYNLINEARENV] = "Linear",
	[SYNARENV] = "AR",

	[SYNTRIGGER] = "Trigger",
	[SYNCYCLE] = "Cycle",

	[SYNNEG] = "neg",

	[SYNADD] = "+",
	[SYNSUB] = "-",
	[SYNMUL] = "*",
	[SYNDIV] = "/",
	[SYNPOW] = "^",

	[SYNMIDIKEY] = "MidiKey",
	[SYNMIDIKEYVEL] = "MidiKeyVel",
	[SYNMIDIKEYTRIGGER] = "MidiKeyTrigger",
	[SYNMIDICONTROL] = "MidiCtl",

	[SYNOUTPUT] = "Output",
};

void DumpSynthNodes() {
	printf("%u SynthNodes\n", nodeCount);
	printf("OutputNode: #%d\n", outputNode);
	
	for(u32 i = 0; i < nodeCount; i++) {
		auto osc = GetSynthNode(i);
		printf("\t#%-2u %-10s ", i, oscTypeNames[osc->type]);

		switch(osc->type) {
			case SYNVALUE: printf("%f", osc->value); break;

			case SYNSINE:
			case SYNTRIANGLE:
			case SYNSAW:
			case SYNSQUARE:
				printf("freq(#%d)   phaseoffset(#%d)   duty(#%d)", osc->frequency, osc->phaseOffset, osc->duty);
				break;

			case SYNLINEARENV:
			case SYNARENV:
				printf("trigger(s#%d)", osc->trigger);
				break;

			case SYNTRIGGER:
				printf("trigger(t#%d)", osc->triggerID);
				break;

			case SYNCYCLE:
				printf("rate(#%d) [ ", osc->seqrate);
				for(u32 i = 0; i < osc->seqlength; i++)
					printf("%d ", osc->sequence[i]);
				printf("]");
				break;

			case SYNNEG:
				printf("operand(#%d)", osc->operand);
				break;

			case SYNADD:
			case SYNSUB:
			case SYNMUL:
			case SYNDIV:
			case SYNPOW:
				printf("left(#%d)   right(#%d)", osc->left, osc->right);
				break;

			case SYNMIDIKEY:
			case SYNMIDIKEYVEL:
			case SYNMIDIKEYTRIGGER:
				printf("key(k#%hhd)   mode(%hhu)", osc->midiKey, osc->midiKeyMode);
				break;
			case SYNMIDICONTROL:
				printf("ctl(c#%hhu)", osc->midiCtl);
				break;

			default: break;
		}

		printf("\n");
	}

	printf("\n");
}

struct SynthValue {
	bool variable;
	union {
		f32 value;
		u32 stackPos;
	};

	SynthValue(u32 sp) : variable{true}, stackPos{sp} {}
	SynthValue(f32 v = 0.f) : variable{false}, value{v} {}
};

struct SynthInst {
	u8 type;

	union {
		struct {
			u16 phaseID;
			SynthValue frequency;
			SynthValue phaseOffset;
			SynthValue duty;
		} osc;

		struct {
			u16 phaseID;
			s32 triggerID;
			SynthValue attack;
			SynthValue release;
		} env;

		struct {
			SynthValue left;
			SynthValue right;
		} binary;

		struct {
			u16 phaseID;
			SynthValue rate;
			u32 seqlength;
			SynthValue* sequence;
		} cycle;

		SynthValue unary;

		u16 triggerID;

		u8 midiCtl;
		struct {
			s8 key;
			u8 mode;
		} key;
	};

	SynthInst(u8 t = 0) : type{t} {}
};

struct Trigger {
	enum {
		StateDefault,
		StateOff,
		StateOn,
		StateOnOff,
		StateHeld,
	};

	u8 state;
};

static f64 phaseBank[1024] = {0};
static u32 numOscillators = 0;

static std::map<u16, u16> triggerMap{}; // Maps trIds from 1st stage to 2nd to avoid duplicates
static Trigger triggerBank[1024] = {0};
static u32 numTriggers = 0;
static std::mutex triggerMutex;

struct MidiKeyVel {u8 key; f32 vel; f32 freq; Trigger trg;};

static f32 midiControls[256] = {0.f};
static std::vector<MidiKeyVel> midiKeyStates;
static std::mutex midiMutex;

static f32 synthStack[1024] = {0};
static u32 stackPos = 0;

static SynthInst instructions[1024] = {0};
static u32 numInstructions = 0;

void SetTrigger(u16 triggerID, bool v) {
	if(triggerID >= numTriggers) return;

	triggerMutex.lock();
	auto trg = &triggerBank[triggerID];

	if(!v) {
		if(trg->state == Trigger::StateOn) {
			trg->state = Trigger::StateOnOff;
		}else if(trg->state != Trigger::StateOnOff) {
			trg->state = Trigger::StateOff;
		}

	}else{
		switch(trg->state) {
		case Trigger::StateOnOff: // NOTE: ???
		case Trigger::StateOn: break;
		case Trigger::StateHeld: trg->state = Trigger::StateHeld; break;

		case Trigger::StateDefault:
		case Trigger::StateOff: trg->state = Trigger::StateOn; break;
		}
	}
	triggerMutex.unlock();
}

void SetMidiControl(u8 ctl, f32 val) {
	midiMutex.lock();
	midiControls[ctl] = std::min(std::max(val, 0.f), 1.f);
	midiMutex.unlock();
}

void NotifyMidiKey(u8 key, f32 vel) {
	midiMutex.lock();
	auto it = std::find_if(midiKeyStates.begin(), midiKeyStates.end(), [key](auto k){return key == k.key;});
	if(it != midiKeyStates.end()) midiKeyStates.erase(it);

	if(vel > 1.f/127.f) { // note on
		f32 freq = 440.f * std::pow(2.f, (key-69.f)/12.f);
		midiKeyStates.push_back({key, vel, freq, {Trigger::StateOnOff}});
		// TODO: Trigger
	}
	midiMutex.unlock();
}

SynthValue CompileNode(s32 id, f32 def = 0.f) {
	if(id < 0) return {def};

	auto node = GetSynthNode(id);

	switch(node->type) {
	case SYNVALUE:
		return {node->value};

	case SYNSINE:
	case SYNTRIANGLE:
	case SYNSAW:
	case SYNSQUARE: {
		auto freq = CompileNode(node->frequency, 110.f);
		auto shift = CompileNode(node->phaseOffset, 0.f);
		auto duty = CompileNode(node->duty, 0.5f);

		auto inst = &instructions[numInstructions++];
		inst->type = node->type;
		inst->osc.phaseID = numOscillators++;
		inst->osc.frequency = freq;
		inst->osc.phaseOffset = shift;
		inst->osc.duty = duty;
		return {stackPos++};
	}

	case SYNNOISE: {
		auto inst = &instructions[numInstructions++];
		inst->type = node->type;
		inst->osc.phaseID = numOscillators++;
		return {stackPos++};
	}

	case SYNLINEARENV:
	case SYNARENV: {
		auto attack = CompileNode(node->attack, 1.f);
		auto release = CompileNode(node->release, 1.f);

		auto inst = &instructions[numInstructions++];
		inst->type = node->type;
		inst->env.phaseID = numOscillators++;
		inst->env.attack = attack;
		inst->env.release = release;

		if(node->trigger >= 1024) {
			inst->env.triggerID = node->trigger;
		}else if(node->trigger >= 0) {
			auto trg = &triggerMap[node->trigger];
			if(*trg > 0) {
				inst->env.triggerID = *trg-1;
			}else{
				inst->env.triggerID = numTriggers++;
				*trg = inst->env.triggerID+1;
			}
		}else{
			inst->env.triggerID = -1;
		}

		return {stackPos++};
	}

	case SYNTRIGGER: {
		auto inst = &instructions[numInstructions++];
		inst->type = node->type;
		inst->triggerID = numTriggers++;
		return {0.f};
		// TODO: ?????
	}

	case SYNCYCLE: {
		auto rate = CompileNode(node->seqrate, 1.f);
		auto sequence = new SynthValue[node->seqlength];
		for(u32 i = 0; i < node->seqlength; i++) {
			sequence[i] = CompileNode(node->sequence[i], 0.f);
		}

		auto inst = &instructions[numInstructions++];
		inst->type = node->type;
		inst->cycle.phaseID = numOscillators++;
		inst->cycle.rate = rate;
		inst->cycle.sequence = sequence;
		inst->cycle.seqlength = node->seqlength;

		return {stackPos++};
	}

	case SYNMIDICONTROL: {
		auto inst = &instructions[numInstructions++];
		inst->type = node->type;
		inst->midiCtl = node->midiCtl;
		return {stackPos++};
	}

	case SYNMIDIKEYTRIGGER: {
		auto inst = &instructions[numInstructions++];
		inst->type = node->type;
		inst->key.key = node->midiKey;
		inst->key.mode = node->midiKeyMode;
		return {0.f};
	}

	case SYNMIDIKEYVEL:
	case SYNMIDIKEY: {
		auto inst = &instructions[numInstructions++];
		inst->type = node->type;
		inst->key.key = node->midiKey;
		inst->key.mode = node->midiKeyMode;
		return {stackPos++};
	}

	case SYNADD:
	case SYNSUB:
	case SYNMUL:
	case SYNDIV:
	case SYNPOW: {
		auto left = CompileNode(node->left);
		auto right = CompileNode(node->right);

		auto inst = &instructions[numInstructions++];
		inst->type = node->type;
		inst->binary.left = left;
		inst->binary.right = right;
		return {stackPos++};
	}

	case SYNNEG: {
		auto val = CompileNode(node->operand);
		auto inst = &instructions[numInstructions++];
		inst->type = node->type;
		inst->unary = val;
		return {stackPos++};
	}

	default:
		return {0.f};
	}
}

void CompileSynth() {
	assert(outputNode >= 0 && outputNode < (s32)nodeCount);
	stackPos = 0;
	CompileNode(outputNode);
	if(numInstructions > 0){
		instructions[numInstructions++] = SynthInst{SYNOUTPUT};
	}else{
		puts("Warning! No instructions generated for synth!");
	}

	printf("numTriggers: %d\n", numTriggers);
	printf("numOscillators: %d\n", numOscillators);

	for(u32 tp = 0; tp < numTriggers; tp++) {
		triggerBank[tp] = {Trigger::StateDefault};
	}

	for(u32 i = 0; i < nodeCount; i++) {
		auto n = &synthNodes[i];
		if(n->type == SYNCYCLE) {
			delete[] n->sequence;
			n->sequence = nullptr;
		}
	}

	for(u32 i = 0; i < numInstructions; i++) {
		auto inst = &instructions[i];
		auto type = inst->type;

		printf("s#%-3d %-10s ", i, oscTypeNames[type]);
		if(type >= SYNSINE && type <= SYNSQUARE) {
			printf("phase(b#%d) ", inst->osc.phaseID);
			if(inst->osc.frequency.variable) {
				printf("frequency(s#%d) ", inst->osc.frequency.stackPos);
			}else{
				printf("frequency(%.2fHz) ", inst->osc.frequency.value);
			}
			if(inst->osc.phaseOffset.variable) {
				printf("phaseOffset(s#%d) ", inst->osc.phaseOffset.stackPos);
			}else{
				printf("phaseOffset(%.0f%%) ", inst->osc.phaseOffset.value*100.f);
			}
			if(inst->osc.duty.variable) {
				printf("duty(s#%d) ", inst->osc.duty.stackPos);
			}else{
				printf("duty(%.2f%%) ", inst->osc.duty.value*100.f);
			}
		}else if(type >= SYNADD && type <= SYNPOW) {
			if(inst->binary.left.variable) {
				printf("left(s#%d) ", inst->binary.left.stackPos);
			}else{
				printf("left(%.2f) ", inst->binary.left.value);
			}

			if(inst->binary.right.variable) {
				printf("right(s#%d) ", inst->binary.right.stackPos);
			}else{
				printf("right(%.2f) ", inst->binary.right.value);
			}			
		}else if(type >= SYNLINEARENV && type <= SYNARENV) {
			if(inst->env.attack.variable) {
				printf("attack(s#%d) ", inst->env.attack.stackPos);
			}else{
				printf("attack(%.2fs) ", inst->env.attack.value);
			}

			if(inst->env.release.variable) {
				printf("release(s#%d) ", inst->env.release.stackPos);
			}else{
				printf("release(%.2fs) ", inst->env.release.value);
			}

			printf("trigger(t#%d) ", inst->env.triggerID);

		}else if(type == SYNCYCLE) {
			printf("[ ");
			for(u32 i = 0; i < inst->cycle.seqlength; i++) {
				auto& val = inst->cycle.sequence[i];
				if(val.variable) {
					printf("s#%d ", val.stackPos);
				}else{
					printf("%.2f ", val.value);
				}
			}
			printf("]");
		}else if(type == SYNMIDICONTROL) {
			printf("ctl(c#%hhu)", inst->midiCtl);
		}else if(type == SYNMIDIKEY || type == SYNMIDIKEYVEL) {
			printf("key(k#%hhu) mode(%hhu)", inst->key.key, inst->key.mode);
		}
		printf("\n");
	}

	fflush(stdout);
}

f32 EvalSynthValue(SynthValue val) {
	if(val.variable) return synthStack[val.stackPos];
	return val.value;
}

u8 EvalTrigger(s32 trId) {
	if(trId < 0) return Trigger::StateOff;

	if(trId >= 1024) {
		trId -= 1024;
		s32 mkssize = midiKeyStates.size();
		s8 key = (s8)trId;

		if(key < 0 && -key <= mkssize){ // Negative are 1-indexed
			return midiKeyStates[mkssize+key].trg.state;
		}else if(key >= 0 && key < mkssize){ // Positive are 0-indexed
			return midiKeyStates[key].trg.state;
		}

		return Trigger::StateOff;
	}

	auto trg = &triggerBank[trId];
	return trg->state;
}

FMOD_RESULT F_CALLBACK DSPCallback(FMOD_DSP_STATE* dsp_state, 
	f32* /*inbuffer*/, f32* outbuffer, u32 length, 
	s32 /*inchannels*/, s32* outchannels){

	assert(*outchannels == 1);

	// FMOD::DSP *thisdsp = (FMOD::DSP *)dsp_state->instance; 

	// void* ud = nullptr;
	// cfmod(thisdsp->getUserData(&ud));

	s32 samplerate = 0;
	cfmod(dsp_state->callbacks->getsamplerate(dsp_state, &samplerate));
	f64 inc = 1.0/samplerate;

	std::lock_guard<std::mutex> triggerLock{triggerMutex};
	std::lock_guard<std::mutex> midiLock{midiMutex};

	static std::vector<MidiKeyVel> sortedKeyStates;
	sortedKeyStates = midiKeyStates;

	std::sort(sortedKeyStates.begin(), sortedKeyStates.end(), [](auto k0, auto k1) {
		return k0.key < k1.key;
	});

	for(u32 i = 0; i < length; i++){
		f32 out = 0.f;

		stackPos = 0;
		for(u32 ip = 0; ip < numInstructions; ip++) {
			auto inst = &instructions[ip];

			switch(inst->type) {
			case SYNSINE: {
				auto shift = EvalSynthValue(inst->osc.phaseOffset);
				synthStack[stackPos++] = Wave::sin(phaseBank[inst->osc.phaseID] + shift);
				phaseBank[inst->osc.phaseID] += inc * EvalSynthValue(inst->osc.frequency);
			}	break;
			case SYNTRIANGLE: {
				auto shift = EvalSynthValue(inst->osc.phaseOffset);
				synthStack[stackPos++] = Wave::tri(phaseBank[inst->osc.phaseID] + shift);
				phaseBank[inst->osc.phaseID] += inc * EvalSynthValue(inst->osc.frequency);
			}	break;
			case SYNSAW: {
				auto shift = EvalSynthValue(inst->osc.phaseOffset);
				synthStack[stackPos++] = Wave::saw(phaseBank[inst->osc.phaseID] + shift);
				phaseBank[inst->osc.phaseID] += inc * EvalSynthValue(inst->osc.frequency);
			}	break;
			case SYNSQUARE: {
				auto shift = EvalSynthValue(inst->osc.phaseOffset);
				auto duty = EvalSynthValue(inst->osc.duty);
				synthStack[stackPos++] = Wave::sqr(phaseBank[inst->osc.phaseID] + shift, duty);
				phaseBank[inst->osc.phaseID] += inc * EvalSynthValue(inst->osc.frequency);
			}	break;

			case SYNNOISE: {
				synthStack[stackPos++] = Wave::noise(phaseBank[inst->osc.phaseID]);
				phaseBank[inst->osc.phaseID] += inc;
			} break;

			case SYNLINEARENV: {
				auto attack = EvalSynthValue(inst->env.attack);

				if(inst->env.triggerID >= 0) {
					auto trigger = EvalTrigger(inst->env.triggerID);
					if(trigger == Trigger::StateOn || trigger == Trigger::StateOnOff) {
						phaseBank[inst->env.phaseID] = 0.f;
						
					}else if(trigger == Trigger::StateDefault) {
						phaseBank[inst->env.phaseID] = 0.f;
						attack = infinity;
					}
				}

				synthStack[stackPos++] = Env::linear(phaseBank[inst->env.phaseID]);
				phaseBank[inst->env.phaseID] += inc / attack;
			} break;
			case SYNARENV: {
				auto attack = EvalSynthValue(inst->env.attack);
				auto release = EvalSynthValue(inst->env.release);
				auto total = attack+release;

				if(inst->env.triggerID >= 0) {
					auto trigger = EvalTrigger(inst->env.triggerID);
					if(trigger == Trigger::StateOn || trigger == Trigger::StateOnOff) {
						phaseBank[inst->env.phaseID] = 0.f;

					}else if(trigger == Trigger::StateDefault) {
						phaseBank[inst->env.phaseID] = 0.f;
						attack = infinity;
					}
				}

				synthStack[stackPos++] = Env::ar(phaseBank[inst->env.phaseID], attack/total);
				phaseBank[inst->env.phaseID] += inc / total;
			} break;

			case SYNTRIGGER: {
				// synthStack[stackPos++] = triggerBank[inst->triggerID];
				// TODO: ?????
			} break;

			case SYNCYCLE: {
				auto rate = EvalSynthValue(inst->cycle.rate);
				auto len = inst->cycle.seqlength;
				auto& ph = phaseBank[inst->cycle.phaseID];
				ph = std::fmod(ph, len);

				synthStack[stackPos++] = EvalSynthValue(inst->cycle.sequence[(u32)ph]);
				ph += inc * rate;
			} break;

			case SYNMIDICONTROL:
				synthStack[stackPos++] = midiControls[inst->midiCtl];
				break;
			case SYNMIDIKEY: {
				auto mkssize = (s32)midiKeyStates.size();
				auto& stateVec = (inst->key.mode)?sortedKeyStates:midiKeyStates;
				s32 imkey = inst->key.key;
				if(imkey < 0 && -imkey <= mkssize){ // Negative are 1-indexed
					synthStack[stackPos++] = stateVec[mkssize+imkey].freq;
				}else if(imkey >= 0 && imkey < mkssize){ // Positive are 0-indexed
					synthStack[stackPos++] = stateVec[imkey].freq;
				}else{
					synthStack[stackPos++] = 0.f;
				}
			}	break;
			case SYNMIDIKEYVEL: {
				auto mkssize = (s32)midiKeyStates.size();
				auto& stateVec = (inst->key.mode)?sortedKeyStates:midiKeyStates;
				s32 imkey = inst->key.key;
				if(imkey < 0 && -imkey <= mkssize){ // Negative are 1-indexed
					synthStack[stackPos++] = stateVec[mkssize+imkey].vel;
				}else if(imkey >= 0 && imkey < mkssize){ // Positive are 0-indexed
					synthStack[stackPos++] = stateVec[imkey].vel;
				}else{
					synthStack[stackPos++] = 0.f;
				}
			}	break;

			case SYNNEG:
				synthStack[stackPos++] = -EvalSynthValue(inst->binary.left);
				break;
			case SYNADD:
				synthStack[stackPos++] = EvalSynthValue(inst->binary.left) + EvalSynthValue(inst->binary.right);
				break;
			case SYNSUB:
				synthStack[stackPos++] = EvalSynthValue(inst->binary.left) - EvalSynthValue(inst->binary.right);
				break;
			case SYNMUL:
				synthStack[stackPos++] = EvalSynthValue(inst->binary.left) * EvalSynthValue(inst->binary.right);
				break;
			case SYNDIV:
				synthStack[stackPos++] = EvalSynthValue(inst->binary.left) / EvalSynthValue(inst->binary.right);
				break;
			case SYNPOW:
				synthStack[stackPos++] = std::pow(EvalSynthValue(inst->binary.left), EvalSynthValue(inst->binary.right));
				break;

			case SYNOUTPUT:
				out = synthStack[--stackPos];
				goto go;

			default: break;
			}
		}

		go:
		// constexpr u32 delaySize = 48000*4;
		// static f32 delayline[delaySize] {0.f};
		// static u32 delayPos = 0;
		// static auto sample = [](u32 x) {
		// 	return delayline[(delayPos-x+delaySize)%delaySize];
		// };

		// http://msp.ucsd.edu/techniques/v0.11/book-html/node111.html
		// http://www.cs.ust.hk/mjg_lib/bibs/DPSu/DPSu.Files/Ga95.PDF 
		// http://www.earlevel.com/main/1997/01/19/a-bit-about-reverb/
		// https://ccrma.stanford.edu/~jos/pasp/Artificial_Reverberation.html

		// out += (sample(1300*48) + sample(700*48))/2.f;
		// delayline[delayPos] = out * 0.1f;
		// delayPos = (delayPos+1)%delaySize;
		
		outbuffer[i] = out;

		for(u32 tp = 0; tp < numTriggers; tp++) {
			auto trg = &triggerBank[tp];

			if(trg->state == Trigger::StateOn) {
				trg->state = Trigger::StateHeld;
			}else if(trg->state == Trigger::StateOnOff) {
				trg->state = Trigger::StateOff;
			}
		}

		for(auto& k: midiKeyStates) {
			if(k.trg.state == Trigger::StateOn) {
				k.trg.state = Trigger::StateHeld;
			}else if(k.trg.state == Trigger::StateOnOff) {
				k.trg.state = Trigger::StateOff;
			}
		}
	}

	return FMOD_OK;
}

void InitAudio(){
	cfmod(FMOD::System_Create(&fmodSystem));

	u32 version = 0;
	cfmod(fmodSystem->getVersion(&version));
	if(version < FMOD_VERSION){
		printf("FMOD version of at least %d required. Version used %u\n", FMOD_VERSION, version);
		throw "FMOD Error";
	}

	cfmod(fmodSystem->init(10, FMOD_INIT_NORMAL, nullptr));

	FMOD::DSP* dsp;
	FMOD::DSP* compressor;

	{	FMOD_DSP_DESCRIPTION desc;
		memset(&desc, 0, sizeof(desc));

		desc.numinputbuffers = 0;
		desc.numoutputbuffers = 1;
		desc.read = DSPCallback;

		cfmod(fmodSystem->createDSP(&desc, &dsp));
		cfmod(dsp->setChannelFormat(FMOD_CHANNELMASK_MONO,1,FMOD_SPEAKERMODE_MONO));
	}

	cfmod(fmodSystem->createDSPByType(FMOD_DSP_TYPE_COMPRESSOR, &compressor));

	cfmod(compressor->setParameterFloat(FMOD_DSP_COMPRESSOR_THRESHOLD, -15));
	cfmod(compressor->setParameterFloat(FMOD_DSP_COMPRESSOR_ATTACK, 1));
	cfmod(compressor->setParameterFloat(FMOD_DSP_COMPRESSOR_RELEASE, 200));
	cfmod(compressor->setBypass(false));
	cfmod(dsp->setBypass(false));

	FMOD::ChannelGroup* mastergroup;
	cfmod(fmodSystem->getMasterChannelGroup(&mastergroup));
	cfmod(mastergroup->addDSP(0, compressor));
	cfmod(fmodSystem->playDSP(dsp, mastergroup, false, &channel));
	cfmod(channel->setMode(FMOD_2D));
	cfmod(channel->setVolume(0.7f));

	// http://www.fmod.org/docs/content/generated/FMOD_REVERB_PROPERTIES.html

	FMOD_REVERB_PROPERTIES rprops = {
		.DecayTime			= 12000.0, //1500.0, /* Reverberation decay time in ms */
		.EarlyDelay			= 7.0, //7.0, /* Initial reflection delay time */
		.LateDelay			= 11.0, //11.0, /* Late reverberation delay time relative to initial reflection */
		.HFReference		= 5000.0, /* Reference high frequency (hz) */
		.HFDecayRatio		= 50.0, /* High-frequency to mid-frequency decay time ratio */
		.Diffusion			= 100.0, /* Value that controls the echo density in the late reverberation decay. */
		.Density			= 100.0, //100.0, /* Value that controls the modal density in the late reverberation decay */
		.LowShelfFrequency	= 250.0, /* Reference low frequency (hz) */
		.LowShelfGain		= 0.0, /* Relative room effect level at low frequencies */
		.HighCut			= 10000.0, /* Relative room effect level at high frequencies */
		.EarlyLateMix		= 50.0, /* Early reflections level relative to room effect */
		.WetLevel			= -4.0, //-6.0, /* Room effect level (at mid frequencies) */
	};

	FMOD::Reverb3D* reverb;
	cfmod(fmodSystem->createReverb3D(&reverb));
	cfmod(reverb->setProperties(&rprops));
}

void DeinitAudio() {
	fmodSystem->release();
}

void UpdateAudio() {
	fmodSystem->update();
}