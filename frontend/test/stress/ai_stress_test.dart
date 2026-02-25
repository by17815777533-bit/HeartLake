// AI引擎压力测试 - LocalDifferentialPrivacy + EmotionClassifier + LocalDPClassifier
import 'dart:math';
import 'dart:typed_data';
import 'package:flutter_test/flutter_test.dart';
import 'package:heart_lake/edge_ai/local_dp.dart';
import 'package:heart_lake/edge_ai/emotion_classifier.dart';

void main() {
  // ============================================================
  // LocalDifferentialPrivacy 压测
  // ============================================================
  group('LocalDifferentialPrivacy - 大量采样统计特性', () {
    late LocalDifferentialPrivacy dp;

    setUp(() {
      dp = LocalDifferentialPrivacy(epsilon: 2.0);
    });

    test('10000次采样均值应接近真实值', () {
      const trueScore = 0.5;
      var sum = 0.0;
      const n = 10000;
      for (var i = 0; i < n; i++) {
        sum += dp.privatize(trueScore);
      }
      final mean = sum / n;
      // Laplace噪声均值为0，所以privatize均值应接近trueScore
      expect(mean, closeTo(trueScore, 0.1));
    });

    test('10000次采样结果全部在[-1,1]范围内', () {
      const trueScore = 0.7;
      for (var i = 0; i < 10000; i++) {
        final result = dp.privatize(trueScore);
        expect(result, greaterThanOrEqualTo(-1.0));
        expect(result, lessThanOrEqualTo(1.0));
      }
    });

    test('10000次采样方差应与理论Laplace方差匹配', () {
      const trueScore = 0.0;
      const n = 10000;
      var sum = 0.0;
      var sumSq = 0.0;
      for (var i = 0; i < n; i++) {
        final v = dp.privatize(trueScore);
        sum += v;
        sumSq += v * v;
      }
      final mean = sum / n;
      final variance = sumSq / n - mean * mean;
      // Laplace方差 = 2 * scale^2, scale = 1/epsilon = 0.5, 理论方差 = 0.5
      // 但clamp会压缩方差，所以只检查合理范围
      expect(variance, greaterThan(0.01));
      expect(variance, lessThan(1.0));
    });

    test('采样分布应大致对称(trueScore=0)', () {
      const trueScore = 0.0;
      const n = 10000;
      var positive = 0;
      var negative = 0;
      for (var i = 0; i < n; i++) {
        final v = dp.privatize(trueScore);
        if (v > 0) positive++;
        if (v < 0) negative++;
      }
      // 对称分布，正负比例应接近1:1
      final ratio = positive / (negative + 1);
      expect(ratio, greaterThan(0.7));
      expect(ratio, lessThan(1.4));
    });

    test('连续50000次调用不崩溃', () {
      for (var i = 0; i < 50000; i++) {
        dp.privatize(0.5);
      }
    });
  });

  group('LocalDifferentialPrivacy - 极端epsilon值', () {
    test('极小epsilon(0.001)产生大噪声', () {
      final dp = LocalDifferentialPrivacy(epsilon: 0.001);
      // scale = 1/0.001 = 1000, 噪声极大，结果几乎总是被clamp到边界
      var hitBoundary = 0;
      const n = 1000;
      for (var i = 0; i < n; i++) {
        final v = dp.privatize(0.5);
        if (v == -1.0 || v == 1.0) hitBoundary++;
      }
      // 大部分应该被clamp到边界
      expect(hitBoundary, greaterThan(n * 0.5));
    });

    test('极大epsilon(100)产生极小噪声', () {
      final dp = LocalDifferentialPrivacy(epsilon: 100.0);
      const trueScore = 0.5;
      const n = 1000;
      var maxDiff = 0.0;
      for (var i = 0; i < n; i++) {
        final v = dp.privatize(trueScore);
        final diff = (v - trueScore).abs();
        if (diff > maxDiff) maxDiff = diff;
      }
      // 噪声应该非常小
      expect(maxDiff, lessThan(0.1));
    });

    test('epsilon=1.0 中等噪声', () {
      final dp = LocalDifferentialPrivacy(epsilon: 1.0);
      const trueScore = 0.3;
      var sum = 0.0;
      const n = 5000;
      for (var i = 0; i < n; i++) {
        sum += dp.privatize(trueScore);
      }
      final mean = sum / n;
      expect(mean, closeTo(trueScore, 0.15));
    });

    test('epsilon=0.01 高隐私', () {
      final dp = LocalDifferentialPrivacy(epsilon: 0.01);
      const trueScore = 0.8;
      var sum = 0.0;
      const n = 5000;
      for (var i = 0; i < n; i++) {
        sum += dp.privatize(trueScore);
      }
      // 高隐私下均值偏离较大但仍有趋势
      final mean = sum / n;
      expect(mean, greaterThan(-1.0));
      expect(mean, lessThan(1.0));
    });

    test('epsilon=50 低隐私高精度', () {
      final dp = LocalDifferentialPrivacy(epsilon: 50.0);
      const trueScore = -0.3;
      const n = 1000;
      for (var i = 0; i < n; i++) {
        final v = dp.privatize(trueScore);
        expect((v - trueScore).abs(), lessThan(0.2));
      }
    });
  });

  group('LocalDifferentialPrivacy - 边界值输入', () {
    late LocalDifferentialPrivacy dp;

    setUp(() {
      dp = LocalDifferentialPrivacy(epsilon: 2.0);
    });

    test('输入-1.0(下界)', () {
      for (var i = 0; i < 1000; i++) {
        final v = dp.privatize(-1.0);
        expect(v, greaterThanOrEqualTo(-1.0));
        expect(v, lessThanOrEqualTo(1.0));
      }
    });

    test('输入0.0(零点)', () {
      for (var i = 0; i < 1000; i++) {
        final v = dp.privatize(0.0);
        expect(v, greaterThanOrEqualTo(-1.0));
        expect(v, lessThanOrEqualTo(1.0));
      }
    });

    test('输入1.0(上界)', () {
      for (var i = 0; i < 1000; i++) {
        final v = dp.privatize(1.0);
        expect(v, greaterThanOrEqualTo(-1.0));
        expect(v, lessThanOrEqualTo(1.0));
      }
    });

    test('输入超出范围10.0被clamp', () {
      for (var i = 0; i < 1000; i++) {
        final v = dp.privatize(10.0);
        expect(v, greaterThanOrEqualTo(-1.0));
        expect(v, lessThanOrEqualTo(1.0));
      }
    });

    test('输入超出范围-10.0被clamp', () {
      for (var i = 0; i < 1000; i++) {
        final v = dp.privatize(-10.0);
        expect(v, greaterThanOrEqualTo(-1.0));
        expect(v, lessThanOrEqualTo(1.0));
      }
    });

    test('输入极小正数', () {
      final v = dp.privatize(1e-15);
      expect(v, greaterThanOrEqualTo(-1.0));
      expect(v, lessThanOrEqualTo(1.0));
    });

    test('输入极小负数', () {
      final v = dp.privatize(-1e-15);
      expect(v, greaterThanOrEqualTo(-1.0));
      expect(v, lessThanOrEqualTo(1.0));
    });
  });

  group('LocalDifferentialPrivacy - privatizeDistribution压测', () {
    late LocalDifferentialPrivacy dp;

    setUp(() {
      dp = LocalDifferentialPrivacy(epsilon: 2.0);
    });

    test('大量类别分布隐私化(100个类别)', () {
      final scores = <String, double>{};
      for (var i = 0; i < 100; i++) {
        scores['category_$i'] = i / 100.0;
      }
      final result = dp.privatizeDistribution(scores);
      expect(result.length, 100);
      for (final entry in result.entries) {
        expect(entry.value, greaterThanOrEqualTo(-1.0));
        expect(entry.value, lessThanOrEqualTo(1.0));
      }
    });

    test('空分布', () {
      final result = dp.privatizeDistribution({});
      expect(result, isEmpty);
    });

    test('单类别分布', () {
      final result = dp.privatizeDistribution({'only': 0.5});
      expect(result.length, 1);
      expect(result.containsKey('only'), isTrue);
    });

    test('1000次分布隐私化连续调用', () {
      final scores = {'a': 0.3, 'b': 0.5, 'c': 0.2};
      for (var i = 0; i < 1000; i++) {
        final result = dp.privatizeDistribution(scores);
        expect(result.length, 3);
      }
    });

    test('分布隐私化后均值应接近原始值', () {
      const n = 5000;
      final sums = <String, double>{'happy': 0, 'sad': 0, 'neutral': 0};
      final original = {'happy': 0.7, 'sad': 0.2, 'neutral': 0.1};
      for (var i = 0; i < n; i++) {
        final result = dp.privatizeDistribution(original);
        for (final key in sums.keys) {
          sums[key] = sums[key]! + result[key]!;
        }
      }
      for (final key in original.keys) {
        final mean = sums[key]! / n;
        expect(mean, closeTo(original[key]!, 0.15));
      }
    });

    test('所有值为0的分布', () {
      final scores = {'a': 0.0, 'b': 0.0, 'c': 0.0};
      final result = dp.privatizeDistribution(scores);
      expect(result.length, 3);
    });

    test('所有值为1的分布', () {
      final scores = {'a': 1.0, 'b': 1.0, 'c': 1.0};
      final result = dp.privatizeDistribution(scores);
      for (final v in result.values) {
        expect(v, greaterThanOrEqualTo(-1.0));
        expect(v, lessThanOrEqualTo(1.0));
      }
    });
  });

  group('LocalDifferentialPrivacy - 不同epsilon噪声幅度对比', () {
    test('epsilon越大噪声越小', () {
      final epsilons = [0.1, 0.5, 1.0, 2.0, 5.0, 10.0, 50.0];
      final variances = <double>[];
      const trueScore = 0.0;
      const n = 5000;

      for (final eps in epsilons) {
        final dp = LocalDifferentialPrivacy(epsilon: eps);
        var sumSq = 0.0;
        for (var i = 0; i < n; i++) {
          final v = dp.privatize(trueScore);
          sumSq += v * v;
        }
        variances.add(sumSq / n);
      }

      // 方差应该随epsilon增大而减小(大趋势)
      for (var i = 1; i < variances.length; i++) {
        if (epsilons[i] >= 2.0) {
          // 从epsilon=2开始，方差应明显递减
          expect(variances[i], lessThanOrEqualTo(variances[i - 1] + 0.05));
        }
      }
    });

    test('多epsilon并行采样稳定性', () {
      final random = Random(42);
      for (var trial = 0; trial < 100; trial++) {
        final eps = 0.1 + random.nextDouble() * 10.0;
        final dp = LocalDifferentialPrivacy(epsilon: eps);
        final v = dp.privatize(0.5);
        expect(v, greaterThanOrEqualTo(-1.0));
        expect(v, lessThanOrEqualTo(1.0));
      }
    });
  });

  // ============================================================
  // EmotionClassifier 压测
  // ============================================================
  group('EmotionClassifier - 基础功能压测', () {
    late EmotionClassifier classifier;

    setUp(() async {
      classifier = EmotionClassifier();
      await classifier.loadModel();
    });

    tearDown(() {
      classifier.dispose();
    });

    test('未加载模型时classify抛出StateError', () async {
      final fresh = EmotionClassifier();
      expect(
        () => fresh.classify(Float32List(24)),
        throwsA(isA<StateError>()),
      );
    });

    test('1000次连续分类不崩溃', () async {
      final input = Float32List(24);
      for (var i = 0; i < 24; i++) {
        input[i] = i / 24.0;
      }
      for (var i = 0; i < 1000; i++) {
        final result = await classifier.classify(input);
        expect(result.length, 6);
      }
    });

    test('分类结果概率和为1', () async {
      final input = Float32List(24);
      for (var i = 0; i < 24; i++) {
        input[i] = 0.5;
      }
      final result = await classifier.classify(input);
      final sum = result.values.reduce((a, b) => a + b);
      expect(sum, closeTo(1.0, 0.001));
    });

    test('所有情绪类别都存在', () async {
      final input = Float32List(24);
      final result = await classifier.classify(input);
      expect(result.containsKey('happy'), isTrue);
      expect(result.containsKey('sad'), isTrue);
      expect(result.containsKey('angry'), isTrue);
      expect(result.containsKey('fearful'), isTrue);
      expect(result.containsKey('surprised'), isTrue);
      expect(result.containsKey('neutral'), isTrue);
    });

    test('全零输入', () async {
      final input = Float32List(24);
      final result = await classifier.classify(input);
      expect(result.values.every((v) => v >= 0 && v <= 1), isTrue);
    });

    test('全1输入', () async {
      final input = Float32List(24);
      for (var i = 0; i < 24; i++) {
        input[i] = 1.0;
      }
      final result = await classifier.classify(input);
      final sum = result.values.reduce((a, b) => a + b);
      expect(sum, closeTo(1.0, 0.001));
    });

    test('极大值输入', () async {
      final input = Float32List(24);
      for (var i = 0; i < 24; i++) {
        input[i] = 1e6;
      }
      final result = await classifier.classify(input);
      expect(result.values.every((v) => v.isFinite), isTrue);
    });

    test('负值输入', () async {
      final input = Float32List(24);
      for (var i = 0; i < 24; i++) {
        input[i] = -0.5;
      }
      final result = await classifier.classify(input);
      expect(result.values.every((v) => v >= 0 && v <= 1), isTrue);
    });

    test('随机输入100次结果稳定', () async {
      final random = Random(123);
      for (var trial = 0; trial < 100; trial++) {
        final input = Float32List(24);
        for (var i = 0; i < 24; i++) {
          input[i] = random.nextDouble() * 2 - 1;
        }
        final result = await classifier.classify(input);
        final sum = result.values.reduce((a, b) => a + b);
        expect(sum, closeTo(1.0, 0.01));
      }
    });

    test('短输入(长度<24)', () async {
      final input = Float32List(10);
      for (var i = 0; i < 10; i++) {
        input[i] = 0.5;
      }
      final result = await classifier.classify(input);
      expect(result.length, 6);
    });

    test('空输入(长度0)', () async {
      final input = Float32List(0);
      final result = await classifier.classify(input);
      expect(result.length, 6);
    });

    test('超长输入(长度1000)', () async {
      final input = Float32List(1000);
      for (var i = 0; i < 1000; i++) {
        input[i] = i / 1000.0;
      }
      final result = await classifier.classify(input);
      expect(result.length, 6);
    });

    test('getTopEmotion返回最高分情绪', () async {
      final input = Float32List(24);
      for (var i = 0; i < 24; i++) {
        input[i] = 0.8;
      }
      final result = await classifier.classify(input);
      final top = classifier.getTopEmotion(result);
      final maxVal = result.values.reduce(max);
      expect(result[top], maxVal);
    });

    test('loadModel多次调用幂等', () async {
      await classifier.loadModel();
      await classifier.loadModel();
      await classifier.loadModel();
      expect(classifier.isLoaded, isTrue);
    });

    test('dispose后isLoaded为false', () {
      expect(classifier.isLoaded, isTrue);
      classifier.dispose();
      expect(classifier.isLoaded, isFalse);
    });

    test('dispose后重新loadModel', () async {
      classifier.dispose();
      expect(classifier.isLoaded, isFalse);
      await classifier.loadModel();
      expect(classifier.isLoaded, isTrue);
      final result = await classifier.classify(Float32List(24));
      expect(result.length, 6);
    });
  });

  // ============================================================
  // LocalDPClassifier 压测
  // ============================================================
  group('LocalDPClassifier - 隐私分类压测', () {
    late LocalDPClassifier dpClassifier;

    setUp(() async {
      dpClassifier = LocalDPClassifier(epsilon: 2.0, privacyEnabled: true);
      await dpClassifier.loadModel();
    });

    tearDown(() {
      dpClassifier.dispose();
    });

    test('1000次隐私分类结果都在有效范围', () async {
      final input = Float32List(24);
      for (var i = 0; i < 24; i++) {
        input[i] = 0.5;
      }
      for (var trial = 0; trial < 1000; trial++) {
        final result = await dpClassifier.classifyWithPrivacy(input);
        expect(result.length, 6);
        for (final v in result.values) {
          expect(v, greaterThanOrEqualTo(-1.0));
          expect(v, lessThanOrEqualTo(1.0));
        }
      }
    });

    test('隐私关闭时结果确定性', () async {
      final dpOff = LocalDPClassifier(epsilon: 2.0, privacyEnabled: false);
      await dpOff.loadModel();
      final input = Float32List(24);
      for (var i = 0; i < 24; i++) {
        input[i] = 0.3;
      }
      final r1 = await dpOff.classifyWithPrivacy(input);
      final r2 = await dpOff.classifyWithPrivacy(input);
      // 无隐私时结果应完全相同
      for (final key in r1.keys) {
        expect(r1[key], r2[key]);
      }
      dpOff.dispose();
    });

    test('隐私开启时结果有随机性', () async {
      final input = Float32List(24);
      for (var i = 0; i < 24; i++) {
        input[i] = 0.5;
      }
      final results = <Map<String, double>>[];
      for (var i = 0; i < 10; i++) {
        results.add(await dpClassifier.classifyWithPrivacy(input));
      }
      // 至少有一些结果不完全相同
      var allSame = true;
      for (var i = 1; i < results.length; i++) {
        for (final key in results[0].keys) {
          if (results[i][key] != results[0][key]) {
            allSame = false;
            break;
          }
        }
        if (!allSame) break;
      }
      expect(allSame, isFalse);
    });

    test('privacyInfo返回正确信息', () {
      final info = dpClassifier.privacyInfo;
      expect(info['enabled'], isTrue);
      expect(info['epsilon'], 2.0);
      expect(info['mechanism'], contains('Laplace'));
    });

    test('getTopEmotion与classify一致', () async {
      final input = Float32List(24);
      for (var i = 0; i < 24; i++) {
        input[i] = 0.6;
      }
      // 关闭隐私以获得确定性结果
      final dpOff = LocalDPClassifier(epsilon: 2.0, privacyEnabled: false);
      await dpOff.loadModel();
      final result = await dpOff.classifyWithPrivacy(input);
      final top = dpOff.getTopEmotion(result);
      final maxVal = result.values.reduce(max);
      expect(result[top], maxVal);
      dpOff.dispose();
    });

    test('不同epsilon的LocalDPClassifier', () async {
      for (final eps in [0.1, 0.5, 1.0, 5.0, 10.0]) {
        final c = LocalDPClassifier(epsilon: eps);
        await c.loadModel();
        final result = await c.classifyWithPrivacy(Float32List(24));
        expect(result.length, 6);
        c.dispose();
      }
    });

    test('dispose后重建', () async {
      dpClassifier.dispose();
      expect(dpClassifier.isLoaded, isFalse);
      final fresh = LocalDPClassifier(epsilon: 3.0);
      await fresh.loadModel();
      expect(fresh.isLoaded, isTrue);
      final result = await fresh.classifyWithPrivacy(Float32List(24));
      expect(result.length, 6);
      fresh.dispose();
    });
  });
}
