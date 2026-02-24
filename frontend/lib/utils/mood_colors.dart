// @file mood_colors.dart
// @brief 情绪色彩映射系统
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
  hopeful, // 希望
  grateful, // 感恩
  lonely, // 孤独
  peaceful, // 平和
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

  /// 情绪色彩映射表
  static const Map<MoodType, MoodColorConfig> _colorConfigs = {
    // 开心 - 暖橙色系，充满阳光与活力
    MoodType.happy: MoodColorConfig(
      primary: Color(0xFFFF8A65),
      gradientStart: Color(0xFFFFF8E1),
      gradientEnd: Color(0xFFFFE0B2),
      lakeColor: Color(0xFFFFCC80),
      rippleColor: Color(0xFFFFAB40),
      textColor: Color(0xFF5D4037),
      iconColor: Color(0xFFFF6D00),
      cardColor: Color(0xFFFFF3E0),
      name: '开心',
      description: '阳光灿烂的心情',
      icon: Icons.sentiment_very_satisfied,
    ),

    // 平静 - 淡蓝绿色系，宁静祥和
    MoodType.calm: MoodColorConfig(
      primary: Color(0xFF26A69A),
      gradientStart: Color(0xFFE0F7FA),
      gradientEnd: Color(0xFFB2DFDB),
      lakeColor: Color(0xFF80CBC4),
      rippleColor: Color(0xFF4DB6AC),
      textColor: Color(0xFF004D40),
      iconColor: Color(0xFF00897B),
      cardColor: Color(0xFFE0F2F1),
      name: '平静',
      description: '内心安宁',
      icon: Icons.spa,
    ),

    // 悲伤 - 深蓝灰色系，静谧沉思
    MoodType.sad: MoodColorConfig(
      primary: Color(0xFF78909C),
      gradientStart: Color(0xFFCFD8DC),
      gradientEnd: Color(0xFF90A4AE),
      lakeColor: Color(0xFF607D8B),
      rippleColor: Color(0xFF546E7A),
      textColor: Color(0xFF37474F),
      iconColor: Color(0xFF455A64),
      cardColor: Color(0xFFECEFF1),
      name: '悲伤',
      description: '需要温暖的陪伴',
      icon: Icons.sentiment_dissatisfied,
    ),

    // 焦虑 - 浅紫色系，带有不安感
    MoodType.anxious: MoodColorConfig(
      primary: Color(0xFFBA68C8),
      gradientStart: Color(0xFFF3E5F5),
      gradientEnd: Color(0xFFE1BEE7),
      lakeColor: Color(0xFFCE93D8),
      rippleColor: Color(0xFFAB47BC),
      textColor: Color(0xFF4A148C),
      iconColor: Color(0xFF8E24AA),
      cardColor: Color(0xFFFCE4EC),
      name: '焦虑',
      description: '心中有些许不安',
      icon: Icons.psychology,
    ),

    // 愤怒 - 暗红色系，压抑的火焰
    MoodType.angry: MoodColorConfig(
      primary: Color(0xFFE57373),
      gradientStart: Color(0xFFFFEBEE),
      gradientEnd: Color(0xFFFFCDD2),
      lakeColor: Color(0xFFEF9A9A),
      rippleColor: Color(0xFFEF5350),
      textColor: Color(0xFFB71C1C),
      iconColor: Color(0xFFD32F2F),
      cardColor: Color(0xFFFFF5F5),
      name: '愤怒',
      description: '需要释放的情绪',
      icon: Icons.sentiment_very_dissatisfied,
    ),

    // 惊喜 - 金黄色系，闪耀的光芒
    MoodType.surprised: MoodColorConfig(
      primary: Color(0xFFFFD54F),
      gradientStart: Color(0xFFFFFDE7),
      gradientEnd: Color(0xFFFFF9C4),
      lakeColor: Color(0xFFFFEB3B),
      rippleColor: Color(0xFFFFC107),
      textColor: Color(0xFFF57F17),
      iconColor: Color(0xFFFFB300),
      cardColor: Color(0xFFFFFBE8),
      name: '惊喜',
      description: '意外的喜悦',
      icon: Icons.celebration,
    ),

    // 迷茫 - 淡紫蓝色系，朦胧的雾气
    MoodType.confused: MoodColorConfig(
      primary: Color(0xFF9FA8DA),
      gradientStart: Color(0xFFE8EAF6),
      gradientEnd: Color(0xFFC5CAE9),
      lakeColor: Color(0xFF7986CB),
      rippleColor: Color(0xFF5C6BC0),
      textColor: Color(0xFF283593),
      iconColor: Color(0xFF3949AB),
      cardColor: Color(0xFFF0F1F8),
      name: '迷茫',
      description: '寻找方向中',
      icon: Icons.help_outline,
    ),

    // 希望 - 绿色系，生机与希望
    MoodType.hopeful: MoodColorConfig(
      primary: Color(0xFF66BB6A),
      gradientStart: Color(0xFFE8F5E9),
      gradientEnd: Color(0xFFC8E6C9),
      lakeColor: Color(0xFF81C784),
      rippleColor: Color(0xFF4CAF50),
      textColor: Color(0xFF1B5E20),
      iconColor: Color(0xFF2E7D32),
      cardColor: Color(0xFFF1F8E9),
      name: '希望',
      description: '充满期待的心情',
      icon: Icons.eco,
    ),

    // 感恩 - 暖橙色系，温暖感恩
    MoodType.grateful: MoodColorConfig(
      primary: Color(0xFFFFB74D),
      gradientStart: Color(0xFFFFF3E0),
      gradientEnd: Color(0xFFFFE0B2),
      lakeColor: Color(0xFFFFCC80),
      rippleColor: Color(0xFFFFA726),
      textColor: Color(0xFFE65100),
      iconColor: Color(0xFFF57C00),
      cardColor: Color(0xFFFFF8E1),
      name: '感恩',
      description: '心怀感激',
      icon: Icons.favorite,
    ),

    // 孤独 - 紫蓝色系，静谧孤寂
    MoodType.lonely: MoodColorConfig(
      primary: Color(0xFF7E57C2),
      gradientStart: Color(0xFFEDE7F6),
      gradientEnd: Color(0xFFD1C4E9),
      lakeColor: Color(0xFF9575CD),
      rippleColor: Color(0xFF673AB7),
      textColor: Color(0xFF311B92),
      iconColor: Color(0xFF512DA8),
      cardColor: Color(0xFFF3EEFB),
      name: '孤独',
      description: '独自一人的时光',
      icon: Icons.nights_stay,
    ),

    // 平和 - 浅蓝绿色系，安详宁静
    MoodType.peaceful: MoodColorConfig(
      primary: Color(0xFF4DD0E1),
      gradientStart: Color(0xFFE0F7FA),
      gradientEnd: Color(0xFFB2EBF2),
      lakeColor: Color(0xFF80DEEA),
      rippleColor: Color(0xFF26C6DA),
      textColor: Color(0xFF006064),
      iconColor: Color(0xFF00838F),
      cardColor: Color(0xFFE0F7FA),
      name: '平和',
      description: '内心祥和安宁',
      icon: Icons.self_improvement,
    ),

    // 中性 - 标准蓝色系
    MoodType.neutral: MoodColorConfig(
      primary: Color(0xFF64B5F6),
      gradientStart: Color(0xFFFFFDF5),
      gradientEnd: Color(0xFFE3F2FD),
      lakeColor: Color(0xFF42A5F5),
      rippleColor: Color(0xFF2196F3),
      textColor: Color(0xFF1565C0),
      iconColor: Color(0xFF1976D2),
      cardColor: Color(0xFFF5F9FF),
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
      case '忧伤':
      case '难过':
      case '伤心':
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
      case 'hopeful':
      case '希望':
      case '期待':
        return MoodType.hopeful;
      case 'grateful':
      case '感恩':
      case '感激':
        return MoodType.grateful;
      case 'lonely':
      case '孤独':
      case '寂寞':
        return MoodType.lonely;
      case 'peaceful':
      case '平和':
      case '祥和':
        return MoodType.peaceful;
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
