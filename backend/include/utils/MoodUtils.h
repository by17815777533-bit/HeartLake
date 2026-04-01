#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace heartlake::utils {

inline std::string trimMood(std::string value) {
    const auto begin = value.find_first_not_of(" \t\n\r");
    if (begin == std::string::npos) {
        return "";
    }
    const auto end = value.find_last_not_of(" \t\n\r");
    return value.substr(begin, end - begin + 1);
}

inline std::string lowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

inline const std::unordered_map<std::string, std::string>& moodAliases() {
    static const std::unordered_map<std::string, std::string> aliases = {
        {"joy", "happy"},
        {"love", "happy"},
        {"happiness", "happy"},
        {"excited", "happy"},
        {"sadness", "sad"},
        {"depressed", "sad"},
        {"fear", "anxious"},
        {"fearful", "anxious"},
        {"stressed", "anxious"},
        {"worried", "anxious"},
        {"anger", "angry"},
        {"surprise", "surprised"},
        {"peaceful", "calm"},
        {"gratitude", "grateful"},
        {"uncertain", "confused"},
    };
    return aliases;
}

inline const std::unordered_set<std::string>& supportedMoods() {
    static const std::unordered_set<std::string> moods = {
        "happy", "calm", "neutral", "anxious", "sad", "angry",
        "surprised", "confused", "hopeful", "grateful", "lonely",
    };
    return moods;
}

inline std::string normalizeMood(std::string moodRaw,
                                 const std::string& fallback = "neutral") {
    auto mood = lowerAscii(trimMood(std::move(moodRaw)));
    if (mood.empty()) {
        return fallback;
    }

    const auto aliasIt = moodAliases().find(mood);
    if (aliasIt != moodAliases().end()) {
        mood = aliasIt->second;
    }

    if (supportedMoods().contains(mood)) {
        return mood;
    }
    return fallback;
}

inline bool isSupportedMood(const std::string& moodRaw) {
    return supportedMoods().contains(normalizeMood(moodRaw, ""));
}

inline double scoreForMood(const std::string& moodRaw, double fallback = 0.0) {
    const auto mood = normalizeMood(moodRaw, "");
    if (mood.empty()) {
        return fallback;
    }
    if (mood == "happy") return 0.75;
    if (mood == "calm") return 0.35;
    if (mood == "hopeful") return 0.55;
    if (mood == "grateful") return 0.65;
    if (mood == "surprised") return 0.45;
    if (mood == "neutral") return 0.0;
    if (mood == "confused") return -0.10;
    if (mood == "anxious") return -0.45;
    if (mood == "lonely") return -0.55;
    if (mood == "angry") return -0.65;
    if (mood == "sad") return -0.75;
    return fallback;
}

inline std::string sqlCanonicalMoodExpr(const std::string& rawExpr) {
    const std::string normalized = "LOWER(BTRIM(COALESCE(" + rawExpr + ", 'neutral')))";
    return "(CASE "
           "WHEN " + normalized + " IN ('joy', 'love', 'happiness', 'excited', 'happy') THEN 'happy' "
           "WHEN " + normalized + " IN ('sadness', 'depressed', 'sad') THEN 'sad' "
           "WHEN " + normalized + " IN ('fear', 'fearful', 'stressed', 'worried', 'anxious') THEN 'anxious' "
           "WHEN " + normalized + " IN ('anger', 'angry') THEN 'angry' "
           "WHEN " + normalized + " IN ('surprise', 'surprised') THEN 'surprised' "
           "WHEN " + normalized + " IN ('peaceful', 'calm') THEN 'calm' "
           "WHEN " + normalized + " IN ('gratitude', 'grateful') THEN 'grateful' "
           "WHEN " + normalized + " IN ('uncertain', 'confused') THEN 'confused' "
           "WHEN " + normalized + " = 'hopeful' THEN 'hopeful' "
           "WHEN " + normalized + " = 'lonely' THEN 'lonely' "
           "WHEN " + normalized + " = 'neutral' THEN 'neutral' "
           "ELSE 'neutral' END)";
}

inline std::string sqlMoodScoreExpr(const std::string& moodExpr,
                                    const std::string& emotionScoreExpr = "") {
    const std::string canonicalMoodExpr = sqlCanonicalMoodExpr(moodExpr);
    const std::string fallbackScoreExpr =
        "(CASE " + canonicalMoodExpr +
        " WHEN 'happy' THEN 0.75"
        " WHEN 'calm' THEN 0.35"
        " WHEN 'hopeful' THEN 0.55"
        " WHEN 'grateful' THEN 0.65"
        " WHEN 'surprised' THEN 0.45"
        " WHEN 'neutral' THEN 0.0"
        " WHEN 'confused' THEN -0.10"
        " WHEN 'anxious' THEN -0.45"
        " WHEN 'lonely' THEN -0.55"
        " WHEN 'angry' THEN -0.65"
        " WHEN 'sad' THEN -0.75"
        " ELSE 0.0 END)";
    if (emotionScoreExpr.empty()) {
        return fallbackScoreExpr;
    }
    return "COALESCE(" + emotionScoreExpr + ", " + fallbackScoreExpr + ")";
}

}  // namespace heartlake::utils
