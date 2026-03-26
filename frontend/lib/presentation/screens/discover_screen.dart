import 'package:flutter/material.dart';
import '../../data/datasources/recommendation_service.dart';
import '../../data/datasources/ai_recommendation_service.dart';
import '../../di/service_locator.dart';
import '../../domain/entities/stone.dart';
import '../widgets/stone_card/stone_card.dart';
import '../widgets/water_background.dart';
import '../widgets/deep_dive_layer.dart';
import '../../utils/app_theme.dart';
import 'personalized_screen.dart';
import 'stone_detail_screen.dart';

/// 共鸣发现页面
///
/// 三个 Tab 组成的内容发现入口：
/// - 热门：按热度排序的石头列表，使用 [DeepDiveSpace] 深潜式布局
/// - 找心声：关键词搜索石头（语义搜索仅管理端可用，前端返回空）
/// - 湖神陪伴：AI 个性化推荐 + 高级共鸣推荐，合并去重后展示
///
/// 切换 Tab 时自动加载对应数据，支持下拉刷新。
/// 点击推荐石头会上报 click 交互事件，用于推荐引擎在线学习。
class DiscoverScreen extends StatefulWidget {
  const DiscoverScreen({super.key});

  @override
  State<DiscoverScreen> createState() => _DiscoverScreenState();
}

