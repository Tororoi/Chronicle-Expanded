#include <algorithm>
#include <cmath>
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

constexpr const GemronPack &kGemron = kIceGemronPack;

// The held atla: the dungeon's own, under the big gem.
constexpr const char *kAtlaMds = "atr.mds";
constexpr const char *kGem = kGemronBigGem;
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
// The wing lights: fly_light's glow pair on each wing (small) gem, its picture half desaturated.
constexpr const char *kFlyMds = "fly_light.mds";
constexpr const char *kFlyImg = "fly_light.img";
constexpr const char *kFlyTexture = "fly_light";
constexpr const char *kFlyNodes[] = {"012__CZAPPBA", "013__CZAPPBA"};
constexpr double      kFlySaturation = 0.5;

constexpr int           kClipFrom = 105, kClipTo = 125;
constexpr int           kTimelineEnd = kAtlaGemronDeathEnd + 1;
constexpr std::uint32_t kCameraChannels[] = {mot::kCameraPosition, mot::kCameraReference, mot::kCameraRoll, mot::kCameraFov};

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
    return Wide(key->Values());
}

std::array<double, 4> LightAt(const MotTrack &track, double frame) {
    return track.w2 == mot::kVisible || track.w2 == mot::kSubtreeVisible ? StepAt(track, frame) : TrackAt(track, frame);
}

