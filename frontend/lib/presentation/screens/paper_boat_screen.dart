// @file paper_boat_screen.dart
// @brief 漂流纸船主界面 - 放船/捞船/回应
// Created by 交互设计师

import 'package:flutter/material.dart';
import '../../data/datasources/paper_boat_service.dart';
import '../../domain/entities/paper_boat.dart';
import '../../domain/entities/mood.dart';
import '../../utils/app_theme.dart';
import '../widgets/boat_style_selector.dart';
import '../widgets/catch_boat_animation.dart';
import '../widgets/mood_selector.dart';

class PaperBoatScreen extends StatefulWidget {
  const PaperBoatScreen({super.key});

  @override
  State<PaperBoatScreen> createState() => _PaperBoatScreenState();
}

class _PaperBoatScreenState extends State<PaperBoatScreen>
    with TickerProviderStateMixin {
  late TabController _tabController;
  late AnimationController _sendAnimController;
  late Animation<double> _sendScaleAnim;
  final _boatService = PaperBoatService();

  // 放船状态
  final _contentController = TextEditingController();
  DriftMode _driftMode = DriftMode.random;
  BoatStyle _boatStyle = BoatStyle.paper;
  MoodType _mood = MoodType.calm;
  bool _isSending = false;

  // 捞船状态
  PaperBoat? _caughtBoat;
  bool _isCatching = false;
  bool _showCatchAnimation = false;

  @override
  void initState() {
    super.initState();
    _tabController = TabController(length: 2, vsync: this);
    _sendAnimController = AnimationController(
      duration: const Duration(milliseconds: 150),
      vsync: this,
    );
    _sendScaleAnim = Tween<double>(begin: 1.0, end: 0.95).animate(
      CurvedAnimation(parent: _sendAnimController, curve: Curves.easeInOut),
    );
  }

  @override
  void dispose() {
    _tabController.dispose();
    _contentController.dispose();
    _sendAnimController.dispose();
    super.dispose();
  }

  Future<void> _sendBoat() async {
    if (_contentController.text.trim().isEmpty) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('请写下你想说的话')),
      );
      return;
    }

    setState(() => _isSending = true);
    try {
      final result = await _boatService.sendBoat(
        content: _contentController.text.trim(),
        mood: _mood.name,
        driftMode: _driftMode.name,
        boatStyle: _boatStyle.name,
      );

      if (mounted) {
        setState(() => _isSending = false);
        if (result['success'] == true) {
          _contentController.clear();
          FocusScope.of(context).unfocus();
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(
              content: Text('纸船已放入湖中，静待有缘人'),
              backgroundColor: AppTheme.secondaryColor,
            ),
          );
        } else {
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(content: Text(result['message'] ?? '发送失败')),
          );
        }
      }
    } catch (e) {
      if (mounted) {
        setState(() => _isSending = false);
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('网络异常，请稍后再试'), backgroundColor: AppTheme.errorColor),
        );
      }
    }
  }

  Future<void> _catchBoat() async {
    if (_isCatching) return;
    setState(() {
      _isCatching = true;
      _showCatchAnimation = true;
    });

    try {
    final result = await _boatService.catchBoat();

    if (mounted) {
      await Future.delayed(const Duration(milliseconds: 1500));
      setState(() {
        _isCatching = false;
        if (result['success'] == true && result['data'] != null) {
          _caughtBoat = PaperBoat.fromJson(result['data']);
        } else {
          _showCatchAnimation = false;
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(content: Text(result['message'] ?? '湖面暂时没有纸船')),
          );
        }
      });
    }
    } catch (e) {
      if (mounted) {
        setState(() {
          _isCatching = false;
          _showCatchAnimation = false;
        });
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('网络异常，请稍后再试'), backgroundColor: AppTheme.errorColor),
        );
      }
    }
  }

  Future<void> _respondToBoat(String content) async {
    if (_caughtBoat == null || content.trim().isEmpty) return;

    try {
    final result = await _boatService.respondToBoat(_caughtBoat!.boatId, content);
    if (mounted) {
      if (result['success'] == true) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(
            content: Text('回应已送出'),
            backgroundColor: AppTheme.secondaryColor,
          ),
        );
        setState(() {
          _caughtBoat = null;
          _showCatchAnimation = false;
        });
      } else {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text(result['message'] ?? '回应失败')),
        );
      }
    }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('网络异常，请稍后再试'), backgroundColor: AppTheme.errorColor),
        );
      }
    }
  }

  Future<void> _releaseBoat() async {
    if (_caughtBoat == null) return;

    final result = await _boatService.releaseBoat(_caughtBoat!.boatId);
    if (mounted && result['success'] == true) {
      setState(() {
        _caughtBoat = null;
        _showCatchAnimation = false;
      });
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('纸船已放回湖中')),
      );
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('漂流纸船'),
        centerTitle: true,
        bottom: TabBar(
          controller: _tabController,
          tabs: const [
            Tab(icon: Icon(Icons.send), text: '放纸船'),
            Tab(icon: Icon(Icons.catching_pokemon), text: '捞纸船'),
          ],
        ),
      ),
      body: Container(
        decoration: BoxDecoration(
          gradient: LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [
              AppTheme.lakeSurface.withOpacity(0.2),
              AppTheme.lakeMiddle.withOpacity(0.1),
            ],
          ),
        ),
        child: TabBarView(
          controller: _tabController,
          children: [
            _buildSendBoatTab(),
            _buildCatchBoatTab(),
          ],
        ),
      ),
    );
  }

  Widget _buildSendBoatTab() {
    return SingleChildScrollView(
      padding: const EdgeInsets.all(16),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          // 漂流模式选择
          Card(
            child: Padding(
              padding: const EdgeInsets.all(16),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  const Text('漂流模式', style: TextStyle(fontWeight: FontWeight.bold)),
                  const SizedBox(height: 12),
                  Wrap(
                    spacing: 8,
                    children: DriftMode.values.map((mode) {
                      final isSelected = _driftMode == mode;
                      return ChoiceChip(
                        label: Text(_getDriftModeLabel(mode)),
                        selected: isSelected,
                        onSelected: (_) => setState(() => _driftMode = mode),
                      );
                    }).toList(),
                  ),
                ],
              ),
            ),
          ),
          const SizedBox(height: 16),

          // 纸船样式选择
          BoatStyleSelector(
            selected: _boatStyle,
            onChanged: (style) => setState(() => _boatStyle = style),
          ),
          const SizedBox(height: 16),

          // 心情选择
          Card(
            child: Padding(
              padding: const EdgeInsets.all(16),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  const Text('此刻心情', style: TextStyle(fontWeight: FontWeight.bold)),
                  const SizedBox(height: 12),
                  MoodSelector(
                    selectedMood: _mood,
                    onMoodSelected: (mood) => setState(() => _mood = mood),
                  ),
                ],
              ),
            ),
          ),
          const SizedBox(height: 16),

          // 内容输入
          Card(
            child: Padding(
              padding: const EdgeInsets.all(16),
              child: TextField(
                controller: _contentController,
                maxLines: 5,
                maxLength: 500,
                decoration: const InputDecoration(
                  hintText: '写下你想对陌生人说的话...',
                  border: InputBorder.none,
                ),
              ),
            ),
          ),
          const SizedBox(height: 24),

          // 发送按钮
          ScaleTransition(
            scale: _sendScaleAnim,
            child: ElevatedButton.icon(
              onPressed: _isSending ? null : () async {
                await _sendAnimController.forward();
                await _sendAnimController.reverse();
                _sendBoat();
              },
              icon: _isSending
                  ? const SizedBox(
                      width: 20,
                      height: 20,
                      child: CircularProgressIndicator(strokeWidth: 2),
                    )
                  : const Icon(Icons.sailing),
              label: Text(_isSending ? '放入湖中...' : '放出纸船'),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildCatchBoatTab() {
    if (_showCatchAnimation && _caughtBoat != null) {
      return _buildRespondView();
    }

    return CatchBoatAnimation(
      isCatching: _isCatching,
      onCatch: _catchBoat,
      caughtBoat: _caughtBoat,
    );
  }

  Widget _buildRespondView() {
    final boat = _caughtBoat!;
    final responseController = TextEditingController();

    return SingleChildScrollView(
      padding: const EdgeInsets.all(16),
      child: Column(
        children: [
          // 捞到的纸船内容
          Card(
            child: Padding(
              padding: const EdgeInsets.all(20),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Row(
                    children: [
                      Icon(_getBoatIcon(boat.boatStyle), color: AppTheme.primaryColor),
                      const SizedBox(width: 8),
                      Text(
                        '来自 ${boat.senderNickname ?? "匿名旅人"}',
                        style: TextStyle(color: Colors.grey[600], fontSize: 14),
                      ),
                    ],
                  ),
                  const SizedBox(height: 16),
                  Text(
                    boat.content,
                    style: const TextStyle(fontSize: 16, height: 1.6),
                  ),
                ],
              ),
            ),
          ),
          const SizedBox(height: 16),

          // 回应输入
          Card(
            child: Padding(
              padding: const EdgeInsets.all(16),
              child: TextField(
                controller: responseController,
                maxLines: 4,
                maxLength: 300,
                decoration: const InputDecoration(
                  hintText: '写下你的回应...',
                  border: InputBorder.none,
                ),
              ),
            ),
          ),
          const SizedBox(height: 16),

          // 操作按钮
          Row(
            children: [
              Expanded(
                child: OutlinedButton.icon(
                  onPressed: _releaseBoat,
                  icon: const Icon(Icons.water_drop),
                  label: const Text('放回湖中'),
                ),
              ),
              const SizedBox(width: 12),
              Expanded(
                child: ElevatedButton.icon(
                  onPressed: () => _respondToBoat(responseController.text),
                  icon: const Icon(Icons.send),
                  label: const Text('送出回应'),
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }

  String _getDriftModeLabel(DriftMode mode) {
    switch (mode) {
      case DriftMode.random:
        return '随机漂流';
      case DriftMode.directed:
        return '定向漂流';
      case DriftMode.wish:
        return '祈愿漂流';
    }
  }

  IconData _getBoatIcon(BoatStyle style) {
    switch (style) {
      case BoatStyle.paper:
        return Icons.sailing;
      case BoatStyle.origami:
        return Icons.auto_awesome;
      case BoatStyle.lotus:
        return Icons.local_florist;
    }
  }
}
