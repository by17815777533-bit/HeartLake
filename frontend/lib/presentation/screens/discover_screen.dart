import 'package:flutter/material.dart';
import '../../data/datasources/recommendation_service.dart';
import '../../data/datasources/ai_recommendation_service.dart';
import '../../domain/entities/stone.dart';
import '../widgets/stone_card/stone_card.dart';
import '../widgets/water_background.dart';
import '../widgets/deep_dive_layer.dart';
import '../../utils/app_theme.dart';
import 'personalized_screen.dart';
import 'stone_detail_screen.dart';

class DiscoverScreen extends StatefulWidget {
  const DiscoverScreen({super.key});

  @override
  State<DiscoverScreen> createState() => _DiscoverScreenState();
}

class _DiscoverScreenState extends State<DiscoverScreen> with SingleTickerProviderStateMixin {
  final RecommendationService _service = RecommendationService();
  final AIRecommendationService _aiService = AIRecommendationService();
  final TextEditingController _searchController = TextEditingController();
  late TabController _tabController;

  List<Stone> _stones = [];
  List<Stone> _keywordResults = [];
  List<Stone> _semanticResults = [];
  List<Stone> _aiRecommendations = [];
  bool _isLoading = false;
  bool _hasSearched = false;

  @override
  void initState() {
    super.initState();
    _tabController = TabController(length: 3, vsync: this);
    _tabController.addListener(_onTabChanged);
    _loadTrending();
  }

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

