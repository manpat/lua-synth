#include "common.h"

#include "recording.h"
#include "synth.h"
#include <lua.hpp>

#include <SDL2/SDL.h>
#include <vector>
#include <chrono>

using namespace synth;

u64 getFileModificationTime(const char*);

s32 main(){
	SDL_Init(SDL_INIT_EVERYTHING);
	auto sdlWindow = SDL_CreateWindow("LuaSynth Test",
		SDL_WINDOWPOS_CENTERED_DISPLAY(1), SDL_WINDOWPOS_UNDEFINED,
		200, 200, SDL_WINDOW_OPENGL);

	if(!sdlWindow) {
		puts("Window creation failed");
		return 1;
	}

	if(!InitRecording("audio.ogg")) {
		puts("Recording init failed!");
		return 1;
	}

	auto l = luaL_newstate();
	if(!l) {
		puts("Lua init failed");
		return 1;
	}

	luaL_openlibs(l);

	if(!InitLuaLib(l)) {
		puts("Synth lua lib init failed!");
		return 1;
	}

	if(!InitAudio()) {
		puts("Audio init failed!");
		return 1;
	}

	SetAudioPostProcessHook([](f32* b, u32 len){
		RecordBuffer(b, len);
	});

	// SetSynthPostProcessHook([](Synth* s, f32* b, u32 len, f32* stereoCoeffs){
	// });

	const char* soundscript = "scripts/new.lua";

	u32 fileModTime = getFileModificationTime(soundscript);
	if(luaL_dofile(l, soundscript)){
		puts(lua_tostring(l, -1));
		lua_pop(l, 1);
	}

	u32 updateRef = 0;
	lua_getglobal(l, "update");
	if(lua_isfunction(l, -1)) {
		updateRef = luaL_ref(l, LUA_REGISTRYINDEX);
	}

	auto synth = GetSynth(0);
	
	using std::chrono::duration;
	using std::chrono::duration_cast;
	using clock = std::chrono::high_resolution_clock;
	auto begin = clock::now();

	f32 pollTimer = 0.f;

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
					TripSynthTrigger(synth, "<global>");
				}
			}
		}

		auto end = clock::now();
		auto diff = (end-begin);
		f32 dt = duration_cast<duration<f32>>(diff).count();
		begin = end;

		pollTimer -= dt;
		if(pollTimer < 0.f) {
			u64 newFileModTime = getFileModificationTime(soundscript);
			if(newFileModTime > fileModTime) {
				DestroyAllSynths();
				if(luaL_dofile(l, soundscript)){
					puts(lua_tostring(l, -1));
					lua_pop(l, 1);
				}

				lua_getglobal(l, "update");
				if(lua_isfunction(l, -1)) {
					luaL_unref(l, LUA_REGISTRYINDEX, updateRef);
					updateRef = luaL_ref(l, LUA_REGISTRYINDEX);
				}

				fileModTime = newFileModTime;
			}
			
			pollTimer = 0.25f;
		}

		UpdateAudio();

		if(updateRef) {
			lua_rawgeti(l, LUA_REGISTRYINDEX, updateRef);
			lua_pushnumber(l, dt);
			if(lua_pcall(l, 1, 0, 0)) {
				puts(lua_tostring(l, -1));
				lua_pop(l, 1);
			}
		}

		SDL_Delay(1);
	}

	DeinitAudio();
	FinishRecording();
	SDL_Quit();

	return 0;
}


#include <sys/stat.h>

u64 getFileModificationTime(const char* path) {
	struct stat attr;
    stat(path, &attr);
    return attr.st_mtime;
}