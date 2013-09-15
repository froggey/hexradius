# -*- mode: python -*-
env = Environment(
	CCFLAGS='-Wall -Wextra -ggdb',
	LINKFLAGS='-lprotobuf -lboost_system -lpthread -lboost_thread -lboost_program_options',
)
env.ParseConfig('pkg-config --cflags --libs sdl SDL_image SDL_ttf')
env.Command(['src/octradius.pb.cc', 'src/octradius.pb.h'], 'src/octradius.proto',
	['protoc --cpp_out=. $SOURCE', '''sed -e 's:#include "src/octradius.pb.h":#include "octradius.pb.h":' -i $TARGET'''])
env.Program('hexradius', Glob('src/*.cpp') + Glob('src/*.cc'))
