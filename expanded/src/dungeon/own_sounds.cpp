#include "expanded/dungeon/own_sounds.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "audio/hdbank.hpp"
#include "audio/mixer.hpp"
#include "data/data_read.hpp"
#include "dataread.hpp"
#include "expanded/core/random.hpp"
#include "expanded/debug/event_log.hpp"
#include "expanded/dungeon/bank_slices.hpp"
#include "sound.hpp"

namespace expanded {

namespace {

struct OwnBank {
    std::shared_ptr<const audio::Bank> bank;
    int                                select = 0; // its CC 0 on the port, bound once
};

std::map<std::pair<std::string, std::string>, OwnBank> own_banks;

// The sounds read as a floor loads.
std::vector<OwnSound> &OwnSounds() {
    static std::vector<OwnSound> sounds;
    return sounds;
}

struct Duck {
    int frames = 0; // since the sound went off
    int length = 0; // the sound's frames; 0 for no duck
};

Duck duck;

// The stretches a sound is made of, as a bank of their own.
std::shared_ptr<const audio::Bank> Cut(const audio::Bank &whole, const OwnSound &sound) {
    std::vector<BankSlice> slices;
    for (int i = 0; i < sound.cut_count; i++) {
        const OwnCut               &cut = sound.cuts[i];
        std::array<audio::Layer, 8> layers;
        if (whole.Resolve(sound.program, cut.note, 0x7F, layers) == 0) {
            return nullptr;
        }
        slices.push_back({layers[0], cut.note, cut.from, cut.to, cut.fade});
    }
    return CreateBankSlices(slices);
}

void Send(std::array<std::uint8_t, 8> message) {
    audio::DefaultMixer().HsMessage(kOwnSoundPort, message);
}

} // namespace

void RegisterOwnSound(const OwnSound &sound) {
    OwnSounds().push_back(sound);
}

bool LoadOwnSound(const OwnSound &sound) {
    auto key = std::make_pair(std::string(sound.path), std::string(sound.bank));
    if (auto found = own_banks.find(key); found != own_banks.end()) {
        return found->second.bank != nullptr;
    }
    std::shared_ptr<const audio::Bank> bank;
    std::uintmax_t                     size = DataFileSize(sound.path);
    if (size > 0) {
        // A buffer of its own (read_buffer may hold the floor's data), whole sectors long.
        std::vector<u_int> pack((size + 2 * 2048) / sizeof(u_int));
        char               path[128];
        std::snprintf(path, sizeof(path), "%s", sound.path);
        if (LoadFile2(path, pack.data(), nullptr, 0) != 0) {
            char body[64];
            std::snprintf(body, sizeof(body), "%s", sound.bank);
            int   header_size = 0, body_size = 0;
            auto *hd = reinterpret_cast<const std::uint8_t *>(GetPackFile(pack.data(), body, &header_size));
            if (char *dot = std::strrchr(body, '.'); dot != nullptr) {
                std::strcpy(dot, ".bd");
            }
            auto *bd = reinterpret_cast<const std::uint8_t *>(GetPackFile(pack.data(), body, &body_size));
            if (hd != nullptr && bd != nullptr) {
                bank = audio::Bank::Create({hd, static_cast<std::size_t>(header_size)}, {bd, static_cast<std::size_t>(body_size)});
            }
            if (bank != nullptr && sound.cuts != nullptr) {
                bank = Cut(*bank, sound);
            }
        }
        DataReadForgetPackCapacities(pack.data());
    }
    // Each bank keeps a CC 0 of its own: binding over a bound bank silences the port.
    int select = static_cast<int>(own_banks.size());
    if (bank != nullptr && select < kOwnSoundBanks) {
        audio::DefaultMixer().BindBank(kOwnSoundPort, select, bank);
        SetPortVolume(kOwnSoundPort, audio::kFullPortVolume);
    } else {
        bank = nullptr;
    }
    own_banks.emplace(key, OwnBank{bank, select});
    return bank != nullptr;
}

bool PlayOwnSound(const OwnSound &sound, bool ducked, int cut) {
    auto found = own_banks.find({sound.path, sound.bank});
    if (found == own_banks.end() || found->second.bank == nullptr) {
        return false;
    }
    // As CSound::SE_Play sends an effect: the program, the bank, the volume and pan, then the key-on.
    audio::Mixer &mixer = audio::DefaultMixer();
    int           program = sound.cuts != nullptr ? 0 : sound.program;
    mixer.ShortMessage(kOwnSoundPort, static_cast<std::uint32_t>(((program & 0x7F) << 8) | 0xC0));
    mixer.ShortMessage(kOwnSoundPort, static_cast<std::uint32_t>(0xB0 | (found->second.select << 16)));
    Send({0xF9, 0, 0, static_cast<std::uint8_t>(sound.volume & 0x7F), 0});
    Send({0xF9, 1, 0, 0x40, 0});
    int note = sound.note + (sound.variants > 1 ? ModRoll(sound.variants) : 0);
    if (sound.cuts != nullptr) {
        if (cut < 0 || cut >= sound.cut_count) {
            cut = ModRoll(sound.cut_count);
        }
        note = SliceNote(cut) + (sound.variants > 1 ? ModRoll(sound.variants) - sound.variants / 2 : 0);
    }
    Send({0xFD, 0x10, 0, static_cast<std::uint8_t>(note), 0, 0x7F, 0});
    if (ducked) {
        duck = {0, std::max(sound.frames, kDuckHold + 1)};
        DuckPorts(kDuckLevel, kOwnSoundPort);
    }
    EventLogSound(sound);
    return true;
}

void StopOwnSound(const OwnSound &sound) {
    auto found = own_banks.find({sound.path, sound.bank});
    if (found == own_banks.end() || found->second.bank == nullptr || sound.cuts != nullptr) {
        return;
    }
    // The key-on's program and bank, then the key-on again at velocity 0.
    audio::Mixer &mixer = audio::DefaultMixer();
    mixer.ShortMessage(kOwnSoundPort, static_cast<std::uint32_t>(((sound.program & 0x7F) << 8) | 0xC0));
    mixer.ShortMessage(kOwnSoundPort, static_cast<std::uint32_t>(0xB0 | (found->second.select << 16)));
    Send({0xFD, 0x10, 0, static_cast<std::uint8_t>(sound.note), 0, 0, 0});
}

float OwnSoundDuck() {
    if (duck.length == 0) {
        return 1.0f;
    }
    if (duck.frames <= kDuckHold) {
        return kDuckLevel;
    }
    float k = static_cast<float>(duck.frames - kDuckHold) / static_cast<float>(duck.length - kDuckHold);
    return std::min(1.0f, kDuckLevel + (1.0f - kDuckLevel) * k);
}

void OwnSoundsFloorLoaded() {
    for (const OwnSound &sound : OwnSounds()) {
        LoadOwnSound(sound);
    }
}

void OwnSoundsStep() {
    if (duck.length == 0) {
        return;
    }
    duck.frames++;
    float level = OwnSoundDuck();
    if (level >= 1.0f) {
        duck = {};
    }
    DuckPorts(level, kOwnSoundPort);
}

void OwnSoundsReset() {
    if (duck.length != 0) {
        duck = {};
        DuckPorts(1.0f, kOwnSoundPort);
    }
}

} // namespace expanded
