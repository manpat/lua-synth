function pulse(rate)
	return (osc.sqr(rate) * 0.5 + 0.5)
end

function modulate(rate)
	return (osc.sin(rate) * 0.5 + 0.5)
end

-- local ratios = {1, 3/2, 5/4, 9/4, 3, 4, 5}
-- local ratios = {1/(3+pulse(1/4)), 1/2, 1, 3/2, 6/4 + (5/4 - 6/4)*pulse(1/4), 9/4 - 1/4*(1-pulse(1/4))}
-- local rate = 1/2
-- local ratios = {
-- 	seq.cycle(rate, {1/3, 1/4}), 1/2, 1, 3/2, seq.cycle(rate, {6/4, 5/4}), seq.cycle(rate, {8/4, 9/4})
-- }
-- local ratios = {1, 5/4, 9/4}
-- local ratios = {1, 3/2, 9/4}

-- local root = 220
-- local o = 0
-- local mod = osc.sin(6)*0.01 *0

-- for i = 1, #ratios do
-- 	o = o + osc.sqr(root * (ratios[i] + mod)) / i
-- end

-- local high = 9/2 + 1/2 * (pulse(1/32)*2*(osc.sqr(4 + osc.sin(1/4)*2)*0.05 + 1)*0 + pulse(1/16)*(osc.sqr(1)*0.1 + 1))

-- local o = (osc.sqr(55) :duty(osc.sin(0.2)*0.3 + 0.5)
-- 	+ osc.sqr(55) :duty(osc.sin(0.5)*0.1 + 0.5) :shift(osc.tri(0.1) * 0.5 + 0.5)) * 2
-- 	+ osc.sqr(55*3/2)
-- 	+ osc.sqr(55*4/2)
-- 	+ osc.saw(55*5/2)
-- 	+ osc.saw(55*6/2):shift(1/4)
-- 	+ osc.saw(55*high):shift(1/3) * 2

-- local o = osc.sin((1400 + osc.tri(1/10)*1200) * osc.sin(110 + osc.tri(6)*4))
-- 	+ osc.saw(800 * osc.tri(110*3/2))
-- 	+ osc.saw(800 * osc.tri(110*5/2))
-- 	+ osc.saw(800 * osc.tri(110*3))

-- local o = 
-- 	osc.sin(55) * (osc.sin(1/4 + modulate(1/12)/6))
-- 	+ osc.sin(51) * (osc.sin(1/3 + modulate(1/10)/4))
-- 	+ osc.noise() * modulate(8 + osc.tri(1/8 + osc.sin(1/12)/9):shift(1/2)*6) * 0.05 * osc.sin(1/9 + osc.sin(1/10)/10)
-- 	+ (osc.saw(55) + osc.saw(30.3):shift(1/2)) * (1-modulate(1/12)) * 0.5

if false then
	local trig0 = env.trigger()
	local trig1 = env.trigger()
	local trig2 = env.trigger()

	local katk = 0.01
	local krel = 0.3
	local kenv = env.ar(katk, krel):trigger(trig0)^2 + env.ar(katk, krel):trigger(trig1)^2
	local tone = env.ar(katk, 0.5):trigger(trig0) + env.ar(katk, 0.5):trigger(trig1)
	local kick = osc.sin(50 + tone*10)

	local snare = osc.tri(600 + env.ar(0.01, 0.09):trigger(trig2)*20)
	local senv = env.ar(0.01, 0.1):trigger(trig2)

	kick = kick * kenv * 2
	snare = snare * senv^3 * 2
	o = kick + snare + o*env.linear(10)^2 * 0.2 * (1-tone^2)
	osc.output(o)

	return
end

-- local t = env.trigger()

-- local e0 = env.linear(0.05):trigger(t)
-- local e1 = env.linear(1):trigger(t)
-- local f = 110 - e0*55
-- local wave = osc.sqr(f) + osc.sqr(f+0.5):duty(0.4) * (1-pulse(12)*0.6)
-- local o = wave * (1-e1^2) * 0.2

-- local e0 = env.linear(15):trigger(t)
-- local f0 = e0^2*(111*5/4 -110)
-- local f1 = e0^2*(112*3/4 -110)
-- local f2 = e0^2*(110.2*2 -110)
-- local f3 = e0^2*(112/2 -110)
-- local f4 = e0^2*(110*3 -110)
-- local f5 = e0^2*(110.5*3/2 -110)
-- local f6 = e0^2*(110.1*4 -110)
-- local wave = osc.saw(110) 
-- 	+ osc.saw(110+f0):shift(e0*0.5) 
-- 	+ osc.saw(110+f1):shift(e0*0.7)
-- 	+ osc.saw(110+f2):shift(e0*0.3)
-- 	+ osc.saw(110+f3):shift(e0*0.4)
-- 	+ osc.saw(110+f4):shift(e0*0.1)
-- 	+ osc.saw(110+f5):shift(e0*0.6)
-- 	+ osc.saw(110+f6):shift(e0*0.7) * 0.5
-- local o = wave * (0.02 * env.linear(4)^2 + e0*0.14)

