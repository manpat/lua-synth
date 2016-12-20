import subprocess

def options(cnf):
	cnf.load('compiler_c compiler_cxx')
	cnf.add_option('-d', '--demo', dest='build_demo', default=False, action='store_true', help='Enables demo build')
	
def configure(cnf):
	cnf.load('compiler_c compiler_cxx')
	cnf.check_cfg(args='--cflags', package='lua5.2', uselib_store='lua')

	cnf.env.DEMO = cnf.options.build_demo

	try:
		if cnf.options.build_demo:
			cnf.check_cfg(path='sdl2-config', args='--cflags --libs',
						package='', uselib_store='SDL2')
	except AttributeError:
		pass

def build(bld):
	cxxflags = ["-O2", "-g", "-std=c++11", "-Wall"]

	bld.stlib(
		target		= 'synth',
		source		= ["synth.cpp", "lib.cpp"],
		cxxflags	= cxxflags
		includes	= bld.env.INCLUDES_lua
	)

	if bld.env.DEMO:
		libs = ['sndfile', 'dl']

		bld.program(
			target		= 'demo',
			source		= bld.path.ant_glob("*.cpp", excl = ['synth.cpp', 'lib.cpp']),
			cxxflags	= cxxflags,

			lib=libs,
			use='SDL2 synth lua'
		)

def run(ctx):
	subprocess.Popen(["build/demo"])
