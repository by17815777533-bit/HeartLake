/**
 * @file WarmQuoteService.h
 * @brief 暖心语录服务 - 根据场景和情绪状态选择合适的关怀语录
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <random>

namespace heartlake::infrastructure {

enum class WarmScene {
    LateNight,           // 深夜发石头 23:00-5:00
    NoResponse,          // 凌晨无人回应
    ContinuousNegative,  // 连续多天负面情绪
    HolidayAlone,        // 节假日独自在线
    BirthdayAlone,       // 生日无人祝福
    ReturnAfterLong,     // 长时间未登录后回归
    FirstStone,          // 首次发石头
    ConsecutiveIgnored,  // 连续被忽视
    SuddenWorsen,        // 情绪突然恶化
    WeekendNight,        // 周末深夜
    FollowUpDay1,        // 回访第1天
    FollowUpDay3,        // 回访第3天
    FollowUpDay7,        // 回访第7天
    Inactive,            // 不活跃用户召回
    Default              // 默认
};

class WarmQuoteService {
public:
    static WarmQuoteService& getInstance();

    struct WarmMessage {
        std::string title;
        std::string content;
        std::string vipGuide;
        bool needVipGuide;
    };

    WarmMessage getQuoteForBurden(float emotionScore);
    WarmMessage getQuoteForScene(WarmScene scene);
    static WarmScene detectScene(int hour, bool isHoliday, bool isBirthday,
                                  int daysSinceLastLogin, int stoneCount,
                                  int noResponseCount, float emotionDelta);

private:
    WarmQuoteService();
    std::mt19937 rng_;
    std::map<WarmScene, std::vector<std::string>> sceneQuotes_;
    void initSceneQuotes();
};

} // namespace heartlake::infrastructure
