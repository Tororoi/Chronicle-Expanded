#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <format>
#include <map>
#include <set>
#include <stdexcept>
#include <string>

#include "expanded/data/atla_gemron_recipe.hpp"
#include "expanded/data/gemron_models.hpp"
#include "expanded/data/gemron_parts.hpp"
#include "expanded/data/model_parts.hpp"
#include "expanded/data/sheet_look.hpp"

namespace expanded {

namespace {

namespace rc = atla_recipe;

constexpr const char *kMds = "e112a.mds";
constexpr const char *kMot = "e112a.mot";
constexpr const char *kShadowMot = "e112s.mot";
constexpr const char *kBbp = "e112a.bbp";
constexpr const char *kImg = "e112a01.img";
constexpr const char *kSheet = "e112a01";
constexpr const char *kSheet2 = "e112a02";

// The held atla: the dungeon's own, under the big gem.
constexpr const char *kAtlaMds = "atr.mds";
constexpr const char *kGem = "tama1__m";
// The forehead Atlamillia: Toan's lens on its patch of his sheet (x, y, width, height).
constexpr const char *kToanMds = "c01d.mds";
constexpr const char *kLens = "renzu__m";
constexpr const char *kLensTexture = "c01d03";
constexpr int         kLensCrop[] = {128, 32, 128, 64};
constexpr const char *kHead = "obj2_2";
// The light: e503ex_atall's glow pair, under a root of its own on the lens.
constexpr const char *kLightMds = "e503ex_atall.mds";
constexpr const char *kLightMot = "e503ex_atall.mot";
constexpr const char *kLightName = "atlight";
constexpr const char *kLightTexture = "e503ex_at03";
// The wing lights: fly_light's glow pair on each wing gem, its picture half desaturated.
constexpr const char *kFlyMds = "fly_light.mds";
constexpr const char *kFlyImg = "fly_light.img";
constexpr const char *kFlyTexture = "fly_light";
constexpr const char *kFlyNodes[] = {"012__CZAPPBA", "013__CZAPPBA"};
constexpr const char *kWingGems[] = {"tamas00__m", "tamas03__m"};
constexpr double      kFlySaturation = 0.5;

constexpr int           kClipFrom = 105, kClipTo = 125;
constexpr int           kTimelineEnd = kAtlaGemronDeathEnd + 1;
constexpr std::uint32_t kSubtreeVisible = 0x33;
constexpr std::uint32_t kCameraChannels[] = {30, 31, 32, 33};
constexpr std::uint32_t kMaterialAlpha = 40;

constexpr Mat3 kIdentity = {
    {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}
};

std::uint16_t Get16(std::span<const unsigned char> data, std::size_t at) {
    return static_cast<std::uint16_t>(data[at] | data[at + 1] << 8);
}

void Set16(Bytes &data, std::size_t at, std::uint16_t value) {
    data[at] = static_cast<unsigned char>(value);
    data[at + 1] = static_cast<unsigned char>(value >> 8);
}

const MdsFrame &Frame(const std::vector<MdsFrame> &frames, std::string_view name, const char *mds) {
    const MdsFrame *f = FindFrame(frames, name);
    if (f == nullptr) {
        throw std::runtime_error(std::format("{} has no {}", mds, name));
    }
    return *f;
}

bool Camera(std::uint32_t channel) {
    return std::ranges::find(kCameraChannels, channel) != std::end(kCameraChannels);
}

// A stepped track's value: the last key on or before the frame (the first before them all).
std::array<double, 4> StepAt(const MotTrack &track, double frame) {
    const MotKeyframe *key = &track.keys.front();
    for (const MotKeyframe &k : track.keys) {
        if (k.Frame() <= frame) {
            key = &k;
        }
    }
    return Wide(KeyValues(*key));
}

std::array<double, 4> LightAt(const MotTrack &track, double frame) {
    return track.w2 == 0x32 || track.w2 == 0x33 ? StepAt(track, frame) : TrackAt(track, frame);
}

// The keys a track keeps: from the first, each next one as far on as the straight blend to it reproduces every key
// between within the channel's tolerance (x, y and z, or the alpha); other channels keep them all.
std::vector<MotKeyframe> Thinned(const std::vector<MotKeyframe> &keys, std::uint32_t channel) {
    double tolerance;
    switch (channel) {
        case 1:
            tolerance = 0.02;
            break;
        case 2:
            tolerance = 0.05;
            break;
        case 40:
            tolerance = 0.03;
            break;
        default:
            return keys;
    }
    int                      n = channel == 40 ? 1 : 3;
    std::vector<MotKeyframe> out = {keys[0]};
    std::size_t              i = 0;
    while (i + 1 < keys.size()) {
        std::size_t j = i + 1;
        while (j + 1 < keys.size()) {
            std::array<float, 4> va = KeyValues(keys[i]), vb = KeyValues(keys[j + 1]);
            double               fa = keys[i].Frame(), fb = keys[j + 1].Frame();
            bool                 ok = true;
            for (std::size_t m = i + 1; m <= j && ok; m++) {
                std::array<float, 4> vm = KeyValues(keys[m]);
                double               t = (keys[m].Frame() - fa) / (fb - fa);
                for (int c = 0; c < n; c++) {
                    if (std::fabs(va[std::size_t(c)] + (double(vb[std::size_t(c)]) - va[std::size_t(c)]) * t - vm[std::size_t(c)]) > tolerance) {
                        ok = false;
                        break;
                    }
                }
            }
            if (!ok) {
                break;
            }
            j++;
        }
        out.push_back(keys[j]);
        i = j;
    }
    return out;
}

// An 8-bit TIM2 (row-major) cut to the w x h patch at (x0, y0): the header but its sizes kept (a picture's size is
// header + 4 x image, as the files have it), the CLUT whole.
Bytes CropTim8(std::span<const unsigned char> tim, int x0, int y0, int w, int h) {
    constexpr std::size_t p = tim8::kPicture;
    std::size_t           clut = Get32(tim, p + 4), image = Get32(tim, p + 8), header = Get16(tim, p + 0xC);
    std::size_t           stride = Get16(tim, p + 0x14);
    Bytes                 out(p + header + std::size_t(w * h) + clut, 0);
    std::copy_n(tim.begin(), p + header, out.begin());
    for (int y = 0; y < h; y++) {
        std::copy_n(tim.begin() + std::ptrdiff_t(p + header + (std::size_t(y0 + y)) * stride + std::size_t(x0)), w,
                    out.begin() + std::ptrdiff_t(p + header + std::size_t(y * w)));
    }
    std::copy_n(tim.begin() + std::ptrdiff_t(p + header + image), clut, out.begin() + std::ptrdiff_t(p + header + std::size_t(w * h)));
    Set32(out, p, std::uint32_t(header + 4 * std::size_t(w * h)));
    Set32(out, p + 8, std::uint32_t(w * h));
    Set16(out, p + 0x14, std::uint16_t(w));
    Set16(out, p + 0x16, std::uint16_t(h));
    return out;
}

// An 8-bit TIM2 with each palette colour moved toward its luminance (k the saturation left).
Bytes Desaturate(std::span<const unsigned char> tim, double k) {
    constexpr std::size_t p = tim8::kPicture;
    Bytes                 out(tim.begin(), tim.end());
    std::size_t           clut = p + Get16(out, p + 0xC) + Get32(out, p + 8);
    for (std::size_t e = 0; e < 256; e++) {
        std::size_t o = clut + e * 4;
        double      lum = 0.299 * out[o] + 0.587 * out[o + 1] + 0.114 * out[o + 2];
        for (std::size_t c = 0; c < 3; c++) {
            out[o + c] = static_cast<unsigned char>(std::clamp(std::nearbyint(lum + (out[o + c] - lum) * k), 0.0, 255.0));
        }
    }
    return out;
}

void Scale(std::vector<std::array<double, 4>> &positions, const Vec3 &s) {
    for (auto &q : positions) {
        q = {q[0] * s[0], q[1] * s[1], q[2] * s[2], q[3]};
    }
}

} // namespace

Bytes BuildAtlaGemronModel(std::span<const unsigned char> gemron, std::span<const unsigned char> atla_pack,
                           std::span<const unsigned char> atla_img, std::span<const unsigned char> toan,
                           std::span<const unsigned char> light_chr, std::span<const unsigned char> fly_chr) {
    ChrPack               pack = ChrPack::Parse(gemron);
    Bytes                 mds = pack.Require(kMds).Payload();
    std::vector<MdsFrame> frames = ReadMdsSkeleton(mds);
    MotFile               mot = MotFile::FromRecord(pack.Require(kMot));
    const MotTrack        reference = mot.tracks.back();
    std::vector<Graft>    grafts;

    // The held atla, under the big gem.
    Bytes                 amds = ChrPack::Parse(atla_pack).Require(kAtlaMds).Payload();
    std::vector<MdsFrame> aframes = ReadMdsSkeleton(amds);
    int                   at = static_cast<int>(frames.size());
    Graft                 held;
    for (const MdsFrame &f : aframes) {
        held.records.push_back(f.parent < 0 ? GraftRecord(f.record, at + f.index, Frame(frames, kGem, kMds).index, rc::kGraftR, rc::kGraftT, 1.0)
                                            : GraftRecord(f.record, at + f.index, at + f.parent, std::nullopt, std::nullopt, rc::kGraftScale));
        if (f.mesh != 0) {
            held.meshes[f.index] = ScaledMesh(amds, std::size_t(f.mesh), rc::kGraftScale);
        }
    }
    grafts.push_back(std::move(held));

    // The forehead Atlamillia: Toan's lens, stretched as his record stretches it and sized to the fit, on the cropped
    // patch of his sheet.
    int                   lens_at = at + static_cast<int>(aframes.size());
    ChrPack               toan_pack = ChrPack::Parse(toan);
    Bytes                 tmds = toan_pack.Require(kToanMds).Payload();
    std::vector<MdsFrame> tframes = ReadMdsSkeleton(tmds);
    const MdsFrame       &lens = Frame(tframes, kLens, kToanMds);
    MdtMesh               lens_mesh = MdtMesh::Parse(MdtAt(tmds, std::size_t(lens.mesh)));
    Scale(lens_mesh.positions, rc::kLensStretch);
    Scale(lens_mesh.positions, {rc::kLensSize, rc::kLensSize, rc::kLensSize});
    for (auto &uv : lens_mesh.uvs) {
        uv[0] = (uv[0] * 256 - kLensCrop[0]) / kLensCrop[2];
        uv[1] = (uv[1] * 256 - kLensCrop[1]) / kLensCrop[3];
    }
    grafts.push_back({{GraftRecord(lens.record, lens_at, Frame(frames, kHead, kMds).index, rc::kLensR, rc::kLensT, 1.0)},
                      {{0, lens_mesh.Build()}}});

    // The light: its root on the lens, e503ex_atall's glow pair and their ancestors under it.
    int                   light_at = lens_at + 1;
    ChrPack               light_pack = ChrPack::Parse(light_chr);
    Bytes                 emds = light_pack.Require(kLightMds).Payload();
    std::vector<MdsFrame> eframes = ReadMdsSkeleton(emds);
    MotFile               emot = MotFile::FromRecord(light_pack.Require(kLightMot));
    std::map<int, int>    placed; // the light's frame -> the Gemron's
    for (std::size_t k = 0; k < std::size(rc::kLightNodes); k++) {
        placed[rc::kLightNodes[k]] = light_at + 1 + int(k);
    }
    Graft light;
    light.records.push_back(PlainRecord(light_at, lens_at, kLightName, rc::kLightR, rc::kLightRecordT));
    for (std::size_t k = 0; k < std::size(rc::kLightNodes); k++) {
        const MdsFrame &f = eframes.at(std::size_t(rc::kLightNodes[k]));
        auto            parent = placed.find(f.parent);
        light.records.push_back(GraftRecord(f.record, placed[f.index], parent != placed.end() ? parent->second : light_at, std::nullopt,
                                            std::nullopt, 1.0));
        if (f.mesh != 0) {
            light.meshes[1 + int(k)] = MdtAt(emds, std::size_t(f.mesh));
        }
    }
    int light_count = static_cast<int>(light.records.size());
    grafts.push_back(std::move(light));

    // The wing lights: fly_light's glow pair on each wing gem, scaled to the chosen frame.
    int                   wing_at = light_at + light_count;
    ChrPack               fly_pack = ChrPack::Parse(fly_chr);
    Bytes                 fmds = fly_pack.Require(kFlyMds).Payload();
    std::vector<MdsFrame> fframes = ReadMdsSkeleton(fmds);
    Graft                 wing;
    for (const char *gem : kWingGems) {
        for (std::size_t n = 0; n < std::size(kFlyNodes); n++) {
            const MdsFrame &f = Frame(fframes, kFlyNodes[n], kFlyMds);
            MdtMesh         m = MdtMesh::Parse(MdtAt(fmds, std::size_t(f.mesh)));
            Scale(m.positions, rc::kFlyScale[n]);
            int place = static_cast<int>(wing.records.size());
            wing.meshes[place] = m.Build();
            wing.records.push_back(GraftRecord(f.record, wing_at + place, Frame(frames, gem, kMds).index, kIdentity, Vec3{0, 0, 0}, 1.0));
        }
    }
    int spawn_at = wing_at + static_cast<int>(wing.records.size());
    grafts.push_back(std::move(wing));

    // The shot's spawn bone, under the root.
    grafts.push_back({{PlainRecord(spawn_at, 0, kAtlaShotSpawn, kIdentity, rc::kSpawnT)}, {}});
    int total = spawn_at + 1;
    pack.Require(kMds).ReplacePayload(GraftMds(mds, grafts, {}));
    Bytes bbp = pack.Require(kBbp).Payload();
    bbp.resize(bbp.size() + 64 * (std::size_t(total) - frames.size()), 0); // not skinned: zero
    pack.Require(kBbp).ReplacePayload(bbp);

    // The death copied and held; the held atla out of the draw from the swap; the light on the Gemron's frames.
    DeathClip(mot, kAtlaGemronDeathStart, kClipFrom, kClipTo, kAtlaGemronDeathEnd);
    std::vector<MotTrack> added;
    MotTrack              shown = TrackLike(reference, std::uint32_t(at), 0, kSubtreeVisible);
    for (auto [f, v] : {
             std::pair{0,                   1.0},
             std::pair{kAtlaGemronSwap - 1, 1.0},
             std::pair{kAtlaGemronSwap,     0.0},
             std::pair{kTimelineEnd,        0.0}
    }) {
        shown.keys.push_back(MakeKey(std::uint32_t(f), {v, 0, 0, 0}));
    }
    added.push_back(std::move(shown));
    for (const MotTrack &t : emot.tracks) {
        auto node = placed.find(int(t.w0));
        if (node == placed.end() || Camera(t.w2) || t.keys.empty()) {
            continue;
        }
        std::vector<MotKeyframe> keys;
        for (auto [frame, source] : rc::kLightFrames) {
            keys.push_back(MakeKey(std::uint32_t(frame), LightAt(t, source)));
        }
        MotTrack track = TrackLike(reference, std::uint32_t(node->second), t.w1, t.w2);
        track.keys = Thinned(keys, t.w2);
        added.push_back(std::move(track));
    }
    const double s = rc::kLightScale;
    const Vec3  &l = rc::kLightT;
    MotTrack     scale = TrackLike(reference, std::uint32_t(light_at), 0, 1);
    scale.keys = {MakeKey(0, {s, s, s, 0}), MakeKey(kTimelineEnd, {s, s, s, 0})};
    MotTrack place = TrackLike(reference, std::uint32_t(light_at), 0, 2);
    place.keys = {MakeKey(0, {l[0], l[1], l[2], 0}), MakeKey(kTimelineEnd, {l[0], l[1], l[2], 0})};
    added.push_back(std::move(scale));
    added.push_back(std::move(place));
    mot.tracks.insert(mot.tracks.end(), added.begin(), added.end());
    pack.Require(kMot).ReplacePayload(mot.BuildPayload());
    MotFile shadow = MotFile::FromRecord(pack.Require(kShadowMot));
    DeathClip(shadow, kAtlaGemronDeathStart, kClipFrom, kClipTo, kAtlaGemronDeathEnd);
    pack.Require(kShadowMot).ReplacePayload(shadow.BuildPayload());
    DeathCfg(pack, kAtlaGemronDeathStart, kAtlaGemronDeathEnd, nullptr);

    // The sheets in the look; the atla's, the lens's, the light's (IMG banks row-major, swizzled) and the wing lights'
    // pictures.
    ImgBank   bank = ImgBank::Parse(pack.Require(kImg).Payload());
    SheetLook look = SheetLook::Parse(kAtlaGemronLook);
    for (auto &[name, block] : bank.entries) {
        if (name == kSheet) {
            block = look.ApplyToSheet(block);
        } else if (name == kSheet2) {
            block = look.ApplyBaseToSheet(block);
        }
    }
    for (const auto &[name, block] : ImgBank::Parse(atla_img).entries) {
        bank.entries.emplace_back(name, tim8::Swizzled(block));
    }
    const ChrRecord *toan_img = nullptr;
    for (const ChrRecord &r : toan_pack.records) {
        if (toan_img == nullptr && r.name.ends_with(".img")) {
            toan_img = &r;
        }
    }
    if (toan_img == nullptr) {
        throw std::runtime_error("c01d.chr has no texture bank");
    }
    Bytes lens_sheet = ImgBank::Parse(toan_img->Payload()).Block(kLensTexture);
    bank.entries.emplace_back(kLensTexture, tim8::Swizzled(CropTim8(lens_sheet, kLensCrop[0], kLensCrop[1], kLensCrop[2], kLensCrop[3])));
    ImgBank light_bank = ImgBank::Parse(light_pack.Require(std::string(kLightTexture) + ".img").Payload());
    bool    blocked = light_bank.magic[2] == '2'; // an IM2 bank's pictures are in block order already
    bank.entries.emplace_back(kLightTexture, blocked ? light_bank.Block(kLightTexture) : tim8::Swizzled(light_bank.Block(kLightTexture)));
    bank.entries.emplace_back(kFlyTexture, Desaturate(ImgBank::Parse(fly_pack.Require(kFlyImg).Payload()).Block(kFlyTexture), kFlySaturation));
    pack.Require(kImg).ReplacePayload(bank.Build());
    return pack.Rebuild();
}

Bytes BuildAtlaShotPack(std::span<const unsigned char> light_chr) {
    ChrPack               light_pack = ChrPack::Parse(light_chr);
    Bytes                 emds = light_pack.Require(kLightMds).Payload();
    std::vector<MdsFrame> eframes = ReadMdsSkeleton(emds);
    MotFile               emot = MotFile::FromRecord(light_pack.Require(kLightMot));
    const MotTrack        reference = emot.tracks.back();
    std::set<int>         drawn(std::begin(rc::kShotMeshes), std::end(rc::kShotMeshes));
    std::map<int, int>    placed;
    for (std::size_t k = 0; k < std::size(rc::kShotNodes); k++) {
        placed[rc::kShotNodes[k]] = 2 + int(k);
    }
    std::vector<Bytes>   records = {PlainRecord(0, -1, kAtlaShotModel, kIdentity, {0, 0, 0}), PlainRecord(1, 0, "fx_shot", rc::kShotRows, {0, 0, 0})};
    std::map<int, Bytes> meshes;
    for (int i : rc::kShotNodes) {
        const MdsFrame &f = eframes.at(std::size_t(i));
        auto            parent = placed.find(f.parent);
        records.push_back(GraftRecord(f.record, placed[i], parent != placed.end() ? parent->second : 1, std::nullopt, std::nullopt, 1.0));
        if (drawn.contains(i)) {
            meshes[placed[i]] = MdtAt(emds, std::size_t(f.mesh));
        }
    }
    Bytes mds(emds.begin(), emds.begin() + 8);
    Bytes words(8);
    Set32(words, 0, std::uint32_t(records.size()));
    Set32(words, 4, 0x10);
    mds.insert(mds.end(), words.begin(), words.end());
    std::size_t base = 0x10 + records.size() * 0x70;
    Bytes       body;
    for (std::size_t k = 0; k < records.size(); k++) {
        Bytes raw = records[k];
        auto  mesh = meshes.find(int(k));
        Set32(raw, 0x28, mesh != meshes.end() ? std::uint32_t(base + body.size()) : 0u);
        if (mesh != meshes.end()) {
            body.insert(body.end(), mesh->second.begin(), mesh->second.end());
            body.resize((body.size() + 15) & ~std::size_t{15}, 0);
        }
        mds.insert(mds.end(), raw.begin(), raw.end());
    }
    mds.insert(mds.end(), body.begin(), body.end());

    // e503ex_atall's key 9 tail.
    MotFile shot;
    for (const MotTrack &t : emot.tracks) {
        auto node = placed.find(int(t.w0));
        if (node == placed.end() || Camera(t.w2) || t.keys.empty()) {
            continue;
        }
        std::vector<MotKeyframe> keys = {MakeKey(kAtlaShotFrom, LightAt(t, kAtlaShotFrom))};
        for (const MotKeyframe &k : t.keys) {
            if (k.Frame() > std::uint32_t(kAtlaShotFrom) && k.Frame() < std::uint32_t(kAtlaShotTo)) {
                keys.push_back(MakeKey(k.Frame(), Wide(KeyValues(k))));
            }
        }
        keys.push_back(MakeKey(kAtlaShotTo, LightAt(t, kAtlaShotTo)));
        MotTrack track = TrackLike(reference, std::uint32_t(node->second), t.w1, t.w2);
        track.keys = Thinned(keys, t.w2);
        shot.tracks.push_back(std::move(track));
    }
    // Each drawn mesh out of the draw while it is clear (a drawn mesh takes room in the frame's packet, seen or not): a
    // visibility key holds to the next, so a frame shows the mesh if it or the next one does.
    std::map<int, const MotTrack *> alpha;
    for (const MotTrack &t : emot.tracks) {
        if (t.w2 == kMaterialAlpha) {
            alpha[int(t.w0)] = &t;
        }
    }
    for (int i : rc::kShotNodes) {
        auto a = alpha.find(i);
        if (!drawn.contains(i) || a == alpha.end()) {
            continue;
        }
        std::vector<bool> seen;
        for (int f = kAtlaShotFrom; f <= kAtlaShotTo; f++) {
            seen.push_back(TrackAt(*a->second, f)[0] < 0.98 || TrackAt(*a->second, f + 1)[0] < 0.98);
        }
        MotTrack track = TrackLike(reference, std::uint32_t(placed[i]), 0, kSubtreeVisible);
        for (std::size_t k = 0; k < seen.size(); k++) {
            if (k == 0 || seen[k] != seen[k - 1]) {
                track.keys.push_back(MakeKey(std::uint32_t(kAtlaShotFrom + int(k)), {seen[k] ? 1.0 : 0.0, 0, 0, 0}));
            }
        }
        if (track.keys.back().Frame() != std::uint32_t(kAtlaShotTo)) {
            track.keys.push_back(MakeKey(kAtlaShotTo, {seen.back() ? 1.0 : 0.0, 0, 0, 0}));
        }
        shot.tracks.push_back(std::move(track));
    }
    // The flight over the window backed off, so the burst stands where it was fired while the shot flies.
    double   back = -rc::kShotFlight * (kAtlaShotTo - kAtlaShotFrom) / kAtlaShotSpeed;
    MotTrack flight = TrackLike(reference, 1, 0, 2);
    flight.keys = {MakeKey(kAtlaShotFrom, {0, 0, 0, 0}), MakeKey(kAtlaShotTo, {0, 0, back, 0})};
    shot.tracks.push_back(std::move(flight));

    std::string cfg = "//atla shot\r\nIMG 0,\"e503ex_at01.img\"\r\nIMG 1,\"e503ex_at03.img\"\r\nIMG_END\r\nMATERIAL_ANIME 1\r\nVERTEX_ANIME 1\r\n\r\n";
    cfg += std::format("MODEL \"{}.mds\"\r\nBODY_SIZE 17,7,60\r\n\r\nMOTION 0, \"{}.mot\", \"\", \"\"\r\nSHADOW_MOTION \"\", \"\", \"\"\r\n"
                       "KEY_START 0\r\nKEY\t{},\t{},\t0.35,\t// 0\r\nMOTION_END\r\n",
                       kAtlaShotModel, kAtlaShotModel, kAtlaShotFrom, kAtlaShotTo);
    ChrPack out;
    out.trailer = light_pack.trailer;
    out.records.push_back(ChrRecord::Make(std::string(kAtlaShotModel) + ".cfg", Bytes(cfg.begin(), cfg.end())));
    out.records.push_back(ChrRecord::Make(std::string(kAtlaShotModel) + ".mds", mds));
    out.records.push_back(ChrRecord::Make(std::string(kAtlaShotModel) + ".mot", shot.BuildPayload()));
    out.records.push_back(light_pack.Require("e503ex_at01.img"));
    out.records.push_back(light_pack.Require("e503ex_at03.img"));
    return out.Rebuild();
}

namespace {

constexpr std::uint32_t kOpRefPush = 2, kOpPushConst = 3, kOpJmp = 16, kOpBrFalse = 17, kOpCall = 19, kOpCallFn = 21,
                        kOpMarker = 22, kOpRet = 15, kOpLess = 14, kOpDrop = 4;
constexpr std::uint32_t kInt = 1, kFloat = 2, kLocalFloat = 8;
constexpr float         kVanillaRange = 70.0f, kShotRange = 50.0f;     // label 100 shoots within it, walks closer beyond
constexpr std::uint32_t kVanillaCooldownFrom = 80, kCooldownFrom = 40; // the shot's cooldown: _GET_RAND(70) + this
constexpr float         kCloseRange = 20.0f;                           // under it: back off or shoot
constexpr std::uint32_t kVanillaBackOff = 40, kBackOffPercent = 60;    // _GET_RAND(100) under it backs off
constexpr float         kAimAhead = 100.0f;                            // the shot's aim, this far along the facing
constexpr float         kAimUp = 7.4f;
// The shot function's locals: the distance, the aim and the facing.
constexpr std::uint32_t kAimDistance = 8, kAimX = 2, kAimY = 3, kAimZ = 4, kAimDirX = 8, kAimDirY = 9, kAimDirZ = 10;
constexpr const char   *kSpawnFrom = "dcol0";

std::uint32_t Bits(float f) {
    return std::bit_cast<std::uint32_t>(f);
}

void Cell(Bytes &b, std::size_t at, std::uint32_t op, std::uint32_t a1, std::uint32_t a2) {
    Set32(b, at, op);
    Set32(b, at + 4, a1);
    Set32(b, at + 8, a2);
}

// The shot's aim, appended past the script's end: v2..v4 its own place (_GET_POSITION(-1)), v8..v10 its facing
// (_GET_DIRECTION(0)), v2 += facing.x x the reach, v4 += facing.z x the reach, v3 += 7.4, then back to the _SET_SHOT.
Bytes AimCells(std::uint32_t set_shot) {
    std::vector<std::array<std::uint32_t, 3>> cells = {
        {3,          1,        11         },
        {3,          1,        1          },
        {11,         0,        0          },
        {kOpRefPush, kAimX,    kLocalFloat},
        {kOpRefPush, kAimY,    kLocalFloat},
        {kOpRefPush, kAimZ,    kLocalFloat},
        {kOpCallFn,  5,        0          },
        {3,          1,        31         },
        {3,          2,        0          },
        {kOpRefPush, kAimDirX, kLocalFloat},
        {kOpRefPush, kAimDirY, kLocalFloat},
        {kOpRefPush, kAimDirZ, kLocalFloat},
        {kOpCallFn,  5,        0          },
    };
    for (auto [v, d] : {
             std::pair{kAimX, kAimDirX},
             std::pair{kAimZ, kAimDirZ}
    }) {
        cells.insert(cells.end(), {
                                      {2, v, kLocalFloat    },
                                      {1, v, kLocalFloat    },
                                      {3, 2, Bits(kAimAhead)},
                                      {1, d, kLocalFloat    },
                                      {8, 0, 0              },
                                      {6, 0, 0              },
                                      {5, 0, 0              },
                                      {4, 0, 0              }
        });
    }
    cells.insert(cells.end(), {
                                  {2, kAimY, kLocalFloat },
                                  {1, kAimY, kLocalFloat },
                                  {3, 2,     Bits(kAimUp)},
                                  {6, 0,     0           },
                                  {5, 0,     0           },
                                  {4, 0,     0           }
    });
    cells.push_back({kOpJmp, set_shot, 0});
    Bytes b(cells.size() * 12);
    for (std::size_t i = 0; i < cells.size(); i++) {
        Cell(b, i * 12, cells[i][0], cells[i][1], cells[i][2]);
    }
    return b;
}

} // namespace

// Holy Gemron's script for a short-range wide shot: its one _SET_SHOT fires from the spawn bone, along its facing (the
// fire frame's hold-fire check becomes a jump to aim cells appended past the end, which jump back); its AI loop shoots
// within 50 (walking closer beyond), every 40-109 frames, and close in backs off 60 % of the time, else shoots (the bite
// before).
Bytes BuildAtlaGemronScript(std::span<const unsigned char> stb) {
    auto        u = [&](std::size_t o) { return Get32(stb, o); };
    auto        f = [&](std::size_t o) { return GetF32(stb, o); };
    auto        op = [&](std::size_t c, std::uint32_t v) { return u(c) == v; };
    auto        push_i = [&](std::size_t c, std::uint32_t v) { return u(c) == kOpPushConst && u(c + 4) == kInt && u(c + 8) == v; };
    auto        push_f = [&](std::size_t c) { return u(c) == kOpPushConst && u(c + 4) == kFloat; };
    auto        bad = [](const std::string &what) { return std::runtime_error("e115a.stb: " + what); };
    std::size_t cb = u(8), table = u(0xC), count = u(0x10);
    std::size_t label100 = 0;
    bool        found = false;
    for (std::size_t i = 0; i < count; i++) {
        if (u(table + i * 8) == 100) {
            label100 = u(table + i * 8 + 4);
            found = true;
        }
    }
    if (!found) {
        throw bad("no label 100");
    }
    Bytes out(stb.begin(), stb.end());
    // The shot's bone: push 133, then the bone's string.
    std::vector<std::size_t> sites;
    for (std::size_t c = cb; c + 24 <= stb.size(); c += 4) {
        if (push_i(c, 133) && u(c + 12) == 3 && u(c + 16) == 3) {
            sites.push_back(c);
        }
    }
    if (sites.size() != 1) {
        throw bad(std::format("{} _SET_SHOT bone strings, expected 1", sites.size()));
    }
    std::size_t bone_at = cb + u(sites[0] + 20);
    if (NameAt(stb, bone_at, 8) != kSpawnFrom) {
        throw bad(std::format("the shot fires from '{}', not {}", NameAt(stb, bone_at, 8), kSpawnFrom));
    }
    std::copy_n(kAtlaShotSpawn, std::strlen(kAtlaShotSpawn), out.begin() + std::ptrdiff_t(bone_at));
    // The AI loop, up to its RET.
    std::size_t start = cb + u(label100), end = start;
    while (end + 12 <= stb.size() && u(end) != kOpRet) {
        end += 12;
    }
    std::vector<std::size_t> ranges;
    long                     cooldown = -1, close = -1, back_off = -1, shot_call = -1;
    for (std::size_t c = start; c < end; c += 12) {
        if (push_f(c) && f(c + 8) == kVanillaRange && op(c + 12, kOpLess)) {
            ranges.push_back(c);
        }
        if (push_i(c, 180) && push_i(c + 12, 70)) { // _GET_RAND(70, ...) then + 80
            for (std::size_t k = c + 24; k < c + 108 && cooldown < 0; k += 12) {
                if (push_i(k, kVanillaCooldownFrom)) {
                    cooldown = long(k);
                }
            }
        }
        if (push_f(c) && f(c + 8) == kCloseRange && op(c + 12, kOpLess) && u(c + 16) == 42 && close < 0) {
            close = long(c);
        }
        if (close >= 0 && back_off < 0 && long(c) > close && push_i(c, 180) && push_i(c + 12, 100)) { // _GET_RAND(100, ...) < 40
            for (std::size_t k = c + 24; k < c + 108 && back_off < 0; k += 12) {
                if (push_i(k, kVanillaBackOff)) {
                    back_off = long(k);
                }
            }
        }
        if (shot_call < 0 && c >= 24 && op(c, kOpCall) && push_f(c - 24) && push_f(c - 12) && op(c + 12, kOpDrop)) {
            shot_call = long(c);
        }
    }
    if (ranges.size() != 3 || cooldown < 0 || close < 0 || back_off < 0 || shot_call < 0) {
        throw bad(std::format("label 100 not as expected ({} ranges, cooldown {:X}, close range {:X}, back-off {:X}, shot call {:X})",
                              ranges.size(), cooldown, close, back_off, shot_call));
    }
    // The close branch: cmp, BR_FALSE to other, CALL back-off, drop, JMP to out; other: 4 cells; out: the closing marker.
    std::size_t bo = std::size_t(back_off), br = bo + 24, other = bo + 72, jmp = bo + 60, outer = other + 48;
    if (!op(br, kOpBrFalse) || u(br + 4) != other - cb || !op(bo + 36, kOpCall) || !op(jmp, kOpJmp) || u(jmp + 4) != other + 36 - cb ||
        !op(outer, kOpMarker) || !op(other, kOpMarker) || !op(other + 12, kOpCall) || !op(other + 24, kOpDrop) || !op(other + 36, kOpMarker)) {
        throw bad(std::format("label 100's close branch at {:X} not as expected", close));
    }
    // The shot function's fire frame: _GET_DISTANCE(&v8), v8 < 20 holds fire, else v3 += 7.4, then the _SET_SHOT.
    std::size_t sc = std::size_t(shot_call);
    std::size_t shot_func = cb + u(sc + 8), shot_code = cb + u(shot_func);
    long        hold = -1;
    for (std::size_t c = shot_code; c < sites[0]; c += 12) {
        if (push_i(c, 10) && u(c + 12) == kOpRefPush && u(c + 16) == kAimDistance && op(c + 24, kOpCallFn) && push_f(c + 48) &&
            f(c + 56) == 20.0f && op(c + 60, kOpLess)) {
            hold = long(c);
        }
    }
    if (hold < 0 || long(sites[0]) - hold != 12 * 15) {
        throw bad("the shot function's hold-fire check not as expected");
    }
    for (std::size_t c : ranges) {
        Set32(out, c + 8, Bits(kShotRange));
    }
    Set32(out, std::size_t(cooldown) + 8, kCooldownFrom);
    Set32(out, bo + 8, kBackOffPercent);
    std::copy_n(stb.begin() + std::ptrdiff_t(sc - 24), 36, out.begin() + std::ptrdiff_t(other));      // push 0.3, push 0.0, CALL the shot
    std::copy_n(stb.begin() + std::ptrdiff_t(sc + 12), 12, out.begin() + std::ptrdiff_t(other + 36)); // its drop
    Set32(out, jmp + 4, std::uint32_t(outer - cb));
    std::size_t aim = (out.size() + 3) & ~std::size_t{3};
    Bytes       tail = AimCells(std::uint32_t(sites[0] - cb));
    out.resize(aim + tail.size(), 0);
    std::copy(tail.begin(), tail.end(), out.begin() + std::ptrdiff_t(aim));
    Cell(out, std::size_t(hold), kOpJmp, std::uint32_t(aim - cb), 0); // to the aim
    return out;
}

} // namespace expanded
