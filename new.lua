local s = synth.new()
local freq = s:value("freq", 220)
local bass = s:value("bass", 55)
local vel = s:value("velocity", 1)
local trg = s:trigger("env")

s:output(vel * (
	s:tri(freq*2) * s:ar(0.01, 0.02, trg)^2 +		-- Treble
	s:tri(freq)   * s:ar(0.01, 0.1, trg)^2 +		-- Treble
	s:sqr(freq/2) * s:ar(0.02, 0.4, trg)^2 +		-- Bass
	-- s:sin(bass + s:sin(220)*50)* 0.4				-- Bass 2
	(s:sqr(bass/2, 0.2) + s:sqr(bass+0.1)) * 0.3	-- Bass 2
))

local i, length = 0, 200
local ratios = {1/4, 1/3, 1/2, 1, 2/3, 3/2, 5/4, 9/8, 2, 4/3};

for k,v in pairs(ratios) do
	ratios[k] = v * 440
end

math.randomseed(0xc0ffee1)

function update()
	i = i+1

	if i%length == 0 then
		local nfreq = ratios[math.random(#ratios)]
		if nfreq <= 220 then bass:set(nfreq/2, 0.1)
		elseif nfreq <= 440 then bass:set(nfreq/4, 0.1) end

		freq:set(nfreq, 0.02)
		vel:set(math.random() * 0.6 + 0.4, 0.001)
		trg:trigger()

		length = 2^math.random(1, 3) * 60
	end
end

