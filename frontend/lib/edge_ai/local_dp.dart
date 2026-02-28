/// 本地差分隐私引擎
///
/// 基于Laplace机制对情绪分数注入噪声，在数据离开设备前完成扰动，
/// 确保上传到服务端的情绪数据满足epsilon-差分隐私保证。
/// 使用密码学安全随机数生成器。

import 'dart:math';

/// 本地差分隐私引擎
///
/// 提供基于Laplace机制的差分隐私保护。
class LocalDifferentialPrivacy {
  /// 隐私预算参数
  ///
  /// 值越小隐私保护越强，数据可用性越低。
  final double epsilon;
  final Random _random = Random.secure();

  LocalDifferentialPrivacy({required this.epsilon});

  /// 对单个分数添加Laplace噪声
  ///
  /// [trueScore] 真实分数
  ///
  /// 返回添加噪声后的分数，范围限制在-1.0到1.0之间。
  double privatize(double trueScore) {
    final noise = _laplaceSample(1.0 / epsilon);
    return (trueScore + noise).clamp(-1.0, 1.0);
  }

  /// 对分数分布添加噪声
  ///
  /// [scores] 原始分数分布
  ///
  /// 返回添加噪声后的分数分布。
  Map<String, double> privatizeDistribution(Map<String, double> scores) {
    return scores.map((key, value) => MapEntry(key, privatize(value)));
  }

  /// Laplace分布采样
  ///
  /// [scale] 尺度参数
  double _laplaceSample(double scale) {
    final u = _random.nextDouble() - 0.5;
    return -scale * u.sign * log(1 - 2 * u.abs());
  }
}
