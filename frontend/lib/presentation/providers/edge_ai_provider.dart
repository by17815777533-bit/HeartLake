// @file edge_ai_provider.dart
// @brief 本地Edge AI情感分类Provider - 集成LocalDPClassifier
// Created by 林子怡

import 'package:flutter/foundation.dart';
import '../../edge_ai/edge_ai.dart';

/// 情感分类缓存条目
class _ClassifyCache {
  final Map<String, double> scores;
  final String topEmotion;
  final DateTime timestamp;

  _ClassifyCache({
    required this.scores,
    required this.topEmotion,
    required this.timestamp,
  });

  bool get isExpired =>
      DateTime.now().difference(timestamp).inSeconds > 30;
}

/// Edge AI 本地情感分类 Provider（单例）
class EdgeAIProvider with ChangeNotifier {
  // 单例
  static final EdgeAIProvider _instance = EdgeAIProvider._internal();
  factory EdgeAIProvider() => _instance;
  EdgeAIProvider._internal();

  final LocalDPClassifier _classifier = LocalDPClassifier(
    epsilon: 2.0,
    privacyEnabled: true,
  );

  bool _isModelLoaded = false;
  bool _isLoading = false;
  Map<String, double>? _lastScores;
  String? _lastEmotion;

  // LRU缓存：最近10条分类结果
  final Map<String, _ClassifyCache> _cache = {};
  static const int _maxCacheSize = 10;

  bool get isModelLoaded => _isModelLoaded;
  bool get isLoading => _isLoading;
  Map<String, double>? get lastScores => _lastScores;
  String? get lastEmotion => _lastEmotion;

  /// 加载模型（app启动时调用）
  Future<void> loadModel() async {
    if (_isModelLoaded || _isLoading) return;
    _isLoading = true;
    notifyListeners();

    try {
      await _classifier.loadModel();
      _isModelLoaded = true;
    } catch (e) {
      if (kDebugMode) {
        debugPrint('EdgeAI模型加载失败: $e');
      }
    } finally {
      _isLoading = false;
      notifyListeners();
    }
  }

  /// 对文本进行情感分类（带差分隐私保护）
  Future<Map<String, double>?> classifyText(String text) async {
    if (!_isModelLoaded || text.trim().isEmpty) return null;

    // 检查缓存
    final cacheKey = text.trim();
    final cached = _cache[cacheKey];
    if (cached != null && !cached.isExpired) {
      _lastScores = cached.scores;
      _lastEmotion = cached.topEmotion;
      notifyListeners();
      return cached.scores;
    }

    try {
      final features = _textToFeatures(text);
      final scores = await _classifier.classifyWithPrivacy(features);
      final topEmotion = _classifier.getTopEmotion(scores);

      _lastScores = scores;
      _lastEmotion = topEmotion;

      // 写入缓存
      if (_cache.length >= _maxCacheSize) {
        _cache.remove(_cache.keys.first);
      }
      _cache[cacheKey] = _ClassifyCache(
        scores: scores,
        topEmotion: topEmotion,
        timestamp: DateTime.now(),
      );

      notifyListeners();
      return scores;
    } catch (e) {
      if (kDebugMode) {
        debugPrint('EdgeAI分类失败: $e');
      }
      return null;
    }
  }

  /// 获取隐私保护信息
  Map<String, dynamic> getPrivacyInfo() => _classifier.privacyInfo;

  /// 基于字符统计的简单特征提取
  Float32List _textToFeatures(String text) {
    final features = Float32List(24); // 24维特征
    for (var i = 0; i < text.length && i < 24; i++) {
      features[i] = text.codeUnitAt(i) / 65536.0;
    }
    // 添加统计特征
    if (text.isNotEmpty) {
      features[0] = text.length / 100.0; // 长度特征
    }
    return features;
  }

  /// 情绪标签中文映射
  static const Map<String, String> emotionLabels = {
    'happy': '开心',
    'sad': '悲伤',
    'angry': '愤怒',
    'fearful': '恐惧',
    'surprised': '惊喜',
    'neutral': '平静',
  };

  /// 情绪对应图标
  static const Map<String, int> emotionIcons = {
    'happy': 0xe5f6,     // Icons.sentiment_very_satisfied
    'sad': 0xe5f2,       // Icons.sentiment_dissatisfied
    'angry': 0xe5f0,     // Icons.sentiment_very_dissatisfied
    'fearful': 0xe002,   // Icons.warning_amber
    'surprised': 0xe5f4, // Icons.sentiment_satisfied_alt
    'neutral': 0xe5f3,   // Icons.sentiment_neutral
  };

  /// 获取情绪中文标签
  String getEmotionLabel(String emotion) =>
      emotionLabels[emotion] ?? emotion;

  /// 清理资源
  void clear() {
    _lastScores = null;
    _lastEmotion = null;
    _cache.clear();
    notifyListeners();
  }

  @override
  void dispose() {
    _classifier.dispose();
    _cache.clear();
    super.dispose();
  }
}
