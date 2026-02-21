// @file mood_colors.dart
// @brief 情绪色彩映射系统 - Material Design 3 风格
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

  /// 情绪色彩映射表 - M3 色彩体系
  static const Map<MoodType, MoodColorConfig> _colorConfigs = {
    // 开心 - M3 暖橙色调，明亮愉悦
    MoodType.happy: MoodColorConfig(
      primary: Color(0xFFFFB74D),
      gradientStart: Color(0xFFFFF8E1),
      gradientEnd: Color(0xFFFFE0B2),
      lakeColor: Color(0xFFF57C00),
      rippleColor: Color(0xFFFFB74D),
      textColor: Color(0xFFE65100),
      iconColor: Color(0xFFF57C00),
      cardColor: Color(0xFFFFF3E0),
      name: '开心',
      description: '阳光灿烂的心情',
      icon: Icons.sentiment_very_satisfied,
    ),

    // 平静 - M3 主色蓝，沉稳安宁
    MoodType.calm: MoodColorConfig(
      primary: Color(0xFF42A5F5),
      gradientStart: Color(0xFFE3F2FD),
      gradientEnd: Color(0xFFBBDEFB),
      lakeColor: Color(0xFF1565C0),
      rippleColor: Color(0xFF42A5F5),
      textColor: Color(0xFF0D47A1),
      iconColor: Color(0xFF1565C0),
      cardColor: Color(0xFFE3F2FD),
      name: '平静',
      description: '内心安宁',
      icon: Icons.spa,
    ),

    // 悲伤 - M3 蓝灰色调，低沉内敛
    MoodType.sad: MoodColorConfig(
      primary: Color(0xFF78909C),
      gradientStart: Color(0xFFECEFF1),
      gradientEnd: Color(0xFFCFD8DC),
      lakeColor: Color(0xFF455A64),
      rippleColor: Color(0xFF78909C),
      textColor: Color(0xFF263238),
      iconColor: Color(0xFF455A64),
      cardColor: Color(0xFFECEFF1),
      name: '悲伤',
      description: '需要温暖的陪伴',
      icon: Icons.sentiment_dissatisfied,
    ),

    // 焦虑 - M3 紫色调，微妙不安
    MoodType.anxious: MoodColorConfig(
      primary: Color(0xFFAB47BC),
      gradientStart: Color(0xFFF3E5F5),
      gradientEnd: Color(0xFFE1BEE7),
      lakeColor: Color(0xFF7B1FA2),
      rippleColor: Color(0xFFAB47BC),
      textColor: Color(0xFF4A148C),
      iconColor: Color(0xFF7B1FA2),
      cardColor: Color(0xFFF3E5F5),
      name: '焦虑',
      description: '心中有些许不安',
      icon: Icons.psychology,
    ),

    // 愤怒 - M3 红色调，强烈鲜明
    MoodType.angry: MoodColorConfig(
      primary: Color(0xFFEF5350),
      gradientStart: Color(0xFFFFEBEE),
      gradientEnd: Color(0xFFFFCDD2),
      lakeColor: Color(0xFFC62828),
      rippleColor: Color(0xFFEF5350),
      textColor: Color(0xFFB71C1C),
      iconColor: Color(0xFFC62828),
      cardColor: Color(0xFFFFEBEE),
      name: '愤怒',
      description: '需要释放的情绪',
      icon: Icons.sentiment_very_dissatisfied,
    ),

    // 惊喜 - M3 青绿色调，清新活力
    MoodType.surprised: MoodColorConfig(
      primary: Color(0xFF26A69A),
      gradientStart: Color(0xFFE0F2F1),
      gradientEnd: Color(0xFFB2DFDB),
      lakeColor: Color(0xFF00796B),
      rippleColor: Color(0xFF26A69A),
      textColor: Color(0xFF004D40),
      iconColor: Color(0xFF00796B),
      cardColor: Color(0xFFE0F2F1),
      name: '惊喜',
      description: '意外的喜悦',
      icon: Icons.celebration,
    ),

    // 迷茫 - M3 深紫色调，朦胧迷离
    MoodType.confused: MoodColorConfig(
      primary: Color(0xFF7E57C2),
      gradientStart: Color(0xFFEDE7F6),
      gradientEnd: Color(0xFFD1C4E9),
      lakeColor: Color(0xFF512DA8),
      rippleColor: Color(0xFF7E57C2),
      textColor: Color(0xFF311B92),
      iconColor: Color(0xFF512DA8),
      cardColor: Color(0xFFEDE7F6),
      name: '迷茫',
      description: '寻找方向中',
      icon: Icons.help_outline,
    ),

    // 中性 - M3 主色蓝，平和稳定
    MoodType.neutral: MoodColorConfig(
      primary: Color(0xFF1565C0),
      gradientStart: Color(0xFFE3F2FD),
      gradientEnd: Color(0xFFBBDEFB),
      lakeColor: Color(0xFF0D47A1),
      rippleColor: Color(0xFF1565C0),
      textColor: Color(0xFF0D47A1),
      iconColor: Color(0xFF1565C0),
      cardColor: Color(0xFFE3F2FD),
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
