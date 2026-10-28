#include "expanded/dungeon/event_sparkle.hpp"

#include <algorithm>

#include "dataalloc.hpp"
#include "dataread.hpp"
#include "dun/gameloop.hpp"
#include "effect.hpp"
#include "expanded/dungeon/floor_characters.hpp"
#include "expanded/dungeon/pack_files.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "mglib.hpp"
#include "rect.hpp"
#include "texture.hpp"

namespace expanded {

namespace {

// The picture could not be entered on this floor: not tried again until the next.
bool failed = false;

void OnFloor() {
    failed = false;
}

[[maybe_unused]] const bool kListening = (AddEnemyFloorListener(OnFloor), true);

CTexture *Picture() {
    return TexManager.GetTexture(const_cast<char *>(kSparklePicture), kFloorCharacterBlock);
}

// The image file out of its pack, read whole into memory of its own (kept for the session).
u_int *DiscImage() {
    static CDataAlloc2<1> arena{-1};
    u_int                *pack = ReadPackIntoArena(kSparklePack, arena, 1);
    return pack != nullptr ? GetPackFile(pack, const_cast<char *>(kSparkleImage), nullptr) : nullptr;
}

SparkleImageRead &ImageRead() {
    static SparkleImageRead read = nullptr;
    return read;
}

} // namespace

bool EventSparkleReady() {
    if (Picture() != nullptr) {
        return true;
    }
    if (failed) {
        return false;
    }
    u_int *image = ImageRead() != nullptr ? ImageRead()() : DiscImage();
    if (image == nullptr || !ImageTexturesFit(image, kFloorCharacterBlock, kSparkleImage)) {
        failed = true;
        return false;
    }
    // Into the floor characters' block beside what is there already.
    LOADTEXTURE_INFO2 entries[] = {
        {reinterpret_cast<char *>(image), kFloorCharacterBlock, 0},
        {nullptr,                         0,                    0},
    };
    TexManager.LoadTextureBlockEX(kFloorCharacterBlock, entries);
    failed = Picture() == nullptr;
    return !failed;
}

void DrawEventSparkle(const float *at, float half_width, float half_height, int brightness) {
    CTexture *texture = Picture();
    if (texture == nullptr || half_width <= 0.0f || half_height <= 0.0f || brightness <= 0) {
        return;
    }
    C3DSprite sprite;
    sprite.texture = texture;
    sprite.texel = CRect_i_(0, 0, kSparkleCell, kSparkleCell);
    auto level = static_cast<u8>(std::min(brightness, 128));
    sprite.colour = {level, level, level, 128};
    std::copy_n(at, 3, sprite.position);
    sprite.half_width = half_width;
    sprite.half_height = half_height;
    TexManager.ReloadTexture(Vif1Packet, kFloorCharacterBlock);
    sprite.Draw();
}

void SetSparkleImageRead(SparkleImageRead read) {
    ImageRead() = read;
    failed = false;
}

} // namespace expanded
