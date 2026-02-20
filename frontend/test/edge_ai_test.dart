import 'dart:typed_data';
import 'package:flutter_test/flutter_test.dart';
import 'package:heartlake/edge_ai/edge_ai.dart';

void main() {
  group('LocalDifferentialPrivacy', () {
    late LocalDifferentialPrivacy dp;

    setUp(() {
      dp = LocalDifferentialPrivacy(epsilon: 2.0);
    });

    test('privatize should add noise to score', () {
      const trueScore = 0.8;
      final privatizedScore = dp.privatize(trueScore);

      // 加噪后的分数应该在有效范围内
      expect(privatizedScore, greaterThanOrEqualTo(-1.0));
      expect(privatizedScore, lessThanOrEqualTo(1.0));

      // 加噪后的分数应该与原始分数不同（概率极高）
      // 注意：理论上有极小概率噪声为0，但实际测试中几乎不会发生
      expect(privatizedScore, isNot(equals(trueScore)));
    });

    test('privatize should clamp values to [-1, 1]', () {
      // 测试边界情况
      final score1 = dp.privatize(1.0);
      final score2 = dp.privatize(-1.0);
      final score3 = dp.privatize(0.0);

      expect(score1, greaterThanOrEqualTo(-1.0));
      expect(score1, lessThanOrEqualTo(1.0));
      expect(score2, greaterThanOrEqualTo(-1.0));
      expect(score2, lessThanOrEqualTo(1.0));
      expect(score3, greaterThanOrEqualTo(-1.0));
      expect(score3, lessThanOrEqualTo(1.0));
    });

    test('privatize should produce different results on multiple calls', () {
      const trueScore = 0.5;
      final results = <double>{};

      // 多次调用应该产生不同的结果
      for (var i = 0; i < 10; i++) {
        results.add(dp.privatize(trueScore));
      }

      // 至少应该有多个不同的值（拉普拉斯噪声是连续分布）
      expect(results.length, greaterThan(1));
    });

    test('privatizeDistribution should add noise to all scores', () {
      final scores = {
        'happy': 0.8,
        'sad': 0.1,
        'angry': 0.05,
        'neutral': 0.05,
      };

      final privatizedScores = dp.privatizeDistribution(scores);

      // 应该包含所有原始键
      expect(privatizedScores.keys, equals(scores.keys));

      // 所有值应该在有效范围内
      for (final value in privatizedScores.values) {
        expect(value, greaterThanOrEqualTo(-1.0));
        expect(value, lessThanOrEqualTo(1.0));
      }
    });

    test('smaller epsilon should add more noise', () {
      final dpLowPrivacy = LocalDifferentialPrivacy(epsilon: 10.0);
      final dpHighPrivacy = LocalDifferentialPrivacy(epsilon: 0.5);

      const trueScore = 0.5;
      const iterations = 100;

      // 计算平均偏差
      var deviationLow = 0.0;
      var deviationHigh = 0.0;

      for (var i = 0; i < iterations; i++) {
        deviationLow += (dpLowPrivacy.privatize(trueScore) - trueScore).abs();
        deviationHigh += (dpHighPrivacy.privatize(trueScore) - trueScore).abs();
      }

      deviationLow /= iterations;
      deviationHigh /= iterations;

      // 更小的 epsilon（更高的隐私）应该产生更大的平均偏差
      expect(deviationHigh, greaterThan(deviationLow));
    });

    test('epsilon parameter should be configurable', () {
      final dp1 = LocalDifferentialPrivacy(epsilon: 1.0);
      final dp2 = LocalDifferentialPrivacy(epsilon: 5.0);

      expect(dp1.epsilon, equals(1.0));
      expect(dp2.epsilon, equals(5.0));
    });
  });

  group('EmotionClassifier', () {
    late EmotionClassifier classifier;

    setUp(() {
      classifier = EmotionClassifier();
    });

    tearDown(() {
      classifier.dispose();
    });

    test('initial state should be not loaded', () {
      expect(classifier.isLoaded, isFalse);
    });

    test('loadModel should fail gracefully when model file missing', () async {
      // 在测试环境中，模型文件不存在，应该抛出异常
      expect(() async => await classifier.loadModel(), throwsException);
      expect(classifier.isLoaded, isFalse);
    });

    test('classify should throw StateError when model not loaded', () async {
      final input = Float32List(128);
      expect(
        () async => await classifier.classify(input),
        throwsStateError,
      );
    });

    test('getTopEmotion should return emotion with highest score', () {
      final scores = {
        'happy': 0.8,
        'sad': 0.1,
        'angry': 0.05,
        'fearful': 0.02,
        'surprised': 0.02,
        'neutral': 0.01,
      };

      final topEmotion = classifier.getTopEmotion(scores);
      expect(topEmotion, equals('happy'));
    });

    test('getTopEmotion should handle tied scores', () {
      final scores = {
        'happy': 0.5,
        'sad': 0.5,
        'angry': 0.0,
      };

      final topEmotion = classifier.getTopEmotion(scores);
      // 应该返回其中一个最高分的情绪
      expect(['happy', 'sad'], contains(topEmotion));
    });

    test('getTopEmotion should work with negative scores', () {
      final scores = {
        'happy': -0.1,
        'sad': -0.5,
        'angry': -0.3,
      };

      final topEmotion = classifier.getTopEmotion(scores);
      expect(topEmotion, equals('happy')); // -0.1 是最大值
    });

    test('dispose should clean up resources', () {
      classifier.dispose();
      expect(classifier.isLoaded, isFalse);
    });
  });

  group('LocalDPClassifier', () {
    late LocalDPClassifier dpClassifier;

    setUp(() {
      dpClassifier = LocalDPClassifier(epsilon: 2.0, privacyEnabled: true);
    });

    tearDown(() {
      dpClassifier.dispose();
    });

    test('initial state should be not loaded', () {
      expect(dpClassifier.isLoaded, isFalse);
    });

    test('privacyInfo should return correct information', () {
      final info = dpClassifier.privacyInfo;

      expect(info['enabled'], isTrue);
      expect(info['epsilon'], equals(2.0));
      expect(info['mechanism'], equals('Laplace (Local DP)'));
      expect(info['reference'], equals('FedMultiEmo arXiv:2507.15470'));
    });

    test('privacyInfo should reflect disabled privacy', () {
      final dpClassifierNoPrivacy = LocalDPClassifier(
        epsilon: 2.0,
        privacyEnabled: false,
      );

      final info = dpClassifierNoPrivacy.privacyInfo;
      expect(info['enabled'], isFalse);

      dpClassifierNoPrivacy.dispose();
    });

    test('classifyWithPrivacy should throw when model not loaded', () async {
      final input = Float32List(128);
      expect(
        () async => await dpClassifier.classifyWithPrivacy(input),
        throwsStateError,
      );
    });

    test('getTopEmotion should delegate to underlying classifier', () {
      final scores = {
        'happy': 0.9,
        'sad': 0.05,
        'angry': 0.03,
        'fearful': 0.01,
        'surprised': 0.01,
        'neutral': 0.0,
      };

      final topEmotion = dpClassifier.getTopEmotion(scores);
      expect(topEmotion, equals('happy'));
    });

    test('custom epsilon should be reflected in privacyInfo', () {
      final customDpClassifier = LocalDPClassifier(epsilon: 5.0);
      expect(customDpClassifier.privacyInfo['epsilon'], equals(5.0));
      customDpClassifier.dispose();
    });

    test('dispose should clean up resources', () {
      dpClassifier.dispose();
      expect(dpClassifier.isLoaded, isFalse);
    });
  });

  group('Integration Tests', () {
    test('LocalDPClassifier should use LocalDifferentialPrivacy correctly', () {
      // 创建一个禁用隐私的分类器作为对照
      final classifierNoPrivacy = LocalDPClassifier(
        epsilon: 2.0,
        privacyEnabled: false,
      );

      // 验证隐私设置
      expect(classifierNoPrivacy.privacyInfo['enabled'], isFalse);

      classifierNoPrivacy.dispose();
    });

    test('different epsilon values should affect privacy level', () {
      final lowPrivacy = LocalDPClassifier(epsilon: 10.0);
      final highPrivacy = LocalDPClassifier(epsilon: 0.5);

      expect(lowPrivacy.privacyInfo['epsilon'], equals(10.0));
      expect(highPrivacy.privacyInfo['epsilon'], equals(0.5));

      lowPrivacy.dispose();
      highPrivacy.dispose();
    });

    test('LocalDifferentialPrivacy should preserve statistical properties', () {
      final dp = LocalDifferentialPrivacy(epsilon: 2.0);
      const trueScore = 0.5;
      const iterations = 1000;

      var sum = 0.0;
      for (var i = 0; i < iterations; i++) {
        sum += dp.privatize(trueScore);
      }

      final average = sum / iterations;

      // 拉普拉斯噪声的期望值为0，所以平均值应该接近原始值
      // 允许一定的统计误差
      expect(average, closeTo(trueScore, 0.1));
    });
  });
}
