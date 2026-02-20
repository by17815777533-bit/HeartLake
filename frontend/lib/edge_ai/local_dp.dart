import 'dart:math';

/// 本地差分隐私层
///
/// 创新点：基于2025年联邦学习论文的本地差分隐私机制
/// 在情绪数据离开设备前添加随机噪声，服务端永远无法获取真实情绪分数
///
/// 参考：FedMultiEmo (arXiv:2507.15470) - Personalized Federated Averaging
class LocalDifferentialPrivacy {
  final double epsilon;
  final Random _random = Random.secure();

  LocalDifferentialPrivacy({this.epsilon = 2.0});

  /// 对情绪分数添加拉普拉斯噪声
  double privatize(double trueScore) {
    final scale = 1.0 / epsilon;
    final noise = _laplaceSample(scale);
    return (trueScore + noise).clamp(-1.0, 1.0);
  }

  /// 对情绪分布向量添加噪声
  Map<String, double> privatizeDistribution(Map<String, double> scores) {
    return scores.map((key, value) => MapEntry(key, privatize(value)));
  }

  /// 拉普拉斯分布采样（逆CDF方法）
  double _laplaceSample(double scale) {
    final u = _random.nextDouble() - 0.5;
    if (u.abs() >= 0.5) return 0.0;
    return -scale * u.sign * log(1 - 2 * u.abs());
  }
}
