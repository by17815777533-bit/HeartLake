// @file edge_ai_provider.dart
// @brief EdgeAI Provider - 管理本地AI推理状态

import 'package:flutter/foundation.dart';
import '../edge_ai/emotion_classifier.dart';
import '../data/datasources/edge_ai_service.dart';

class EdgeAIProvider extends ChangeNotifier {
  static final EdgeAIProvider _instance = EdgeAIProvider._();
  factory EdgeAIProvider() => _instance;
  EdgeAIProvider._();

  final LocalDPClassifier _classifier = LocalDPClassifier(epsilon: 2.0);
  final EdgeAIService _edgeService = EdgeAIService();
  bool _isReady = false;
  Map<String, double>? _lastResult;
  String? _lastEmotion;

  bool get isReady => _isReady;
  Map<String, double>? get lastResult => _lastResult;
  String? get lastEmotion => _lastEmotion;
  Map<String, dynamic> get privacyInfo => _classifier.privacyInfo;

  Future<void> initialize() async {
    if (_isReady) return;
    await _classifier.loadModel();
    _isReady = true;
    notifyListeners();
  }

  Future<Map<String, double>> classifyText(String text) async {
    if (!_isReady) await initialize();
    late Map<String, double> result;

    // 优先使用后端情感分析，失败时降级到本地模型
    final remote = await analyzeRemote(text);
    if (remote != null) {
      final mood = (remote['mood'] ?? remote['sentiment'] ?? 'neutral')
          .toString()
          .toLowerCase();
      final confidence = _extractConfidence(remote);
      final normalizedMood = _normalizeMood(mood);
      final corrected = _applyContextCorrection(text, normalizedMood, confidence);
      final shouldFallback = _shouldFallbackFromRemote(remote, corrected.value);
      if (shouldFallback) {
        final fallback = await _classifyFallback(text);
        final remoteHint = _buildDistribution(corrected.key, corrected.value);
        final hintWeight = _isRemoteAbstained(remote) ? 0.08 : 0.18;
        result = _blendDistributions(fallback, remoteHint, hintWeight);
      } else {
        result = _buildDistribution(corrected.key, corrected.value);
      }
    } else {
      result = await _classifyFallback(text);
    }

    _lastResult = result;
    _lastEmotion = _classifier.getTopEmotion(result);
    notifyListeners();
    return result;
  }

  Float32List _textToFeatures(String text) {
    final features = Float32List(24);
    final runes = text.runes.toList();
    for (var i = 0; i < runes.length && i < 20; i++) {
      features[i] = runes[i] / 65536.0;
    }
    features[20] = text.length / 200.0;
    features[21] = runes.where((r) => r > 0x4E00 && r < 0x9FFF).length /
        (runes.length + 1);
    features[22] = text.contains(RegExp(r'[!！？?]')) ? 1.0 : 0.0;
    features[23] = text.contains(RegExp(r'[。，、；]')) ? 0.5 : 0.0;
    return features;
  }

  static const Map<String, String> emotionLabels = {
    'happy': '开心',
    'calm': '平静',
    'sad': '悲伤',
    'angry': '愤怒',
    'anxious': '焦虑',
    'fearful': '焦虑',
    'surprised': '惊喜',
    'neutral': '中性',
  };

  String getEmotionLabel(String emotion) => emotionLabels[emotion] ?? emotion;

  String _normalizeMood(String mood) {
    switch (mood) {
      case 'joy':
      case 'excited':
      case 'grateful':
      case 'love':
        return 'happy';
      case 'calm':
        return 'calm';
      case 'anxious':
      case 'fear':
      case 'fearful':
      case 'lonely':
      case 'confused':
        return 'anxious';
      case 'surprise':
        return 'surprised';
      default:
        return emotionLabels.containsKey(mood) ? mood : 'neutral';
    }
  }

  Map<String, double> _buildDistribution(String topEmotion, double confidence) {
    const labels = [
      'happy',
      'calm',
      'neutral',
      'surprised',
      'anxious',
      'sad',
      'angry'
    ];
    final result = <String, double>{for (final e in labels) e: 0.0};
    final top = confidence.clamp(0.2, 0.9);
    final rest = (1.0 - top) / (labels.length - 1);
    for (final label in labels) {
      result[label] = label == topEmotion ? top : rest;
    }
    return result;
  }

