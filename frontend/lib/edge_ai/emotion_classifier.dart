import 'dart:typed_data';
import 'package:tflite_flutter/tflite_flutter.dart';
import 'local_dp.dart';

class EmotionClassifier {
  static const List<String> _labels = [
    'happy', 'sad', 'angry', 'fearful', 'surprised', 'neutral'
  ];

  Interpreter? _interpreter;
  bool _isLoaded = false;

  bool get isLoaded => _isLoaded;

  Future<void> loadModel() async {
    if (_isLoaded) return;
    try {
      _interpreter = await Interpreter.fromAsset('models/emotion_model.tflite');
      _isLoaded = true;
    } catch (e) {
      _isLoaded = false;
      rethrow;
    }
  }

  Future<Map<String, double>> classify(Float32List input) async {
    if (!_isLoaded || _interpreter == null) {
      throw StateError('Model not loaded');
    }

    final output = List.filled(_labels.length, 0.0).reshape([1, _labels.length]);
    _interpreter!.run(input.reshape([1, input.length]), output);

    final scores = <String, double>{};
    for (var i = 0; i < _labels.length; i++) {
      scores[_labels[i]] = output[0][i];
    }
    return scores;
  }

  String getTopEmotion(Map<String, double> scores) {
    return scores.entries.reduce((a, b) => a.value > b.value ? a : b).key;
  }

  void dispose() {
    _interpreter?.close();
    _interpreter = null;
    _isLoaded = false;
  }
}

/// 本地差分隐私包装器
///
/// 在情绪分类结果发送到服务端之前添加拉普拉斯噪声，
/// 确保服务端永远无法获取用户的真实情绪分数。
/// 基于 FedMultiEmo (arXiv:2507.15470) 的本地差分隐私机制。
class LocalDPClassifier {
  final EmotionClassifier _classifier;
  final LocalDifferentialPrivacy _dp;

  /// 隐私保护是否启用
  final bool privacyEnabled;

  LocalDPClassifier({
    EmotionClassifier? classifier,
    double epsilon = 2.0,
    this.privacyEnabled = true,
  })  : _classifier = classifier ?? EmotionClassifier(),
        _dp = LocalDifferentialPrivacy(epsilon: epsilon);

  bool get isLoaded => _classifier.isLoaded;

  Future<void> loadModel() => _classifier.loadModel();

  /// 分类并自动添加差分隐私噪声
  Future<Map<String, double>> classifyWithPrivacy(Float32List input) async {
    final rawScores = await _classifier.classify(input);
    if (!privacyEnabled) return rawScores;
    return _dp.privatizeDistribution(rawScores);
  }

  /// 获取加噪后的主要情绪
  String getTopEmotion(Map<String, double> scores) {
    return _classifier.getTopEmotion(scores);
  }

  /// 隐私状态信息（可用于UI展示隐私徽章）
  Map<String, dynamic> get privacyInfo => {
    'enabled': privacyEnabled,
    'epsilon': _dp.epsilon,
    'mechanism': 'Laplace (Local DP)',
    'reference': 'FedMultiEmo arXiv:2507.15470',
  };

  void dispose() => _classifier.dispose();
}
