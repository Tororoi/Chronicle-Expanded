#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cctype>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>

#include "expanded/quests/quests.hpp"

namespace expanded::test {

namespace {

// What a translation must keep as the English has it: every {code} and every %N.
std::string Tokens(const std::string &text) {
    std::string out;
    for (size_t at = 0; at < text.size(); at++) {
        if (text[at] == '%' && at + 1 < text.size() && std::isdigit(static_cast<unsigned char>(text[at + 1]))) {
            out += text.substr(at, 2);
        } else if (text[at] == '{' && text.find('}', at) != std::string::npos) {
            out += text.substr(at, text.find('}', at) - at + 1);
        }
    }
    return out;
}

TEST(QuestText, EveryLanguageHasEveryLine) {
    std::filesystem::path lang = std::filesystem::path(__FILE__).parent_path();
    while (!std::filesystem::exists(lang / "lang") && lang.has_parent_path() && lang != lang.parent_path()) {
        lang = lang.parent_path();
    }
    lang /= "lang";

    std::set<std::string> keys;
    for (const dc::quest::Text &text : QuestTexts()) {
        EXPECT_TRUE(keys.insert(std::string(text.key)).second) << "two texts called " << text.key;
        EXPECT_TRUE(text.key.starts_with("expanded.quest.")) << text.key;
    }

    for (const char *name : {"fr_fr", "de_de", "it_it", "es_es"}) {
        std::ifstream stream(lang / (std::string(name) + ".json"));
        ASSERT_TRUE(stream) << name;
        const auto shipped = nlohmann::json::parse(stream);
        for (const dc::quest::Text &text : QuestTexts()) {
            const std::string key(text.key);
            if (!shipped.contains(key)) {
                ADD_FAILURE() << name << " lacks " << key;
                continue;
            }
            const std::string translated = shipped[key];
            EXPECT_FALSE(translated.empty()) << name << " " << key;
            EXPECT_EQ(Tokens(translated), Tokens(std::string(text.english))) << name << " " << key;
        }
        for (const auto &item : shipped.items()) {
            if (item.key().starts_with("expanded.quest.")) {
                EXPECT_TRUE(keys.contains(item.key())) << name << " has " << item.key() << " that the quests do not";
            }
        }
    }
}

} // namespace

} // namespace expanded::test
