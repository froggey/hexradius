env = Environment(
	CCFLAGS='-Wall -ggdb `pkg-config --cflags sdl`',
	LINKFLAGS='`pkg-config --libs sdl` -lSDL_image -lSDL_ttf'
)
env.Program('octradius', Glob('src/*.cpp'))