class _DiscoverScreenState extends State<DiscoverScreen>
    with SingleTickerProviderStateMixin {
  final RecommendationService _service = sl<RecommendationService>();
  final AIRecommendationService _aiService = sl<AIRecommendationService>();
  final TextEditingController _searchController = TextEditingController();
  late TabController _tabController;

  List<Stone> _stones = [];
  List<Stone> _keywordResults = [];
  List<Stone> _semanticResults = [];
  List<Stone> _personalizedAIStones = [];
  List<Stone> _advancedAIStones = [];
  bool _trendingLoading = false;
  bool _searchLoading = false;
  bool _aiLoading = false;
  bool _hasSearched = false;
  String? _trendingErrorMessage;
  String? _personalizedAIErrorMessage;
  String? _advancedAIErrorMessage;

  @override
  void initState() {
    super.initState();
    _tabController = TabController(length: 3, vsync: this);
    _tabController.addListener(_onTabChanged);
    _loadTrending();
  }

  /// Tab 切换回调，热门和湖神陪伴 Tab 自动加载数据
  void _onTabChanged() {
    if (_tabController.indexIsChanging) return;
    if (_tabController.index == 0) {
      _loadTrending();
    } else if (_tabController.index == 2) {
      _loadAIRecommendations();
    }
  }

  @override
  void dispose() {
    _tabController.removeListener(_onTabChanged);
    _tabController.dispose();
    _searchController.dispose();
    super.dispose();
  }

  void _reportUiError(
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

  void _showMessage(String message) {
    final messenger = ScaffoldMessenger.maybeOf(context);
    if (messenger == null) return;
    messenger
      ..hideCurrentSnackBar()
      ..showSnackBar(SnackBar(content: Text(message)));
  }

  String _resolveErrorMessage(Object error, String fallback) {
    final message = error.toString().trim();
    if (message.isEmpty) return fallback;
    if (message.startsWith('Bad state: ')) {
      return message.substring('Bad state: '.length).trim();
    }
    if (message.startsWith('Exception: ')) {
      return message.substring('Exception: '.length).trim();
    }
    return message;
  }

  Future<_AIRecommendationLoadResult> _loadAIBranch(
    Future<List<Map<String, dynamic>>> Function() request,
    String context,
    String fallbackMessage,
  ) async {
    try {
      final payload = await request();
      return _AIRecommendationLoadResult.success(
        payload.map((item) => Stone.fromJson(item)).toList(),
      );
    } catch (error, stackTrace) {
      _reportUiError(error, stackTrace, context);
      return _AIRecommendationLoadResult.failure(
        _resolveErrorMessage(error, fallbackMessage),
      );
    }
  }

  /// 加载热门石头列表
  Future<void> _loadTrending({bool showFeedback = false}) async {
    final hadData = _stones.isNotEmpty;
    if (!hadData && mounted) {
      setState(() => _trendingLoading = true);
    }
    try {
      final items = await _service.getTrending();
      if (mounted) {
        setState(() {
          _stones = items.map((e) => Stone.fromJson(e)).toList();
          _trendingErrorMessage = null;
        });
      }
    } catch (error, stackTrace) {
      _reportUiError(error, stackTrace, 'DiscoverScreen._loadTrending');
      final message = _resolveErrorMessage(error, '加载热门内容失败，请稍后再试');
      if (mounted) {
        setState(() => _trendingErrorMessage = message);
        if (showFeedback || !hadData) {
          _showMessage(message);
        }
      }
    } finally {
      if (mounted) setState(() => _trendingLoading = false);
    }
  }

  /// 执行心声查找（深层语义匹配端点当前仅在管理端使用）
  Future<void> _searchStones(String query) async {
    if (query.isEmpty) return;
    if (mounted) {
      setState(() => _searchLoading = true);
    }
    try {
      final results = await _service.search(query);

      if (mounted) {
        setState(() {
          _keywordResults = results.map((e) => Stone.fromJson(e)).toList();
          // 向量搜索为admin端点(/api/admin/edge-ai/vector-search)，前端返回空
          _semanticResults = [];
          _hasSearched = true;
        });
      }
    } catch (error, stackTrace) {
      _reportUiError(error, stackTrace, 'DiscoverScreen._searchStones');
      final message = _resolveErrorMessage(error, '暂时没接住你的心声，请稍后再试');
      if (mounted) {
        _showMessage(message);
      }
    } finally {
      if (mounted) setState(() => _searchLoading = false);
    }
  }

  /// 加载湖神个性化推荐
  Future<void> _loadAIRecommendations({bool showFeedback = false}) async {
    final hadVisibleData =
        _personalizedAIStones.isNotEmpty || _advancedAIStones.isNotEmpty;
    if (!hadVisibleData && mounted) {
      setState(() => _aiLoading = true);
    }

    final results = await Future.wait<_AIRecommendationLoadResult>([
      _loadAIBranch(
        () => _aiService.getPersonalizedRecommendations(limit: 10),
        'DiscoverScreen._loadAIRecommendations.personalized',
        '加载个性化推荐失败',
      ),
      _loadAIBranch(
        () => _aiService.getAdvancedRecommendations(limit: 10),
        'DiscoverScreen._loadAIRecommendations.advanced',
        '加载高级共鸣推荐失败',
      ),
    ]);

    if (!mounted) return;

    final personalized = results[0];
    final advanced = results[1];

    setState(() {
      if (personalized.items != null) {
        _personalizedAIStones = personalized.items!;
      }
      if (advanced.items != null) {
        _advancedAIStones = advanced.items!;
      }
      _personalizedAIErrorMessage = personalized.errorMessage;
      _advancedAIErrorMessage = advanced.errorMessage;
      _aiLoading = false;
    });

    if (!showFeedback) return;

    final failureCount =
        results.where((item) => item.errorMessage != null).length;
    if (failureCount == 2) {
      _showMessage('湖神陪伴内容刷新失败，请稍后再试');
      return;
    }
    if (failureCount == 1) {
      _showMessage('部分湖神陪伴内容未更新，已保留上次结果');
    }
  }

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return Scaffold(
      extendBodyBehindAppBar: true,
      backgroundColor: Colors.transparent,
      appBar: AppBar(
        title: Text('共鸣',
            style: TextStyle(
                fontWeight: FontWeight.w600,
                color: isDark ? Colors.white : const Color(0xFF5D4037))),
        backgroundColor: Colors.transparent,
        elevation: 0,
        scrolledUnderElevation: 0,
        actions: [
          IconButton(
            icon: Icon(Icons.auto_awesome,
                color: isDark ? Colors.white : const Color(0xFF5D4037)),
            tooltip: '贴心内容',
            onPressed: () => Navigator.push(context,
                MaterialPageRoute(builder: (_) => const PersonalizedScreen())),
          ),
        ],
        bottom: TabBar(
          controller: _tabController,
          indicatorColor: AppTheme.accentColor,
          indicatorWeight: 3,
          labelColor: isDark ? Colors.white : const Color(0xFF5D4037),
          unselectedLabelColor:
              isDark ? Colors.white70 : const Color(0xFF8D6E63),
          tabs: const [
            Tab(icon: Icon(Icons.local_fire_department), text: '热门'),
            Tab(icon: Icon(Icons.search), text: '找心声'),
            Tab(icon: Icon(Icons.auto_awesome), text: '湖神陪伴'),
          ],
        ),
      ),
      body: Stack(
        children: [
          const WaterBackground(),
          SafeArea(
            child: TabBarView(
              controller: _tabController,
              children: [
                RefreshIndicator(
                  onRefresh: () => _loadTrending(showFeedback: true),
                  color: AppTheme.accentColor,
                  child: _buildStoneList(
                    _stones,
                    isLoading: _trendingLoading,
                    errorMessage: _trendingErrorMessage,
                  ),
                ),
                _buildSearchTab(),
                RefreshIndicator(
                  onRefresh: () => _loadAIRecommendations(showFeedback: true),
                  color: AppTheme.accentColor,
                  child: _buildAITab(),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildSearchTab() {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return Column(
      children: [
        Padding(
          padding: const EdgeInsets.all(16),
          child: SearchBar(
            controller: _searchController,
            hintText: '写下你想找的心声...',
            elevation: WidgetStateProperty.all(0),
            backgroundColor: WidgetStateProperty.all(
                (isDark ? const Color(0xFF16213E) : Colors.white)
                    .withValues(alpha: 0.9)),
            trailing: [
              IconButton(
                icon: const Icon(Icons.search),
                onPressed: () => _searchStones(_searchController.text),
              ),
            ],
            onSubmitted: _searchStones,
          ),
        ),
        Expanded(
          child: _searchLoading
              ? _buildLoadingIndicator()
              : _hasSearched
                  ? _buildSearchResults()
                  : _buildStoneList(
                      _stones,
                      isLoading: _trendingLoading,
                      errorMessage: _trendingErrorMessage,
                    ),
        ),
      ],
    );
  }

  /// 构建搜索结果区域，区分文字相近和心意相近两个分组
  Widget _buildSearchResults() {
    if (_keywordResults.isEmpty && _semanticResults.isEmpty) {
      return Center(
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(Icons.search_off,
                size: 64, color: Colors.white.withValues(alpha: 0.5)),
            const SizedBox(height: 16),
            Text('还没找到相近的心声',
                style: TextStyle(color: Colors.white.withValues(alpha: 0.7))),
          ],
        ),
      );
    }

    return ListView(
      physics:
          const AlwaysScrollableScrollPhysics(parent: BouncingScrollPhysics()),
      padding: const EdgeInsets.symmetric(horizontal: 8),
      children: [
        if (_keywordResults.isNotEmpty) ...[
          _buildResultSectionHeader(
            '文字相近',
            Icons.text_fields,
            '${_keywordResults.length}条',
          ),
          ..._keywordResults.map((s) => StoneCard(stone: s)),
        ],
        if (_semanticResults.isNotEmpty) ...[
          const SizedBox(height: 8),
          _buildResultSectionHeader(
            '心意相近',
            Icons.psychology,
            '${_semanticResults.length}条',
          ),
          ..._semanticResults.map((s) => StoneCard(stone: s)),
        ],
        const SizedBox(height: 24),
      ],
    );
  }

  Widget _buildResultSectionHeader(
      String title, IconData icon, String subtitle) {
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 10),
      child: Row(
        children: [
          Icon(icon, size: 18, color: Colors.white.withValues(alpha: 0.9)),
          const SizedBox(width: 8),
          Text(
            title,
            style: TextStyle(
              fontSize: 15,
              fontWeight: FontWeight.w600,
              color: Colors.white.withValues(alpha: 0.95),
            ),
          ),
          const SizedBox(width: 8),
          Text(
            subtitle,
            style: TextStyle(
              fontSize: 12,
              color: Colors.white.withValues(alpha: 0.6),
            ),
          ),
        ],
      ),
    );
  }

  /// 湖神推荐Tab
  Widget _buildAITab() {
    final warningMessages = [
      _personalizedAIErrorMessage,
      _advancedAIErrorMessage,
    ].whereType<String>().toList();
    final hasVisibleResults =
        _personalizedAIStones.isNotEmpty || _advancedAIStones.isNotEmpty;
    if (_aiLoading) return _buildLoadingIndicator();
    if (!hasVisibleResults) {
      return ListView(
        physics: const AlwaysScrollableScrollPhysics(
            parent: BouncingScrollPhysics()),
        children: [
          const SizedBox(height: 120),
          Center(
            child: Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                Icon(Icons.auto_awesome,
                    size: 64, color: Colors.white.withValues(alpha: 0.5)),
                const SizedBox(height: 16),
                Text('暂时还没有更贴合你的内容，多投石头会更懂你',
                    style:
                        TextStyle(color: Colors.white.withValues(alpha: 0.7))),
                if (warningMessages.isNotEmpty) ...[
                  const SizedBox(height: 12),
                  Padding(
                    padding: const EdgeInsets.symmetric(horizontal: 24),
                    child: _buildWarningCard(warningMessages.join('；')),
                  ),
                  const SizedBox(height: 12),
                  OutlinedButton.icon(
                    onPressed: () => _loadAIRecommendations(showFeedback: true),
                    icon: const Icon(Icons.refresh, size: 18),
                    label: const Text('重试加载'),
                    style: OutlinedButton.styleFrom(
                      foregroundColor: Colors.white.withValues(alpha: 0.9),
                      side: BorderSide(
                          color: Colors.white.withValues(alpha: 0.4)),
                      shape: RoundedRectangleBorder(
                          borderRadius: BorderRadius.circular(20)),
                    ),
                  ),
                ],
                const SizedBox(height: 24),
                OutlinedButton.icon(
                  onPressed: () => Navigator.push(
                    context,
                    MaterialPageRoute(
                        builder: (_) => const PersonalizedScreen()),
                  ),
                  icon: const Icon(Icons.explore, size: 18),
                  label: const Text('看看为你准备的内容'),
                  style: OutlinedButton.styleFrom(
                    foregroundColor: Colors.white.withValues(alpha: 0.9),
                    side:
                        BorderSide(color: Colors.white.withValues(alpha: 0.4)),
                    shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(20)),
                  ),
                ),
              ],
            ),
          ),
        ],
      );
    }

    return ListView(
      physics:
          const AlwaysScrollableScrollPhysics(parent: BouncingScrollPhysics()),
      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
      children: [
        _buildResultSectionHeader(
          '湖神为你准备',
          Icons.auto_awesome,
          '${_personalizedAIStones.length + _advancedAIStones.length}条',
        ),
        if (warningMessages.isNotEmpty)
          Padding(
            padding:
                const EdgeInsets.only(left: 8, right: 8, top: 4, bottom: 12),
            child: _buildWarningCard(warningMessages.join('；')),
          ),
        _buildAISection(
          title: '个性化推荐',
          subtitle: '基于你的互动和情绪轨迹',
          stones: _personalizedAIStones,
          errorMessage: _personalizedAIErrorMessage,
        ),
        _buildAISection(
          title: '高级共鸣推荐',
          subtitle: '仅展示高级算法真实产出',
          stones: _advancedAIStones,
          errorMessage: _advancedAIErrorMessage,
        ),
        Padding(
          padding: const EdgeInsets.symmetric(vertical: 16, horizontal: 40),
          child: OutlinedButton.icon(
            onPressed: () => Navigator.push(
              context,
              MaterialPageRoute(builder: (_) => const PersonalizedScreen()),
            ),
            icon: const Icon(Icons.arrow_forward, size: 16),
            label: const Text('查看更多贴心内容'),
            style: OutlinedButton.styleFrom(
              foregroundColor: Colors.white.withValues(alpha: 0.9),
              side: BorderSide(color: Colors.white.withValues(alpha: 0.4)),
              shape: RoundedRectangleBorder(
                  borderRadius: BorderRadius.circular(20)),
              padding: const EdgeInsets.symmetric(vertical: 12),
            ),
          ),
        ),
      ],
    );
  }

  Widget _buildAISection({
    required String title,
    required String subtitle,
    required List<Stone> stones,
    required String? errorMessage,
  }) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        _buildResultSectionHeader(title, Icons.water_drop_outlined, subtitle),
        if (errorMessage != null)
          Padding(
            padding:
                const EdgeInsets.only(left: 8, right: 8, top: 4, bottom: 12),
            child: _buildWarningCard(errorMessage),
          ),
        if (stones.isEmpty)
          Padding(
            padding:
                const EdgeInsets.only(left: 16, right: 16, top: 4, bottom: 20),
            child: Text(
              errorMessage == null ? '当前没有可展示的结果' : '当前分支没有成功产出',
              style: TextStyle(color: Colors.white.withValues(alpha: 0.68)),
            ),
          )
        else
          ...stones.map(
            (stone) => GestureDetector(
              onTap: () {
                _aiService.trackInteraction(
                    stoneId: stone.stoneId, interactionType: 'click');
                Navigator.push(
                  context,
                  MaterialPageRoute(
                      builder: (_) => StoneDetailScreen(stone: stone)),
                );
              },
              child: StoneCard(stone: stone),
            ),
          ),
      ],
    );
  }

  Widget _buildLoadingIndicator() {
    return Center(
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          const CircularProgressIndicator(color: Colors.white),
          const SizedBox(height: 16),
          Text(
            '正在为你整理温暖内容...',
            style: TextStyle(color: Colors.white.withValues(alpha: 0.8)),
          ),
        ],
      ),
    );
  }

  Widget _buildWarningCard(String message) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 12),
      decoration: BoxDecoration(
        color: Colors.amber.withValues(alpha: 0.18),
        borderRadius: BorderRadius.circular(18),
        border: Border.all(color: Colors.amber.withValues(alpha: 0.28)),
      ),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          const Icon(Icons.warning_amber_rounded,
              size: 18, color: Colors.amber),
          const SizedBox(width: 10),
          Expanded(
            child: Text(
              message,
              style: TextStyle(
                color: Colors.white.withValues(alpha: 0.88),
                height: 1.5,
              ),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildStoneList(
    List<Stone> stones, {
    required bool isLoading,
    String? errorMessage,
  }) {
    if (isLoading) return _buildLoadingIndicator();
    if (stones.isEmpty) {
      return ListView(
        physics: const AlwaysScrollableScrollPhysics(
            parent: BouncingScrollPhysics()),
        children: [
          const SizedBox(height: 120),
          Center(
            child: Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                Icon(Icons.explore_off,
                    size: 64, color: Colors.white.withValues(alpha: 0.5)),
                const SizedBox(height: 16),
                Text(
                  errorMessage ?? '暂无内容，试试写下想找的心声或看看湖神陪伴',
                  textAlign: TextAlign.center,
                  style: TextStyle(color: Colors.white.withValues(alpha: 0.7)),
                ),
                if (errorMessage != null) ...[
                  const SizedBox(height: 20),
                  OutlinedButton.icon(
                    onPressed: () => _loadTrending(showFeedback: true),
                    icon: const Icon(Icons.refresh, size: 18),
                    label: const Text('重新加载'),
                    style: OutlinedButton.styleFrom(
                      foregroundColor: Colors.white.withValues(alpha: 0.9),
                      side: BorderSide(
                          color: Colors.white.withValues(alpha: 0.4)),
                      shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(20),
                      ),
                    ),
                  ),
                ],
              ],
            ),
          ),
        ],
      );
    }
    return DeepDiveSpace(
      stones: stones,
      itemBuilder: (stone, layer) => StoneCard(stone: stone),
    );
  }
}

class _AIRecommendationLoadResult {
  final List<Stone>? items;
  final String? errorMessage;

  const _AIRecommendationLoadResult._({
    this.items,
    this.errorMessage,
  });

  factory _AIRecommendationLoadResult.success(List<Stone> items) {
    return _AIRecommendationLoadResult._(items: items);
  }

  factory _AIRecommendationLoadResult.failure(String message) {
    return _AIRecommendationLoadResult._(errorMessage: message);
  }
}