  Future<void> _loadTrending() async {
    setState(() => _isLoading = true);
    try {
      final items = await _service.getTrending();
      if (mounted) {
        setState(() => _stones = items.map((e) => Stone.fromJson(e)).toList());
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('加载失败，请稍后再试')),
        );
      }
    } finally {
      if (mounted) setState(() => _isLoading = false);
    }
  }

  /// 执行关键词搜索（向量语义搜索为admin端点，前端不可用）
  Future<void> _searchStones(String query) async {
    if (query.isEmpty) return;
    setState(() {
      _isLoading = true;
      _hasSearched = false;
    });
    try {
      final results = await _service.search(query);

      if (mounted) {
        setState(() {
          _keywordResults = results
              .map((e) => Stone.fromJson(e))
              .toList();
          // 向量搜索为admin端点(/api/admin/edge-ai/vector-search)，前端返回空
          _semanticResults = [];
          _hasSearched = true;
        });
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('搜索失败，请稍后再试')),
        );
      }
    } finally {
      if (mounted) setState(() => _isLoading = false);
    }
  }

  /// 加载湖神个性化推荐
  Future<void> _loadAIRecommendations() async {
    setState(() => _isLoading = true);
    try {
      final results = await Future.wait([
        _aiService.getPersonalizedRecommendations(limit: 10),
        _aiService.getAdvancedRecommendations(limit: 10),
      ]);
      if (mounted) {
        final personalized = results[0].map((e) => Stone.fromJson(e)).toList();
        final advanced = results[1].map((e) => Stone.fromJson(e)).toList();
        // 合并去重
        final seen = <String>{};
        final merged = <Stone>[];
        for (final s in [...personalized, ...advanced]) {
          if (seen.add(s.stoneId)) merged.add(s);
        }
        setState(() => _aiRecommendations = merged);
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('加载湖神推荐失败，请稍后再试')),
        );
      }
    } finally {
      if (mounted) setState(() => _isLoading = false);
    }
  }

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;
    return Scaffold(
      extendBodyBehindAppBar: true,
      backgroundColor: Colors.transparent,
      appBar: AppBar(
        title: Text('发现', style: TextStyle(fontWeight: FontWeight.w600, color: isDark ? Colors.white : const Color(0xFF5D4037))),
        backgroundColor: Colors.transparent,
        elevation: 0,
        scrolledUnderElevation: 0,
        actions: [
          IconButton(
            icon: Icon(Icons.auto_awesome, color: isDark ? Colors.white : const Color(0xFF5D4037)),
            tooltip: '个性化推荐',
            onPressed: () => Navigator.push(context, MaterialPageRoute(builder: (_) => const PersonalizedScreen())),
          ),
        ],
        bottom: TabBar(
          controller: _tabController,
          indicatorColor: AppTheme.accentColor,
          indicatorWeight: 3,
          labelColor: isDark ? Colors.white : const Color(0xFF5D4037),
          unselectedLabelColor: isDark ? Colors.white70 : const Color(0xFF8D6E63),
          tabs: const [
            Tab(icon: Icon(Icons.local_fire_department), text: '热门'),
            Tab(icon: Icon(Icons.search), text: '搜索'),
            Tab(icon: Icon(Icons.auto_awesome), text: '湖神推荐'),
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
                  onRefresh: _loadTrending,
                  color: AppTheme.accentColor,
                  child: _buildStoneList(_stones),
                ),
                _buildSearchTab(),
                RefreshIndicator(
                  onRefresh: _loadAIRecommendations,
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
            hintText: '搜索石头（支持湖神语义搜索）...',
            elevation: WidgetStateProperty.all(0),
            backgroundColor: WidgetStateProperty.all((isDark ? const Color(0xFF16213E) : Colors.white).withValues(alpha: 0.9)),
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
          child: _isLoading
              ? _buildLoadingIndicator()
              : _hasSearched
                  ? _buildSearchResults()
                  : _buildStoneList(_stones),
        ),
      ],
    );
  }

  /// 构建搜索结果，区分关键词匹配和语义相似
  Widget _buildSearchResults() {
    if (_keywordResults.isEmpty && _semanticResults.isEmpty) {
      return Center(
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(Icons.search_off, size: 64, color: Colors.white.withValues(alpha: 0.5)),
            const SizedBox(height: 16),
            Text('未找到相关内容', style: TextStyle(color: Colors.white.withValues(alpha: 0.7))),
          ],
        ),
      );
    }

    return ListView(
      physics: const AlwaysScrollableScrollPhysics(parent: BouncingScrollPhysics()),
      padding: const EdgeInsets.symmetric(horizontal: 8),
      children: [
        if (_keywordResults.isNotEmpty) ...[
          _buildResultSectionHeader(
            '关键词匹配',
            Icons.text_fields,
            '${_keywordResults.length}条结果',
          ),
          ..._keywordResults.map((s) => StoneCard(stone: s)),
        ],
        if (_semanticResults.isNotEmpty) ...[
          const SizedBox(height: 8),
          _buildResultSectionHeader(
            '语义相似',
            Icons.psychology,
            '${_semanticResults.length}条结果',
          ),
          ..._semanticResults.map((s) => StoneCard(stone: s)),
        ],
        const SizedBox(height: 24),
      ],
    );
  }

  Widget _buildResultSectionHeader(String title, IconData icon, String subtitle) {
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
    if (_isLoading) return _buildLoadingIndicator();
    if (_aiRecommendations.isEmpty) {
      return ListView(
        physics: const AlwaysScrollableScrollPhysics(parent: BouncingScrollPhysics()),
        children: [
          const SizedBox(height: 120),
          Center(
            child: Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                Icon(Icons.auto_awesome, size: 64, color: Colors.white.withValues(alpha: 0.5)),
                const SizedBox(height: 16),
                Text('暂无湖神推荐，多投石头解锁更多', style: TextStyle(color: Colors.white.withValues(alpha: 0.7))),
                const SizedBox(height: 24),
                OutlinedButton.icon(
                  onPressed: () => Navigator.push(
                    context,
                    MaterialPageRoute(builder: (_) => const PersonalizedScreen()),
                  ),
                  icon: const Icon(Icons.explore, size: 18),
                  label: const Text('探索个性化推荐'),
                  style: OutlinedButton.styleFrom(
                    foregroundColor: Colors.white.withValues(alpha: 0.9),
                    side: BorderSide(color: Colors.white.withValues(alpha: 0.4)),
                    shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(20)),
                  ),
                ),
              ],
            ),
          ),
        ],
      );
    }

    return ListView.builder(
      physics: const AlwaysScrollableScrollPhysics(parent: BouncingScrollPhysics()),
      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
      itemCount: _aiRecommendations.length + 2,
      itemBuilder: (context, index) {
        if (index == 0) {
          return _buildResultSectionHeader(
            '湖神为你推荐',
            Icons.auto_awesome,
            '${_aiRecommendations.length}条',
          );
        }
        if (index == _aiRecommendations.length + 1) {
          // 底部"查看更多"入口
          return Padding(
            padding: const EdgeInsets.symmetric(vertical: 16, horizontal: 40),
            child: OutlinedButton.icon(
              onPressed: () => Navigator.push(
                context,
                MaterialPageRoute(builder: (_) => const PersonalizedScreen()),
              ),
              icon: const Icon(Icons.arrow_forward, size: 16),
              label: const Text('查看更多个性化推荐'),
              style: OutlinedButton.styleFrom(
                foregroundColor: Colors.white.withValues(alpha: 0.9),
                side: BorderSide(color: Colors.white.withValues(alpha: 0.4)),
                shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(20)),
                padding: const EdgeInsets.symmetric(vertical: 12),
              ),
            ),
          );
        }
        final stone = _aiRecommendations[index - 1];
        return GestureDetector(
          onTap: () {
            _aiService.trackInteraction(stoneId: stone.stoneId, interactionType: 'click');
            Navigator.push(
              context,
              MaterialPageRoute(builder: (_) => StoneDetailScreen(stone: stone)),
            );
          },
          child: StoneCard(stone: stone),
        );
      },
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
            '正在探索心湖深处...',
            style: TextStyle(color: Colors.white.withValues(alpha: 0.8)),
          ),
        ],
      ),
    );
  }

  Widget _buildStoneList(List<Stone> stones) {
    if (_isLoading) return _buildLoadingIndicator();
    if (stones.isEmpty) {
      return ListView(
        physics: const AlwaysScrollableScrollPhysics(parent: BouncingScrollPhysics()),
        children: [
          const SizedBox(height: 120),
          Center(
            child: Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                Icon(Icons.explore_off, size: 64, color: Colors.white.withValues(alpha: 0.5)),
                const SizedBox(height: 16),
                Text('暂无内容，试试搜索或湖神推荐', style: TextStyle(color: Colors.white.withValues(alpha: 0.7))),
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
