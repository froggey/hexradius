# -*- mode: python -*-
env = Environment(
	CCFLAGS='-Wall -Wextra -Wno-narrowing -ggdb',
	LINKFLAGS='-lprotobuf -lboost_system -lpthread -lboost_thread -lboost_program_options -lboost_filesystem',
)
env.ParseConfig('pkg-config --cflags --libs sdl SDL_image SDL_ttf SDL_gfx')
env.Command(['src/hexradius.pb.cc', 'src/hexradius.pb.h'], 'src/hexradius.proto',
	['protoc --cpp_out=. $SOURCE', '''sed -e 's:#include "src/hexradius.pb.h":#include "hexradius.pb.h":' -i $TARGET'''])
env.Program('hexradius', Glob('src/*.cpp') + Glob('src/*.cc'))
