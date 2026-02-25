import 'dart:typed_data';
import 'package:flutter_test/flutter_test.dart';
import 'package:heart_lake/edge_ai/emotion_classifier.dart';
import 'package:heart_lake/edge_ai/local_dp.dart';

void main() {
  // ==================== EmotionClassifier ====================
  group('EmotionClassifier initial state', () {
    test('should not be loaded initially', () {
      final classifier = EmotionClassifier();
      expect(classifier.isLoaded, false);
    });

    test('should be loaded after loadModel', () async {
      final classifier = EmotionClassifier();
      await classifier.loadModel();
      expect(classifier.isLoaded, true);
    });

    test('should throw if classify before load', () {
      final classifier = EmotionClassifier();
      expect(
        () => classifier.classify(Float32List(24)),
        throwsA(isA<StateError>()),
      );
    });

    test('should not be loaded after dispose', () async {
      final classifier = EmotionClassifier();
      await classifier.loadModel();
      classifier.dispose();
      expect(classifier.isLoaded, false);
    });
  });

  group('EmotionClassifier classify', () {
    late EmotionClassifier classifier;

    setUp(() async {
      classifier = EmotionClassifier();
      await classifier.loadModel();
    });

    tearDown(() => classifier.dispose());

    test('should return scores for all 6 emotions', () async {
      final input = Float32List(24);
      final scores = await classifier.classify(input);
      expect(scores.length, 6);
      expect(scores.containsKey('happy'), true);
      expect(scores.containsKey('sad'), true);
      expect(scores.containsKey('angry'), true);
      expect(scores.containsKey('fearful'), true);
      expect(scores.containsKey('surprised'), true);
      expect(scores.containsKey('neutral'), true);
    });

    test('scores should sum to approximately 1.0 (softmax)', () async {
      final input = Float32List(24);
      for (int i = 0; i < 24; i++) {
        input[i] = 0.5;
      }
      final scores = await classifier.classify(input);
      final sum = scores.values.reduce((a, b) => a + b);
      expect(sum, closeTo(1.0, 0.01));
    });

    test('all scores should be non-negative', () async {
      final input = Float32List(24);
      final scores = await classifier.classify(input);
      for (final v in scores.values) {
        expect(v, greaterThanOrEqualTo(0.0));
      }
    });

    test('all scores should be <= 1.0', () async {
      final input = Float32List(24);
      final scores = await classifier.classify(input);
      for (final v in scores.values) {
        expect(v, lessThanOrEqualTo(1.0));
      }
    });

    test('should handle zero input', () async {
      final input = Float32List(24);
      final scores = await classifier.classify(input);
      expect(scores.length, 6);
      final sum = scores.values.reduce((a, b) => a + b);
      expect(sum, closeTo(1.0, 0.01));
    });

    test('should handle all-ones input', () async {
      final input = Float32List(24);
      for (int i = 0; i < 24; i++) { input[i] = 1.0; }
      final scores = await classifier.classify(input);
      expect(scores.length, 6);
      final sum = scores.values.reduce((a, b) => a + b);
      expect(sum, closeTo(1.0, 0.01));
    });

    test('should handle negative input values', () async {
      final input = Float32List(24);
      for (int i = 0; i < 24; i++) { input[i] = -0.5; }
      final scores = await classifier.classify(input);
      expect(scores.length, 6);
    });

    test('should handle large input values', () async {
      final input = Float32List(24);
      for (int i = 0; i < 24; i++) { input[i] = 100.0; }
      final scores = await classifier.classify(input);
      final sum = scores.values.reduce((a, b) => a + b);
      expect(sum, closeTo(1.0, 0.01));
    });

    test('should handle small input (less than 24 elements)', () async {
      final input = Float32List(10);
      for (int i = 0; i < 10; i++) { input[i] = 0.5; }
      final scores = await classifier.classify(input);
      expect(scores.length, 6);
    });

    test('should handle empty input', () async {
      final input = Float32List(0);
      final scores = await classifier.classify(input);
      expect(scores.length, 6);
    });

    test('high lexical signal should favor happy', () async {
      final input = Float32List(24);
      for (int i = 0; i < 20; i++) { input[i] = 1.0; }
      input[22] = 1.0; // exclaim signal
      final scores = await classifier.classify(input);
      expect(scores['happy'], greaterThan(scores['sad']!));
    });

    test('high punctuation signal should favor sad/fearful', () async {
      final input = Float32List(24);
      input[23] = 1.0; // punctuation signal
      final scores = await classifier.classify(input);
      expect(scores['sad']! + scores['fearful']!, greaterThan(scores['happy']!));
    });

    test('high exclaim signal should favor surprised', () async {
      final input = Float32List(24);
      input[22] = 1.0; // exclaim signal
      final scores = await classifier.classify(input);
      // surprised should be among the top
      final sortedEntries = scores.entries.toList()..sort((a, b) => b.value.compareTo(a.value));
      final topTwo = sortedEntries.take(2).map((e) => e.key).toList();
      expect(topTwo, contains('surprised'));
    });
  });

  group('EmotionClassifier getTopEmotion', () {
    late EmotionClassifier classifier;

    setUp(() async {
      classifier = EmotionClassifier();
      await classifier.loadModel();
    });

    tearDown(() => classifier.dispose());

    test('should return key with highest score', () {
      final scores = {'happy': 0.8, 'sad': 0.1, 'neutral': 0.1};
      expect(classifier.getTopEmotion(scores), 'happy');
    });

    test('should return first max when tied', () {
      final scores = {'happy': 0.5, 'sad': 0.5, 'neutral': 0.0};
      final top = classifier.getTopEmotion(scores);
      expect(['happy', 'sad'], contains(top));
    });

    test('should handle single entry', () {
      expect(classifier.getTopEmotion({'happy': 1.0}), 'happy');
    });

    test('should handle all zero scores', () {
      final scores = {'happy': 0.0, 'sad': 0.0, 'neutral': 0.0};
      expect(classifier.getTopEmotion(scores), isNotEmpty);
    });

    test('should handle negative scores', () {
      final scores = {'happy': -0.1, 'sad': -0.5, 'neutral': -0.2};
      expect(classifier.getTopEmotion(scores), 'happy');
    });

    test('should work with classify output', () async {
      final input = Float32List(24);
      final scores = await classifier.classify(input);
      final top = classifier.getTopEmotion(scores);
      expect(scores.containsKey(top), true);
      expect(scores[top], scores.values.reduce((a, b) => a > b ? a : b));
    });
  });

  // ==================== LocalDPClassifier ====================
  group('LocalDPClassifier', () {
    test('should not be loaded initially', () {
      final dpClassifier = LocalDPClassifier();
      expect(dpClassifier.isLoaded, false);
    });

    test('should be loaded after loadModel', () async {
      final dpClassifier = LocalDPClassifier();
      await dpClassifier.loadModel();
      expect(dpClassifier.isLoaded, true);
    });

    test('classifyWithPrivacy should return scores', () async {
      final dpClassifier = LocalDPClassifier(epsilon: 2.0);
      await dpClassifier.loadModel();
      final input = Float32List(24);
      for (int i = 0; i < 24; i++) { input[i] = 0.5; }
      final scores = await dpClassifier.classifyWithPrivacy(input);
      expect(scores.length, 6);
      dpClassifier.dispose();
    });

    test('classifyWithPrivacy without privacy should match raw', () async {
      final dpClassifier = LocalDPClassifier(epsilon: 2.0, privacyEnabled: false);
      await dpClassifier.loadModel();
      final classifier = EmotionClassifier();
      await classifier.loadModel();

      final input = Float32List(24);
      for (int i = 0; i < 24; i++) { input[i] = 0.5; }

      final rawScores = await classifier.classify(input);
      final dpScores = await dpClassifier.classifyWithPrivacy(input);

      // Without privacy, scores should be identical
      for (final key in rawScores.keys) {
        expect(dpScores[key], closeTo(rawScores[key]!, 0.001));
      }

      classifier.dispose();
      dpClassifier.dispose();
    });

    test('classifyWithPrivacy with privacy should add noise', () async {
      final dpClassifier = LocalDPClassifier(epsilon: 0.5, privacyEnabled: true);
      await dpClassifier.loadModel();

      final input = Float32List(24);
      for (int i = 0; i < 24; i++) { input[i] = 0.5; }

      // Run multiple times and check variance
      final results = <Map<String, double>>[];
      for (int i = 0; i < 10; i++) {
        results.add(await dpClassifier.classifyWithPrivacy(input));
      }

      // With privacy enabled, results should vary
      final happyScores = results.map((r) => r['happy']!).toSet();
      expect(happyScores.length, greaterThan(1));

      dpClassifier.dispose();
    });

    test('getTopEmotion should work', () async {
      final dpClassifier = LocalDPClassifier();
      await dpClassifier.loadModel();
      final input = Float32List(24);
      final scores = await dpClassifier.classifyWithPrivacy(input);
      final top = dpClassifier.getTopEmotion(scores);
      expect(scores.containsKey(top), true);
      dpClassifier.dispose();
    });

    test('privacyInfo should return correct info', () {
      final dpClassifier = LocalDPClassifier(epsilon: 3.0, privacyEnabled: true);
      final info = dpClassifier.privacyInfo;
      expect(info['enabled'], true);
      expect(info['epsilon'], 3.0);
      expect(info['mechanism'], contains('Laplace'));
      dpClassifier.dispose();
    });

    test('privacyInfo with disabled privacy', () {
      final dpClassifier = LocalDPClassifier(privacyEnabled: false);
      expect(dpClassifier.privacyInfo['enabled'], false);
      dpClassifier.dispose();
    });

    test('dispose should unload model', () async {
      final dpClassifier = LocalDPClassifier();
      await dpClassifier.loadModel();
      expect(dpClassifier.isLoaded, true);
      dpClassifier.dispose();
      expect(dpClassifier.isLoaded, false);
    });

    test('default epsilon should be 2.0', () {
      final dpClassifier = LocalDPClassifier();
      expect(dpClassifier.privacyInfo['epsilon'], 2.0);
      dpClassifier.dispose();
    });

    test('custom epsilon should be stored', () {
      final dpClassifier = LocalDPClassifier(epsilon: 5.0);
      expect(dpClassifier.privacyInfo['epsilon'], 5.0);
      dpClassifier.dispose();
    });
  });

  // ==================== LocalDifferentialPrivacy 扩展 ====================
  group('LocalDifferentialPrivacy extended', () {
    test('privatize with epsilon=0.01 should add large noise', () {
      final dp = LocalDifferentialPrivacy(epsilon: 0.01);
      var totalDeviation = 0.0;
      const iterations = 100;
      for (int i = 0; i < iterations; i++) {
        totalDeviation += (dp.privatize(0.5) - 0.5).abs();
      }
      final avgDeviation = totalDeviation / iterations;
      expect(avgDeviation, greaterThan(0.1));
    });

    test('privatize with epsilon=100 should add tiny noise', () {
      final dp = LocalDifferentialPrivacy(epsilon: 100.0);
      var totalDeviation = 0.0;
      const iterations = 100;
      for (int i = 0; i < iterations; i++) {
        totalDeviation += (dp.privatize(0.5) - 0.5).abs();
      }
      final avgDeviation = totalDeviation / iterations;
      expect(avgDeviation, lessThan(0.1));
    });

    test('privatize should always clamp to [-1, 1]', () {
      final dp = LocalDifferentialPrivacy(epsilon: 0.01);
      for (int i = 0; i < 100; i++) {
        final result = dp.privatize(0.99);
        expect(result, greaterThanOrEqualTo(-1.0));
        expect(result, lessThanOrEqualTo(1.0));
      }
    });

    test('privatizeDistribution should preserve keys', () {
      final dp = LocalDifferentialPrivacy(epsilon: 2.0);
      final scores = {'a': 0.5, 'b': 0.3, 'c': 0.2};
      final result = dp.privatizeDistribution(scores);
      expect(result.keys, equals(scores.keys));
    });

    test('privatizeDistribution should handle empty map', () {
      final dp = LocalDifferentialPrivacy(epsilon: 2.0);
      final result = dp.privatizeDistribution({});
      expect(result, isEmpty);
    });

    test('privatizeDistribution should handle single entry', () {
      final dp = LocalDifferentialPrivacy(epsilon: 2.0);
      final result = dp.privatizeDistribution({'only': 0.5});
      expect(result.length, 1);
      expect(result.containsKey('only'), true);
    });

    test('mean of privatized values should approximate true value', () {
      final dp = LocalDifferentialPrivacy(epsilon: 5.0);
      const trueVal = 0.3;
      var sum = 0.0;
      const n = 2000;
      for (int i = 0; i < n; i++) {
        sum += dp.privatize(trueVal);
      }
      expect(sum / n, closeTo(trueVal, 0.15));
    });

    test('privatize boundary value 1.0', () {
      final dp = LocalDifferentialPrivacy(epsilon: 2.0);
      for (int i = 0; i < 50; i++) {
        final r = dp.privatize(1.0);
        expect(r, lessThanOrEqualTo(1.0));
        expect(r, greaterThanOrEqualTo(-1.0));
      }
    });

    test('privatize boundary value -1.0', () {
      final dp = LocalDifferentialPrivacy(epsilon: 2.0);
      for (int i = 0; i < 50; i++) {
        final r = dp.privatize(-1.0);
        expect(r, lessThanOrEqualTo(1.0));
        expect(r, greaterThanOrEqualTo(-1.0));
      }
    });

    test('privatize boundary value 0.0', () {
      final dp = LocalDifferentialPrivacy(epsilon: 2.0);
      for (int i = 0; i < 50; i++) {
        final r = dp.privatize(0.0);
        expect(r, lessThanOrEqualTo(1.0));
        expect(r, greaterThanOrEqualTo(-1.0));
      }
    });
  });
}
