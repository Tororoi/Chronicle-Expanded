#include <gtest/gtest.h>

#include <cstring>

#include "expanded/data/gemron_models.hpp"
#include "expanded/enemies/monsters.hpp"
#include "expanded/enemies/species.hpp"
#include "itemdata.hpp"
#include "monstorunit.hpp"

using expanded::Monster;
using expanded::No;

TEST(AtlaGemronSpecies, IsHolyGemronsRecordWithItsOwnModelScriptAndElements) {
    const MONSTOR_MODEL *atla = expanded::Species(No(Monster::ATLA_GEMRON));
    ASSERT_NE(atla, nullptr);
    const MONSTOR_MODEL &holy = *expanded::Species(No(Monster::GEMRON_HOLY));
    EXPECT_STREQ(atla->model_name[0], "e169a");
    EXPECT_STREQ(atla->script_name, holy.script_name);
    EXPECT_EQ(atla->attachment_weight[3], 150);
    for (int i : {0, 1, 2, 4}) {
        EXPECT_EQ(atla->attachment_weight[i], 0) << i;
    }
    EXPECT_EQ(atla->exp, 40);
    EXPECT_EQ(atla->steal_item, ITEM_DRAN_S_FEATHER);
    EXPECT_EQ(atla->rare_item, ITEM_POTATO_CAKE);
    EXPECT_EQ(atla->name_no, 322);
    EXPECT_EQ(expanded::SpeciesStandIn(No(Monster::ATLA_GEMRON)), No(Monster::GEMRON_HOLY));
    std::string script = "dun/monstor/e115a.stb";
    expanded::SpeciesScriptFile(script, No(Monster::ATLA_GEMRON));
    EXPECT_STREQ(script.c_str(), expanded::kAtlaGemronScriptFile);
}
