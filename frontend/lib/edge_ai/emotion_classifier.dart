import 'dart:typed_data';
import 'local_dp.dart';

/// 情绪分类器 - 本地推理（无 tflite 时使用规则引擎降级）
class EmotionClassifier {
  static const List<String> _labels = [
    'happy', 'sad', 'angry', 'fearful', 'surprised', 'neutral'
  ];

  bool _isLoaded = false;

  bool get isLoaded => _isLoaded;

  Future<void> loadModel() async {
    // tflite_flutter 暂未集成，使用规则引擎降级
    _isLoaded = true;
  }

  Future<Map<String, double>> classify(Float32List input) async {
    if (!_isLoaded) {
      throw StateError('Model not loaded');
    }

    // 降级：基于输入特征的简单规则引擎
    final scores = <String, double>{};
    final len = input.length;
    for (var i = 0; i < _labels.length; i++) {
      // 使用输入特征的统计量生成伪分数
      double sum = 0;
      for (var j = i; j < len; j += _labels.length) {
        sum += input[j];
      }
      scores[_labels[i]] = sum.abs() / (len / _labels.length + 1);
    }

    // 归一化为概率分布
    final total = scores.values.fold(0.0, (a, b) => a + b);
    if (total > 0) {
      for (final key in scores.keys.toList()) {
        scores[key] = scores[key]! / total;
      }
    } else {
      // 均匀分布
      for (final key in _labels) {
        scores[key] = 1.0 / _labels.length;
      }
    }

    return scores;
  }

  String getTopEmotion(Map<String, double> scores) {
    return scores.entries.reduce((a, b) => a.value > b.value ? a : b).key;
  }

  void dispose() {
    _isLoaded = false;
  }
}

/// 本地差分隐私包装器
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

  Future<void> loadModel() => _classifier.loadModel();

  Future<Map<String, double>> classifyWithPrivacy(Float32List input) async {
    final rawScores = await _classifier.classify(input);
    if (!privacyEnabled) return rawScores;
    return _dp.privatizeDistribution(rawScores);
  }

  String getTopEmotion(Map<String, double> scores) {
    return _classifier.getTopEmotion(scores);
  }

  Map<String, dynamic> get privacyInfo => {
    'enabled': privacyEnabled,
    'epsilon': _dp.epsilon,
    'mechanism': 'Laplace (Local DP)',
    'reference': 'FedMultiEmo arXiv:2507.15470',
  };

  void dispose() => _classifier.dispose();
}
