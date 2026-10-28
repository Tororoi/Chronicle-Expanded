#include "expanded/data/fishing_scripts.hpp"

#include <algorithm>
#include <format>
#include <functional>
#include <initializer_list>
#include <utility>

namespace expanded {

namespace {

// The town script commands the scripts call.
enum : int {
    kGetPadDown = 1,
    kSetReturnCode = 3,
    kSetWorldCoord = 7,
    kInitialize = 8,
    kDrawExclamationMark = 10,
    kGotoFpChange = 24,
    kGotoChangeEsa = 25,
    kGotoFishRanking = 26,
    kLoadSync = 34,
    kClearVillagerBuff = 38,
    kClearEventBuff = 39,
    kActiveFileBuffer = 44,
    kLoadItemFile = 49,
    kLoadItem = 50,
    kLoadVillager = 57,
    kGetNpcPos = 131,
    kSetNpcMotion = 133,
    kSetNpcPos = 137,
    kSetNpcRot = 138,
    kGetNpcRot = 139,
    kMesMake = 192,
    kMesClose = 193,
    kSetMesAutoset = 195,
    kSetMesShippo = 196,
    kSetMesPos = 197,
    kSetMesDrawSpeed = 198,
    kSetMesCursor = 199,
    kFadeIn = 500,
    kFadeOut = 501,
    kCheckFade = 502,
    kSItemCheck = 707,
    kGetApad = 903,
    kSetFishingEsa = 994,
    kExitFishing = 995,
    kInitFish = 996,
    kGotoFishing = 997,
    kLoadFishingData = 998,
    kLoadMainChara = 999,
};

// _GET_PADDOWN's bits (Cross after the game swaps it with Circle), and the fishing rod.
constexpr int kPadCross = 0x20, kPadUp = 0x1000, kPadDown = 0x4000;
constexpr int kFishingRod = 185;
// The return code that keeps a fishing session running.
constexpr int kKeepFishing = 11;
// The message window the menus use: the town's event messages.
constexpr int kWindow = 1;
// The local a wait loop polls into.
constexpr int kGate = 1;

constexpr const char *kFishingModel = "chara/c01d_turi.chr", *kFishingModelCfg = "c01d_turi.cfg";
constexpr const char *kPlayerModel = "chara/c01d.chr", *kPlayerModelCfg = "info.cfg";

void Command(stb::Writer &w, int command, std::initializer_list<int> arguments = {}) {
    w.PushInt(command);
    for (int argument : arguments) {
        w.PushInt(argument);
    }
    w.Ext(static_cast<int>(arguments.size()) + 1);
}

void End(stb::Writer &w) {
    w.PushInt(0);
    w.Return();
}

void SetLocal(stb::Writer &w, int local, const std::function<void()> &push) {
    w.PushAddress(local);
    push();
    w.Store();
    w.Pop();
}

// while (poll(&gate) is busy) yield; _LOAD_SYNC is busy while non-zero, _CHECK_FADE until non-zero.
void WaitFor(stb::Writer &w, int poll, bool done_when_set) {
    w.UseLocals(kGate + 1);
    int retry = w.Mark();
    w.PushInt(poll);
    w.PushAddress(kGate);
    w.Ext(2);
    w.PushLocal(kGate);
    int done = w.MarkForward();
    if (done_when_set) {
        w.JumpTrue(done);
    } else {
        w.JumpFalse(done);
    }
    w.Yield();
    w.Jump(retry);
    w.Place(done);
}

// _SET_WORLD_COORD with no arguments: positions are the world's.
void WorldCoordinates(stb::Writer &w) {
    Command(w, kSetWorldCoord);
}

void CloseWindow(stb::Writer &w) {
    Command(w, kMesClose, {kWindow});
}

// Message lines as a menu until Cross, the chosen line in sel. The window stays open (the no-rod line reuses it).
void MenuBody(stb::Writer &w, const std::function<void()> &push_message, const std::function<void()> &push_last, int sel, int pad, int ly,
              int held, int scratch) {
    w.UseLocals(std::max({sel, pad, ly, held, scratch}) + 1);
    Command(w, kSetMesShippo, {kWindow, 0});
    w.PushInt(kSetMesDrawSpeed);
    w.PushInt(kWindow);
    w.PushFloat(0.0f);
    w.Ext(3);
    w.PushInt(kMesMake);
    w.PushInt(kWindow);
    push_message();
    w.Ext(3);
    Command(w, kSetMesPos, {kWindow, 9});
    Command(w, kSetMesAutoset, {kWindow, 0, 0, 0, 0});
    SetLocal(w, sel, [&] { w.PushInt(0); });
    SetLocal(w, held, [&] { w.PushInt(0); });

    auto step = [&](int delta) {
        SetLocal(w, sel, [&] {
            w.PushLocal(sel);
            w.PushInt(1);
            delta > 0 ? w.Add() : w.Subtract();
        });
    };
    // Down, unless on the last line; up, unless on the first.
    auto step_down = [&] {
        int skip = w.MarkForward();
        w.PushLocal(sel);
        push_last();
        w.Compare(stb::kLessThan);
        w.JumpFalse(skip);
        step(1);
        w.Place(skip);
    };
    auto step_up = [&] {
        int skip = w.MarkForward();
        w.PushLocal(sel);
        w.PushInt(0);
        w.Compare(stb::kGreaterThan);
        w.JumpFalse(skip);
        step(-1);
        w.Place(skip);
    };

    int loop = w.Mark();
    int after_stick = w.MarkForward();
    w.PushInt(kSetMesCursor);
    w.PushInt(kWindow);
    w.PushLocal(sel);
    w.Ext(3);
    w.Yield();
    w.PushInt(kGetApad);
    w.PushAddress(scratch);
    w.PushAddress(ly);
    w.Ext(3);
    w.PushInt(kGetPadDown);
    w.PushAddress(pad);
    w.Ext(2);

    // The stick moves a line as it leaves the middle (past 0.5), as the game's menus do.
    for (int direction : {1, -1}) {
        int not_pushed = w.MarkForward();
        w.PushLocal(ly);
        w.PushFloat(0.5f * static_cast<float>(direction));
        w.Compare(direction > 0 ? stb::kGreaterThan : stb::kLessThan);
        w.JumpFalse(not_pushed);
        int already = w.MarkForward();
        w.PushLocal(held);
        w.PushInt(0);
        w.Compare(stb::kEqual);
        w.JumpFalse(already);
        direction > 0 ? step_down() : step_up();
        w.Place(already);
        SetLocal(w, held, [&] { w.PushInt(1); });
        w.Jump(after_stick);
        w.Place(not_pushed);
    }
    SetLocal(w, held, [&] { w.PushInt(0); });
    w.Place(after_stick);

    for (int button : {kPadDown, kPadUp}) {
        int not_pressed = w.MarkForward();
        w.PushLocal(pad);
        w.PushInt(button);
        w.And();
        w.JumpFalse(not_pressed);
        button == kPadDown ? step_down() : step_up();
        w.Place(not_pressed);
    }

    w.PushLocal(pad);
    w.PushInt(kPadCross);
    w.And();
    w.JumpFalse(loop);
    Command(w, kSetMesCursor, {kWindow, -1});
    Command(w, kSetMesPos, {kWindow, 0});
    Command(w, kSetMesShippo, {kWindow, 1});
}

// sel = the menu's line for message's count lines, through the shared menu.
void Menu(stb::Writer &w, int message, int count, int sel, std::size_t menu) {
    w.UseLocals(sel + 1);
    w.PushAddress(sel);
    w.PushInt(message);
    w.PushInt(count);
    w.Call(menu);
    w.Store();
    w.Pop();
}

// A message in the open window, low as talk is, until Cross.
void ShowMessage(stb::Writer &w, int message, int pad) {
    w.UseLocals(pad + 1);
    Command(w, kSetMesShippo, {kWindow, 0});
    Command(w, kMesMake, {kWindow, message});
    Command(w, kSetMesPos, {kWindow, 8});
    Command(w, kSetMesAutoset, {kWindow, 0, 0, 0, 0});
    Command(w, kSetMesPos, {kWindow, 0});
    int loop = w.Mark();
    w.Yield();
    w.PushInt(kGetPadDown);
    w.PushAddress(pad);
    w.Ext(2);
    w.PushLocal(pad);
    w.PushInt(kPadCross);
    w.And();
    w.JumpFalse(loop);
    CloseWindow(w);
}

void LoadModel(stb::Writer &w, const char *model, const char *cfg, int villager_arena) {
    w.PushInt(kLoadMainChara);
    w.PushString(model);
    w.PushString(cfg);
    w.PushInt(villager_arena);
    w.Ext(4);
}

void FloatCommand(stb::Writer &w, int command, int chara, std::initializer_list<float> values) {
    w.PushInt(command);
    w.PushInt(chara);
    for (float value : values) {
        w.PushFloat(value);
    }
    w.Ext(static_cast<int>(values.size()) + 2);
}

} // namespace

stb::Writer FishingMenuScript() {
    constexpr int kMessage = 0, kCount = 1;
    stb::Writer   w;
    w.SetArguments(2);
    MenuBody(
        w, [&] { w.PushLocal(kMessage); },
        [&] {
            w.PushLocal(kCount);
            w.PushInt(1);
            w.Subtract();
        },
        2, 3, 4, 5, 6);
    w.PushLocal(2);
    w.Return();
    return w;
}

stb::Writer FishingEntryScript(const FishingSpot &spot, std::size_t menu) {
    constexpr int kPad = 0, kChoice = 2, kScratchPad = 3;
    stb::Writer   w;
    w.UseLocals(2);

    // The trigger runs this every frame the player stands at the sign: the "!" and Cross, without a yield until Cross,
    // so it stays a simple event and the player keeps walking.
    Command(w, kDrawExclamationMark);
    w.PushInt(kGetPadDown);
    w.PushAddress(kPad);
    w.Ext(2);
    w.PushLocal(kPad);
    w.PushInt(kPadCross);
    w.And();
    int idle = w.MarkForward();
    w.JumpFalse(idle);

    Command(w, kSetNpcMotion, {-1, 0});
    Menu(w, kFishingEntryMessage, 4, kChoice, menu);
    // Fish (0) goes on; Exchange FP (1) and Fishing log (2) open their menus; Quit (3) ends.
    int fish = w.MarkForward();
    w.PushLocal(kChoice);
    w.PushInt(0);
    w.Compare(stb::kEqual);
    w.JumpTrue(fish);
    for (auto [choice, command] : {
             std::pair{1, kGotoFpChange   },
             std::pair{2, kGotoFishRanking}
    }) {
        int other = w.MarkForward();
        w.PushLocal(kChoice);
        w.PushInt(choice);
        w.Compare(stb::kEqual);
        w.JumpFalse(other);
        CloseWindow(w);
        Command(w, command);
        w.Yield();
        End(w);
        w.Place(other);
    }
    CloseWindow(w);
    End(w);
    w.Place(fish);

    w.PushInt(kSItemCheck);
    w.PushInt(kFishingRod);
    w.PushAddress(kChoice);
    w.Ext(3);
    int have_rod = w.MarkForward();
    w.PushLocal(kChoice);
    w.PushInt(0);
    w.Compare(stb::kLessThan);
    w.JumpFalse(have_rod);
    ShowMessage(w, kFishingNoRodMessage, kScratchPad);
    End(w);
    w.Place(have_rod);
    CloseWindow(w);

    // Behind black: the villagers go (their memory takes the fishing Toan), the event arena starts over, and the
    // fishing Toan, the spot and its fish load.
    Command(w, kFadeOut, {30});
    WaitFor(w, kCheckFade, true);
    Command(w, kInitialize);
    Command(w, kClearVillagerBuff);
    Command(w, kClearEventBuff);
    LoadModel(w, kFishingModel, kFishingModelCfg, 1);
    w.Yield();
    w.PushInt(kLoadFishingData);
    w.PushInt(spot.area);
    for (float value : {spot.x1, spot.z1, spot.x2, spot.z2, spot.water, spot.ground}) {
        w.PushFloat(value);
    }
    w.Ext(8);
    w.PushInt(kInitFish);
    for (float value : {spot.x1, spot.z1, spot.x2, spot.z2}) {
        w.PushFloat(value);
    }
    w.Ext(5);
    WorldCoordinates(w);
    FloatCommand(w, kSetNpcPos, -1, {spot.stance[0], spot.stance[1], spot.stance[2]});
    FloatCommand(w, kSetNpcRot, -1, {0.0f, spot.facing, 0.0f});
    Command(w, kGotoFishing);
    Command(w, kFadeIn, {60});

    w.Place(idle);
    End(w);
    return w;
}

stb::Writer FishingQuitScript(std::size_t menu) {
    constexpr int kPosition = 2, kRotation = 5, kChoice = 8;
    stb::Writer   w;
    w.Yield();
    w.Yield();
    Command(w, kSetNpcMotion, {-1, 0});
    Menu(w, kFishingQuitMessage, 2, kChoice, menu);
    // Continue (0) keeps fishing; Quit (1) goes on.
    int quit = w.MarkForward();
    w.PushLocal(kChoice);
    w.PushInt(0);
    w.Compare(stb::kEqual);
    w.JumpFalse(quit);
    CloseWindow(w);
    Command(w, kSetReturnCode, {kKeepFishing});
    w.Yield();
    End(w);
    w.Place(quit);
    CloseWindow(w);

    // Behind black: Toan back where the player stands (loading him resets his place), the session over, the
    // villagers back.
    Command(w, kFadeOut, {30});
    WaitFor(w, kCheckFade, true);
    w.UseLocals(kChoice);
    WorldCoordinates(w);
    for (auto [command, first] : {
             std::pair{kGetNpcPos, kPosition},
             std::pair{kGetNpcRot, kRotation}
    }) {
        w.PushInt(command);
        w.PushInt(-1);
        for (int k = 0; k < 3; k++) {
            w.PushAddressFloat(first + k);
        }
        w.Ext(5);
    }
    LoadModel(w, kPlayerModel, kPlayerModelCfg, 0);
    WorldCoordinates(w);
    for (auto [command, first] : {
             std::pair{kSetNpcPos, kPosition},
             std::pair{kSetNpcRot, kRotation}
    }) {
        w.PushInt(command);
        w.PushInt(-1);
        for (int k = 0; k < 3; k++) {
            w.PushLocalFloat(first + k);
        }
        w.Ext(5);
    }
    Command(w, kExitFishing);
    Command(w, kLoadVillager);
    WaitFor(w, kLoadSync, false);
    Command(w, kFadeIn, {60});
    End(w);
    return w;
}

stb::Writer FishingBaitScript() {
    constexpr int kBait = 0;
    stb::Writer   w;
    w.UseLocals(1);
    w.Yield();
    w.PushInt(kGotoChangeEsa);
    w.PushAddress(kBait);
    w.Ext(2);
    w.Yield();
    // A bait chosen (the menu leaves 0 on a cancel): its model loaded and hung on the hook.
    w.PushLocal(kBait);
    w.PushInt(0);
    w.Compare(stb::kGreaterThan);
    int cancelled = w.MarkForward();
    w.JumpFalse(cancelled);
    w.PushInt(kLoadItemFile);
    w.PushLocal(kBait);
    w.Ext(2);
    WaitFor(w, kLoadSync, false);
    Command(w, kClearEventBuff);
    Command(w, kActiveFileBuffer, {0, 0});
    Command(w, kLoadItem, {0});
    w.Yield();
    w.PushInt(kSetFishingEsa);
    w.PushLocal(kBait);
    w.Ext(2);
    w.Place(cancelled);
    // Back to fishing: a fishing event that just returns ends the session.
    Command(w, kGotoFishing);
    End(w);
    return w;
}

Bytes AddFishingScripts(std::span<const unsigned char> script, const FishingSpot &spot) {
    for (std::uint32_t id : {kFishingMenuLabel, static_cast<std::uint32_t>(kFishingSignLabel), kFishingQuitLabel, kFishingBaitLabel}) {
        if (stb::Label(script, id)) {
            Bad(std::format("the {} event script has a label {} already", spot.code, id));
        }
    }
    stb::Appender appender(script);
    std::size_t   menu = appender.Next();
    appender.Add(kFishingMenuLabel, FishingMenuScript());
    appender.Add(kFishingSignLabel, FishingEntryScript(spot, menu));
    appender.Add(kFishingQuitLabel, FishingQuitScript(menu));
    appender.Add(kFishingBaitLabel, FishingBaitScript());
    return appender.Build();
}

} // namespace expanded
