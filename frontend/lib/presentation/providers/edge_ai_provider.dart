// 端侧AI推理状态管理
//
// 采用「远程优先 + 本地降级」的混合推理策略：
// 1. 优先调用后端SentimentAnalyzer获取情绪分析结果
// 2. 后端弃权或置信度过低时，降级到本地规则引擎 + tflite模型
// 3. 两路结果按置信度加权融合，输出七维情绪概率分布
//
// 本地推理链路经过LocalDPClassifier的Laplace噪声注入，
// 确保上传数据满足epsilon-DP隐私保证。

import 'package:flutter/foundation.dart';
import '../../edge_ai/emotion_classifier.dart';
import '../../data/datasources/edge_ai_service.dart';
import '../../di/service_locator.dart';

/// 端侧 AI 推理状态管理器
///
/// 采用「远程优先 + 本地降级」的混合推理策略：
/// 1. 优先调用后端 SentimentAnalyzer 获取情绪分析结果
/// 2. 后端弃权（abstain）或置信度过低时，降级到本地规则引擎 + tflite 模型
/// 3. 两路结果按置信度加权融合，输出七维情绪概率分布
///
/// 本地推理链路经过 [LocalDPClassifier] 的 Laplace 噪声注入，
/// 确保上传数据满足 epsilon-DP 隐私保证。
///
/// 单例模式，通过 [ChangeNotifier] 驱动 UI 刷新。
class EdgeAIProvider extends ChangeNotifier {
  static final EdgeAIProvider _instance = EdgeAIProvider._();
  factory EdgeAIProvider() => _instance;
  EdgeAIProvider._();

  final LocalDPClassifier _classifier = LocalDPClassifier(epsilon: 2.0);
  final EdgeAIService _edgeService = sl<EdgeAIService>();
  bool _isReady = false;
  Map<String, double>? _lastResult;
  String? _lastEmotion;
  String? _lastRemoteWarning;
  bool _usedLocalFallback = false;

  bool get isReady => _isReady;
  Map<String, double>? get lastResult => _lastResult;
  String? get lastEmotion => _lastEmotion;
  String? get lastRemoteWarning => _lastRemoteWarning;
  bool get usedLocalFallback => _usedLocalFallback;
  Map<String, dynamic> get privacyInfo => _classifier.privacyInfo;

  void _reportProviderError(
    Object error,
    StackTrace stackTrace,
    String context,
  ) {
    FlutterError.reportError(
      FlutterErrorDetails(
        exception: error,
        stack: stackTrace,
        library: 'heartlake',
        context: ErrorDescription(context),
      ),
    );
  }

  /// 加载本地分类模型，首次调用后标记就绪
  Future<void> initialize() async {
    if (_isReady) return;
    await _classifier.loadModel();
    _isReady = true;
    notifyListeners();
  }

  /// 对输入文本进行情绪分类，返回七维概率分布
  ///
  /// 推理流程：远程分析 -> 置信度校验 -> 上下文修正 -> 本地降级融合。
  /// 结果缓存在 [lastResult] 和 [lastEmotion] 中，并触发 UI 刷新。
  Future<Map<String, double>> classifyText(String text) async {
    if (!_isReady) await initialize();
    late Map<String, double> result;
    _usedLocalFallback = false;

    // 优先使用后端情感分析，失败时降级到本地模型
    final remote = await analyzeRemote(text);
    if (remote != null) {
      final mood = (remote['mood'] ?? remote['sentiment'] ?? 'neutral')
          .toString()
          .toLowerCase();
      final confidence = _extractConfidence(remote);
      final normalizedMood = _normalizeMood(mood);
      final corrected =
          _applyContextCorrection(text, normalizedMood, confidence);
      final shouldFallback = _shouldFallbackFromRemote(remote, corrected.value);
      if (shouldFallback) {
        _usedLocalFallback = true;
        final fallback = await _classifyFallback(text);
        final remoteHint = _buildDistribution(corrected.key, corrected.value);
        if (_isRemoteAbstained(remote)) {
          result = _blendDistributions(fallback, remoteHint, 0.10);
        } else {
          final fallbackWeight = corrected.value < 0.5 ? 0.28 : 0.18;
          result = _blendDistributions(remoteHint, fallback, fallbackWeight);
        }
      } else {
        result = _buildDistribution(corrected.key, corrected.value);
      }
    } else {
      _usedLocalFallback = true;
      result = await _classifyFallback(text);
    }

    _lastResult = result;
    _lastEmotion = _classifier.getTopEmotion(result);
    notifyListeners();
    return result;
  }

