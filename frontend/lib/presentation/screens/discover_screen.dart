import 'package:flutter/material.dart';
import '../../data/datasources/recommendation_service.dart';
import '../../data/datasources/ai_recommendation_service.dart';
import '../../domain/entities/stone.dart';
import '../widgets/stone_card.dart';
import '../widgets/sky_scaffold.dart';
import '../widgets/sky_glass_card.dart';
import '../widgets/sky_input.dart';
import '../widgets/deep_dive_layer.dart';
import '../../utils/app_theme.dart';
import '../../utils/mood_colors.dart';

class DiscoverScreen extends StatefulWidget {
  const DiscoverScreen({super.key});

  @override
  State<DiscoverScreen> createState() => _DiscoverScreenState();
}

class _DiscoverScreenState extends State<DiscoverScreen> with SingleTickerProviderStateMixin {
  final RecommendationService _recService = RecommendationService();
  final AIRecommendationService _aiService = AIRecommendationService();
  final TextEditingController _searchController = TextEditingController();
  late TabController _tabController;

  List<Stone> _stones = [];
  List<Map<String, dynamic>> _resonanceStones = [];
  List<Map<String, dynamic>> _personalizedStones = [];
  bool _isLoading = false;
  bool _resonanceLoading = false;
  bool _personalizedLoading = false;
  String? _selectedMood;

  final List<String> _moods = ['开心', '平静', '忧伤', '焦虑', '愤怒', '迷茫', '感恩', '期待'];

  @override
  void initState() {
    super.initState();
    _tabController = TabController(length: 5, vsync: this);
    _loadTrending();
  }

  @override
  void dispose() {
    _tabController.dispose();
    _searchController.dispose();
    super.dispose();
  }

