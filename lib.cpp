#include "lib.h"
#include "audio.h"

#define LUAFUNC(x) static int x(LuaState l)
#define LUALAMBDA [](LuaState l) -> s32

LuaState l;

LUAFUNC(oscToString);
LUAFUNC(seqCycles);

LUAFUNC(modTrigger);
LUAFUNC(modShift);
LUAFUNC(modDuty);

s32 PushLuaSynthNode(LuaState l, SynthNode* node);
SynthNode* NewLuaSynthNode(LuaState l, u8 type);
s32 GetSynthNodeArg(LuaState l, u32 a);

void InitLua() {
	l = luaL_newstate();
	if(!l) {
		puts("Lua context acquisition failed");
		return;
	}

	luaL_openlibs(l);

	static LibraryType osclib = {
		{"output", LUALAMBDA {
			SetOutputNode(*(u32*) luaL_checkudata(l, 1, "OscMT"));
			return 0;
		}},
		{"sin", LUALAMBDA {
			auto osc = NewSynthNode(SYNSINE);
			osc->frequency = GetSynthNodeArg(l, 1);
			return PushLuaSynthNode(l, osc);
		}},
		{"tri", LUALAMBDA {
			auto osc = NewSynthNode(SYNTRIANGLE);
			osc->frequency = GetSynthNodeArg(l, 1);
			return PushLuaSynthNode(l, osc);
		}},
		{"saw", LUALAMBDA {
			auto osc = NewSynthNode(SYNSAW);
			osc->frequency = GetSynthNodeArg(l, 1);
			return PushLuaSynthNode(l, osc);
		}},
		{"sqr", LUALAMBDA {
			auto osc = NewSynthNode(SYNSQUARE);
			osc->frequency = GetSynthNodeArg(l, 1);
			osc->duty = GetSynthNodeArg(l, 2);
			return PushLuaSynthNode(l, osc);
		}},
		{"noise", LUALAMBDA {
			NewLuaSynthNode(l, SYNNOISE);
			return 1;
		}},

		{0,0}
	};

	static LibraryType envlib = {
		{"trigger", LUALAMBDA {
			NewLuaSynthNode(l, SYNTRIGGER);
			return 1;
		}},
		{"linear", LUALAMBDA {
			if(lua_gettop(l) < 1) return 0;
			auto env = NewLuaSynthNode(l, SYNLINEARENV);
			env->attack = GetSynthNodeArg(l, 1);
			return 1;
		}},
		{"ar", LUALAMBDA { 
			if(lua_gettop(l) < 2) return 0;
			auto env = NewLuaSynthNode(l, SYNARENV);
			env->attack = GetSynthNodeArg(l, 1);
			env->release = GetSynthNodeArg(l, 2);
			return 1;
		}},
		{0,0}
	};

	static LibraryType opslib = {
		{"__tostring", oscToString},
		{"__add", LUALAMBDA {
			auto osc = NewSynthNode(SYNADD);
			osc->left = GetSynthNodeArg(l, 1);
			osc->right = GetSynthNodeArg(l, 2);
			return PushLuaSynthNode(l, osc);
		}},

		{"__sub", LUALAMBDA {
			auto osc = NewSynthNode(SYNSUB);
			osc->left = GetSynthNodeArg(l, 1);
			osc->right = GetSynthNodeArg(l, 2);
			return PushLuaSynthNode(l, osc);
		}},

		{"__mul", LUALAMBDA {
			auto osc = NewSynthNode(SYNMUL);
			osc->left = GetSynthNodeArg(l, 1);
			osc->right = GetSynthNodeArg(l, 2);
			return PushLuaSynthNode(l, osc);
		}},

		{"__div", LUALAMBDA {
			auto osc = NewSynthNode(SYNDIV);
			osc->left = GetSynthNodeArg(l, 1);
			osc->right = GetSynthNodeArg(l, 2);
			return PushLuaSynthNode(l, osc);
		}},

		{"__pow", LUALAMBDA {
			auto osc = NewSynthNode(SYNPOW);
			osc->left = GetSynthNodeArg(l, 1);
			osc->right = GetSynthNodeArg(l, 2);
			return PushLuaSynthNode(l, osc);
		}},

		{"__unm", LUALAMBDA {
			auto osc = NewSynthNode(SYNNEG);
			osc->operand = GetSynthNodeArg(l, 1);
			return PushLuaSynthNode(l, osc);
		}},

		{0,0}
	};

	static LibraryType seqlib = {
		{"cycle", seqCycles},
		{0,0}
	};

	static LibraryType modlib = {
		{"trigger", modTrigger},
		{"shift", modShift},
		{"duty", modDuty},
		{0,0}
	};

	static LibraryType midilib = {
		{"ctl", LUALAMBDA {
			auto o = NewSynthNode(SYNMIDICONTROL);
			o->midiCtl = (u8) luaL_checkunsigned(l, 1);
			return PushLuaSynthNode(l, o);
		}},
		{"key", LUALAMBDA {
			auto o = NewSynthNode(SYNMIDIKEY);
			o->midiKey = (s8) luaL_optinteger(l, 1, -1);
			o->midiKeyMode = (u8) luaL_optinteger(l, 2, 0);
			return PushLuaSynthNode(l, o);
		}},
		{"vel", LUALAMBDA {
			auto o = NewSynthNode(SYNMIDIKEYVEL);
			o->midiKey = (s8) luaL_optinteger(l, 1, -1);
			o->midiKeyMode = (u8) luaL_optinteger(l, 2, 0);
			return PushLuaSynthNode(l, o);
		}},
		{"trg", LUALAMBDA {
			auto o = NewSynthNode(SYNMIDIKEYTRIGGER);
			o->midiKey = (s8) luaL_optinteger(l, 1, -1);
			o->midiKeyMode = (u8) luaL_optinteger(l, 2, 0);
			return PushLuaSynthNode(l, o);
		}},
		{0,0}
	};

	luaL_newlib(l, osclib);
	lua_setglobal(l, "osc");

	luaL_newlib(l, envlib);
	lua_setglobal(l, "env");

	luaL_newlib(l, midilib);
	lua_setglobal(l, "midi");

	luaL_newlib(l, seqlib);
	lua_setglobal(l, "seq");

	luaL_newmetatable(l, "OscMT");
	luaL_setfuncs(l, opslib, 0);

	// Modifiers
	luaL_newlib(l, modlib);
	lua_setfield(l, -2, "__index");
}

