local s = synth.new()

local tempo = 120/60
local barlen = 16
local subdivs = 15

local function ntof(n)
	return 220 * (2 ^ (n/12))
end

local function modstep(f)
	return (s:sqr(f, 1) * 0.5 + 0.5)
end

local kicktrg = s:trigger("kick")
local kickenv = s:ar(0.01, 0.3, kicktrg)

local kick = (
	s:lowpass(s:tri(28, 0.1)*3 + s:sqr(55 + kickenv^2 * 50, 1, 0.3), 20)
) * kickenv * 10

local snaretrg = s:trigger("snare")
local snare = s:noise() * s:ar(0.01, 0.1, snaretrg)

local hihattrg = s:trigger("hihat")
local hihat = s:highpass(s:noise()*2, 6000) * s:ar(0.01, 0.045, hihattrg)

local scalemod = modstep(tempo/8)

local scale = {
	ntof( 0),
	ntof( 2),
	ntof( 4 - scalemod*1),
	ntof( 5),
	ntof( 7),
	ntof( 9),
	ntof(11 - scalemod*4),
	ntof(12),
}

local chord =
	s:saw(scale[1], 0.1) +
	s:tri(scale[3], 0.4) +
	s:tri(scale[5], 0.2) +
	s:saw(scale[7], 0.3) +

	s:saw(scale[3] * 2, 0.5) * 0.5 +
	s:saw(scale[7] * 2, 0.5) * 0.5 +
	s:saw(scale[1] * 3, 0.5) * 0.5 +
	s:saw(scale[3] * 4, 0.5) +
	s:saw(scale[7] * 4, 0.5) * 1.5 +
	0

chord = s:lowpass(chord, 400 * (s:sin(tempo*3) * 0.45 + 0.5) ^ 2)
chord = chord * (1 - kickenv)

local dronemod = scalemod * 8 - modstep(tempo/16) * 3

local drone = s:saw(ntof(-36), 0.2) + s:sqr(ntof(-36) + 0.1, 1, 0.7)
	 + s:sqr(ntof(-24+dronemod) + 0.2, 1, 0.4)
	 + s:saw(ntof(-12+dronemod) + 0.2, 0.4)
	 + s:noise() 

local dronefiltermod = (s:tri(tempo*3, 0.25)*0.5 + 0.5)^2
drone = s:lowpass(drone*2, 400 + dronefiltermod * 1000)
drone = s:lowpass(drone, 120 + dronefiltermod * 1000)
drone = s:lowpass(drone, 80 + dronefiltermod * 1000)

local o = kick 
	+ snare * 0.5
	+ hihat * 0.5
	+ chord * 0.25
	+ drone * 0.3
s:output(o)

local beat = nil
local subdiv = nil

function update(elapsed, dt)
	local nbeat = math.floor(elapsed * tempo)
	local nsubdiv = math.floor((elapsed * tempo - nbeat) * subdivs)

	if not beat then
		beat = nbeat
		subdiv = nsubdiv
	end

	if nbeat + nsubdiv/subdivs > beat + subdiv/subdivs then
		onbeat(nbeat%barlen, nsubdiv)
		beat = nbeat
		subdiv = nsubdiv
	end
end

function onbeat(b, sub)
	if sub == 0 then
		kicktrg:trigger()
	end

	if (b%2 == 1) and (sub == 0) then
		snaretrg:trigger()
	end

	if (b == 15) and (sub == 4) then
		kicktrg:trigger()
	end

	if (b%8 == 7) and (sub == 5) then
		snaretrg:trigger()
	end

	if (sub == 7) then
		hihattrg:trigger()
	end
end