import 'package:flutter/material.dart';
import '../../data/datasources/recommendation_service.dart';
import '../../domain/entities/stone.dart';
import '../widgets/stone_card.dart';
import '../widgets/water_background.dart';
import '../widgets/deep_dive_layer.dart';
import '../../utils/app_theme.dart';
import '../../utils/mood_colors.dart';
import 'personalized_screen.dart';

class DiscoverScreen extends StatefulWidget {
  const DiscoverScreen({super.key});

  @override
  State<DiscoverScreen> createState() => _DiscoverScreenState();
}

class _DiscoverScreenState extends State<DiscoverScreen> with SingleTickerProviderStateMixin {
  final RecommendationService _service = RecommendationService();
  final TextEditingController _searchController = TextEditingController();
  late TabController _tabController;

  List<Stone> _stones = [];
  bool _isLoading = false;
  String? _selectedMood;

  final List<String> _moods = ['开心', '平静', '悲伤', '焦虑', '愤怒', '迷茫', '惊喜', '中性'];

  @override
  void initState() {
    super.initState();
    _tabController = TabController(length: 3, vsync: this);
    _tabController.addListener(_onTabChanged);
    _loadTrending();
  }

  void _onTabChanged() {
    if (_tabController.index == 0 && !_tabController.indexIsChanging) {
      _loadTrending();
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
      setState(() => _stones = items.map((e) => Stone.fromJson(e)).toList());
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('加载失败，请稍后再试')),
        );
      }
    } finally {
      setState(() => _isLoading = false);
    }
  }

  Future<void> _searchStones(String query) async {
    if (query.isEmpty) return;
    setState(() => _isLoading = true);
    try {
      final items = await _service.search(query);
      setState(() => _stones = items.map((e) => Stone.fromJson(e)).toList());
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('搜索失败，请稍后再试')),
        );
      }
    } finally {
      setState(() => _isLoading = false);
    }
  }

  Future<void> _discoverByMood(String mood) async {
    setState(() {
      _isLoading = true;
      _selectedMood = mood;
    });
    try {
      final items = await _service.discoverByMood(mood);
      setState(() => _stones = items.map((e) => Stone.fromJson(e)).toList());
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('加载失败，请稍后再试')),
        );
      }
    } finally {
      setState(() => _isLoading = false);
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      extendBodyBehindAppBar: true,
      backgroundColor: Colors.transparent,
      appBar: AppBar(
        title: const Text('发现', style: TextStyle(fontWeight: FontWeight.w600, color: Color(0xFF5D4037))),
        backgroundColor: Colors.transparent,
        elevation: 0,
        scrolledUnderElevation: 0,
        actions: [
          IconButton(
            icon: const Icon(Icons.auto_awesome, color: Color(0xFF5D4037)),
            tooltip: '个性化推荐',
            onPressed: () => Navigator.push(context, MaterialPageRoute(builder: (_) => const PersonalizedScreen())),
          ),
        ],
        bottom: TabBar(
          controller: _tabController,
          indicatorColor: AppTheme.accentColor,
          indicatorWeight: 3,
          labelColor: const Color(0xFF5D4037),
          unselectedLabelColor: const Color(0xFF8D6E63),
          tabs: const [
            Tab(icon: Icon(Icons.local_fire_department), text: '热门'),
            Tab(icon: Icon(Icons.search), text: '搜索'),
            Tab(icon: Icon(Icons.mood), text: '情绪'),
          ],
          // Tab切换由 _onTabChanged listener 统一处理
        ),
      ),
      body: Stack(
        children: [
          const WaterBackground(),
          SafeArea(
            child: TabBarView(
              controller: _tabController,
              children: [
                _buildStoneList(),
                _buildSearchTab(),
                _buildMoodTab(),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildSearchTab() {
    return Column(
      children: [
        Padding(
          padding: const EdgeInsets.all(16),
          child: SearchBar(
            controller: _searchController,
            hintText: '搜索石头...',
            elevation: WidgetStateProperty.all(0),
            backgroundColor: WidgetStateProperty.all(Colors.white.withValues(alpha: 0.9)),
            trailing: [
              IconButton(
                icon: const Icon(Icons.search),
                onPressed: () => _searchStones(_searchController.text),
              ),
            ],
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
          child: Wrap(
            spacing: 8,
            runSpacing: 8,
            children: _moods.map((mood) {
              final isSelected = _selectedMood == mood;
              final color = MoodColors.getConfig(MoodColors.fromString(mood)).primary;
              return ChoiceChip(
                label: Text(mood),
                selected: isSelected,
                selectedColor: color.withValues(alpha: 0.8),
                backgroundColor: color.withValues(alpha: 0.3),
                onSelected: (_) => _discoverByMood(mood),
              );
            }).toList(),
          ),
        ),
        Expanded(child: _buildStoneList()),
      ],
    );
  }

  Widget _buildStoneList() {
    if (_isLoading) {
      return Center(child: Column(mainAxisSize: MainAxisSize.min, children: [const CircularProgressIndicator(color: Colors.white), const SizedBox(height: 16), Text('正在探索心湖深处...', style: TextStyle(color: Colors.white.withValues(alpha: 0.8)))]));
    }
    if (_stones.isEmpty) {
      return Center(
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(Icons.explore_off, size: 64, color: Colors.white.withValues(alpha: 0.5)),
            const SizedBox(height: 16),
            Text('暂无内容，试试搜索或按心情发现', style: TextStyle(color: Colors.white.withValues(alpha: 0.7))),
          ],
        ),
      );
    }
    return DeepDiveSpace(
      stones: _stones,
      itemBuilder: (stone, layer) => StoneCard(stone: stone),
    );
  }
}
