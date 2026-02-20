/**
 * @file WarmQuoteService.cpp
 * @brief 暖心语录服务实现 - 多场景触发
 */

#include "infrastructure/services/WarmQuoteService.h"
#include <chrono>

namespace heartlake::infrastructure {

WarmQuoteService::WarmQuoteService()
    : rng_(std::chrono::steady_clock::now().time_since_epoch().count()) {
    initSceneQuotes();
}

WarmQuoteService& WarmQuoteService::getInstance() {
    static WarmQuoteService instance;
    return instance;
}

void WarmQuoteService::initSceneQuotes() {
    sceneQuotes_[WarmScene::LateNight] = {
        "夜深了，还有人在听你说话。",
        "深夜的湖底，心湖都懂。",
        "夜色温柔，愿你安眠。"
    };
    sceneQuotes_[WarmScene::NoResponse] = {
        "湖面很静，但你的声音已被记住。",
        "有些回响需要时间，别着急。",
        "涟漪正在传递，耐心等待。"
    };
    sceneQuotes_[WarmScene::ContinuousNegative] = {
        "生活负重前行，但会有人给你点灯。",
        "你不是一个人在承受，我们一直都在。",
        "再难的日子也会过去，让我们陪你走过这段路。"
    };
    sceneQuotes_[WarmScene::HolidayAlone] = {
        "节日里，心湖陪你一起。",
        "团圆的日子，你并不孤单。",
        "节日快乐，愿温暖常伴。"
    };
    sceneQuotes_[WarmScene::BirthdayAlone] = {
        "生日快乐，愿你被世界温柔以待。",
        "又长大一岁，愿所有美好如期而至。",
        "今天是属于你的日子，心湖为你庆祝。"
    };
    sceneQuotes_[WarmScene::ReturnAfterLong] = {
        "欢迎回来，心湖一直在这里等你。",
        "好久不见，想你了。",
        "你回来了，真好。"
    };
    sceneQuotes_[WarmScene::FirstStone] = {
        "勇敢迈出第一步，你不是一个人。",
        "欢迎来到心湖，这里有人愿意倾听。",
        "第一颗石头，愿它带给你温暖的回响。"
    };
    sceneQuotes_[WarmScene::ConsecutiveIgnored] = {
        "有些声音需要时间被听见，别放弃。",
        "你的故事值得被倾听，再等等。",
        "涟漪会来的，相信心湖。"
    };
    sceneQuotes_[WarmScene::SuddenWorsen] = {
        "我们注意到了，想陪你聊聊吗？",
        "发生什么了？我们在这里。",
        "无论发生什么，你都不必独自面对。"
    };
    sceneQuotes_[WarmScene::WeekendNight] = {
        "周末夜晚，给自己一个拥抱。",
        "放松一下，你值得休息。",
        "周末愉快，好好照顾自己。"
    };
    sceneQuotes_[WarmScene::Default] = {
        "每个人都有低落的时候，这很正常。",
        "给自己一点时间，慢慢来。",
        "今天辛苦了，记得好好休息。"
    };
    sceneQuotes_[WarmScene::FollowUpDay1] = {
        "昨天的你辛苦了，今天有没有好一点？心湖一直在这里。"
    };
    sceneQuotes_[WarmScene::FollowUpDay3] = {
        "三天过去了，希望你的心情有所好转。如果还需要倾诉，我们随时都在。"
    };
    sceneQuotes_[WarmScene::FollowUpDay7] = {
        "时间会治愈一切，但在那之前，记得你不是一个人。免费心理咨询仍然为你保留。"
    };
    sceneQuotes_[WarmScene::Inactive] = {
        "心湖想你了，有空来看看吧。"
    };
}

WarmQuoteService::WarmMessage WarmQuoteService::getQuoteForBurden(float emotionScore) {
    if (emotionScore <= -0.7f) {
        return getQuoteForScene(WarmScene::ContinuousNegative);
    } else if (emotionScore <= -0.4f) {
        return getQuoteForScene(WarmScene::SuddenWorsen);
    }
    return getQuoteForScene(WarmScene::Default);
}

WarmQuoteService::WarmMessage WarmQuoteService::getQuoteForScene(WarmScene scene) {
    WarmMessage msg;
    msg.needVipGuide = false;

    auto it = sceneQuotes_.find(scene);
    if (it == sceneQuotes_.end()) {
        scene = WarmScene::Default;
        it = sceneQuotes_.find(scene);
    }

    const auto& quotes = it->second;
    std::uniform_int_distribution<size_t> dist(0, quotes.size() - 1);
    msg.content = quotes[dist(rng_)];

    switch (scene) {
        case WarmScene::ContinuousNegative:
        case WarmScene::SuddenWorsen:
        case WarmScene::ConsecutiveIgnored:
            msg.title = "心灵之灯已点亮";
            msg.vipGuide = "我们为你准备了一次免费心理咨询，随时可以预约。";
            msg.needVipGuide = true;
            break;
        case WarmScene::LateNight:
        case WarmScene::WeekendNight:
        case WarmScene::NoResponse:
            msg.title = "温馨提醒";
            break;
        case WarmScene::FollowUpDay1:
            msg.title = "今天感觉怎么样？";
            break;
        case WarmScene::FollowUpDay3:
            msg.title = "想你了";
            break;
        case WarmScene::FollowUpDay7:
            msg.title = "一周了，还好吗？";
            msg.vipGuide = "免费心理咨询仍然为你保留。";
            msg.needVipGuide = true;
            break;
        case WarmScene::Inactive:
            msg.title = "好久不见";
            break;
        default:
            msg.title = "心湖寄语";
            msg.vipGuide = "";
            break;
    }
    return msg;
}

WarmScene WarmQuoteService::detectScene(int hour, bool isHoliday, bool isBirthday,
                                         int daysSinceLastLogin, int stoneCount,
                                         int noResponseCount, float emotionDelta) {
    if (isBirthday) return WarmScene::BirthdayAlone;
    if (isHoliday) return WarmScene::HolidayAlone;
    if (daysSinceLastLogin >= 30) return WarmScene::ReturnAfterLong;
    if (stoneCount == 0) return WarmScene::FirstStone;
    if (emotionDelta < -0.5f) return WarmScene::SuddenWorsen;
    if (noResponseCount >= 3) return WarmScene::ConsecutiveIgnored;
    if (hour >= 23 || hour < 5) {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        struct tm tm_buf;
        localtime_r(&t, &tm_buf);
        int dayOfWeek = tm_buf.tm_wday; // 0=Sunday, 6=Saturday
        if (dayOfWeek == 0 || dayOfWeek == 6) return WarmScene::WeekendNight;
        return WarmScene::LateNight;
    }
    return WarmScene::Default;
}

} // namespace heartlake::infrastructure
