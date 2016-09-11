local s = synth.new()
local freq = s:value("freq", 220)
local bass = s:value("bass", 55)
local vel = s:value("velocity", 1)
local trg = s:trigger("env")

local s2 = synth.new()
s2:output((s2:sin(55) + s2:tri(110) + s2:tri(220)*0.5) * s2:fade(10) ^ 2)

-- s:output(s:time() * vel * (
-- 	s:tri(freq*2) * s:ar(0.01, 0.02, trg)^2 +		-- Treble
-- 	s:tri(freq)   * s:ar(0.01, 0.1, trg)^2 +		-- Treble
-- 	s:sqr(freq/2) * s:ar(0.02, 0.4, trg)^2 +		-- Bass
-- 	-- s:sin(bass + s:sin(220)*50)* 0.4				-- Bass 2
-- 	(s:sqr(bass/2, 0.2) + s:sqr(bass+0.1)) * 0.3	-- Bass 2
-- ))

s:output(vel * s:ar(0.001, 1, trg)^2 * (
	s:sin(freq) + 
	s:tri(freq*2) * s:fade(10)^2 +
	s:saw(freq*2) * s:fade(40)^2
))

local ratios = {
	1, 1/2, 3/4, 4/3, 1/3, 3/8, 5/8, 2/3, 5/6
}

local i = 0
local length = 1

function update()
	i = i+1

	if i%length == 0 then
		local nfreq = ratios[math.random(#ratios)] * 220.0

		freq:set(nfreq)
		vel:set(1)
		trg:trigger()

		length = math.floor((2^math.random(1, 3) + math.random(0, 2) / 3) * 150)
	end
end

