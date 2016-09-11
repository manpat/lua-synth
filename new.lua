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

s:output(s:fade(5) * s:ar(0.001, 1.0, trg) * s:lowpass(
	s:saw(freq) + s:sqr(freq + 0.7), 
	s:ar(0.1, 0.3, trg)^2 * 500 + 55
))

local ratios = {
	1, 1/2, 3/4, 4/3, 1/3, 3/8, 5/8, 2/3, 5/6
}

local i = 0
local length = 1

function update()
	i = i+1

	if i%length == 0 then
		local nfreq = ratios[math.random(#ratios)] * 110.0

		freq:set(nfreq)
		trg:trigger()

		length = math.floor((2^math.random(1, 3) + math.random(0, 2) / 3) * 120)
	end
end