  Map<String, double> _classifyByRules(String text) {
    final lower = text.toLowerCase();
    final scores = <String, double>{
      'happy': 0.08,
      'calm': 0.08,
      'neutral': 0.12,
      'surprised': 0.06,
      'anxious': 0.06,
      'sad': 0.06,
      'angry': 0.06,
    };

    const happyWords = [
      '开心',
      '快乐',
      '高兴',
      '礼物',
      '收到',
      '夸',
      '表扬',
      '认可',
      '肯定',
      '成功',
      '通过',
      '惊喜',
      '感谢',
      '感恩',
      '喜欢',
      '幸福',
      '老师夸',
      '收到了'
    ];
    const calmWords = ['平静', '安心', '放松', '宁静', '治愈', '舒心', '稳定'];
    const anxiousWords = [
      '焦虑',
      '担心',
      '不安',
      '紧张',
      '害怕',
      '恐惧',
      '心慌',
      '压力'
    ];
    const sadWords = ['难过', '伤心', '失落', '委屈', '沮丧', 'emo', '难受', '哭'];
    const angryWords = ['生气', '愤怒', '烦躁', '火大', '讨厌', '崩溃', '暴怒', '气死'];
    const surpriseWords = ['惊喜', '意外', '没想到', '突然', '居然', '哇', '太棒了'];
    const contrastWords = ['但是', '但', '不过', '然而', '可是', '只是'];

    for (final w in happyWords) {
      if (lower.contains(w)) scores['happy'] = scores['happy']! + 0.48;
    }
    for (final w in calmWords) {
      if (lower.contains(w)) scores['calm'] = scores['calm']! + 0.38;
    }
    for (final w in anxiousWords) {
      if (lower.contains(w)) scores['anxious'] = scores['anxious']! + 0.5;
    }
    for (final w in sadWords) {
      if (lower.contains(w)) scores['sad'] = scores['sad']! + 0.5;
    }
    for (final w in angryWords) {
      if (lower.contains(w)) scores['angry'] = scores['angry']! + 0.5;
    }
    for (final w in surpriseWords) {
      if (lower.contains(w)) scores['surprised'] = scores['surprised']! + 0.42;
    }

    for (final marker in contrastWords) {
      final idx = lower.lastIndexOf(marker);
      if (idx >= 0 && idx + marker.length < lower.length) {
        final tail = lower.substring(idx + marker.length);
        final tailNegative = _containsAny(tail, anxiousWords) ||
            _containsAny(tail, sadWords) ||
            _containsAny(tail, angryWords);
        final tailPositive = _containsAny(tail, happyWords) || _containsAny(tail, surpriseWords);
        if (tailNegative) {
          scores['anxious'] = scores['anxious']! + 0.28;
          scores['sad'] = scores['sad']! + 0.16;
        } else if (tailPositive) {
          scores['happy'] = scores['happy']! + 0.24;
        }
        break;
      }
    }

    if (lower.contains('!') || lower.contains('！')) {
      scores['surprised'] = scores['surprised']! + 0.1;
    }

    final hasPositiveEvent = _containsAny(lower, happyWords) &&
        !_containsAny(lower, anxiousWords) &&
        !_containsAny(lower, sadWords) &&
        !_containsAny(lower, angryWords);
    if (hasPositiveEvent) {
      scores['happy'] = scores['happy']! + 0.25;
    }

    return _normalizeDistribution(scores);
  }

  Map<String, double> _normalizeLocalDistribution(Map<String, double> localRaw) {
    final normalized = <String, double>{
      'happy': 0.0,
      'calm': 0.0,
      'neutral': 0.0,
      'surprised': 0.0,
      'anxious': 0.0,
      'sad': 0.0,
      'angry': 0.0,
    };

    localRaw.forEach((key, value) {
      final k = _normalizeMood(key.toLowerCase());
      if (normalized.containsKey(k)) {
        normalized[k] = normalized[k]! + value;
      } else {
        normalized['neutral'] = normalized['neutral']! + value;
      }
    });

    // 无 calm 输出时，从 neutral 拆分一部分，避免“全是中性”带来的失真。
    if ((normalized['calm'] ?? 0.0) < 0.05 && (normalized['neutral'] ?? 0.0) > 0.18) {
      final transfer = normalized['neutral']! * 0.35;
      normalized['neutral'] = normalized['neutral']! - transfer;
      normalized['calm'] = normalized['calm']! + transfer;
    }

    return _normalizeDistribution(normalized);
  }

  Map<String, double> _mergeFallback(
      Map<String, double> ruleBased, Map<String, double> localBased) {
    final sorted = ruleBased.values.toList()..sort((a, b) => b.compareTo(a));
    final margin = sorted.length >= 2 ? (sorted[0] - sorted[1]) : 0.0;
    final ruleWeight = margin >= 0.12 ? 0.78 : 0.58;

    final merged = <String, double>{};
    for (final key in ruleBased.keys) {
      final rv = ruleBased[key] ?? 0.0;
      final lv = localBased[key] ?? 0.0;
      merged[key] = rv * ruleWeight + lv * (1.0 - ruleWeight);
    }
    return _normalizeDistribution(merged);
  }

  Future<Map<String, double>> _classifyFallback(String text) async {
    final ruleBased = _classifyByRules(text);
    final features = _textToFeatures(text);
    final localRaw = await _classifier.classifyWithPrivacy(features);
    final localNormalized = _normalizeLocalDistribution(localRaw);
    return _mergeFallback(ruleBased, localNormalized);
  }