  /// 将文本转为 24 维特征向量，供本地分类器使用
  ///
  /// 特征布局：[0..19] Unicode 码点归一化，[20] 文本长度，
  /// [21] 汉字比例，[22] 感叹/问号信号，[23] 标点密度。
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

  /// 情绪英文标签 -> 中文显示名映射
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

  /// 获取情绪的中文标签，未知标签原样返回
  String getEmotionLabel(String emotion) => emotionLabels[emotion] ?? emotion;

  /// 将后端返回的多样化情绪标签统一映射到七维标准标签
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

  /// 根据主情绪标签和置信度构造七维概率分布
  ///
  /// [topEmotion] 获得 [confidence] 的概率，其余标签均分剩余概率。
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

  /// 基于中文情绪词典的规则引擎分类
  ///
  /// 匹配七类情绪关键词并累加分数，同时处理转折词（但是/不过等）
  /// 对转折后的情绪倾向做额外加权，最后归一化输出概率分布。
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
    const anxiousWords = ['焦虑', '担心', '不安', '紧张', '害怕', '恐惧', '心慌', '压力'];
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
        final tailPositive =
            _containsAny(tail, happyWords) || _containsAny(tail, surpriseWords);
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

  /// 将本地分类器的六维输出映射到标准七维标签并归一化
  Map<String, double> _normalizeLocalDistribution(
      Map<String, double> localRaw) {
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

    return _normalizeDistribution(normalized);
  }

  /// 融合规则引擎和本地模型的分类结果
  ///
  /// 当规则引擎的 top-1 与 top-2 差距 >= 0.12 时，规则权重提升到 0.78，
  /// 否则降为 0.58，让本地模型有更多话语权。
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

  /// 本地降级分类：规则引擎 + 本地 DP 模型融合
  Future<Map<String, double>> _classifyFallback(String text) async {
    final ruleBased = _classifyByRules(text);
    final features = _textToFeatures(text);
    final localRaw = await _classifier.classifyWithPrivacy(features);
    final localNormalized = _normalizeLocalDistribution(localRaw);
    return _mergeFallback(ruleBased, localNormalized);
  }

  /// 判断后端是否弃权（abstain），即模型不确定无法给出可靠结果
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

  /// 判断是否需要降级到本地推理
  ///
  /// 触发条件：后端弃权、不确定性 > 0.74、或可靠性为 low 且置信度 < 0.30。
  bool _shouldFallbackFromRemote(
      Map<String, dynamic> remote, double confidence) {
    if (_isRemoteAbstained(remote)) return true;
    final uncertaintyRaw = remote['uncertainty'];
    if (uncertaintyRaw is num && uncertaintyRaw.toDouble() > 0.74) return true;
    final tier = (remote['reliability_tier'] ?? '').toString().toLowerCase();
    if (tier == 'low' && confidence < 0.30) return true;
    return false;
  }

  /// 按权重混合两个概率分布，[secondaryWeight] 上限 0.4
  Map<String, double> _blendDistributions(Map<String, double> primary,
      Map<String, double> secondary, double secondaryWeight) {
    final w2 = secondaryWeight.clamp(0.0, 0.4);
    final w1 = 1.0 - w2;
    final keys = <String>{...primary.keys, ...secondary.keys};
    final merged = <String, double>{};
    for (final key in keys) {
      merged[key] = (primary[key] ?? 0.0) * w1 + (secondary[key] ?? 0.0) * w2;
    }
    return _normalizeDistribution(merged);
  }

