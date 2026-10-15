#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "expanded/core/save.hpp"
#include "memorycardaccess.hpp"
#include "platform/paths.hpp"
#include "platform_fixture.hpp"
#include "save_slots.hpp"
#include "savedata.hpp"

namespace fs = std::filesystem;

namespace {

const expanded::SaveValue<int>              kStage{"test.stage", -1};
const expanded::SaveValue<std::string>      kName{"test.name", "none"};
const expanded::SaveValue<std::vector<int>> kDone{"test.done"};
const expanded::SaveValue<bool>             kPending{"test.pending"};
const expanded::SaveValue<float>            kRate{"test.rate", 1.5f};

fs::path TempFolder(const char *name) {
    fs::path root = fs::temp_directory_path() / (std::string(name) + std::to_string(dc::test::ProcessId()));
    fs::remove_all(root);
    fs::create_directories(root);
    return root;
}

alignas(64) char g_menu_buffer[0x30000];
alignas(64) CSaveData g_save;
CMemoryCardAccess g_mc;

int RunOperation(int operation) {
    g_mc.SetFuncNo(operation);
    for (int step = 0; step < 100; ++step) {
        if (int result = g_mc.Step(); result != 0) {
            return result;
        }
    }
    ADD_FAILURE() << "operation never finished";
    return 0;
}

int SaveTo(int file_no, bool new_save) {
    SaveSlotNew = new_save;
    g_mc.file_no = file_no;
    int result = RunOperation(MC_OPERATION_SAVE);
    SaveSlotNew = false;
    return result;
}

int LoadFrom(int file_no) {
    g_mc.file_no = file_no;
    return RunOperation(MC_OPERATION_LOAD);
}

} // namespace

TEST(ExpandedSave, ValuesFallBackAndKeepTheirType) {
    expanded::SaveClear();
    EXPECT_EQ(kStage.Get(), -1);
    EXPECT_EQ(kName.Get(), "none");
    EXPECT_TRUE(kDone.Get().empty());
    EXPECT_FALSE(kPending.Has());
    EXPECT_EQ(kRate.Get(), 1.5f);

    kStage.Set(3);
    kName.Set("Toan");
    kDone.Set({1, 4});
    kPending.Set(true);
    kRate.Set(0.25f);
    EXPECT_EQ(kStage.Get(), 3);
    EXPECT_EQ(kName.Get(), "Toan");
    EXPECT_EQ(kDone.Get(), (std::vector<int>{1, 4}));
    EXPECT_TRUE(kPending.Get());
    EXPECT_EQ(kRate.Get(), 0.25f);

    // A value of another type reads as the fallback.
    const expanded::SaveValue<std::string> stage_text{"test.stage", "?"};
    EXPECT_EQ(stage_text.Get(), "?");
    stage_text.Set("three");
    EXPECT_EQ(kStage.Get(), -1);
    kPending.Reset();
    EXPECT_FALSE(kPending.Has());
    EXPECT_FALSE(kPending.Get());
    expanded::SaveClear();
    EXPECT_TRUE(expanded::SaveEmpty());
}

TEST(ExpandedSave, FileRoundTripsAndIsOptional) {
    fs::path folder = TempFolder("dc_expanded_save_");
    expanded::SaveClear();
    kStage.Set(5);
    kDone.Set({30});
    ASSERT_TRUE(expanded::SaveWrite(folder));
    ASSERT_TRUE(fs::exists(folder / expanded::kSaveFile));

    expanded::SaveClear();
    ASSERT_TRUE(expanded::SaveRead(folder));
    EXPECT_EQ(kStage.Get(), 5);
    EXPECT_EQ(kDone.Get(), std::vector<int>{30});

    // An older save has no file: an empty state, without error.
    ASSERT_TRUE(expanded::SaveRead(folder / "older"));
    EXPECT_TRUE(expanded::SaveEmpty());

    // A broken file reads as empty.
    std::ofstream(folder / expanded::kSaveFile) << "[1, 2";
    kStage.Set(1);
    EXPECT_FALSE(expanded::SaveRead(folder));
    EXPECT_TRUE(expanded::SaveEmpty());

    // An empty state leaves no file.
    ASSERT_TRUE(expanded::SaveWrite(folder));
    EXPECT_FALSE(fs::exists(folder / expanded::kSaveFile));
    fs::remove_all(folder);
}

TEST(ExpandedSave, TravelsWithTheSave) {
    fs::path root = TempFolder("dc_expanded_slots_");
    PathsSetSaveRoot(root);
    g_save.Initialize();
    SaveData = &g_save;
    ASSERT_EQ(g_mc.InitForMC(), 0);
    g_mc.SetBuff(g_menu_buffer);

    // Saved beside save.dat, and read back by the load.
    expanded::SaveClear();
    kStage.Set(2);
    ASSERT_EQ(SaveTo(0, true), 1);
    fs::path slot = root / "saves" / "1";
    ASSERT_TRUE(fs::exists(slot / "save.dat"));
    ASSERT_TRUE(fs::exists(slot / expanded::kSaveFile));
    expanded::SaveClear();
    ASSERT_EQ(LoadFrom(0), 1);
    EXPECT_EQ(kStage.Get(), 2);

    // A save without the file loads with an empty state.
    fs::create_directories(root / "saves" / "2");
    fs::copy_file(slot / "save.dat", root / "saves" / "2" / "save.dat");
    ASSERT_EQ(LoadFrom(1), 1);
    EXPECT_TRUE(expanded::SaveEmpty());

    // Saving over a save with an empty state removes its file.
    ASSERT_EQ(SaveTo(0, false), 1);
    EXPECT_FALSE(fs::exists(slot / expanded::kSaveFile));

    // The autosave has its own.
    kStage.Set(9);
    ASSERT_TRUE(SaveSlotWriteAutosave());
    EXPECT_TRUE(fs::exists(root / "saves" / "autosave" / expanded::kSaveFile));
    expanded::SaveClear();
    ASSERT_EQ(LoadFrom(kSaveSlotAutosave), 1);
    EXPECT_EQ(kStage.Get(), 9);

    // Deleting a save takes the file and the folder.
    ASSERT_EQ(SaveTo(0, false), 1);
    ASSERT_TRUE(fs::exists(slot / expanded::kSaveFile));
    g_mc.file_no = 0;
    ASSERT_EQ(RunOperation(MC_OPERATION_DELETE), 1);
    EXPECT_FALSE(fs::exists(slot));

    SaveData = nullptr;
    fs::remove_all(root);
}
