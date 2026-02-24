import 'dart:typed_data';
import 'dart:math' as math;
import 'local_dp.dart';

/// 情绪分类器 - 本地推理（无 tflite 时使用规则引擎降级）
class EmotionClassifier {
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

    // 降级：基于结构化特征的轻量启发式推理（比随机统计更稳定）。
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

  String getTopEmotion(Map<String, double> scores) {
    return scores.entries.reduce((a, b) => a.value > b.value ? a : b).key;
  }

  void dispose() {
    _isLoaded = false;
  }

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
