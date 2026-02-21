// @file mood_colors.dart
// @brief 情绪色彩映射系统 - 光遇温馨风格
// Created by 王璐瑶

library;

import 'package:flutter/material.dart';

/// 情绪类型枚举
enum MoodType {
  happy, // 开心
  calm, // 平静
  sad, // 悲伤
  anxious, // 焦虑
  angry, // 愤怒
  surprised, // 惊喜
  confused, // 迷茫
  neutral, // 中性（默认）
}

/// 情绪色彩配置
class MoodColorConfig {
  /// 主色调
  final Color primary;

  /// 背景渐变起始色
  final Color gradientStart;

  /// 背景渐变结束色
  final Color gradientEnd;

  /// 湖面颜色
  final Color lakeColor;

  /// 粒子/波纹颜色
  final Color rippleColor;

  /// 文字颜色
  final Color textColor;

  /// 图标颜色
  final Color iconColor;

  /// 卡片背景色
  final Color cardColor;

  /// 情绪名称
  final String name;

  /// 情绪描述
  final String description;

  /// 情绪图标
  final IconData icon;

  const MoodColorConfig({
    required this.primary,
    required this.gradientStart,
    required this.gradientEnd,
    required this.lakeColor,
    required this.rippleColor,
    required this.textColor,
    required this.iconColor,
    required this.cardColor,
    required this.name,
    required this.description,
    required this.icon,
  });
}

/// 情绪色彩管理器
class MoodColors {
  MoodColors._();

  /// 情绪色彩映射表 - 光遇暖色体系
  static const Map<MoodType, MoodColorConfig> _colorConfigs = {
    // 开心 - 烛光金
    MoodType.happy: MoodColorConfig(
      primary: Color(0xFFF2CC8F),
      gradientStart: Color(0xFFFFF9ED),
      gradientEnd: Color(0xFFFFE8C7),
      lakeColor: Color(0xFFE8A87C),
      rippleColor: Color(0xFFF2CC8F),
      textColor: Color(0xFF6A4B21),
      iconColor: Color(0xFFD08D53),
      cardColor: Color(0xFFFFF5E8),
      name: '开心',
      description: '阳光灿烂的心情',
      icon: Icons.sentiment_very_satisfied,
    ),

    // 平静 - 晨岛湖蓝
    MoodType.calm: MoodColorConfig(
      primary: Color(0xFF7BA3C4),
      gradientStart: Color(0xFFEFF6FC),
      gradientEnd: Color(0xFFD8E8F6),
      lakeColor: Color(0xFF5A83A4),
      rippleColor: Color(0xFF7BA3C4),
      textColor: Color(0xFF2D4863),
      iconColor: Color(0xFF4A7394),
      cardColor: Color(0xFFF1F7FC),
      name: '平静',
      description: '内心安宁',
      icon: Icons.spa,
    ),

    // 悲伤 - 雨林蓝灰
    MoodType.sad: MoodColorConfig(
      primary: Color(0xFF6B7A8E),
      gradientStart: Color(0xFFE8ECF1),
      gradientEnd: Color(0xFFC9D3DF),
      lakeColor: Color(0xFF4A5568),
      rippleColor: Color(0xFF6B7A8E),
      textColor: Color(0xFF2D3748),
      iconColor: Color(0xFF4A5568),
      cardColor: Color(0xFFEFF3F8),
      name: '悲伤',
      description: '需要温暖的陪伴',
      icon: Icons.sentiment_dissatisfied,
    ),

    // 焦虑 - 暮土淡紫
    MoodType.anxious: MoodColorConfig(
      primary: Color(0xFF9B7EBE),
      gradientStart: Color(0xFFF5EEF9),
      gradientEnd: Color(0xFFE1D0EF),
      lakeColor: Color(0xFF7B68AE),
      rippleColor: Color(0xFF9B7EBE),
      textColor: Color(0xFF4A3A64),
      iconColor: Color(0xFF7B68AE),
      cardColor: Color(0xFFF7F0FB),
      name: '焦虑',
      description: '心中有些许不安',
      icon: Icons.psychology,
    ),

    // 愤怒 - 日落赤橙
    MoodType.angry: MoodColorConfig(
      primary: Color(0xFFE07A5F),
      gradientStart: Color(0xFFFFF1ED),
      gradientEnd: Color(0xFFFFD8CF),
      lakeColor: Color(0xFFCC5A3F),
      rippleColor: Color(0xFFE07A5F),
      textColor: Color(0xFF7E2F1E),
      iconColor: Color(0xFFCC5A3F),
      cardColor: Color(0xFFFFF4F0),
      name: '愤怒',
      description: '需要释放的情绪',
      icon: Icons.sentiment_very_dissatisfied,
    ),

    // 惊喜 - 星光青绿
    MoodType.surprised: MoodColorConfig(
      primary: Color(0xFF7BC9D6),
      gradientStart: Color(0xFFEAF9FB),
      gradientEnd: Color(0xFFD3F0F5),
      lakeColor: Color(0xFF4EA7B7),
      rippleColor: Color(0xFF7BC9D6),
      textColor: Color(0xFF2A5E67),
      iconColor: Color(0xFF4EA7B7),
      cardColor: Color(0xFFEFFBFD),
      name: '惊喜',
      description: '意外的喜悦',
      icon: Icons.celebration,
    ),

    // 迷茫 - 禁阁雾紫
    MoodType.confused: MoodColorConfig(
      primary: Color(0xFF8E8EBE),
      gradientStart: Color(0xFFF0F0F8),
      gradientEnd: Color(0xFFDADAF0),
      lakeColor: Color(0xFF6E6E9E),
      rippleColor: Color(0xFF8E8EBE),
      textColor: Color(0xFF3C3C66),
      iconColor: Color(0xFF6E6E9E),
      cardColor: Color(0xFFF4F4FA),
      name: '迷茫',
      description: '寻找方向中',
      icon: Icons.help_outline,
    ),

    // 中性 - 柔和湖蓝
    MoodType.neutral: MoodColorConfig(
      primary: Color(0xFF7BA3C4),
      gradientStart: Color(0xFFF5F8FC),
      gradientEnd: Color(0xFFE4ECF6),
      lakeColor: Color(0xFF6B93B4),
      rippleColor: Color(0xFF7BA3C4),
      textColor: Color(0xFF2B4A68),
      iconColor: Color(0xFF5A83A4),
      cardColor: Color(0xFFF7FAFD),
      name: '中性',
      description: '平常的一天',
      icon: Icons.sentiment_neutral,
    ),
  };

