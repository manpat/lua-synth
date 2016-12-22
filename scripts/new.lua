
local r = math.random

local tempo = 130/60
local s = synth.new()
local noteVal = s:value("note", 110)

local function ntof(n)
	return noteVal * (2^(n/12))
end

local scale = {
	ntof( 0),
	ntof( 2),
	ntof( 4),
	ntof( 5),
	ntof( 7),
	ntof( 9),
	ntof(11),
	ntof(12)
}

local root = 1
local bass = s:tri(scale[root], r()) + (s:sin(scale[root]/2, r()) + s:sin(scale[root]/4, r())) * 2

local o = bass
	+ s:sqr(scale[3], r())
	+ s:sqr(scale[5], r())
	+ s:saw(scale[5], r())
	+ s:saw(scale[8] * 2, r())

local belltrg = s:trigger("bell")
local bell = s:saw(scale[1] * 3, r()) * s:ar(0.01, 0.4, belltrg)
	-- + s:tri(scale[1] * 2, r()) * (s:ar(0.01, 0.05, belltrg)^2) * 3
	+ s:tri(scale[1] * 4, r()) * (s:ar(0.01, 0.05, belltrg)^2) * 3
	+ s:noise() * (s:ar(0.01, 0.5, belltrg)^9) * 4

local kicktrg = s:trigger("kick")
local kick = (s:sin(51) + s:lowpass(s:saw(53), 10)) * (s:ar(0.01, 0.3, kicktrg)^2) * 20

o = s:lowpass(o, 50 + ((s:sin(tempo*4)*0.5 + 0.5)^3) * 1600)
o = s:lowpass(o, 50 + (s:fade(0.5, kicktrg)^3) * 1600)
o = o + bell
o = o + kick

s:output(o)

local time = 0
local beat = 0

kicktrg:trigger()

function update(dt)
	time = time + dt

	local btime = time * tempo
	local nbeat = math.floor(btime + (1 - btime % 7) * 0.2)
	if nbeat > beat then
		beat = nbeat

		kicktrg:trigger()

		if beat % 2 == 1 then
			belltrg:trigger()
		end
	end

	noteVal:set(220 + math.sin(time * 2 * math.pi * 6) * 3)
end