
local r = function() return math.random() ^ 2 end
math.randomseed(os.time())

r(); r(); r()

function create_synth()
	local s = synth.new()
	local o = 0

	local a, b, c, d, e = r(), r(), r(), r(), r()

	for i=1,100 do
		if (i%2) == 1 then
			o = o + s:sin(55 * i) / i * a
			o = o + s:tri(55 * i) / i * b
			o = o + s:sqr(55 * i) / i * c
			o = o + s:saw(55 * i) / i * d
			o = o + s:noise() / i * d
		end
	end

	s:output(o * 0.01)
	return s
end

for i = 1,4 do
	create_synth()
end