-- osc.output(o)
-- local seq0 = seq.cycle(16, {55, 110, 220, 330, 440, 550, 660, 880})
-- local seq1 = seq.cycle(4, {seq.cycle(24, {55, 110}), 220 + osc.sin(10)*3})
-- local wave = seq.cycle(0.5, {osc.saw(seq0), osc.sin(seq1)})

function mix(x, a, b)
	return a*(1-x) + b*x
end

function flatten(t)
	local n = {}
	for k,v in ipairs(t) do
		if type(v) == "table" then 
			for _,v2 in ipairs(flatten(v)) do
				n[#n+1] = v2
			end
		else
			n[#n+1] = v
		end
	end

	return n
end

function rep2(x)
	return {x, x}
end

--[[ 
	U	1 
	m2	25/24
	M2	9/8
	m3	6/5
	M3	5/4
	P4	4/3
	d5	45/32
	P5	3/2
	m6	8/5
	M6	5/3
	m7	9/5
	M7	15/8
	U	2
]]

-- http://xenharmonic.wikispaces.com/Gallery+of+Just+Intervals
-- http://www.huygens-fokker.org/docs/intervals.html
-- http://xenharmonic.wikispaces.com/List+of+Superparticular+Intervals

local bassseq = seq.cycle(1, {55, 55, 55*2, 55*3/2})
local melodyseq = seq.cycle(4 + pulse(1/2)*osc.sqr(1/3), flatten{
	rep2(1), rep2(5/4), rep2(5/3), rep2(5/2), 
	rep2(5), rep2(5/2), rep2(5/3), rep2(5/4),

	rep2(15/8), rep2(3/2), rep2(9/8), rep2(4/3), 
	rep2(5/4), rep2(3/2), rep2(5/3), rep2(2/1), 
	-- rep2(10/9), -- rep2(4/3),
	-- rep2(9/4), 
	9/8, 4/3, 9/4, 10/4
})

-- local wave = (osc.sqr(bassseq)*0.5 + osc.tri(bassseq*(5/4 + osc.sin(6)*0.02)):shift(0.5)^4*0.6)*env.linear(48)^2
-- 	+ mix(env.linear(32)^2 * 0.5, osc.sin(melodyseq*(110*6/2)), osc.saw(melodyseq*(110*3/2)):shift(0.25))*0.5

-- osc.output(wave * env.linear(8)^2)

-- local wave = (osc.sqr(bassseq)*0.5 + osc.tri(bassseq*(5/4 + osc.sin(6)*0.02)):shift(0.5)^4*0.6)*midi.ctl(2)^2
-- 	+ mix(midi.ctl(3)^2 * 0.5, osc.sin(melodyseq*(110*6/2)), osc.saw(melodyseq*(110*3/2)):shift(0.25))*0.5

-- osc.output(wave * (midi.ctl(1) ^ 2));

-- osc.output(osc.saw(
-- 	(seq.cycle(4, {
-- 		midi.ctl(1), midi.ctl(2), midi.ctl(3), midi.ctl(4), 
-- 		midi.ctl(5), midi.ctl(6), midi.ctl(7), midi.ctl(8), 
-- 	}) + 1) * 55
-- ))

-- local key = midi.key(0)

-- local out = osc.sin((4000*midi.ctl(1)) * osc.tri(key) * midi.vel(0))

-- out = out + osc.tri(key * 3/2)*midi.ctl(2)
-- out = out + osc.tri(key * 2)*midi.ctl(3)
-- out = out + osc.tri(key * 3)*midi.ctl(4)

local arp = seq.cycle(midi.ctl(2)*32, {1, 2, 4, 3})
local out = 0
for i=0,15 do
	-- local e = env.ar(midi.ctl(5)*4, midi.ctl(6)*4):trigger(midi.trg(i))
	local e = env.linear(midi.ctl(5)*4):trigger(midi.trg(i))
	out = out + osc.sin((4000*midi.ctl(1)) * osc.sin(midi.key(i) * arp) * midi.vel(i)) * e
end

local basskey = midi.key(0, 1)
local trebkey = midi.key(-1, 1)
-- out = out + (osc.tri(basskey/4) + osc.tri(basskey/2) + osc.tri(basskey)) * 0.3
-- out = out + osc.tri(trebkey*2)*0.5

osc.output(out * 0.5)
