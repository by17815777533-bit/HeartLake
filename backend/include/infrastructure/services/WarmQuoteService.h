/**
 * @brief 暖心语录服务 -- 根据场景和情绪状态选择合适的关怀语录
 *
 * @details
 * 维护一套按场景分类的暖心语录库，在用户处于特定情境时
 * 随机抽取一条贴合当前状态的关怀文案。支持 15 种场景识别：
 *
 * - 时间维度：深夜发石头、周末深夜、凌晨无人回应
 * - 情绪维度：连续负面情绪、情绪突然恶化
 * - 社交维度：连续被忽视、节假日/生日独自在线
 * - 生命周期：首次发石头、长期未登录后回归、不活跃用户召回
 * - 回访维度：干预后第 1/3/7 天的持续关怀
 *
 * detectScene() 是纯函数，根据时间、社交、情绪等多维信号
 * 自动判定当前最匹配的场景。WarmMessage 中可选附带 VIP 引导，
 * 在用户情绪低落时温和推荐 VIP 心理咨询等增值服务。
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <random>

namespace heartlake::infrastructure {

/// 暖心语录触发场景
enum class WarmScene {
    LateNight,           ///< 深夜发石头 23:00-5:00
    NoResponse,          ///< 凌晨无人回应
    ContinuousNegative,  ///< 连续多天负面情绪
    HolidayAlone,        ///< 节假日独自在线
    BirthdayAlone,       ///< 生日无人祝福
    ReturnAfterLong,     ///< 长时间未登录后回归
    FirstStone,          ///< 首次发石头
    ConsecutiveIgnored,  ///< 连续被忽视
    SuddenWorsen,        ///< 情绪突然恶化
    WeekendNight,        ///< 周末深夜
    FollowUpDay1,        ///< 回访第 1 天
    FollowUpDay3,        ///< 回访第 3 天
    FollowUpDay7,        ///< 回访第 7 天
    Inactive,            ///< 不活跃用户召回
    Default              ///< 默认兜底
};

class WarmQuoteService {
public:
    static WarmQuoteService& getInstance();

    /// 暖心消息结构
    struct WarmMessage {
        std::string title;       ///< 消息标题
        std::string content;     ///< 暖心语录正文
        std::string vipGuide;    ///< VIP 引导文案（可选）
        bool needVipGuide;       ///< 是否需要展示 VIP 引导
    };

    /**
     * @brief 根据情绪分数获取暖心语录
     * @param emotionScore 情绪分数 [-1, 1]，越低越需要关怀
     * @return 包含标题、正文和可选 VIP 引导的暖心消息
     */
    WarmMessage getQuoteForBurden(float emotionScore);

    /**
     * @brief 根据场景获取暖心语录
     * @param scene 触发场景
     * @return 从该场景语录库中随机抽取的暖心消息
     */
    WarmMessage getQuoteForScene(WarmScene scene);

    /**
     * @brief 根据多维信号自动检测当前场景（纯函数）
     * @param hour 当前小时 (0-23)
     * @param isHoliday 是否节假日
     * @param isBirthday 是否用户生日
     * @param daysSinceLastLogin 距上次登录的天数
     * @param stoneCount 用户历史石头总数
     * @param noResponseCount 连续无回应次数
     * @param emotionDelta 近期情绪变化量（负值表示恶化）
     * @return 最匹配的场景枚举
     */
    static WarmScene detectScene(int hour, bool isHoliday, bool isBirthday,
                                  int daysSinceLastLogin, int stoneCount,
                                  int noResponseCount, float emotionDelta);

private:
    WarmQuoteService();
    std::mt19937 rng_;                                        ///< 随机数生成器
    std::map<WarmScene, std::vector<std::string>> sceneQuotes_;  ///< 场景 -> 语录列表
    void initSceneQuotes();  ///< 初始化各场景的语录库
};

} // namespace heartlake::infrastructure
