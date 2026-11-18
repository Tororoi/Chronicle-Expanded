#include "expanded/quests/givers.hpp"

#include <format>
#include <optional>
#include <string>
#include <utility>

#include "clsmes.hpp"
#include "expanded/core/menu_text.hpp"
#include "expanded/debug/event_log.hpp"
#include "localize/localize.hpp"
#include "sysmes.hpp"

namespace expanded {

namespace {

constexpr int kNoticeFrames = 240;

std::string notice;

std::optional<std::string> NoticeLine() {
    return notice.empty() ? std::nullopt : std::optional<std::string>(notice);
}

[[maybe_unused]] const bool kNoticeLine = (SetSystemMessage(kQuestNoticeMessage, NoticeLine), true);

} // namespace

dc::quest::Builder &Offers(dc::quest::Builder &quest, const Giver &giver, std::initializer_list<std::string_view> offered,
                           dc::quest::SceneFn offer, std::initializer_list<std::string_view> ongoing,
                           dc::quest::SceneFn about) {
    return quest.Option(giver.Match(), dc::quest::AtStages(offered), kOfferLabel, std::move(offer))
        .Option(giver.Match(), dc::quest::AtStages(ongoing), kAboutLabel, std::move(about));
}

std::string QuestText(dc::quest::Text text) {
    return LocalizeText(text.key, text.english);
}

void LogQuest(const dc::quest::Context &ctx) {
    if (EventLogOn()) {
        LogEvent("quest", std::format("\"quest\":\"{}\",\"stage\":\"{}\",\"left\":{}", ctx.QuestId(), ctx.Stage(),
                                      ctx.Var("left")));
    }
}

void QuestNotice(std::string text) {
    notice = std::move(text);
    // The window needs the system message bank, which a test has not loaded.
    if (SystemMessageWindow().buff != nullptr) {
        SetSystemMes(kQuestNoticeMessage, kNoticeFrames, MES_POS_BOTTOM, 0, nullptr, nullptr);
    }
}

const std::string &QuestNoticeShown() {
    return notice;
}

} // namespace expanded
