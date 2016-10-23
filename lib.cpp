#include "synth.h"
#include "common.h"

using namespace synth;

#define LUAFUNC(x) static int x(LuaState l)
#define LUALAMBDA [](LuaState l) -> s32

using LuaState = lua_State*;
using LibraryType = const luaL_Reg[];

LuaState l;

namespace{ void stackdump(); }

struct LuaSynthNode {
	Synth* synth;
	bool isNode;
	union {
		u32 node;
		f32 value;
	};

	operator SynthParam() const {
		return SynthParam{isNode, node};
	}
};

struct LuaTrigger {
	Synth* synth;
	u32 trigger;
};

s32 PushLuaSynthNode(Synth* s, u32 node) {
	*(LuaSynthNode*) lua_newuserdata(l, sizeof(LuaSynthNode)) = {s, true, node};
	luaL_setmetatable(l, "nodemt");
	return 1;
}

s32 PushLuaSynthTrigger(Synth* s, u32 trigger) {
	*(LuaTrigger*) lua_newuserdata(l, sizeof(LuaTrigger)) = {s, trigger};
	luaL_setmetatable(l, "triggermt");
	return 1;
}

Synth* GetSynthArg(u32 a) {
	return *(Synth**)luaL_checkudata(l, a, "synthmt");
}

LuaTrigger* GetSynthTriggerArg(u32 a) {
	return (LuaTrigger*)luaL_testudata(l, a, "triggermt");
}

u32 GetSynthTriggerID(u32 a) {
	if(auto trg = (LuaTrigger*)luaL_testudata(l, a, "triggermt"))
		return trg->trigger;

	return ~0u;
}

LuaSynthNode GetSynthNodeArg(u32 a, f32 def = 0.f) {
	LuaSynthNode n {nullptr, false, 0};
	if(lua_isnumber(l, a)) {
		n.value = lua_tonumber(l, a);
		return n;
	}else if(auto node = (LuaSynthNode*)luaL_testudata(l, a, "nodemt")) {
		return *node;
	}

	n.value = def;
	return n;
}

Synth* synth::GetSynthLua(lua_State* l, u32 a) {
	auto s = (Synth**)luaL_testudata(l, a, "synthmt");
	if(s) return *s;
	return nullptr;
}