  bool _isRemoteAbstained(Map<String, dynamic> remote) {
    final abstained = remote['abstained'];
    if (abstained is bool) return abstained;
    if (abstained is String) {
      final normalized = abstained.toLowerCase();
      if (normalized == 'true' || normalized == '1') return true;
    }
    final decision = (remote['decision'] ?? '').toString().toLowerCase();
    return decision == 'abstain';
  }

  bool _shouldFallbackFromRemote(Map<String, dynamic> remote, double confidence) {
    if (_isRemoteAbstained(remote)) return true;
    final uncertaintyRaw = remote['uncertainty'];
    if (uncertaintyRaw is num && uncertaintyRaw.toDouble() > 0.62) return true;
    final tier = (remote['reliability_tier'] ?? '').toString().toLowerCase();
    if (tier == 'low' && confidence < 0.55) return true;
    return confidence < 0.40;
  }

  Map<String, double> _blendDistributions(
      Map<String, double> primary,
      Map<String, double> secondary,
      double secondaryWeight) {
    final w2 = secondaryWeight.clamp(0.0, 0.4);
    final w1 = 1.0 - w2;
    final keys = <String>{...primary.keys, ...secondary.keys};
    final merged = <String, double>{};
    for (final key in keys) {
      merged[key] = (primary[key] ?? 0.0) * w1 + (secondary[key] ?? 0.0) * w2;
    }
    return _normalizeDistribution(merged);
  }

  MapEntry<String, double> _applyContextCorrection(
      String text, String mood, double confidence) {
    final lower = text.toLowerCase();
    const positiveEventWords = [
      '礼物',
      '收到',
      '收到了',
      '夸',
      '表扬',
      '认可',
      '肯定',
      '成功',
      '通过',
      '晋级',
      '获奖',
      '惊喜'
    ];
    const negativeWords = [
      '焦虑',
      '担心',
      '不安',
      '难过',
      '伤心',
      '害怕',
      '恐惧',
      '压力',
      '烦躁',
      '痛苦',
      '绝望',
      '崩溃'
    ];

    final hasPositiveEvent = _containsAny(lower, positiveEventWords);
    final hasNegative = _containsAny(lower, negativeWords);

    var correctedMood = mood;
    var correctedConfidence = confidence;

    if (hasPositiveEvent && !hasNegative) {
      if (correctedMood == 'anxious' ||
          correctedMood == 'sad' ||
          correctedMood == 'neutral' ||
          correctedMood == 'calm') {
        correctedMood = 'happy';
      }
      correctedConfidence = correctedConfidence < 0.72 ? 0.72 : correctedConfidence;
    }

    if (_containsAny(lower, const ['夸', '表扬', '认可', '肯定']) && !hasNegative) {
      correctedMood = 'happy';
      correctedConfidence = correctedConfidence < 0.75 ? 0.75 : correctedConfidence;
    }

    return MapEntry(correctedMood, correctedConfidence.clamp(0.2, 0.95));
  }

  Map<String, double> _normalizeDistribution(Map<String, double> raw) {
    final result = <String, double>{};
    double sum = 0.0;
    raw.forEach((k, v) {
      final clipped = v < 0 ? 0.0 : v;
      result[k] = clipped;
      sum += clipped;
    });
    if (sum <= 0.0) {
      final uniform = 1.0 / result.length;
      for (final k in result.keys) {
        result[k] = uniform;
      }
      return result;
    }
    for (final k in result.keys.toList()) {
      result[k] = result[k]! / sum;
    }
    return result;
  }

  bool _containsAny(String text, List<String> phrases) {
    for (final phrase in phrases) {
      if (text.contains(phrase)) return true;
    }
    return false;
  }

  double _extractConfidence(Map<String, dynamic> remote) {
    final calibratedRaw = remote['calibrated_confidence'];
    if (calibratedRaw is num) {
      return calibratedRaw.toDouble().clamp(0.0, 1.0);
    }
    final confidenceRaw = remote['confidence'];
    if (confidenceRaw is num) {
      return confidenceRaw.toDouble().clamp(0.0, 1.0);
    }
    final scoreRaw = remote['score'];
    if (scoreRaw is num) {
      return (0.45 + scoreRaw.toDouble().abs() * 0.4).clamp(0.2, 0.85);
    }
    return 0.5;
  }

  /// 后端情感分析
  Future<Map<String, dynamic>?> analyzeRemote(String text) async {
    try {
      final resp = await _edgeService.analyzeSentiment(text);
      if (resp['success'] == true && resp['data'] != null) {
        return resp['data'] as Map<String, dynamic>;
      }
    } catch (_) {}
    return null;
  }

  // 注意：内容审核(moderateContent)为admin专用端点(/api/admin/edge-ai/moderate)
  // 前端不应直接调用，后端在createStone和lakeGodChat中已内置审核逻辑

  @override
  void dispose() {
    _classifier.dispose();
    super.dispose();
  }
}
