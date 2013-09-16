#ifndef OR_ANIMATOR_HPP
#define OR_ANIMATOR_HPP

#include <SDL/SDL.h>

#include "octradius.hpp"

namespace Animators {
	struct Generic {
		virtual ~Generic();
		virtual bool render() = 0;
	};

	struct PawnCrush : Generic {
		int tx, ty;
		Uint32 init_ticks;

		PawnCrush(int tile_x, int tile_y);

		bool render();
	};
}

#endif /* !OR_ANIMATOR_HPP */
