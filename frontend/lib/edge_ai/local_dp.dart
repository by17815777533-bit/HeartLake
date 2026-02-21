// @file local_dp.dart
// @brief 本地差分隐私引擎 - Laplace机制

import 'dart:math';

class LocalDifferentialPrivacy {
  final double epsilon;
  final Random _random = Random();

  LocalDifferentialPrivacy({required this.epsilon});

  /// 对单个分数添加 Laplace 噪声
  double privatize(double trueScore) {
    final noise = _laplaceSample(1.0 / epsilon);
    return (trueScore + noise).clamp(-1.0, 1.0);
  }

  /// 对分数分布添加噪声
  Map<String, double> privatizeDistribution(Map<String, double> scores) {
    return scores.map((key, value) => MapEntry(key, privatize(value)));
  }

  /// Laplace 分布采样
  double _laplaceSample(double scale) {
    final u = _random.nextDouble() - 0.5;
    return -scale * u.sign * log(1 - 2 * u.abs());
  }
}
