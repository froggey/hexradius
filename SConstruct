env = Environment(
	CCFLAGS='`pkg-config --cflags sdl SDL_image`',
	LINKFLAGS='`pkg-config --libs sdl SDL_image`'
)
env.Program('octradius', Glob('src/*.cpp'))
