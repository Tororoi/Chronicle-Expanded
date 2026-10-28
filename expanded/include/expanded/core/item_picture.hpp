#pragma once

namespace dc::game {
class CRect_i_;
} // namespace dc::game

namespace expanded {

// Draws a weapon's or an attachment's picture, the menus' icon of it, filling screen (the game's 640-wide screen) at
// alpha (0x80 solid). The menu language's wepicon.img is read the first time; false when the item has no picture there.
bool DrawItemPicture(int item_no, const CRect_i_ &screen, int alpha);

} // namespace expanded
