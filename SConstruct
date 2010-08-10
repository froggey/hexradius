env = Environment(
	CCFLAGS='`pkg-config --cflags sdl`',
	LINKFLAGS='`pkg-config --libs sdl` -lSDL_image'
)
env.Program('octradius', Glob('src/*.cpp'))
