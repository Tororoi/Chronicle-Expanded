#pragma once

// What the Expanded tests share: a scratch folder, settings and data roots set for one test, and a dungeon floor of
// monsters set up as the game sets them up. Each puts back what it changed.

#include <atomic>
#include <filesystem>
#include <format>
#include <initializer_list>
#include <memory>
#include <string_view>
#include <system_error>

#include "dun/gameloop.hpp"
#include "expanded/core/options.hpp"
#include "expanded/enemies/enemy_events.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/enemies/roster.hpp"
#include "expanded/enemies/species.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"
#include "platform/config.hpp"
#include "platform/paths.hpp"
#include "platform_fixture.hpp"
#include "savedata.hpp"
#include "userstatus.hpp"

namespace expanded::test {

// A folder of its own under the temp directory, the process and a count in its name; removed with it.
class ScratchDir {
public:
    explicit ScratchDir(std::string_view name) {
        static std::atomic<int> count{0};
        path_ = std::filesystem::temp_directory_path() / std::format("dc_{}_{}_{}", name, dc::test::ProcessId(), count++);
        std::filesystem::remove_all(path_);
        std::filesystem::create_directories(path_);
    }

    ~ScratchDir() {
        std::error_code error;
        std::filesystem::remove_all(path_, error);
    }

    ScratchDir(const ScratchDir &) = delete;
    ScratchDir &operator=(const ScratchDir &) = delete;

    const std::filesystem::path &path() const { return path_; }

private:
    std::filesystem::path path_;
};

// The data directory set for a test.
class DataRootScope {
public:
    explicit DataRootScope(const std::filesystem::path &root) : saved_(PathsDataRoot()) { PathsSetDataRoot(root); }

    ~DataRootScope() { PathsSetDataRoot(saved_); }

    DataRootScope(const DataRootScope &) = delete;
    DataRootScope &operator=(const DataRootScope &) = delete;

private:
    std::filesystem::path saved_;
};

// Expanded's settings changed for a test, saved in a folder of their own.
class OptionScope {
public:
    OptionScope() : saved_root_(PathsSaveRoot()), saved_(ConfigGet()) { PathsSetSaveRoot(folder_.path()); }

    ~OptionScope() {
        ConfigChange(saved_);
        PathsSetSaveRoot(saved_root_);
    }

    OptionScope(const OptionScope &) = delete;
    OptionScope &operator=(const OptionScope &) = delete;

    // Turns a toggle on or off, or picks a choice; false when the setting does not take it.
    bool Set(std::string_view key, bool on) {
        Config config = ConfigGet();
        return SetOption(config.expanded_options, key, on) && ConfigChange(config);
    }

    bool Choose(std::string_view key, int choice) {
        Config config = ConfigGet();
        return SetOptionChoice(config.expanded_options, key, choice) && ConfigChange(config);
    }

private:
    ScratchDir            folder_{"options"};
    std::filesystem::path saved_root_;
    Config                saved_;
};

// A dungeon floor (off the dungeons by default, so no feature scales what it sets up) with every monster slot empty,
// the party status a fresh one.
class TestFloor {
public:
    explicit TestFloor(int dungeon = -1, int floor = 0) : saved_status_(UserStatus), saved_map_(selectMapNo) {
        UserStatus = status_.get();
        status_->cur_floor = floor;
        selectMapNo = dungeon;
        for (MONSTOR &monster : unit_->monster) {
            monster.state = -1;
        }
    }

    ~TestFloor() {
        UserStatus = saved_status_;
        selectMapNo = saved_map_;
    }

    TestFloor(const TestFloor &) = delete;
    TestFloor &operator=(const TestFloor &) = delete;

    CMonstorUnit &unit() { return *unit_; }

    CUserStatus &status() { return *status_; }

    // SetupBaseModel's part: the kind's record in the model slot.
    void SetUpModel(int model_slot, int model_no) {
        unit_->model[model_slot] = *Species(model_no);
        EnemyModelSetUp(*unit_, model_slot, model_no);
    }

    void SetUpModel(int model_slot, Monster kind) { SetUpModel(model_slot, No(kind)); }

    // SetupViewMonstor's part: the slot's record on the monster.
    void Spawn(int monster, int model_slot) {
        MONSTOR             &live = unit_->monster[monster];
        const MONSTOR_MODEL &model = unit_->model[model_slot];
        live.state = 1;
        live.base_model = model_slot;
        live.max_hp = live.hp = model.max_hp;
        live.defense = model.defense;
        live.hardness = model.hardness;
        live.exp = model.exp;
        live.palette_alpha = 128.0f;
        EnemySpawned(*unit_, monster);
        unit_->alive_count++;
    }

    // BtLoadMonstor's read of the first dungeon's floor 2, its front these kinds (staged first when staged, so that
    // Expanded's staging applies), then each kind of the front it loaded set up.
    const Roster &LoadRoster(bool staged, std::initializer_list<Monster> kinds) {
        rosters_ = {};
        for (Monster kind : kinds) {
            rosters_.front.Add(kind, SpawnsOnceNatively(kind));
        }
        if (staged) {
            StageFloor(2, rosters_);
        }
        selectMapNo = 0;
        status_->cur_floor = 2;
        RosterFloorLayout(BtEnemyLayoutList[0] + 2, 0);
        const Roster &loaded = staged ? StagedFloor(2)->front : rosters_.front;
        for (int slot = 0; slot < loaded.Size(); slot++) {
            SetUpModel(slot, loaded.model_no[slot]);
        }
        return loaded;
    }

private:
    std::unique_ptr<CMonstorUnit> unit_ = std::make_unique<CMonstorUnit>();
    std::unique_ptr<CUserStatus>  status_ = std::make_unique<CUserStatus>();
    FloorRosters                  rosters_;
    CUserStatus                  *saved_status_;
    s32                           saved_map_;
};

// A fresh save (SaveData) for a test: its dungeon status (the party's weapons) and storage.
class TestSave {
public:
    TestSave() { SaveData = &image_->data; }

    ~TestSave() { SaveData = saved_; }

    TestSave(const TestSave &) = delete;
    TestSave &operator=(const TestSave &) = delete;

    CUserStatus &status() { return *image_->data.GetDngStatus(); }

    CStockItem &stock() { return *image_->data.GetStockItem(); }

private:
    struct alignas(64) Image {
        CSaveData data;
    };

    std::unique_ptr<Image> image_ = std::make_unique<Image>();
    CSaveData             *saved_ = SaveData;
};

// The active character's weapon for a test (NowWeaponHave): weapon_no in hand, carrying sphere's SynthSphere when
// one is given (Super Steve's, for a sphere's effect).
class Wielding {
public:
    explicit Wielding(int weapon_no, int sphere = 0) {
        weapon_.item_no = static_cast<s16>(weapon_no);
        if (sphere != 0) {
            weapon_.attach[0].item_no = ITEM_ATTACH_SYNTHESIS_SPHERE;
            weapon_.attach[0].sphere_weapon_no = static_cast<s16>(sphere);
        }
        NowWeaponHave = &weapon_;
    }

    ~Wielding() { NowWeaponHave = saved_; }

    Wielding(const Wielding &) = delete;
    Wielding &operator=(const Wielding &) = delete;

    WEAPON_HAVE &weapon() { return weapon_; }

private:
    WEAPON_HAVE  weapon_ = {};
    WEAPON_HAVE *saved_ = NowWeaponHave;
};

} // namespace expanded::test
