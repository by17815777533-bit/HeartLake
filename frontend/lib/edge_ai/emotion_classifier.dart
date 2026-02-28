/// 端侧情绪分类器
///
/// 在设备本地完成情绪推理。当tflite运行时不可用时，降级为基于词法特征
/// 的启发式规则引擎，从输入向量中提取词汇信号、长度、汉字比例、感叹号
/// 密度等特征，计算六类情绪的logits并经softmax归一化输出概率分布。

import 'dart:typed_data';
import 'dart:math' as math;
import 'local_dp.dart';

/// 端侧情绪分类器
///
/// 提供本地情绪推理能力。
class EmotionClassifier {
  bool _isLoaded = false;

  bool get isLoaded => _isLoaded;

  /// 加载模型
  ///
  /// 规则引擎降级模式，无需加载模型文件。
  Future<void> loadModel() async {
    _isLoaded = true;
  }

  /// 情绪分类
  ///
  /// 基于结构化特征的启发式推理。
  ///
  /// [input] 输入特征向量
  ///
  /// 返回六类情绪的概率分布。
  Future<Map<String, double>> classify(Float32List input) async {
    if (!_isLoaded) {
      throw StateError('Model not loaded');
    }

    // 基于结构化特征的启发式推理
    final lexicalSignal = _mean(input, 0, 20).clamp(0.0, 1.0);
    final lengthSignal = input.length > 20 ? input[20].toDouble().clamp(0.0, 1.0) : 0.0;
    final hanRatio = input.length > 21 ? input[21].toDouble().clamp(0.0, 1.0) : 0.0;
    final exclaimSignal = input.length > 22 ? input[22].toDouble().clamp(0.0, 1.0) : 0.0;
    final punctuationSignal =
        input.length > 23 ? input[23].toDouble().clamp(0.0, 1.0) : 0.0;

    final logits = <String, double>{
      'happy': 0.22 +
          lexicalSignal * 0.58 +
          exclaimSignal * 0.24 +
          hanRatio * 0.10 -
          punctuationSignal * 0.08,
      'sad': 0.16 +
          punctuationSignal * 0.34 +
          (1.0 - lexicalSignal) * 0.32 +
          lengthSignal * 0.06,
      'angry': 0.14 +
          exclaimSignal * 0.46 +
          punctuationSignal * 0.22 +
          (1.0 - hanRatio) * 0.12,
      'fearful': 0.14 +
          punctuationSignal * 0.30 +
          (1.0 - lexicalSignal) * 0.26 +
          lengthSignal * 0.10,
      'surprised': 0.18 +
          exclaimSignal * 0.50 +
          lexicalSignal * 0.18 +
          lengthSignal * 0.06,
      'neutral': 0.20 +
          (1.0 - exclaimSignal) * 0.24 +
          (1.0 - punctuationSignal) * 0.22 +
          hanRatio * 0.12,
    };

    return _softmax(logits);
  }

  /// 获取概率最高的情绪标签
  ///
  /// [scores] 情绪概率分布
  String getTopEmotion(Map<String, double> scores) {
    return scores.entries.reduce((a, b) => a.value > b.value ? a : b).key;
  }

  /// 释放资源
  void dispose() {
    _isLoaded = false;
  }

  /// 计算均值
  ///
  /// 计算输入向量指定区间的均值。
  ///
  /// [input] 输入向量
  /// [start] 起始索引
  /// [endExclusive] 结束索引（不包含）
  double _mean(Float32List input, int start, int endExclusive) {
    if (start >= endExclusive || start >= input.length) return 0.0;
    final end = endExclusive > input.length ? input.length : endExclusive;
    if (end <= start) return 0.0;
    var sum = 0.0;
    for (var i = start; i < end; i++) {
      sum += input[i].toDouble();
    }
    return sum / (end - start);
  }

  /// Softmax归一化
  ///
  /// 数值稳定的softmax实现，先减去最大值防止溢出，
  /// 温度系数2.2增强区分度。
  ///
  /// [logits] 原始logits
  Map<String, double> _softmax(Map<String, double> logits) {
    var maxLogit = logits.values.first;
    for (final v in logits.values) {
      if (v > maxLogit) maxLogit = v;
    }

    final expMap = <String, double>{};
    var total = 0.0;
    logits.forEach((key, value) {
      final shifted = (value - maxLogit) * 2.2;
      final expVal = shifted > 20 ? math.exp(20) : shifted < -20 ? 0.0 : math.exp(shifted);
      expMap[key] = expVal;
      total += expVal;
    });

    if (total <= 1e-9) {
      final uniform = 1.0 / logits.length;
      return {for (final key in logits.keys) key: uniform};
    }
    return {for (final entry in expMap.entries) entry.key: entry.value / total};
  }
}

/// 带本地差分隐私保护的情绪分类器
///
/// 在EmotionClassifier推理结果上叠加Laplace噪声，确保上传的情绪概率
/// 分布满足epsilon-DP保证。
class LocalDPClassifier {
  final EmotionClassifier _classifier;
  final LocalDifferentialPrivacy _dp;
  final bool privacyEnabled;

  LocalDPClassifier({
    EmotionClassifier? classifier,
    double epsilon = 2.0,
    this.privacyEnabled = true,
  })  : _classifier = classifier ?? EmotionClassifier(),
        _dp = LocalDifferentialPrivacy(epsilon: epsilon);

  bool get isLoaded => _classifier.isLoaded;

  /// 加载模型
  Future<void> loadModel() => _classifier.loadModel();

  /// 带隐私保护的情绪分类
  ///
  /// [input] 输入特征向量
  ///
  /// 返回添加噪声后的情绪概率分布（如果启用隐私保护）。
  Future<Map<String, double>> classifyWithPrivacy(Float32List input) async {
    final rawScores = await _classifier.classify(input);
    if (!privacyEnabled) return rawScores;
    return _dp.privatizeDistribution(rawScores);
  }

  /// 获取概率最高的情绪标签
  ///
  /// [scores] 情绪概率分布
  String getTopEmotion(Map<String, double> scores) {
    return _classifier.getTopEmotion(scores);
  }

  /// 隐私保护信息
  ///
  /// 返回包含隐私保护配置的Map。
  Map<String, dynamic> get privacyInfo => {
    'enabled': privacyEnabled,
    'epsilon': _dp.epsilon,
    'mechanism': 'Laplace (Local DP)',
    'reference': 'FedMultiEmo arXiv:2507.15470',
  };

  /// 释放资源
  void dispose() => _classifier.dispose();
}
