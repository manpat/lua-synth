local s = synth.new()
local freq = s:value("freq", 220)
local trg = s:trigger("env")

-- local s2 = synth.new()
-- s2:output((s2:sin(55) + s2:tri(110) + s2:tri(220)*0.5) * s2:fade(10) ^ 2)

-- s:output(s:ar(0.001, 1, trg)^2 * (
-- 	s:sin(freq) + 
-- 	s:tri(freq*2) * s:fade(10)^2 +
-- 	s:saw(freq*2) * s:fade(40)^2
-- ))

-- s:output(s:fade(5) * s:ar(0.001, 1.5, trg) * s:lowpass(
-- 	s:sqr(freq/2) + s:saw(freq) + s:sqr(freq + 0.7) + s:noise()*0.2, 
-- 	s:ar(0.01, 0.3, trg)^2 * 10000 + 100
-- ))
-- local signal = s:sqr(freq/2) + s:saw(freq) + s:sqr(freq + 0.7) + s:noise()*0.2
local signal = s:noise()
-- local high = s:highpass(signal, 10000-s:time()*100)
local low = s:lowpass(signal + (s:sqr(40) + s:saw(50))*0.07*s:sin(0.3)^2, 80)
low = s:lowpass(low*5, 50)
low = s:lowpass(low*5, 35)
-- low = s:lowpass(low*5, 30)
-- s:output(s:fade(5) * (high + low))

local dronecutoff = 5 + s:fade(60)^2*5000
local drone = s:lowpass(s:saw(30) + s:saw(30.1*3/2) * 0.2 + s:saw(60.5) * 0.2, dronecutoff + 80)
drone = s:lowpass(drone, dronecutoff + 50)
drone = s:lowpass(drone, 70) * s:fade(30)^2

local bellenv = s:ar(0.02, 4, trg)^2 * s:fade(10)^2
local bell = s:lowpass(s:sin(55) + s:noise()*0.3, 80) * bellenv

local center = 200 + (s:sqr(6 + s:sin(1/9)*4)*0.5+0.5) * 50 + s:sin(1/6) * 150
local width = 10
signal = s:lowpass(signal*2, center+width+80)
signal = s:lowpass(signal, center+width)
-- signal = s:highpass(signal, center-width)
s:output(s:fade(5) * (signal*0.3 + low + drone * 0.7 + bell*0.7))

local ratios = {
	1, 1/2, 3/4, 4/3, 1/3, 3/8, 5/8, 2/3, 5/6
}

local i = 0
local length = 4000

function update(dt)
	i = i+1

	if i%length == 0 then
		-- local nfreq = ratios[math.random(#ratios)] * 220.0

		-- freq:set(nfreq)
		trg:trigger()

		-- length = math.floor((2^math.random(1, 3) + math.random(0, 2) / 3) * 280)
	end
end

