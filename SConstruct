env = Environment(
	CCFLAGS='-Wall -ggdb `pkg-config --cflags sdl`',
	LINKFLAGS='`pkg-config --libs sdl` -lSDL_image -lSDL_ttf -lprotobuf'
)
env.Command('src/octradius.pb.cc', 'src/octradius.proto', ['protoc --cpp_out=. $SOURCE', '''sed -e 's:#include "src/octradius.pb.h":#include "octradius.pb.h":' -i $TARGET'''])
env.Program('octradius', Glob('src/*.cpp') + Glob('src/*.cc'))
