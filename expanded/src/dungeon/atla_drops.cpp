#include "dungeon.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <optional>
#include <string>

#include "clsmes.hpp"
#include "dngstatusdata.hpp"
#include "dun/gameloop.hpp"
#include "dungeonmap.hpp"
#include "expanded/core/menu_text.hpp"
#include "expanded/dungeon/atla_drops.hpp"
#include "expanded/weapons/abilities.hpp"
#include "frame.hpp"
#include "itemdata.hpp"
#include "localize/localize.hpp"
#include "mathutil.hpp"
#include "mglib.hpp"
#include "sysmes.hpp"
#include "texture.hpp"
#include "userstatus.hpp"

namespace dc::game {
extern s32 atraGetStatus;
} // namespace dc::game

namespace expanded {

namespace {

constexpr int   kAtlas = 8;
constexpr float kPickupRadius = 13.0f; // SetAtraBoll's
constexpr float kLift = 24.0f;         // 6 at the middle of the way
constexpr int   kAtlaTextureBlock = 0x16;
constexpr int   kMessageFrames = 0x28;

// The SynthSphere's line, a system message of its own (the bank has none past 78): the weapon's name in it.
std::optional<std::string> SphereLine() {
    return LocalizeText("expanded.atla_drop.sphere", "{icon-orb}\"{insert1}\" SynthSphere acquired.");
}

[[maybe_unused]] const bool kSphereLine = (SetSystemMessage(kAtlaSphereMessage, SphereLine), true);
// A SynthSphere's values: a sixth of its weapon's, rounded up.
constexpr double kSphereShare = 6.0;

struct Placed {
    bool     drop_set = false;
    AtlaDrop drop;
    bool     moving = false;
    bool     over_shadows = false;
    AtlaPath path{};
    float    omega = 0, inv_sin = 1;
    float    progress = 0;
};

Placed   placed[kAtlas];
AtlaDrop named;
bool     naming = false;

DUNGEON_EVENT *AtlaEvent(CDungeonMap &map, int atla) {
    for (DUNGEON_EVENT &event : map.events) {
        if (event.kind == DNG_EVENT_ATRA && event.index == atla) {
            return &event;
        }
    }
    return nullptr;
}

DUNGEON_EVENT *FreeEvent(CDungeonMap &map) {
    for (DUNGEON_EVENT &event : map.events) {
        if (event.kind == DNG_EVENT_NONE) {
            return &event;
        }
    }
    return nullptr;
}

void SetPlace(CDungeonMap &map, int atla, const float *pos) {
    std::copy_n(pos, 3, map.atra[atla].pos);
    map.atra[atla].pos[3] = 1.0f;
    map.atra[atla].phase = 0.0f;
    if (DUNGEON_EVENT *event = AtlaEvent(map, atla)) {
        std::copy_n(pos, 3, event->pos);
        event->pos[3] = 1.0f;
    }
}

int WeaponMessage(int weapon) {
    COM_ITEM_INFO *info = GetCommonItemInfo(weapon);
    return info != nullptr ? info->msg + 100 : -1;
}

s16 Sixth(int value) {
    return static_cast<s16>(std::ceil(value / kSphereShare));
}

// A weapon's SynthSphere onto the attachment board, with the drop's own values if it has them; false with the
// board full.
bool GiveSphere(const AtlaDrop &drop) {
    int             weapon = drop.weapon;
    WEAPON_DATA    *data = GetWeaponData(weapon);
    CDngStatusData *status = static_cast<CDngStatusData *>(UserStatus);
    int             slot = data != nullptr ? status->GetItem(ITEM_ATTACH_SYNTHESIS_SPHERE, 1) : -1;
    if (slot < 0) {
        return false;
    }
    ATTACH_LIST &sphere = *reinterpret_cast<ATTACH_LIST *>(&status->consumable_items[slot]);
    std::memset(&sphere, 0, sizeof(sphere));
    sphere.item_no = ITEM_ATTACH_SYNTHESIS_SPHERE;
    sphere.sphere_weapon_no = static_cast<s16>(weapon);
    if (drop.own_values) {
        std::copy_n(drop.status, 4, sphere.status);
        std::copy_n(drop.elem, 5, sphere.elem);
        std::copy_n(drop.vs_monster, 10, sphere.vs_monster);
        sphere.sphere_flags = drop.flags;
        sphere.sphere_level = drop.level;
        return true;
    }
    sphere.sphere_flags = static_cast<s16>(WeaponAbilities(weapon, data->flags));
    const s16 stats[] = {data->attack, data->endurance, data->speed, data->magic};
    for (int i = 0; i < 4; i++) {
        sphere.status[i] = Sixth(stats[i]);
    }
    for (int i = 0; i < 5; i++) {
        sphere.elem[i] = static_cast<s8>(Sixth(data->elem[i]));
    }
    for (int i = 0; i < 10; i++) {
        sphere.vs_monster[i] = static_cast<s8>(Sixth(data->vs_monster[i]));
    }
    return true;
}

// The drop into the player's hands: the weapon as a chest gives one (its SynthSphere if its owner has no room), a
// sphere onto the board. What was given, for its name.
AtlaDrop Deliver(AtlaDrop drop) {
    if (!drop.sphere && static_cast<CDngStatusData *>(UserStatus)->GetItem(drop.weapon, 1) >= 0) {
        return drop;
    }
    drop.sphere = true;
    if (!GiveSphere(drop)) {
        drop.weapon = 0; // the board full: lost
    }
    return drop;
}

void Name(const AtlaDrop &drop) {
    if (drop.weapon == 0) {
        return;
    }
    if (!drop.sphere) {
        ItemGetMes(drop.weapon, -1, kMessageFrames, 1);
        return;
    }
    int args[4] = {WeaponMessage(drop.weapon), -1, -1, -1};
    SetSystemMes(kAtlaSphereMessage, kMessageFrames, MES_POS_BOTTOM, 1, args, nullptr);
}

} // namespace

int PlaceAtlaDrop(const float *pos, const AtlaDrop &drop) {
    CDungeonMap *map = NowDngMap;
    if (map == nullptr || map->atra_model == nullptr || FreeEvent(*map) == nullptr) {
        return -1;
    }
    float place[4] = {pos[0], pos[1], pos[2], 1.0f};
    int   atla = -1;
    if (map->atra_num < kAtlas) {
        atla = map->atra_num;
        map->SetAtraBoll(place, -1);
    } else {
        for (int i = 0; i < kAtlas && atla < 0; i++) {
            if (map->atra[i].used == 0) {
                atla = i;
            }
        }
        if (atla < 0) {
            return -1;
        }
        // A collected atla's place, as SetAtraBoll lays one (it only appends).
        DUNGEON_EVENT *event = FreeEvent(*map);
        map->atra[atla].atra_no = -1;
        map->atra[atla].used = 1;
        event->kind = DNG_EVENT_ATRA;
        event->placed_flag = 0;
        event->index = atla;
        event->radius = kPickupRadius;
        SetPlace(*map, atla, place);
    }
    placed[atla] = {};
    placed[atla].drop_set = true;
    placed[atla].drop = drop;
    return atla;
}

int TakeOverAtla(const float *pos) {
    CDungeonMap *map = NowDngMap;
    if (map == nullptr || map->atra_model == nullptr || map->atra_num < kAtlas) {
        return -1;
    }
    for (int i = 0; i < kAtlas; i++) {
        if (map->atra[i].used == 0) {
            return -1;
        }
    }
    float player[4];
    sceVu0CopyVector(player, CharaMain.pos);
    int   farthest = -1;
    float most = -1;
    for (int i = 0; i < kAtlas; i++) {
        float dx = map->atra[i].pos[0] - player[0], dz = map->atra[i].pos[2] - player[2];
        if (!placed[i].moving && dx * dx + dz * dz > most) {
            most = dx * dx + dz * dz;
            farthest = i;
        }
    }
    if (farthest >= 0) {
        SetPlace(*map, farthest, pos);
    }
    return farthest;
}

void MoveAtla(int atla, const AtlaPath &path, float progress) {
    CDungeonMap *map = NowDngMap;
    if (map == nullptr || atla < 0 || atla >= kAtlas) {
        return;
    }
    Placed &p = placed[atla];
    p.path = path;
    // The shorter way round.
    float dot = 0;
    for (int i = 0; i < 4; i++) {
        dot += path.q0[i] * path.q1[i];
    }
    if (dot < 0) {
        for (float &c : p.path.q0) {
            c = -c;
        }
        dot = -dot;
    }
    p.omega = std::max(1e-4f, std::acos(std::min(1.0f, dot)));
    p.inv_sin = 1.0f / std::sin(p.omega);
    p.progress = progress;
    p.moving = progress < 1.0f;
    SetPlace(*map, atla, path.to);
    if (DUNGEON_EVENT *event = AtlaEvent(*map, atla)) {
        event->radius = p.moving ? 0.0f : kPickupRadius;
    }
}

void DrawAtlaOverShadows(int atla, bool on) {
    if (atla >= 0 && atla < kAtlas) {
        placed[atla].over_shadows = on;
    }
}

void GiveAtlaDrop(const AtlaDrop &drop) {
    Name(Deliver(drop));
}

namespace {

// An atla on its way into place, drawn at its progress (below 0 out of the draw).
void DrawMoving(CDungeonMap &map, int atla) {
    const Placed &p = placed[atla];
    map.atra[atla].phase = 0.0f; // it bobs from rest once settled
    if (p.progress < 0.0f) {
        return;
    }
    float   t = p.progress, e = 1.0f - (1.0f - t) * (1.0f - t);
    float   w0 = std::sin((1.0f - e) * p.omega) * p.inv_sin, w1 = std::sin(e * p.omega) * p.inv_sin;
    float   turn[4], place[4];
    float   scale = p.path.scale + (1.0f - p.path.scale) * e;
    CFrame *model = map.atra_model;
    for (int i = 0; i < 4; i++) {
        turn[i] = p.path.q0[i] * w0 + p.path.q1[i] * w1;
    }
    for (int i = 0; i < 3; i++) {
        place[i] = p.path.from[i] + (p.path.to[i] - p.path.from[i]) * e;
    }
    place[1] += kLift * t * (1.0f - t);
    place[3] = 1.0f;
    sceVu0FMATRIX own;
    sceVu0CopyMatrix(own, model->local);
    model->SetTransMatrix(turn);
    model->SetScale(scale, scale, scale);
    model->SetPosition(place);
    MGDraw(model);
    model->SetTransMatrix(own);
    model->SetScale(1.0f, 1.0f, 1.0f);
}

} // namespace

bool AtlaDrawn(CDungeonMap &map, int atla) {
    if (atla < 0 || atla >= kAtlas || (!placed[atla].moving && !placed[atla].over_shadows)) {
        return false;
    }
    if (!placed[atla].over_shadows) {
        DrawMoving(map, atla);
    }
    return true;
}

// As CDungeonMap::DrawAtraBoll draws an atla (the port's: Moon Sea's in sight however far), after the shadow pass.
void AtlaDropsDrawOverShadows(const float *pos) {
    CDungeonMap *map = NowDngMap;
    if (atraGetStatus != 0 || map == nullptr || map->atra_model == nullptr) { // not while one is being taken
        return;
    }
    bool loaded = false;
    for (int i = 0; i < std::min(map->atra_num, kAtlas); i++) {
        if (!placed[i].over_shadows || map->atra[i].used == 0 ||
            (selectMapNo != DUNGEON_MOON_SEA && DistVector(map->atra[i].pos, const_cast<float *>(pos)) > 160.0f * map->draw_dist_scale)) {
            continue;
        }
        if (!loaded) {
            TexManager.ReloadTexture(Vif1Packet, kAtlaTextureBlock);
            loaded = true;
        }
        if (placed[i].moving) {
            DrawMoving(*map, i);
            continue;
        }
        float draw_pos[4];
        sceVu0CopyVector(draw_pos, map->atra[i].pos);
        draw_pos[1] += std::sin(map->atra[i].phase);
        map->atra_model->SetPosition(draw_pos);
        MGDraw(map->atra_model);
        map->atra[i].phase = map->atra[i].phase <= 360.0f ? map->atra[i].phase + 0.1f : 0.0f;
    }
}

bool AtlaDropTaken(int atla) {
    if (atla < 0 || atla >= kAtlas) {
        return false;
    }
    bool carried = placed[atla].drop_set;
    if (carried) {
        named = Deliver(placed[atla].drop);
        naming = true;
        if (placed[atla].drop.taken != nullptr) {
            placed[atla].drop.taken(placed[atla].drop.tag);
        }
    }
    placed[atla] = {};
    return carried;
}

bool AtlaDropNamed() {
    if (!naming) {
        return false;
    }
    naming = false;
    Name(named);
    return true;
}

void AtlaDropsFloorLoaded() {
    for (Placed &p : placed) {
        p = {};
    }
    naming = false;
}

} // namespace expanded
