#include "expanded/dungeon/bank_slices.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "platform/vfs.hpp"

namespace expanded {

namespace {

// The slices' samples as files of their own, for the bank's loader to read.
class MemoryMount : public vfs::Mount {
public:
    void Add(std::string name, std::vector<std::uint8_t> bytes) { files_.push_back({std::move(name), std::move(bytes)}); }

    std::size_t Count() const override { return files_.size(); }

    std::string Name(std::size_t index) const override { return files_[index].name; }

    std::string Display(std::size_t index) const override { return "bank slices!" + files_[index].name; }

    std::optional<std::size_t> Find(std::string_view relative) const override {
        for (std::size_t i = 0; i < files_.size(); i++) {
            if (vfs::FoldKey(files_[i].name) == vfs::FoldKey(relative)) {
                return i;
            }
        }
        return std::nullopt;
    }

    std::optional<std::uint64_t> Size(std::size_t index) const override { return files_[index].bytes.size(); }

    bool Read(std::size_t index, std::vector<std::uint8_t> &bytes, std::uint64_t max_bytes) const override {
        if (files_[index].bytes.size() > max_bytes) {
            return false;
        }
        bytes = files_[index].bytes;
        return true;
    }

    bool ReadInto(std::size_t index, void *buffer, std::size_t size) const override {
        if (size != files_[index].bytes.size()) {
            return false;
        }
        std::copy(files_[index].bytes.begin(), files_[index].bytes.end(), static_cast<std::uint8_t *>(buffer));
        return true;
    }

    std::string Stamp(std::size_t index) const override { return files_[index].name; }

    std::vector<vfs::Member> List(std::string_view directory) const override {
        std::vector<vfs::Member> members;
        if (directory.empty()) {
            for (const File &file : files_) {
                members.push_back({file.name, false});
            }
        }
        return members;
    }

private:
    struct File {
        std::string               name;
        std::vector<std::uint8_t> bytes;
    };

    std::vector<File> files_;
};

void Put32(std::vector<std::uint8_t> &out, std::uint32_t value) {
    for (int i = 0; i < 4; i++) {
        out.push_back(static_cast<std::uint8_t>(value >> (8 * i)));
    }
}

void Put16(std::vector<std::uint8_t> &out, std::uint16_t value) {
    out.push_back(static_cast<std::uint8_t>(value));
    out.push_back(static_cast<std::uint8_t>(value >> 8));
}

// Mono 16-bit PCM as a WAV file.
std::vector<std::uint8_t> Wav(const std::vector<std::int16_t> &pcm, int rate) {
    const auto                data = static_cast<std::uint32_t>(pcm.size() * 2);
    std::vector<std::uint8_t> out;
    out.reserve(44 + data);
    out.insert(out.end(), {'R', 'I', 'F', 'F'});
    Put32(out, 36 + data);
    out.insert(out.end(), {'W', 'A', 'V', 'E', 'f', 'm', 't', ' '});
    Put32(out, 16);
    Put16(out, 1); // PCM
    Put16(out, 1); // mono
    Put32(out, static_cast<std::uint32_t>(rate));
    Put32(out, static_cast<std::uint32_t>(rate) * 2);
    Put16(out, 2);
    Put16(out, 16);
    out.insert(out.end(), {'d', 'a', 't', 'a'});
    Put32(out, data);
    for (std::int16_t sample : pcm) {
        Put16(out, static_cast<std::uint16_t>(sample));
    }
    return out;
}

// [from, to) seconds of a layer's sample, faded out over its last fade seconds.
std::vector<std::int16_t> Cut(const BankSlice &slice) {
    const std::vector<std::int16_t> &pcm = slice.layer.vag->pcm;
    const float                      rate = static_cast<float>(slice.layer.rate);
    const std::size_t                from = std::min(pcm.size(), static_cast<std::size_t>(std::max(0.0f, slice.from) * rate));
    const std::size_t                to = std::clamp(static_cast<std::size_t>(std::max(0.0f, slice.to) * rate), from, pcm.size());
    std::vector<std::int16_t>        cut(pcm.begin() + static_cast<std::ptrdiff_t>(from), pcm.begin() + static_cast<std::ptrdiff_t>(to));
    const std::size_t                fade = std::min(cut.size(), static_cast<std::size_t>(std::max(0.0f, slice.fade) * rate));
    for (std::size_t k = 0; k < fade; k++) {
        const std::size_t at = cut.size() - fade + k;
        cut[at] = static_cast<std::int16_t>(cut[at] * static_cast<float>(fade - k) / static_cast<float>(fade));
    }
    return cut;
}

} // namespace

std::shared_ptr<audio::Bank> CreateBankSlices(std::span<const BankSlice> slices) {
    auto           mount = std::make_shared<MemoryMount>();
    nlohmann::json splits = nlohmann::json::array(), sets = nlohmann::json::array(), samples = nlohmann::json::array(),
                   vags = nlohmann::json::array();
    for (std::size_t i = 0; i < slices.size(); i++) {
        const BankSlice &slice = slices[i];
        if (slice.layer.split == nullptr || slice.layer.sample == nullptr || slice.layer.vag == nullptr) {
            return nullptr;
        }
        const int             note = SliceNote(static_cast<int>(i));
        const audio::HdSplit &split = *slice.layer.split;
        splits.push_back({
            {"sample_set", i              },
            {"key_low",    note - 2       },
            {"key_high",   note + 2       },
            {"bend_low",   split.bend_low },
            {"bend_high",  split.bend_high},
            {"volume",     split.volume   },
            {"pan",        split.pan      },
            {"transpose",  split.transpose},
            {"detune",     split.detune   }
        });
        sets.push_back({
            {"vel_curve", slice.layer.vel_curve     },
            {"vel_low",   0                         },
            {"vel_high",  127                       },
            {"samples",   nlohmann::json::array({i})}
        });
        // Its pitch at its note is its source's at slice.note.
        const audio::HdSample &sample = *slice.layer.sample;
        samples.push_back({
            {"vag", i},
            {"vel_low", sample.vel_low},
            {"vel_high", sample.vel_high},
            {"base_note", std::clamp(sample.base_note + note - slice.note, 0, 127)},
            {"detune", sample.detune},
            {"pan", sample.pan},
            {"volume", sample.volume},
            {"adsr1", sample.adsr1},
            {"adsr2", sample.adsr2},
            {"spu_attr", sample.spu_attr}
        });
        std::vector<std::int16_t> cut = Cut(slice);
        const std::string         name = "slice" + std::to_string(i) + ".wav";
        vags.push_back({
            {"wav",        name      },
            {"loop_start", 0         },
            {"loop_end",   cut.size()},
            {"loops",      false     },
            {"attr",       0         }
        });
        mount->Add(name, Wav(cut, slice.layer.rate));
    }
    nlohmann::json program = {
        {"present",   true  },
        {"volume",    127   },
        {"pan",       64    },
        {"transpose", 0     },
        {"detune",    0     },
        {"splits",    splits}
    };
    nlohmann::json bank = {
        {"schema_version", 1                               },
        {"kind",           "bank"                          },
        {"programs",       nlohmann::json::array({program})},
        {"sample_sets",    sets                            },
        {"samples",        samples                         },
        {"vags",           vags                            }
    };
    std::shared_ptr<const vfs::Mount> shared = mount;
    return audio::Bank::CreateFromJsonCandidates(bank.dump(), [&](const std::filesystem::path &relative) {
        return std::vector<vfs::Ref>{vfs::Find(shared, relative.generic_string())};
    });
}

} // namespace expanded