bool synth::InitLuaLib(LuaState _l) {
	l = _l;
	if(!l) {
		puts("Lua context acquisition failed");
		return false;
	}

	static LibraryType synthLib = {
		{"new", LUALAMBDA {
			auto synth = CreateSynth();
			*(Synth**) lua_newuserdata(l, sizeof(Synth*)) = synth;
			luaL_setmetatable(l, "synthmt");
			return 1;
		}},
		{nullptr, nullptr}
	};

	static LibraryType synthOPS = {
		{"output", LUALAMBDA {
			auto s = GetSynthArg(1);
			auto f = GetSynthNodeArg(2);
			if(f.isNode) {
				s->outputNode = f.node;
				s->playing = true;
			}
			return 0;
		}},

		{"setvalue", LUALAMBDA {
			auto s = GetSynthArg(1);
			auto name = luaL_checkstring(l, 2);
			f32 v = luaL_checknumber(l, 3);
			SetSynthControl(s, name, v);
			return 0;
		}},
		// {"triptrigger", LUALAMBDA {
		// 	auto s = GetSynthArg(1);
		// 	auto name = luaL_checkstring(l, 2);
		// 	TripSynthTrigger(s, name);
		// 	return 0;
		// }},

		{"sin", LUALAMBDA {
			auto s = GetSynthArg(1);
			auto f = GetSynthNodeArg(2);
			auto p = GetSynthNodeArg(3);
			return PushLuaSynthNode(s, NewSinOscillator(s, f, p));
		}},
		{"tri", LUALAMBDA {
			auto s = GetSynthArg(1);
			auto f = GetSynthNodeArg(2);
			auto p = GetSynthNodeArg(3);
			return PushLuaSynthNode(s, NewTriOscillator(s, f, p));
		}},
		{"sqr", LUALAMBDA {
			auto s = GetSynthArg(1);
			auto f = GetSynthNodeArg(2);
			auto d = GetSynthNodeArg(3, 1.f);
			auto p = GetSynthNodeArg(4);
			return PushLuaSynthNode(s, NewSqrOscillator(s, f, p, d));
		}},
		{"saw", LUALAMBDA {
			auto s = GetSynthArg(1);
			auto f = GetSynthNodeArg(2);
			auto p = GetSynthNodeArg(3);
			return PushLuaSynthNode(s, NewSawOscillator(s, f, p));
		}},
		{"noise", LUALAMBDA {
			auto s = GetSynthArg(1);
			return PushLuaSynthNode(s, NewNoiseSource(s));
		}},
		{"time", LUALAMBDA {
			auto s = GetSynthArg(1);
			return PushLuaSynthNode(s, NewTimeSource(s));
		}},

		{"fade", LUALAMBDA {
			auto s = GetSynthArg(1);
			auto f = GetSynthNodeArg(2);
			u32 trg = GetSynthTriggerID(3);
			return PushLuaSynthNode(s, NewFadeEnvelope(s, f, trg));
		}},
		{"ar", LUALAMBDA {
			auto s = GetSynthArg(1);
			auto a = GetSynthNodeArg(2);
			auto r = GetSynthNodeArg(3);
			u32 trg = GetSynthTriggerID(4);
			return PushLuaSynthNode(s, NewADSREnvelope(s, a, 0.f, 0.f, 1.f, r, trg));
		}},

		{"lowpass", LUALAMBDA {
			auto s = GetSynthArg(1);
			auto i = GetSynthNodeArg(2);
			auto f = GetSynthNodeArg(3);
			return PushLuaSynthNode(s, NewLowPassEffect(s, i, f));
		}},
		{"highpass", LUALAMBDA {
			auto s = GetSynthArg(1);
			auto i = GetSynthNodeArg(2);
			auto f = GetSynthNodeArg(3);
			return PushLuaSynthNode(s, NewHighPassEffect(s, i, f));
		}},

		{"value", LUALAMBDA {
			auto s = GetSynthArg(1);
			auto name = luaL_checkstring(l, 2);
			f32 def = luaL_optnumber(l, 3, 0.f);
			return PushLuaSynthNode(s, NewSynthControl(s, name, def));
		}},
		{"trigger", LUALAMBDA {
			auto s = GetSynthArg(1);
			auto name = luaL_checkstring(l, 2);
			return PushLuaSynthTrigger(s, NewSynthTrigger(s, name));
		}},
		{nullptr, nullptr}
	};

	static LibraryType nodeMT = {
		{"__add", LUALAMBDA {
			auto left = GetSynthNodeArg(1);
			auto right = GetSynthNodeArg(2);
			auto s = left.synth?left.synth:right.synth;
			assert(s && ((left.synth == right.synth) || !left.synth || !right.synth));
			return PushLuaSynthNode(s, NewAddOperation(s, left, right));
		}},

		{"__sub", LUALAMBDA {
			auto left = GetSynthNodeArg(1);
			auto right = GetSynthNodeArg(2);
			auto s = left.synth?left.synth:right.synth;
			assert(s && ((left.synth == right.synth) || !left.synth || !right.synth));
			return PushLuaSynthNode(s, NewSubtractOperation(s, left, right));
		}},

		{"__mul", LUALAMBDA {
			auto left = GetSynthNodeArg(1);
			auto right = GetSynthNodeArg(2);
			auto s = left.synth?left.synth:right.synth;
			assert(s && ((left.synth == right.synth) || !left.synth || !right.synth));
			return PushLuaSynthNode(s, NewMultiplyOperation(s, left, right));
		}},

		{"__div", LUALAMBDA {
			auto left = GetSynthNodeArg(1);
			auto right = GetSynthNodeArg(2);
			auto s = left.synth?left.synth:right.synth;
			assert(s && ((left.synth == right.synth) || !left.synth || !right.synth));
			return PushLuaSynthNode(s, NewDivideOperation(s, left, right));
		}},

		{"__pow", LUALAMBDA {
			auto left = GetSynthNodeArg(1);
			auto right = GetSynthNodeArg(2);
			auto s = left.synth?left.synth:right.synth;
			assert(s && ((left.synth == right.synth) || !left.synth || !right.synth));
			return PushLuaSynthNode(s, NewPowOperation(s, left, right));
		}},

		{"__unm", LUALAMBDA {
			auto a = GetSynthNodeArg(1);
			auto s = a.synth;
			assert(s);
			return PushLuaSynthNode(s, NewNegateOperation(s, a));
		}},
		{nullptr, nullptr}
	};

	static LibraryType nodeLib = {
		{"set", LUALAMBDA {
			auto a = GetSynthNodeArg(1);
			if(a.isNode) {
				// TODO: Safety
				auto node = &a.synth->controls[a.node];
				f32 v = luaL_checknumber(l, 2);
				f32 lerpTime = luaL_optnumber(l, 3, 0.f);
				SetSynthControl(a.synth, node->name, v, lerpTime);
			}
			return 0;
		}},

		{nullptr, nullptr}
	};

	static LibraryType triggerMT = {
		{nullptr, nullptr}
	};

	static LibraryType triggerLib = {
		{"trigger", LUALAMBDA {
			// TODO: Safety
			auto a = GetSynthTriggerArg(1);
			assert(a);
			auto trg = &a->synth->triggers[a->trigger];
			TripSynthTrigger(a->synth, trg->name);
			return 0;
		}},

		{nullptr, nullptr}
	};

	luaL_newlib(l, synthLib);
	lua_setglobal(l, "synth");

	luaL_newmetatable(l, "synthmt");
	luaL_newlib(l, synthOPS);
	lua_setfield(l, -2, "__index");

	luaL_newmetatable(l, "nodemt");
	luaL_setfuncs(l, nodeMT, 0);
	luaL_newlib(l, nodeLib);
	lua_setfield(l, -2, "__index");

	luaL_newmetatable(l, "triggermt");
	luaL_setfuncs(l, triggerMT, 0);
	luaL_newlib(l, triggerLib);
	lua_setfield(l, -2, "__index");

	return true;
}

namespace {
	
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
	
}