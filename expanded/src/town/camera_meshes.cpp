// The drawn mesh of a frame, apart from expanded/src/town/camera.cpp: visual.hpp's CVisualVu1 (the one the built
// visuals use) cannot share a file with visualvu1.hpp's, which the character headers bring.

#include "framevu1.hpp"
#include "render/draw3d.hpp"
#include "visual.hpp"

namespace expanded {

const dc::render::Draw3DVisual *DrawnMesh(dc::game::CFrame *frame) {
    dc::game::CVisualVu1 *visual = static_cast<dc::game::CFrameVu1 *>(frame)->GetVisual();
    return visual != nullptr ? dc::render::Draw3DFindVisual(visual->vu_data) : nullptr;
}

} // namespace expanded