  /// 上下文修正：根据文本中的事件词和情绪词修正远程分析结果
  ///
  /// 例如文本含「收到礼物」但远程判为 anxious，则修正为 happy。
  /// 返回修正后的 (情绪标签, 置信度) 对。
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
      '悲伤',
      '难过',
      '伤心',
      '难受',
      '低落',
      '压抑',
      '委屈',
      '失落',
      '想哭',
      '害怕',
      '恐惧',
      '压力',
      '烦躁',
      '痛苦',
      '绝望',
      '崩溃'
    ];
    const anxietyWords = [
      '焦虑',
      '担心',
      '不安',
      '心慌',
      '害怕',
      '恐惧',
      '紧张',
      '压力',
      '睡不着',
      '失眠'
    ];
    const sadnessWords = [
      '悲伤',
      '难过',
      '伤心',
      '难受',
      '低落',
      '失落',
      '压抑',
      '委屈',
      '想哭',
      '无助',
      '痛苦',
      '绝望'
    ];

    final hasPositiveEvent = _containsAny(lower, positiveEventWords);
    final hasNegative = _containsAny(lower, negativeWords);
    final hasAnxietyCue = _containsAny(lower, anxietyWords);
    final hasSadCue = _containsAny(lower, sadnessWords);

    var correctedMood = mood;
    var correctedConfidence = confidence;

    if (hasPositiveEvent && !hasNegative) {
      if (correctedMood == 'anxious' ||
          correctedMood == 'sad' ||
          correctedMood == 'neutral' ||
          correctedMood == 'calm') {
        correctedMood = 'happy';
      }
      correctedConfidence =
          correctedConfidence < 0.72 ? 0.72 : correctedConfidence;
    }

    if (_containsAny(lower, const ['夸', '表扬', '认可', '肯定']) && !hasNegative) {
      correctedMood = 'happy';
      correctedConfidence =
          correctedConfidence < 0.75 ? 0.75 : correctedConfidence;
    }

    if (hasNegative &&
        (correctedMood == 'calm' ||
            correctedMood == 'neutral' ||
            correctedMood == 'happy')) {
      if (hasAnxietyCue) {
        correctedMood = 'anxious';
        correctedConfidence =
            correctedConfidence < 0.70 ? 0.70 : correctedConfidence;
      } else if (hasSadCue) {
        correctedMood = 'sad';
        correctedConfidence =
            correctedConfidence < 0.70 ? 0.70 : correctedConfidence;
      }
    }

    return MapEntry(correctedMood, correctedConfidence.clamp(0.2, 0.95));
  }

  /// 将概率分布归一化到 [0, 1] 且总和为 1，负值截断为 0
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

  /// 检查文本中是否包含给定短语列表中的任意一个
  bool _containsAny(String text, List<String> phrases) {
    for (final phrase in phrases) {
      if (text.contains(phrase)) return true;
    }
    return false;
  }

  /// 从后端响应中提取置信度，优先级：calibrated_confidence > confidence > score
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

  /// 调用后端情感分析接口，失败时记录警告并返回 null 由调用方降级处理
  Future<Map<String, dynamic>?> analyzeRemote(String text) async {
    try {
      final resp = await _edgeService.analyzeSentiment(text);
      if (resp['success'] == true && resp['data'] is Map) {
        _lastRemoteWarning = null;
        return Map<String, dynamic>.from(resp['data'] as Map);
      }
      _lastRemoteWarning = resp['message']?.toString() ?? '远端情绪分析失败，已降级到本地模型';
      _reportProviderError(
        StateError(
            resp['message']?.toString() ?? 'Remote sentiment analysis failed'),
        StackTrace.current,
        'EdgeAIProvider.analyzeRemote',
      );
    } catch (error, stackTrace) {
      _lastRemoteWarning = '远端情绪分析失败，已降级到本地模型';
      _reportProviderError(
        error,
        stackTrace,
        'EdgeAIProvider.analyzeRemote',
      );
    }
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
