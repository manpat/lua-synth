#include "common.h"
// #include "oldaudio.h"
#include "audio.h"
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

	if(!InitLua()) {
		puts("Lua init failed!");
		return 1;
	}

	extern LuaState l;

	if(!InitAudio()) {
		puts("Audio init failed!");
		return 1;
	}

	if(luaL_dofile(l, "new.lua")){
		puts(lua_tostring(l, -1));
		lua_pop(l, 1);
	}
	
	auto synth = GetSynth(0);

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
				if(k == SDLK_SPACE){
					// static f32 freqs[] {1./3.f, 1.f/2.f, 1.f, 2.f/3.f, 3.f/2.f, 4.f/5.f, 9.f/8.f};
					// SetSynthControl(synth, "freq", freqs[rand()%sizeof(freqs)/4]*220.f);
					TripSynthTrigger(synth, "env");
				}
			}
		}

		UpdateAudio();

		luaL_dostring(l, "update()");

		SDL_Delay(1);
	}

	DeinitAudio();
	SDL_Quit();

	return 0;
}