s32 GetSynthNodeArg(LuaState l, u32 a) {
	if(lua_isnumber(l, a)) {
		return NewValue(lua_tonumber(l, a));

	}else if(auto osc = (u32*)luaL_testudata(l, a, "OscMT")) {
		return *osc;
	}

	return -1;
}

s32 PushLuaSynthNode(LuaState l, SynthNode* node) {
	if(!node) return 0;
	*(u32*) lua_newuserdata(l, sizeof(u32)) = node->id;
	luaL_setmetatable(l, "OscMT");
	return 1;
}

SynthNode* NewLuaSynthNode(LuaState l, u8 type) {
	auto node = NewSynthNode(type);
	PushLuaSynthNode(l, node);
	return node;
}

LUAFUNC(oscToString) {
	auto oscID = *(u32*) luaL_checkudata(l, 1, "OscMT");
	auto osc = GetSynthNode(oscID);

	extern const char* oscTypeNames[];
	char buf[256];

	std::snprintf(buf, 256, "OSC #%d (%s)", oscID, oscTypeNames[osc->type]);
	lua_pushstring(l, buf);

	return 1;
}

LUAFUNC(modShift) {
	if(lua_gettop(l) < 2) return 0;

	s32 synid = GetSynthNodeArg(l, 1);
	s32 operand = GetSynthNodeArg(l, 2);

	auto syn = GetSynthNode(synid);
	if(syn->type >= SYNSINE && syn->type <= SYNSQUARE) {
		syn->phaseOffset = operand;
		lua_pop(l, 1);
		return 1;
	}else{
		puts("Note: Shift can only be set directly on waves!");
	}

	lua_pop(l, 1);
	return 1;
}

LUAFUNC(modDuty) {
	if(lua_gettop(l) < 2) return 0;

	u32 synid = GetSynthNodeArg(l, 1);
	u32 operand = GetSynthNodeArg(l, 2);

	auto syn = GetSynthNode(synid);
	if(syn->type == SYNSQUARE) {
		syn->duty = operand;
	}else{
		puts("Note: Duty can only be set on square waves!");
	}

	lua_pop(l, 1);
	return 1;
}

LUAFUNC(modTrigger) {
	if(lua_gettop(l) < 2) return 0;

	u32 synid = GetSynthNodeArg(l, 1);
	u32 trigid = GetSynthNodeArg(l, 2);

	auto syn = GetSynthNode(synid);
	auto trg = GetSynthNode(trigid);

	if((trg->type != SYNTRIGGER) && (trg->type != SYNMIDIKEYTRIGGER)) {
		// Create wrapped trigger
		return 0;
	}

	if(syn->type >= SYNLINEARENV && syn->type <= SYNARENV) {
		if(trg->type == SYNMIDIKEYTRIGGER) {
			syn->trigger = 1024+u8(trg->midiKey); // NOTE: Loses mode
		}else{
			syn->trigger = trigid;
		}
		lua_pop(l, 1);
		return 1;
	}else{
		// TODO: Error
	}

	return 0;
}

LUAFUNC(seqCycles) {
	if(lua_gettop(l) < 2) return 0;

	auto osc = NewSynthNode(SYNCYCLE);
	osc->seqrate = GetSynthNodeArg(l, 1);
	osc->seqlength = 0;
	u32 seqcapacity = 8;
	osc->sequence = new s32[seqcapacity];

	lua_insert(l, 2);

	lua_pushnil(l);
	while(lua_next(l, -2) != 0) {
		if(lua_type(l, -2) != LUA_TNUMBER) break;

		if(osc->seqlength >= seqcapacity) {
			auto nseq = new s32[seqcapacity*2];
			std::memcpy(nseq, osc->sequence, seqcapacity*sizeof(s32));

			delete[] osc->sequence;
			osc->sequence = nseq;
			seqcapacity *= 2;
		}

		if(lua_isnumber(l, -1)) {
			osc->sequence[osc->seqlength++] = NewValue(lua_tonumber(l, -1));

		}else if(auto posc = (u32*)luaL_testudata(l, -1, "OscMT")) {
			osc->sequence[osc->seqlength++] = *posc;
		}

		lua_pop(l, 1);
	}

	// lua_pop(l, 1);
	return PushLuaSynthNode(l, osc);
}

void stackdump(){
	int i;
	int top = lua_gettop(l);
	printf("[LuaStack] ");
	for (i = 1; i <= top; i++) {  /* repeat for each level */
		int t = lua_type(l, i);
		switch (t) {

			case LUA_TSTRING:  /* strings */
			printf("'%s'", lua_tostring(l, i));
			break;

			case LUA_TBOOLEAN:  /* booleans */
			printf(lua_toboolean(l, i) ? "true" : "false");
			break;

			case LUA_TNUMBER:  /* numbers */
			printf("%g", lua_tonumber(l, i));
			break;

			default:  /* other values */
			printf("%s", lua_typename(l, t));
			break;

		}
		printf("  ");  /* put a separator */
	}
	printf("\n");  /* end the listing */
}