// The keys a track keeps: from the first, each next one as far on as the straight blend to it reproduces every key
// between within the channel's tolerance (x, y and z, or the alpha); other channels keep them all.
std::vector<MotKeyframe> Thinned(const std::vector<MotKeyframe> &keys, std::uint32_t channel) {
    double tolerance;
    switch (channel) {
        case mot::kScale:
            tolerance = 0.02;
            break;
        case mot::kTranslation:
            tolerance = 0.05;
            break;
        case mot::kMaterialAlpha:
            tolerance = 0.03;
            break;
        default:
            return keys;
    }
    int                      n = channel == mot::kMaterialAlpha ? 1 : 3;
    std::vector<MotKeyframe> out = {keys[0]};
    std::size_t              i = 0;
    while (i + 1 < keys.size()) {
        std::size_t j = i + 1;
        while (j + 1 < keys.size()) {
            std::array<float, 4> va = keys[i].Values(), vb = keys[j + 1].Values();
            double               fa = keys[i].Frame(), fb = keys[j + 1].Frame();
            bool                 ok = true;
            for (std::size_t m = i + 1; m <= j && ok; m++) {
                std::array<float, 4> vm = keys[m].Values();
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
    std::size_t           clut = Get32(tim, p + tim8::kClutSize), image = Get32(tim, p + tim8::kImageSize), header = Get16(tim, p + tim8::kHeaderSize);
    std::size_t           stride = Get16(tim, p + tim8::kWidth);
    Bytes                 out(p + header + std::size_t(w * h) + clut, 0);
    std::copy_n(tim.begin(), p + header, out.begin());
    for (int y = 0; y < h; y++) {
        std::copy_n(tim.begin() + std::ptrdiff_t(p + header + (std::size_t(y0 + y)) * stride + std::size_t(x0)), w,
                    out.begin() + std::ptrdiff_t(p + header + std::size_t(y * w)));
    }
    std::copy_n(tim.begin() + std::ptrdiff_t(p + header + image), clut, out.begin() + std::ptrdiff_t(p + header + std::size_t(w * h)));
    Set32(out, p + tim8::kTotalSize, std::uint32_t(header + 4 * std::size_t(w * h)));
    Set32(out, p + tim8::kImageSize, std::uint32_t(w * h));
    Set16(out, p + tim8::kWidth, std::uint16_t(w));
    Set16(out, p + tim8::kHeight, std::uint16_t(h));
    return out;
}

// An 8-bit TIM2 with each palette colour moved toward its luminance (k the saturation left).
Bytes Desaturate(std::span<const unsigned char> tim, double k) {
    constexpr std::size_t p = tim8::kPicture;
    Bytes                 out(tim.begin(), tim.end());
    std::size_t           clut = p + Get16(out, p + tim8::kHeaderSize) + Get32(out, p + tim8::kImageSize);
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
    Bytes                 mds = pack.Require(kGemron.mds).Payload();
    std::vector<MdsFrame> frames = ReadMdsSkeleton(mds);
    MotFile               mot = MotFile::FromRecord(pack.Require(kGemron.mot));
    const MotTrack        reference = mot.tracks.back();
    std::vector<Graft>    grafts;

    // The held atla, under the big gem.
    Bytes                 amds = ChrPack::Parse(atla_pack).Require(kAtlaMds).Payload();
    std::vector<MdsFrame> aframes = ReadMdsSkeleton(amds);
    int                   at = static_cast<int>(frames.size());
    Graft                 held;
    for (const MdsFrame &f : aframes) {
        held.records.push_back(f.parent < 0 ? GraftRecord(f.record, at + f.index, RequireFrame(frames, kGem, kGemron.mds).index, rc::kGraftR, rc::kGraftT, 1.0)
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
    const MdsFrame       &lens = RequireFrame(tframes, kLens, kToanMds);
    MdtMesh               lens_mesh = MdtMesh::Parse(MdtAt(tmds, std::size_t(lens.mesh)));
    Scale(lens_mesh.positions, rc::kLensStretch);
    Scale(lens_mesh.positions, {rc::kLensSize, rc::kLensSize, rc::kLensSize});
    for (auto &uv : lens_mesh.uvs) {
        uv[0] = (uv[0] * 256 - kLensCrop[0]) / kLensCrop[2];
        uv[1] = (uv[1] * 256 - kLensCrop[1]) / kLensCrop[3];
    }
    grafts.push_back({{GraftRecord(lens.record, lens_at, RequireFrame(frames, kHead, kGemron.mds).index, rc::kLensR, rc::kLensT, 1.0)},
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
    light.records.push_back(FrameRecord(light_at, kLightName, lens_at, rc::kLightR, rc::kLightRecordT));
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
    for (const char *gem : kGemronSmallGems) {
        for (std::size_t n = 0; n < std::size(kFlyNodes); n++) {
            const MdsFrame &f = RequireFrame(fframes, kFlyNodes[n], kFlyMds);
            MdtMesh         m = MdtMesh::Parse(MdtAt(fmds, std::size_t(f.mesh)));
            Scale(m.positions, rc::kFlyScale[n]);
            int place = static_cast<int>(wing.records.size());
            wing.meshes[place] = m.Build();
            wing.records.push_back(GraftRecord(f.record, wing_at + place, RequireFrame(frames, gem, kGemron.mds).index, kIdentity, Vec3{0, 0, 0}, 1.0));
        }
    }
    int spawn_at = wing_at + static_cast<int>(wing.records.size());
    grafts.push_back(std::move(wing));

    // The shot's spawn bone, under the root.
    grafts.push_back({{FrameRecord(spawn_at, kAtlaShotSpawn, 0, kIdentity, rc::kSpawnT)}, {}});
    int total = spawn_at + 1;
    pack.Require(kGemron.mds).ReplacePayload(GraftMds(mds, grafts, {}));
    AppendBindPoses(pack, kGemron.bbp, std::size_t(total) - frames.size());

    // The death copied and held; the held atla out of the draw from the swap; the light on the Gemron's frames.
    DeathClip(mot, kAtlaGemronDeathStart, kClipFrom, kClipTo, kAtlaGemronDeathEnd);
    std::vector<MotTrack> added;
    MotTrack              shown = TrackLike(reference, std::uint32_t(at), 0, mot::kSubtreeVisible);
    for (auto [f, v] : {
             std::pair{0,                   1.0},
             std::pair{kAtlaGemronSwap - 1, 1.0},
             std::pair{kAtlaGemronSwap,     0.0},
             std::pair{kTimelineEnd,        0.0}
    }) {
        shown.keys.push_back(MotKeyframe::Make(std::uint32_t(f), {v, 0, 0, 0}));
    }
    added.push_back(std::move(shown));
    for (const MotTrack &t : emot.tracks) {
        auto node = placed.find(int(t.w0));
        if (node == placed.end() || Camera(t.w2) || t.keys.empty()) {
            continue;
        }
        std::vector<MotKeyframe> keys;
        for (auto [frame, source] : rc::kLightFrames) {
            keys.push_back(MotKeyframe::Make(std::uint32_t(frame), LightAt(t, source)));
        }
        MotTrack track = TrackLike(reference, std::uint32_t(node->second), t.w1, t.w2);
        track.keys = Thinned(keys, t.w2);
        added.push_back(std::move(track));
    }
    const double s = rc::kLightScale;
    const Vec3  &l = rc::kLightT;
    MotTrack     scale = TrackLike(reference, std::uint32_t(light_at), 0, mot::kScale);
    scale.keys = {MotKeyframe::Make(0, {s, s, s, 0}), MotKeyframe::Make(kTimelineEnd, {s, s, s, 0})};
    MotTrack place = TrackLike(reference, std::uint32_t(light_at), 0, mot::kTranslation);
    place.keys = {MotKeyframe::Make(0, {l[0], l[1], l[2], 0}), MotKeyframe::Make(kTimelineEnd, {l[0], l[1], l[2], 0})};
    added.push_back(std::move(scale));
    added.push_back(std::move(place));
    mot.tracks.insert(mot.tracks.end(), added.begin(), added.end());
    pack.Require(kGemron.mot).ReplacePayload(mot.BuildPayload());
    DeathClip(pack.Require(kGemron.shadow_mot), kAtlaGemronDeathStart, kClipFrom, kClipTo, kAtlaGemronDeathEnd);
    DeathCfg(pack, kAtlaGemronDeathStart, kAtlaGemronDeathEnd, nullptr);

    // The sheets in the look; the atla's, the lens's, the light's (IMG banks row-major, swizzled) and the wing lights'
    // pictures.
    ImgBank   bank = ImgBank::Parse(pack.Require(kGemron.img).Payload());
    SheetLook look = SheetLook::Parse(kAtlaGemronLook);
    ApplyLook(bank, look, kGemron);
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
    pack.Require(kGemron.img).ReplacePayload(bank.Build());
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
    std::vector<Bytes>   records = {FrameRecord(0, kAtlaShotModel, -1, kIdentity, {0, 0, 0}), FrameRecord(1, "fx_shot", 0, rc::kShotRows, {0, 0, 0})};
    std::map<int, Bytes> meshes;
    for (int i : rc::kShotNodes) {
        const MdsFrame &f = eframes.at(std::size_t(i));
        auto            parent = placed.find(f.parent);
        records.push_back(GraftRecord(f.record, placed[i], parent != placed.end() ? parent->second : 1, std::nullopt, std::nullopt, 1.0));
        if (drawn.contains(i)) {
            meshes[placed[i]] = MdtAt(emds, std::size_t(f.mesh));
        }
    }
    std::vector<std::optional<Bytes>> laid(records.size());
    for (auto &[k, mesh] : meshes) {
        laid[std::size_t(k)] = mesh;
    }
    Bytes mds = LayOutMds(emds, std::move(records), laid);

    // e503ex_atall's key 9 tail.
    MotFile shot;
    for (const MotTrack &t : emot.tracks) {
        auto node = placed.find(int(t.w0));
        if (node == placed.end() || Camera(t.w2) || t.keys.empty()) {
            continue;
        }
        std::vector<MotKeyframe> keys = {MotKeyframe::Make(kAtlaShotFrom, LightAt(t, kAtlaShotFrom))};
        for (const MotKeyframe &k : t.keys) {
            if (k.Frame() > std::uint32_t(kAtlaShotFrom) && k.Frame() < std::uint32_t(kAtlaShotTo)) {
                keys.push_back(MotKeyframe::Make(k.Frame(), Wide(k.Values())));
            }
        }
        keys.push_back(MotKeyframe::Make(kAtlaShotTo, LightAt(t, kAtlaShotTo)));
        MotTrack track = TrackLike(reference, std::uint32_t(node->second), t.w1, t.w2);
        track.keys = Thinned(keys, t.w2);
        shot.tracks.push_back(std::move(track));
    }
    // Each drawn mesh out of the draw while it is clear (a drawn mesh takes room in the frame's packet, seen or not): a
    // visibility key holds to the next, so a frame shows the mesh if it or the next one does.
    std::map<int, const MotTrack *> alpha;
    for (const MotTrack &t : emot.tracks) {
        if (t.w2 == mot::kMaterialAlpha) {
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
        MotTrack track = TrackLike(reference, std::uint32_t(placed[i]), 0, mot::kSubtreeVisible);
        for (std::size_t k = 0; k < seen.size(); k++) {
            if (k == 0 || seen[k] != seen[k - 1]) {
                track.keys.push_back(MotKeyframe::Make(std::uint32_t(kAtlaShotFrom + int(k)), {seen[k] ? 1.0 : 0.0, 0, 0, 0}));
            }
        }
        if (track.keys.back().Frame() != std::uint32_t(kAtlaShotTo)) {
            track.keys.push_back(MotKeyframe::Make(kAtlaShotTo, {seen.back() ? 1.0 : 0.0, 0, 0, 0}));
        }
        shot.tracks.push_back(std::move(track));
    }
    // The flight over the window backed off, so the burst stands where it was fired while the shot flies.
    double   back = -rc::kShotFlight * (kAtlaShotTo - kAtlaShotFrom) / kAtlaShotSpeed;
    MotTrack flight = TrackLike(reference, 1, 0, mot::kTranslation);
    flight.keys = {MotKeyframe::Make(kAtlaShotFrom, {0, 0, 0, 0}), MotKeyframe::Make(kAtlaShotTo, {0, 0, back, 0})};
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

} // namespace expanded
