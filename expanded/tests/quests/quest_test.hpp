#pragma once

// A fixture for Expanded's quests: the quests on a fake game, and a villager's conversation through the dialog
// framework as the player has it.

#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <vector>

#include "dialog/framework.hpp"
#include "expanded/core/random.hpp"
#include "expanded/quests/givers.hpp"
#include "mainselect.hpp"
#include "quest/framework.hpp"
#include "tests/quest_fake_game.hpp"

namespace expanded::test {

class QuestTest : public ::testing::Test {
protected:
    void SetUp() override {
        LanguageCode = 2; // en_gb: the built-in English.
        dc::quest::Clear();
        dc::quest::SetGame(&game);
        dc::dialog::Shutdown();
        dc::dialog::Init();
        dc::dialog::SetTextMeasurer([](std::string_view text) { return 10.0f * static_cast<float>(text.size()); });
        dc::dialog::SetLayoutConfig(dc::dialog::LayoutConfig{});
        dc::dialog::RegisterOptionProvider(dc::quest::CollectOptions);
        SeedModRandom(1);
    }

    void TearDown() override {
        dc::dialog::Shutdown();
        dc::quest::SetGame(nullptr);
        dc::quest::Clear();
    }

    // Talks to the giver: their menu, as the dialog framework puts it together.
    static void Talk(const Giver &giver) {
        dc::dialog::NpcContext npc;
        npc.map_no = giver.map_no;
        npc.npc_id = giver.npc_id;
        npc.host_plays_original = true;
        ASSERT_TRUE(dc::dialog::TryInterceptOpen(npc, {})) << "no choice for the giver";
    }

    static const dc::dialog::ActiveView &View() {
        const dc::dialog::ActiveView       *view = dc::dialog::GetActiveView();
        static const dc::dialog::ActiveView none;
        return view != nullptr ? *view : none;
    }

    static std::vector<std::string> Labels() { return View().formatted_choice_labels; }

    // Whether talking to the giver offers a choice labelled label.
    static bool Offered(const Giver &giver, std::string_view label) {
        dc::dialog::NpcContext npc;
        npc.map_no = giver.map_no;
        npc.npc_id = giver.npc_id;
        std::vector<dc::dialog::ChoiceItem> out;
        dc::quest::CollectOptions(npc, out);
        for (const dc::dialog::ChoiceItem &item : out) {
            if (item.fallback_label == label) {
                return true;
            }
        }
        return false;
    }

    static void Pick(std::string_view label) {
        const std::vector<std::string> labels = Labels();
        int                            target = -1;
        for (size_t i = 0; i < labels.size(); ++i) {
            if (labels[i] == label) {
                target = static_cast<int>(i);
            }
        }
        ASSERT_GE(target, 0) << "no choice \"" << label << "\"";
        while (View().selected_choice != target) {
            ASSERT_TRUE(dc::dialog::InputMoveDown());
        }
        ASSERT_TRUE(dc::dialog::InputConfirm());
    }

    // The text of the page in view, every slice of it.
    static std::string Said() {
        std::string text;
        for (const std::string &line : View().current_slice.wrapped_markup_lines) {
            text += line + " ";
        }
        return text;
    }

    // Talks to the giver, takes the choice and gives back what they say first.
    std::string Ask(const Giver &giver, std::string_view label) {
        Talk(giver);
        Pick(label);
        std::string said = Said();
        dc::dialog::Shutdown();
        dc::dialog::Init();
        dc::dialog::SetTextMeasurer([](std::string_view text) { return 10.0f * static_cast<float>(text.size()); });
        dc::dialog::SetLayoutConfig(dc::dialog::LayoutConfig{});
        dc::dialog::RegisterOptionProvider(dc::quest::CollectOptions);
        return said;
    }

    static std::string Stage(std::string_view quest) {
        const dc::quest::Progress *progress = dc::quest::FindProgress(quest);
        return progress != nullptr ? progress->stage : "<none>";
    }

    static int Var(std::string_view quest, const char *name) {
        const dc::quest::Progress *progress = dc::quest::FindProgress(quest);
        if (progress == nullptr) {
            return -1;
        }
        const auto it = progress->vars.find(name);
        return it == progress->vars.end() ? 0 : it->second;
    }

    dc::test::FakeGame game;
};

} // namespace expanded::test
