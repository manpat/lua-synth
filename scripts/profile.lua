
local r = function() return math.random() ^ 2 end
math.randomseed(os.time())

r(); r(); r()

function create_synth()
	local s = synth.new()
	local o = 0

	local a, b, c, d, e = r(), r(), r(), r(), r()

	for i=1,10 do
		local o2 = 
			s:sin(55 * i) * a
			+ s:tri(55 * 3/2 * i) * b
			+ s:sqr(55 * 4/2* i) * c
			+ s:saw(55 * 5/2 * i) * d
			+ s:noise() * e

		o = o + o2 / i
	end

	s:output(o * 0.01)
	return s
end

for i = 1,10 do
	create_synth()
end