  Future<void> _loadTrending() async {
    setState(() => _isLoading = true);
    try {
      final items = await _recService.getTrending();
      setState(() => _stones = items.map((e) => Stone.fromJson(e)).toList());
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

  Future<void> _searchStones(String query) async {
    if (query.isEmpty) return;
    setState(() => _isLoading = true);
    try {
      final items = await _recService.search(query);
      setState(() => _stones = items.map((e) => Stone.fromJson(e)).toList());
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

  Future<void> _discoverByMood(String mood) async {
    setState(() {
      _isLoading = true;
      _selectedMood = mood;
    });
    try {
      final items = await _recService.discoverByMood(mood);
      setState(() => _stones = items.map((e) => Stone.fromJson(e)).toList());
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

  /// 加载共鸣推荐（高级推荐 - DTW情绪轨迹匹配）
  Future<void> _loadResonance() async {
    if (_resonanceStones.isNotEmpty) return;
    setState(() => _resonanceLoading = true);
    try {
      final stones = await _aiService.getAdvancedRecommendations(limit: 20);
      if (mounted) setState(() => _resonanceStones = stones);
    } catch (_) {}
    if (mounted) setState(() => _resonanceLoading = false);
  }

  /// 加载个性化推荐
  Future<void> _loadPersonalized() async {
    if (_personalizedStones.isNotEmpty) return;
    setState(() => _personalizedLoading = true);
    try {
      final stones = await _aiService.getPersonalizedRecommendations(limit: 20);
      if (mounted) setState(() => _personalizedStones = stones);
    } catch (_) {}
    if (mounted) setState(() => _personalizedLoading = false);
  }

  @override
  Widget build(BuildContext context) {
    return SkyScaffold(
      showWater: true,
      showParticles: true,
      appBar: AppBar(
        title: const Text('发现',
            style: TextStyle(
                fontWeight: FontWeight.w600, color: Colors.white)),
        backgroundColor: Colors.transparent,
        elevation: 0,
        scrolledUnderElevation: 0,
        foregroundColor: Colors.white,
        bottom: TabBar(
          controller: _tabController,
          indicatorColor: AppTheme.primaryColor,
          indicatorWeight: 3,
          labelColor: Colors.white,
          unselectedLabelColor: Colors.white.withValues(alpha: 0.5),
          tabs: const [
            Tab(icon: Icon(Icons.local_fire_department), text: '热门'),
            Tab(icon: Icon(Icons.search), text: '搜索'),
            Tab(icon: Icon(Icons.mood), text: '情绪'),
            Tab(icon: Icon(Icons.favorite), text: '共鸣'),
            Tab(icon: Icon(Icons.auto_awesome), text: '为你'),
          ],
          onTap: (index) {
            if (index == 0) _loadTrending();
            if (index == 3) _loadResonance();
            if (index == 4) _loadPersonalized();
          },
        ),
      ),
      body: SafeArea(
        child: TabBarView(
          controller: _tabController,
          children: [
            _buildStoneList(),
            _buildSearchTab(),
            _buildMoodTab(),
            _buildResonanceTab(),
            _buildPersonalizedTab(),
          ],
        ),
      ),
    );
  }

  Widget _buildSearchTab() {
    return Column(
      children: [
        Padding(
          padding: const EdgeInsets.all(16),
          child: SkyInput(
            controller: _searchController,
            hintText: '搜索石头...',
            prefixIcon: Icon(Icons.search,
                color: Colors.white.withValues(alpha: 0.5)),
            suffixIcon: IconButton(
              icon: Icon(Icons.send,
                  color: Colors.white.withValues(alpha: 0.7)),
              onPressed: () => _searchStones(_searchController.text),
            ),
            onSubmitted: _searchStones,
          ),
        ),
        Expanded(child: _buildStoneList()),
      ],
    );
  }

  Widget _buildMoodTab() {
    return Column(
      children: [
        Padding(
          padding: const EdgeInsets.all(16),
          child: SkyGlassCard(
            borderRadius: 20,
            enableGlow: false,
            child: Wrap(
              spacing: 8,
              runSpacing: 8,
              children: _moods.map((mood) {
                final isSelected = _selectedMood == mood;
                final color = MoodColors.getConfig(MoodColors.fromString(mood)).primary;
                return GestureDetector(
                  onTap: () => _discoverByMood(mood),
                  child: AnimatedContainer(
                    duration: const Duration(milliseconds: 200),
                    padding: const EdgeInsets.symmetric(
                        horizontal: 14, vertical: 8),
                    decoration: BoxDecoration(
                      color: isSelected
                          ? color.withValues(alpha: 0.8)
                          : color.withValues(alpha: 0.2),
                      borderRadius: BorderRadius.circular(20),
                      border: Border.all(
                        color: isSelected
                            ? color
                            : Colors.white.withValues(alpha: 0.2),
                        width: isSelected ? 2 : 1,
                      ),
                      boxShadow: isSelected
                          ? [
                              BoxShadow(
                                color: color.withValues(alpha: 0.4),
                                blurRadius: 8,
                                spreadRadius: 1,
                              )
                            ]
                          : null,
                    ),
                    child: Text(
                      mood,
                      style: TextStyle(
                        color: isSelected
                            ? Colors.white
                            : Colors.white.withValues(alpha: 0.7),
                        fontWeight:
                            isSelected ? FontWeight.w600 : FontWeight.w400,
                        fontSize: 14,
                      ),
                    ),
                  ),
                );
              }).toList(),
            ),
          ),
        ),
        Expanded(child: _buildStoneList()),
      ],
    );
  }

  Widget _buildStoneList() {
    if (_isLoading) {
      return Center(
        child: Column(mainAxisSize: MainAxisSize.min, children: [
          const CircularProgressIndicator(color: AppTheme.warmOrange),
          const SizedBox(height: 16),
          Text('正在探索心湖深处...',
              style: TextStyle(
                  color: Colors.white.withValues(alpha: 0.8))),
        ]),
      );
    }
    if (_stones.isEmpty) {
      return Center(
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(Icons.explore_off,
                size: 64,
                color: Colors.white.withValues(alpha: 0.5)),
            const SizedBox(height: 16),
            Text('暂无内容，试试搜索或按心情发现',
                style: TextStyle(
                    color: Colors.white.withValues(alpha: 0.7))),
          ],
        ),
      );
    }
    return DeepDiveSpace(
      stones: _stones,
      itemBuilder: (stone, layer) => StoneCard(stone: stone),
    );
  }

  /// 共鸣推荐 Tab - 光遇柔光风格
  Widget _buildResonanceTab() {
    if (_resonanceLoading) {
      return Center(
        child: Column(mainAxisSize: MainAxisSize.min, children: [
          const CircularProgressIndicator(
              color: AppTheme.primaryColor, strokeWidth: 2),
          const SizedBox(height: 16),
          Text('正在寻找共鸣之声...',
              style: TextStyle(
                  color: Colors.white.withValues(alpha: 0.7),
                  letterSpacing: 1)),
        ]),
      );
    }
    if (_resonanceStones.isEmpty) {
      return Center(
        child: Column(mainAxisSize: MainAxisSize.min, children: [
          Icon(Icons.favorite_border,
              size: 64,
              color: Colors.white.withValues(alpha: 0.4)),
          const SizedBox(height: 16),
          Text('多投几颗石头，共鸣就会来',
              style: TextStyle(
                  color: Colors.white.withValues(alpha: 0.6))),
        ]),
      );
    }
    return ListView.builder(
      padding: const EdgeInsets.all(16),
      itemCount: _resonanceStones.length,
      itemBuilder: (context, index) {
        final stone = _resonanceStones[index];
        final content = stone['content'] as String? ?? '';
        final score = (stone['resonance_score'] ?? stone['score'] ?? 0).toDouble();
        final mood = stone['mood_type'] as String? ?? 'neutral';
        final moodConfig = MoodColors.getConfig(MoodColors.fromString(mood));

        return Padding(
          padding: const EdgeInsets.only(bottom: 12),
          child: SkyGlassCard(
            borderRadius: 16,
            enableGlow: true,
            glowColor: moodConfig.primary,
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                // 共鸣分数标签
                Row(
                  children: [
                    Container(
                      padding: const EdgeInsets.symmetric(
                          horizontal: 8, vertical: 3),
                      decoration: BoxDecoration(
                        gradient: LinearGradient(colors: [
                          moodConfig.primary.withValues(alpha: 0.3),
                          AppTheme.primaryColor.withValues(alpha: 0.2),
                        ]),
                        borderRadius: BorderRadius.circular(10),
                      ),
                      child: Row(
                          mainAxisSize: MainAxisSize.min,
                          children: [
                            Icon(Icons.favorite,
                                size: 10,
                                color:
                                    Colors.white.withValues(alpha: 0.8)),
                            const SizedBox(width: 4),
                            Text(
                                '${(score * 100).toInt()}% 共鸣',
                                style: TextStyle(
                                    fontSize: 10,
                                    color: Colors.white
                                        .withValues(alpha: 0.8))),
                          ]),
                    ),
                    const Spacer(),
                    Container(
                      width: 8,
                      height: 8,
                      decoration: BoxDecoration(
                        color: moodConfig.primary,
                        shape: BoxShape.circle,
                        boxShadow: [
                          BoxShadow(
                              color: moodConfig.primary
                                  .withValues(alpha: 0.5),
                              blurRadius: 6)
                        ],
                      ),
                    ),
                  ],
                ),
                const SizedBox(height: 10),
                // 内容
                Text(content,
                    maxLines: 4,
                    overflow: TextOverflow.ellipsis,
                    style: TextStyle(
                        fontSize: 14,
                        height: 1.6,
                        color: Colors.white.withValues(alpha: 0.8))),
              ],
            ),
          ),
        );
      },
    );
  }

  /// 个性化推荐 Tab - 光遇飘浮光球风格
  Widget _buildPersonalizedTab() {
    if (_personalizedLoading) {
      return Center(
        child: Column(mainAxisSize: MainAxisSize.min, children: [
          const CircularProgressIndicator(
              color: AppTheme.primaryColor, strokeWidth: 2),
          const SizedBox(height: 16),
          Text('星光正在为你汇聚...',
              style: TextStyle(
                  color: Colors.white.withValues(alpha: 0.7),
                  letterSpacing: 1)),
        ]),
      );
    }
    if (_personalizedStones.isEmpty) {
      return Center(
        child: Column(mainAxisSize: MainAxisSize.min, children: [
          Icon(Icons.auto_awesome,
              size: 64,
              color: Colors.white.withValues(alpha: 0.4)),
          const SizedBox(height: 16),
          Text('继续探索，推荐会越来越懂你',
              style: TextStyle(
                  color: Colors.white.withValues(alpha: 0.6))),
        ]),
      );
    }
    return GridView.builder(
      padding: const EdgeInsets.all(16),
      gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
        crossAxisCount: 2,
        mainAxisSpacing: 12,
        crossAxisSpacing: 12,
        childAspectRatio: 0.85,
      ),
      itemCount: _personalizedStones.length,
      itemBuilder: (context, index) {
        final stone = _personalizedStones[index];
        final content = stone['content'] as String? ?? '';
        final mood = stone['mood_type'] as String? ?? 'neutral';
        final moodConfig = MoodColors.getConfig(MoodColors.fromString(mood));

        return SkyGlassCard(
          borderRadius: 18,
          enableGlow: true,
          glowColor: moodConfig.primary,
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              // 光球情绪指示
              Container(
                width: 24,
                height: 24,
                decoration: BoxDecoration(
                  shape: BoxShape.circle,
                  gradient: RadialGradient(colors: [
                    moodConfig.primary.withValues(alpha: 0.8),
                    moodConfig.primary.withValues(alpha: 0.2),
                  ]),
                  boxShadow: [
                    BoxShadow(
                        color:
                            moodConfig.primary.withValues(alpha: 0.4),
                        blurRadius: 8)
                  ],
                ),
                child: Icon(moodConfig.icon,
                    size: 12, color: Colors.white),
              ),
              const SizedBox(height: 10),
              // 内容
              Expanded(
                child: Text(content,
                    overflow: TextOverflow.fade,
                    style: TextStyle(
                        fontSize: 13,
                        height: 1.5,
                        color:
                            Colors.white.withValues(alpha: 0.75))),
              ),
              // AI标签
              Align(
                alignment: Alignment.bottomRight,
                child: Text('✨ AI推荐',
                    style: TextStyle(
                        fontSize: 9,
                        color:
                            Colors.white.withValues(alpha: 0.4))),
              ),
            ],
          ),
        );
      },
    );
  }
}
