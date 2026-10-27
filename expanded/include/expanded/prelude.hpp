#pragma once

// Included ahead of every unit when Chronicle-Expanded is built: the mod's code names the game's code and the
// platform layer's as if they were its own, so namespace expanded sees dc::game and the subsystems it calls into.
// Its own names still come first.

namespace dc::game {}

namespace dc::data {}

namespace dc::localize {}

namespace dc::options {}

namespace dc::platform {}

namespace dc::render {}

namespace dc::text {}

namespace dc::app {}

namespace expanded {
using namespace dc::game;
using namespace dc::data;
using namespace dc::localize;
using namespace dc::options;
using namespace dc::platform;
using namespace dc::render;
using namespace dc::text;
using namespace dc::app;
} // namespace expanded
