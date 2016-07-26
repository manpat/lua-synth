#include "common.h"
#include "audio.h"
#include "midi.h"
#include "lib.h"

#include <SDL2/SDL.h>
#include <vector>

int main(){
	SDL_Init(SDL_INIT_EVERYTHING);
	auto sdlWindow = SDL_CreateWindow("FMOD Test",
		SDL_WINDOWPOS_CENTERED_DISPLAY(1), SDL_WINDOWPOS_UNDEFINED,
		200, 200, SDL_WINDOW_OPENGL);

	if(!sdlWindow) {
		puts("Window creation failed");
		return 1;
	}

	InitMidi();
	InitLua();

	extern LuaState l;
	if(luaL_dofile(l, "main.lua")){
		puts(lua_tostring(l, -1));
		lua_pop(l, 1);
	}

	DumpSynthNodes();
	CompileSynth();

	InitAudio();

	std::vector<std::pair<u8, u8>> keyStack;

	bool running = true;
	while(running){
		SDL_Event e;
		while(SDL_PollEvent(&e)){
			if (e.type == SDL_QUIT
			|| (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE)
			|| (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)){
				running = false;
			}

			if(e.type == SDL_KEYDOWN && e.key.repeat == 0) {
				auto k = e.key.keysym.sym;
				if(k == SDLK_SPACE) {
					SetTrigger(0, true);
				}else if(k == SDLK_a) {
					SetTrigger(1, true);
				}else if(k == SDLK_d) {
					SetTrigger(2, true);
				}

			}else if(e.type == SDL_KEYUP) {
				auto k = e.key.keysym.sym;
				if(k == SDLK_SPACE) {
					SetTrigger(0, false);
				}else if(k == SDLK_a) {
					SetTrigger(1, false);
				}else if(k == SDLK_d) {
					SetTrigger(2, false);
				}
			}
		}

		while(auto m = GetMidiMessage()) {
			switch(m.type) {
				case MidiMessage::Control:
					SetMidiControl(m.b0, m.b1/127.f);
					break;

				case MidiMessage::Note:
					NotifyMidiKey(m.b0, m.b1/127.f);
					break;

				default: break;
			}
		}

		UpdateAudio();

		SDL_Delay(1);
	}

	DeinitMidi();
	DeinitAudio();
	SDL_Quit();

	return 0;
}