  /// 获取情绪色彩配置
  static MoodColorConfig getConfig(MoodType mood) {
    return _colorConfigs[mood] ?? _colorConfigs[MoodType.neutral]!;
  }

  /// 根据字符串获取情绪类型
  static MoodType fromString(String? mood) {
    if (mood == null || mood.isEmpty) return MoodType.neutral;

    switch (mood.toLowerCase()) {
      case 'happy':
      case '开心':
      case '快乐':
      case '高兴':
        return MoodType.happy;
      case 'calm':
      case '平静':
      case '安宁':
      case '宁静':
        return MoodType.calm;
      case 'sad':
      case '悲伤':
      case '难过':
      case '伤心':
      case '忧伤':
        return MoodType.sad;
      case 'anxious':
      case '焦虑':
      case '担忧':
      case '紧张':
        return MoodType.anxious;
      case 'angry':
      case '愤怒':
      case '生气':
        return MoodType.angry;
      case 'surprised':
      case '惊喜':
      case '惊讶':
        return MoodType.surprised;
      case 'confused':
      case '迷茫':
      case '困惑':
        return MoodType.confused;
      default:
        return MoodType.neutral;
    }
  }

  /// 根据情感分数推断情绪类型
  /// sentimentScore: -1.0 到 1.0，负数为消极，正数为积极
  static MoodType fromSentimentScore(double score) {
    if (score >= 0.6) return MoodType.happy;
    if (score >= 0.3) return MoodType.calm;
    if (score >= 0.0) return MoodType.neutral;
    if (score >= -0.3) return MoodType.confused;
    if (score >= -0.6) return MoodType.sad;
    return MoodType.angry;
  }

  /// 获取所有可用的情绪类型
  static List<MoodType> get allMoods => MoodType.values;

  /// 创建情绪背景渐变
  static LinearGradient createGradient(
    MoodType mood, {
    AlignmentGeometry begin = Alignment.topCenter,
    AlignmentGeometry end = Alignment.bottomCenter,
  }) {
    final config = getConfig(mood);
    return LinearGradient(
      begin: begin,
      end: end,
      colors: [
        config.gradientStart,
        config.gradientEnd,
      ],
    );
  }

  /// 创建完整的湖面渐变（天空 + 湖水）
  static LinearGradient createLakeGradient(MoodType mood) {
    final config = getConfig(mood);
    return LinearGradient(
      begin: Alignment.topCenter,
      end: Alignment.bottomCenter,
      colors: [
        config.gradientStart,
        config.gradientEnd,
        config.lakeColor.withValues(alpha: 0.6),
        config.lakeColor,
        config.lakeColor.withValues(alpha: 0.8),
      ],
      stops: const [0.0, 0.35, 0.45, 0.7, 1.0],
    );
  }
}
