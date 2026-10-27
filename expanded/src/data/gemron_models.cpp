#include "expanded/data/gemron_models.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <fstream>
#include <functional>
#include <map>
#include <numbers>
#include <stdexcept>
#include <string>

#include "expanded/data/cfg_text.hpp"
#include "expanded/data/data_build.hpp"
#include "expanded/data/gemron_parts.hpp"
#include "expanded/data/model_parts.hpp"
#include "expanded/data/sheet_look.hpp"

namespace expanded {

namespace {

namespace fs = std::filesystem;

constexpr double kDegrees = std::numbers::pi / 180.0;

// The bomb's body radius in its own model (the wick rises to 1.98), and its turns about the wick before it is aimed.
constexpr double kBombRadius = 1.22;
constexpr double kBodySpin = (90 - 200 - 45 - 50) * kDegrees;
constexpr double kWingSpin = 180 * kDegrees;
// The big bomb turned after it is aimed (chosen on the preview page): pitch about the Gemron's left, roll about its
// forward, yaw about its up, in that order about the bomb's centre.
constexpr double kBodyPitch = -2.5, kBodyRoll = -45.0, kBodyYaw = -3.5;
constexpr Vec3   kForward = {0.0, 0.0, 1.0}, kUp = {0.0, 1.0, 0.0}; // the rig at rest: head at +Z, Y up

constexpr const char *kMds = "e112a.mds";
constexpr const char *kImg = "e112a01.img";
constexpr const char *kBbp = "e112a.bbp";
constexpr const char *kSheet = "e112a01";
constexpr const char *kSheet2 = "e112a02";
constexpr const char *kMotions[] = {"e112a.mot", "e112s.mot"}; // the body's and the shadow's
constexpr const char *kBodyFrame = kGemronBigGem;
constexpr const char *kWingFrames[] = {"tamas00__m", "tamas03__m"};
constexpr const char *kGlowFrames[] = {"tama__appz", "tamas01__appz", "tamas02__appz"};

// The glow disc: a game glow sprite's quad (unlit, no depth write, additive, camera-facing) re-laid as a 30-wide
// square 9.4 toward the camera, on the torch glow in the Fire ramp at 128 x 128 (no picture in a monster's IM2 bank is
// narrower). A scale track grows it across from nothing as the fuse burns; Z stays 1, the camera pull.
constexpr const char *kGlowTexture = "bombglow";
constexpr const char *kQuadMds = "b_boll.mds";
constexpr const char *kQuadFrame = "bool__czappba";
constexpr double      kGlowDiameter = 30.0;
constexpr double      kGlowPull = 9.4;
constexpr float       kGlowNone = 0.001f; // a scale of 0 would zero the frame's axes
constexpr int         kGlowTextureSize = 128;
// (frame, scale): none until a fuse, full at its blast, gone the frame after: the death 105-122, the self-destruct
// 200-217. The last key lies past the self-destruct's end, as the engine plays a track only between two keys.
constexpr std::pair<std::uint32_t, float> kGlowKeys[] = {
    {0,                        kGlowNone},
    {105,                      kGlowNone},
    {122,                      1.0f     },
    {123,                      kGlowNone},
    {kBombSelfDestructStart,   kGlowNone},
    {kBombSelfDestructEnd - 1, 1.0f     },
    {kBombSelfDestructEnd,     kGlowNone},
    {kBombSelfDestructEnd + 1, kGlowNone},
};

// The self-destruct's pieces at its key's 0.15: motion 7 (the guard's return, 185-195) reversed (source from, to, key
// frames), then motion 6 (the guard loop, 170-180, which starts and ends on 185's pose) once on its own keys.
struct Piece {
    int    from, to;
    double frames;
};

constexpr Piece kReversedReturn[] = {
    {195, 191, 4   },
    {191, 188, 1.5 },
    {188, 185, 0.75}
};
constexpr std::pair<int, int> kGuardLoop = {170, 180};
constexpr const char         *kSelfDestructSpeed = "0.15";

double ReturnFrames() {
    double total = 0;
    for (const Piece &piece : kReversedReturn) {
        total += piece.frames;
    }
    return total;
}

int LoopStart() {
    return static_cast<int>(std::ceil(ReturnFrames()));
}

// The source range tau key frames into the self-destruct, and its source frame there.
std::pair<std::pair<int, int>, double> PieceAt(double tau) {
    double lead = LoopStart() - ReturnFrames(); // the first pose held, so the return ends on a whole frame
    if (tau <= lead) {
        return {
            {kReversedReturn[0].from, kReversedReturn[0].to},
            kReversedReturn[0].from
        };
    }
    double at = lead;
    for (const Piece &piece : kReversedReturn) {
        if (tau <= at + piece.frames) {
            return {
                {piece.from, piece.to},
                piece.from + (piece.to - piece.from) * (tau - at) / piece.frames
            };
        }
        at += piece.frames;
    }
    const Piece &last = kReversedReturn[std::size(kReversedReturn) - 1];
    if (tau <= LoopStart()) {
        return {
            {last.from, last.to},
            last.to
        };
    }
    return {kGuardLoop, std::min<double>(kGuardLoop.second, kGuardLoop.first + (tau - LoopStart()))};
}

// A track's pose at source frame s as a key on frame: the key itself on one, else its neighbours blended (rotation
// slerped, the rest lerped).
MotKeyframe Sample(const std::map<std::uint32_t, MotKeyframe> &keys, std::uint32_t channel, double s, std::uint32_t frame) {
    auto        after = keys.upper_bound(static_cast<std::uint32_t>(std::floor(s)));
    auto        before = std::prev(after);
    MotKeyframe key = before->second;
    key.SetFrame(frame);
    if (s == before->first) {
        return key;
    }
    double                t = (s - before->first) / (after->first - before->first);
    std::array<double, 4> v = BlendKeys(before->second.Values(), after->second.Values(), t, channel == mot::kRotation);
    for (int i = 0; i < 4; i++) {
        key.SetValue(i, static_cast<float>(v[std::size_t(i)]));
    }
    return key;
}

void BakeSelfDestruct(ChrPack &pack) {
    if (kBombSelfDestructStart + LoopStart() + (kGuardLoop.second - kGuardLoop.first) + 1 != kBombSelfDestructEnd) {
        throw std::runtime_error("the self-destruct's pieces do not end on its last frame");
    }
    for (const char *name : kMotions) {
        ChrRecord &record = pack.Require(name);
        MotFile    mot = MotFile::FromRecord(record);
        for (MotTrack &track : mot.tracks) {
            if (!track.keys.empty() && track.keys.back().Frame() >= kBombSelfDestructStart) {
                throw std::runtime_error(std::format("{}: a track already reaches frame {}", name, track.keys.back().Frame()));
            }
            std::vector<MotKeyframe> made;
            for (int f = kBombSelfDestructStart; f <= kBombSelfDestructEnd + 1; f++) {
                auto [range, s] = PieceAt(f - kBombSelfDestructStart);
                int                                  lo = std::min(range.first, range.second), hi = std::max(range.first, range.second);
                std::map<std::uint32_t, MotKeyframe> keys;
                for (const MotKeyframe &key : track.keys) {
                    if (key.Frame() >= std::uint32_t(lo) && key.Frame() <= std::uint32_t(hi)) {
                        keys.emplace(key.Frame(), key);
                    }
                }
                if (!keys.contains(std::uint32_t(lo)) || !keys.contains(std::uint32_t(hi))) {
                    throw std::runtime_error(std::format("{}: track {}/{} has no key on {} or {}", name, track.w0, track.w2, lo, hi));
                }
                made.push_back(Sample(keys, track.w2, s, std::uint32_t(f)));
            }
            track.keys.insert(track.keys.end(), made.begin(), made.end());
        }
        record.ReplacePayload(mot.BuildPayload());
    }
    // KEY 14 in info.cfg becomes the self-destruct.
    ChrRecord &cfg = pack.Require("info.cfg");
    cfg.ReplacePayload(SetCfgKey(cfg.Payload(), kBombSelfDestructKey,
                                 std::format("KEY\t{},\t{},\t{},\t", kBombSelfDestructStart, kBombSelfDestructEnd, kSelfDestructSpeed)));
}

// The bomb's mesh with every position and normal scaled and turned: p' = R . spin . (scale . p).
Bytes BombMesh(std::span<const unsigned char> bomb_mds, double scale, const Mat3 &turn, double spin) {
    std::vector<MdsFrame> frames = ReadMdsSkeleton(bomb_mds);
    MdtMesh               mesh = MdtMesh::Parse(MdtAt(bomb_mds, std::size_t(frames.at(0).mesh)));
    double                c = std::cos(spin), s = std::sin(spin);
    Mat3                  r = MatMul(turn, Mat3{
                                               {{c, 0, s}, {0, 1, 0}, {-s, 0, c}}
    }); // about the bomb's own +Y, the wick
    for (auto &p : mesh.positions) {
        Vec3 v = MatVec(r, {p[0] * scale, p[1] * scale, p[2] * scale});
        p = {v[0], v[1], v[2], p[3]};
    }
    for (auto &n : mesh.normals) {
        Vec3 v = MatVec(r, {n[0], n[1], n[2]});
        n = {v[0], v[1], v[2], n[3]};
    }
    return mesh.Build();
}

// The big bomb's turn in its bone's frame: yaw . roll . pitch about the Gemron's up, forward and left there.
Mat3 BodyTurn(const MdsFrame &body) {
    Vec3 right = Cross(kForward, kUp);
    Vec3 left = Unit(body.ToLocal(Scale(right, -1.0))), forward = Unit(body.ToLocal(kForward)), up = Unit(body.ToLocal(kUp));
    return MatMul(AxisRotation(up, kBodyYaw * kDegrees), MatMul(AxisRotation(forward, kBodyRoll * kDegrees), AxisRotation(left, kBodyPitch * kDegrees)));
}

// The glow sprite's quad re-laid as the disc, its material on the disc's picture.
Bytes GlowQuad(std::span<const unsigned char> quad_mds) {
    std::vector<MdsFrame> frames = ReadMdsSkeleton(quad_mds);
    const MdsFrame       *quad = FindFrame(frames, kQuadFrame);
    if (quad == nullptr) {
        throw std::runtime_error(std::format("{} has no {}", kQuadMds, kQuadFrame));
    }
    MdtMesh mesh = MdtMesh::Parse(MdtAt(quad_mds, std::size_t(quad->mesh)));
    double  r = kGlowDiameter / 2;
    // (x, y, u, v) per position, in the source quad's winding.
    const std::array<std::array<double, 4>, 4> corners = {
        {{-r, r, 0.0, 0.0}, {-r, -r, 0.0, 1.0}, {r, r, 1.0, 0.0}, {r, -r, 1.0, 1.0}}
    };
    mesh.positions.clear();
    for (const auto &corner : corners) {
        mesh.positions.push_back({corner[0], corner[1], kGlowPull, 1.0});
    }
    // The quad pairs positions and texture coordinates crosswise, so each coordinate comes from the position its records use.
    std::vector<std::optional<std::array<double, 4>>> uv(mesh.uvs.size());
    for (const MdtMesh::Submesh &sub : mesh.submeshes) {
        for (const std::vector<int> &record : sub.records) {
            std::array<double, 4> want = {corners[std::size_t(record[0])][2], corners[std::size_t(record[0])][3], 1.0, 0.0};
            auto                 &slot = uv.at(std::size_t(record[2]));
            if (slot && *slot != want) {
                throw std::runtime_error(std::format("{}: a texture coordinate is shared by two corners", kQuadFrame));
            }
            slot = want;
        }
    }
    mesh.uvs.clear();
    for (const auto &coordinate : uv) {
        if (!coordinate) {
            throw std::runtime_error(std::format("{}: a texture coordinate no record uses", kQuadFrame));
        }
        mesh.uvs.push_back(*coordinate);
    }
    Bytes material = mesh.materials.at(0);
    std::fill_n(material.begin() + MdtMesh::kMaterialTexture, 0x20, 0);
    std::copy_n(kGlowTexture, std::char_traits<char>::length(kGlowTexture), material.begin() + MdtMesh::kMaterialTexture);
    mesh.materials = {material};
    return mesh.Build();
}

void AddGlowTrack(ChrPack &pack, int frame) {
    ChrRecord &record = pack.Require(kMotions[0]);
    MotFile    mot = MotFile::FromRecord(record);
    MotTrack   track;
    track.w0 = std::uint32_t(frame);
    track.w2 = mot::kScale;
    track.w3 = mot.tracks.back().w3;
    track.w6 = mot.tracks.back().w6;
    track.w7 = mot.tracks.back().w7;
    for (const auto &[at, scale] : kGlowKeys) {
        MotKeyframe key;
        key.SetFrame(at);
        key.SetValue(0, scale);
        key.SetValue(1, scale);
        key.SetValue(2, 1.0f);
        track.keys.push_back(key);
    }
    mot.tracks.push_back(track);
    record.ReplacePayload(mot.BuildPayload());
}

} // namespace

Bytes BuildBombGemronModel(std::span<const unsigned char> gemron, std::span<const unsigned char> bomb_mds,
                           std::span<const unsigned char> bomb_img, std::span<const unsigned char> quad_chr,
                           std::span<const unsigned char> glow_pack) {
    ChrPack               pack = ChrPack::Parse(gemron);
    Bytes                 mds = pack.Require(kMds).Payload();
    std::vector<MdsFrame> frames = ReadMdsSkeleton(mds);
    const MdsFrame       *body = FindFrame(frames, kBodyFrame);
    if (body == nullptr) {
        throw std::runtime_error(std::format("{} has no {}", kMds, kBodyFrame));
    }
    Vec3                                        right = Cross(kForward, kUp);
    std::map<std::string, std::optional<Bytes>> replaced;
    for (const char *glow : kGlowFrames) {
        replaced[glow] = std::nullopt;
    }
    // The big bomb: wick down, forward and to the right.
    Vec3 aim = Unit(Add(Add(Scale(kUp, -1.0), kForward), right));
    replaced[kBodyFrame] = BombMesh(bomb_mds, 3.5 / kBombRadius, MatMul(BodyTurn(*body), RotationYTo(Unit(body->ToLocal(aim)))), kBodySpin);
    // The wing bombs: wick up, toward the body and forward.
    for (const char *name : kWingFrames) {
        const MdsFrame *wing = FindFrame(frames, name);
        if (wing == nullptr) {
            throw std::runtime_error(std::format("{} has no {}", kMds, name));
        }
        Vec3 to_body = Unit(Add(body->WorldPosition(), Scale(wing->WorldPosition(), -1.0)));
        aim = Unit(Add(Add(kUp, to_body), kForward));
        replaced[name] = BombMesh(bomb_mds, 1.3 / kBombRadius, RotationYTo(Unit(wing->ToLocal(aim))), kWingSpin);
    }
    // The glow disc: a frame of its own on the big bomb's bone, after the last.
    int   glow = static_cast<int>(frames.size());
    Bytes quad_mds = ChrPack::Parse(quad_chr).Require(kQuadMds).Payload();
    pack.Require(kMds).ReplacePayload(ReplaceMeshes(mds, replaced, {
                                                                       {FrameRecord(glow, kBombGlowFrame, body->index), GlowQuad(quad_mds)}
    }));
    AppendBindPoses(pack, kBbp, 1); // the glow disc's
    // The sheets in the look, the bomb's sheet and the glow disc added to the bank.
    ImgBank   bank = ImgBank::Parse(pack.Require(kImg).Payload());
    ImgBank   bomb = ImgBank::Parse(bomb_img);
    SheetLook look = SheetLook::Parse(kBombGemronLook);
    Bytes     sheet = bank.Block(kSheet);
    for (auto &[name, block] : bank.entries) {
        if (name == kSheet) {
            block = look.ApplyToSheet(block);
        } else if (name == kSheet2) {
            block = look.ApplyBaseToSheet(block);
        }
    }
    bank.entries.insert(bank.entries.end(), bomb.entries.begin(), bomb.entries.end());
    Bytes torch = ImgBank::Parse(ChrPack::Parse(glow_pack).Require(kGlowSourceBank).Payload()).Block(kGlowSourcePicture);
    Bytes disc = BuildGlowDisc(sheet, torch, kFireGlowCore, kFireGlowOuter);
    bank.entries.emplace_back(kGlowTexture, tim8::Swizzled(tim8::Resample(disc, false, kGlowTextureSize)));
    pack.Require(kImg).ReplacePayload(bank.Build());
    BakeSelfDestruct(pack);
    AddGlowTrack(pack, glow);
    return pack.Rebuild();
}

Bytes BuildBombThrowPack(std::span<const unsigned char> apple, std::span<const unsigned char> bomb_mds,
                         std::span<const unsigned char> bomb_img) {
    ChrPack               pack = ChrPack::Parse(apple);
    std::vector<MdsFrame> frames = ReadMdsSkeleton(bomb_mds);
    Bytes                 bomb = MdtAt(bomb_mds, std::size_t(frames.at(0).mesh));
    ChrRecord            &mds = pack.Require("ringo_ex.mds");
    mds.ReplacePayload(ReplaceMeshes(mds.Payload(), {
                                                        {"dokuring__m", bomb}
    }));
    ChrRecord &img = pack.Require("dokuring.img");
    ImgBank    bank = ImgBank::Parse(img.Payload());
    ImgBank    pictures = ImgBank::Parse(bomb_img);
    bank.entries.insert(bank.entries.end(), pictures.entries.begin(), pictures.entries.end());
    img.ReplacePayload(bank.Build());
    return pack.Rebuild();
}

int BuildGemronModels(const fs::path &data, std::FILE *log) {
    // Each built from its sources in turn.
    auto from = [](std::vector<std::string> sources, auto build) {
        return std::pair{sources, [sources, build](const DiscReader &read) {
                             std::vector<Bytes> files;
                             for (const std::string &source : sources) {
                                 files.push_back(read(source));
                             }
                             return build(files);
                         }};
    };
    std::vector<DataBuild> builds;

    auto add = [&](const char *name, const char *target, auto made) { builds.push_back({name, target, made.first, made.second}); };
    using Files = const std::vector<Bytes> &;
    // clang-format off
    add("Bomb Gemron",             kBombGemronModelFile,     from({kBombGemronSource, kBombModelFile, kBombPictureFile, kGlowQuadFile, kGlowSourcePack},                    [](Files s) { return BuildBombGemronModel(s[0], s[1], s[2], s[3], s[4]); }));
    add("Bomb Gemron's bomb",      kBombThrowFile,           from({kBombThrowSource, kBombModelFile, kBombPictureFile},                                                     [](Files s) { return BuildBombThrowPack(s[0], s[1], s[2]); }));
    add("Crystal Gemron",          kCrystalGemronModelFile,  from({kCrystalGemronSource, kCrystalBallFile},                                                                 [](Files s) { return BuildCrystalGemronModel(s[0], s[1]); }));
    add("Crystal Gemron's script", kCrystalGemronScriptFile, from({kCrystalGemronScriptSource},                                                                             [](Files s) { return BuildCrystalGemronScript(s[0]); }));
    add("Atla Gemron",             kAtlaGemronModelFile,     from({kAtlaGemronSource, kAtlaPackFile, kAtlaPictureFile, kToanPackFile, kAtlamilliaLightFile, kFlyLightFile}, [](Files s) { return BuildAtlaGemronModel(s[0], s[1], s[2], s[3], s[4], s[5]); }));
    add("Atla Gemron's shot",      kAtlaShotFile,            from({kAtlamilliaLightFile},                                                                                   [](Files s) { return BuildAtlaShotPack(s[0]); }));
    add("Atla Gemron's script",    kAtlaGemronScriptFile,    from({kAtlaGemronScriptSource},                                                                                [](Files s) { return BuildAtlaGemronScript(s[0]); }));
    // clang-format on
    return RunDataBuilds(data, "gemron model", builds, log);
}

} // namespace expanded
