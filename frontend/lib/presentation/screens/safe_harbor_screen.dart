// 安全港湾 - 心理支持与自助资源页面

import 'package:flutter/material.dart';
import 'package:url_launcher/url_launcher.dart';
import '../../data/datasources/psych_support_service.dart';
import '../../di/service_locator.dart';
import '../../utils/app_theme.dart';
import '../widgets/water_background.dart';

class SafeHarborScreen extends StatefulWidget {
  const SafeHarborScreen({super.key});

  @override
  State<SafeHarborScreen> createState() => _SafeHarborScreenState();
}

class _SafeHarborScreenState extends State<SafeHarborScreen>
    with SingleTickerProviderStateMixin {
  final _service = sl<PsychSupportService>();
  late AnimationController _animController;
  late Animation<double> _fadeAnim;

  bool _loading = true;
  String? _error;

  String _promptText = '';
  List<dynamic> _hotlines = [];
  List<dynamic> _tools = [];
  List<dynamic> _resources = [];

  // 自助工具展开状态
  final Set<int> _expandedTools = {};

  @override
  void initState() {
    super.initState();
    _animController = AnimationController(
      duration: const Duration(milliseconds: 600),
      vsync: this,
    );
    _fadeAnim = Tween<double>(begin: 0.0, end: 1.0).animate(
      CurvedAnimation(parent: _animController, curve: Curves.easeOut),
    );
    _loadAll();
  }

  @override
  void dispose() {
    _animController.dispose();
    super.dispose();
  }

  Future<void> _loadAll() async {
    setState(() {
      _loading = true;
      _error = null;
    });

    try {
      final results = await Future.wait([
        _service.getPrompt(),
        _service.getHotlines(),
        _service.getTools(),
        _service.getResources(),
      ]);

      // 记录访问
      _service.recordAccess(resourceType: 'page_view');

      if (mounted) {
        setState(() {
          final promptData = results[0];
          _promptText = promptData['data'] is Map
              ? (promptData['data']['text'] ?? '你不是一个人，这里永远是你的安全港湾')
              : '你不是一个人，这里永远是你的安全港湾';

          final hotlineData = results[1];
          _hotlines = hotlineData['data'] is List
              ? hotlineData['data']
              : (hotlineData['data'] is Map &&
                      hotlineData['data']['items'] is List)
                  ? hotlineData['data']['items']
                  : [];

          final toolData = results[2];
          _tools = toolData['data'] is List
              ? toolData['data']
              : (toolData['data'] is Map && toolData['data']['items'] is List)
                  ? toolData['data']['items']
                  : [];

          final resourceData = results[3];
          _resources = resourceData['data'] is List
              ? resourceData['data']
              : (resourceData['data'] is Map &&
                      resourceData['data']['items'] is List)
                  ? resourceData['data']['items']
                  : [];

          _loading = false;
        });
        _animController.forward();
      }
    } catch (e) {
      if (mounted) {
        setState(() {
          _loading = false;
          _error = '加载失败，请下拉刷新重试';
        });
      }
    }
  }

  Future<void> _callHotline(String phone, String name) async {
    final confirmed = await showDialog<bool>(
      context: context,
      builder: (ctx) => AlertDialog(
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
        title: const Row(
          children: [
            Icon(Icons.phone, color: AppTheme.secondaryColor, size: 22),
            SizedBox(width: 8),
            Text('拨打热线'),
          ],
        ),
        content: Text('即将拨打 $name\n号码: $phone'),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx, false),
            child: const Text('取消'),
          ),
          FilledButton(
            onPressed: () => Navigator.pop(ctx, true),
            child: const Text('拨打'),
          ),
        ],
      ),
    );

    if (confirmed == true) {
      final uri = Uri(scheme: 'tel', path: phone);
      if (await canLaunchUrl(uri)) {
        await launchUrl(uri);
      } else if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('无法拨打 $phone')),
        );
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    final isDark = Theme.of(context).brightness == Brightness.dark;

    return Scaffold(
      extendBodyBehindAppBar: true,
      appBar: AppBar(
        title: const Text('安全港湾',
            style: TextStyle(color: Colors.white, fontWeight: FontWeight.bold)),
        centerTitle: true,
        backgroundColor: Colors.transparent,
        elevation: 0,
        scrolledUnderElevation: 0,
        foregroundColor: Colors.white,
      ),
      body: Stack(
        children: [
          const Positioned.fill(child: WaterBackground()),
          _loading
              ? const Center(
                  child: CircularProgressIndicator(color: Colors.white))
              : _error != null
                  ? _buildErrorView()
                  : FadeTransition(
                      opacity: _fadeAnim,
                      child: RefreshIndicator(
                        onRefresh: _loadAll,
                        color: Colors.blue[900],
                        backgroundColor: Colors.white,
                        child: ListView(
                          padding: EdgeInsets.only(
                            top: MediaQuery.of(context).padding.top +
                                kToolbarHeight +
                                16,
                            bottom: 32,
                            left: 16,
                            right: 16,
                          ),
                          children: [
                            _buildPromptCard(isDark),
                            const SizedBox(height: 16),
                            _buildHotlinesSection(isDark),
                            const SizedBox(height: 16),
                            _buildToolsSection(isDark),
                            if (_resources.isNotEmpty) ...[
                              const SizedBox(height: 16),
                              _buildResourcesSection(isDark),
                            ],
                            const SizedBox(height: 24),
                            _buildPrivacyNote(isDark),
                          ],
                        ),
                      ),
                    ),
        ],
      ),
    );
  }

  Widget _buildErrorView() {
    return Center(
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Icon(Icons.cloud_off,
              size: 48, color: Colors.white.withValues(alpha: 0.7)),
          const SizedBox(height: 12),
          Text(_error!,
              style: const TextStyle(color: Colors.white70, fontSize: 15)),
          const SizedBox(height: 16),
          FilledButton.icon(
            onPressed: _loadAll,
            icon: const Icon(Icons.refresh, size: 18),
            label: const Text('重试'),
          ),
        ],
      ),
    );
  }

  // 温暖提示卡片
  Widget _buildPromptCard(bool isDark) {
    return Card(
      color: isDark
          ? Colors.white.withValues(alpha: 0.1)
          : Colors.white.withValues(alpha: 0.92),
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
      child: Padding(
        padding: const EdgeInsets.all(20),
        child: Row(
          children: [
            Container(
              padding: const EdgeInsets.all(10),
              decoration: BoxDecoration(
                color: AppTheme.warmPink.withValues(alpha: 0.15),
                shape: BoxShape.circle,
              ),
              child: const Icon(Icons.favorite,
                  color: AppTheme.warmPink, size: 28),
            ),
            const SizedBox(width: 16),
            Expanded(
              child: Text(
                _promptText,
                style: TextStyle(
                  fontSize: 16,
                  height: 1.5,
                  color:
                      isDark ? AppTheme.darkTextPrimary : AppTheme.textPrimary,
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }

  // 心理热线区
  Widget _buildHotlinesSection(bool isDark) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        _buildSectionTitle('心理热线', Icons.phone_in_talk, isDark),
        const SizedBox(height: 8),
        if (_hotlines.isEmpty)
          _buildEmptyHint('暂无热线信息', isDark)
        else
          ..._hotlines.map((h) {
            final name = h['name'] ?? '热线';
            final phone = h['phone'] ?? '';
            final desc = h['description'] ?? '';
            return Card(
              color: isDark
                  ? Colors.white.withValues(alpha: 0.1)
                  : Colors.white.withValues(alpha: 0.92),
              shape: RoundedRectangleBorder(
                  borderRadius: BorderRadius.circular(12)),
              margin: const EdgeInsets.only(bottom: 8),
              child: ListTile(
                leading: Container(
                  padding: const EdgeInsets.all(8),
                  decoration: BoxDecoration(
                    color: AppTheme.secondaryColor.withValues(alpha: 0.12),
                    borderRadius: BorderRadius.circular(10),
                  ),
                  child: const Icon(Icons.call,
                      color: AppTheme.secondaryColor, size: 22),
                ),
                title: Text(name,
                    style: TextStyle(
                        fontWeight: FontWeight.w600,
                        color: isDark
                            ? AppTheme.darkTextPrimary
                            : AppTheme.textPrimary)),
                subtitle: Text(
                  desc.isNotEmpty ? '$phone · $desc' : phone,
                  style: TextStyle(
                      fontSize: 13,
                      color: isDark
                          ? AppTheme.darkTextSecondary
                          : AppTheme.textSecondary),
                ),
                trailing: Icon(Icons.phone_forwarded,
                    color: AppTheme.secondaryColor.withValues(alpha: 0.7)),
                onTap:
                    phone.isNotEmpty ? () => _callHotline(phone, name) : null,
              ),
            );
          }),
      ],
    );
  }

  // 自助工具区
  Widget _buildToolsSection(bool isDark) {
    final toolIcons = <String, IconData>{
      '深呼吸': Icons.air,
      '五感着陆': Icons.self_improvement,
      '情绪日记': Icons.edit_note,
      '正念冥想': Icons.spa,
      '身体扫描': Icons.accessibility_new,
      '安全空间': Icons.shield,
    };

    final toolColors = <String, Color>{
      '深呼吸': AppTheme.skyBlue,
      '五感着陆': AppTheme.purpleColor,
      '情绪日记': AppTheme.warmOrange,
      '正念冥想': AppTheme.secondaryColor,
      '身体扫描': AppTheme.peachPink,
      '安全空间': AppTheme.primaryColor,
    };

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        _buildSectionTitle('自助工具', Icons.psychology, isDark),
        const SizedBox(height: 8),
        if (_tools.isEmpty)
          _buildEmptyHint('暂无自助工具', isDark)
        else
          ...List.generate(_tools.length, (i) {
            final tool = _tools[i];
            final name = tool['name'] ?? '工具';
            final desc = tool['description'] ?? '';
            final guide = tool['guide'] ?? tool['content'] ?? '';
            final isExpanded = _expandedTools.contains(i);
            final icon = toolIcons[name] ?? Icons.build_circle_outlined;
            final color = toolColors[name] ?? AppTheme.primaryColor;

            return Card(
              color: isDark
                  ? Colors.white.withValues(alpha: 0.1)
                  : Colors.white.withValues(alpha: 0.92),
              shape: RoundedRectangleBorder(
                  borderRadius: BorderRadius.circular(12)),
              margin: const EdgeInsets.only(bottom: 8),
              clipBehavior: Clip.antiAlias,
              child: InkWell(
                onTap: guide.toString().isNotEmpty
                    ? () {
                        setState(() {
                          if (isExpanded) {
                            _expandedTools.remove(i);
                          } else {
                            _expandedTools.add(i);
                          }
                        });
                        _service.recordAccess(resourceType: 'tool_$name');
                      }
                    : null,
                child: Column(
                  children: [
                    ListTile(
                      leading: Container(
                        padding: const EdgeInsets.all(8),
                        decoration: BoxDecoration(
                          color: color.withValues(alpha: 0.12),
                          borderRadius: BorderRadius.circular(10),
                        ),
                        child: Icon(icon, color: color, size: 22),
                      ),
                      title: Text(name,
                          style: TextStyle(
                              fontWeight: FontWeight.w600,
                              color: isDark
                                  ? AppTheme.darkTextPrimary
                                  : AppTheme.textPrimary)),
                      subtitle: desc.isNotEmpty
                          ? Text(desc,
                              style: TextStyle(
                                  fontSize: 13,
                                  color: isDark
                                      ? AppTheme.darkTextSecondary
                                      : AppTheme.textSecondary))
                          : null,
                      trailing: guide.toString().isNotEmpty
                          ? Icon(
                              isExpanded
                                  ? Icons.expand_less
                                  : Icons.expand_more,
                              color: isDark
                                  ? AppTheme.darkTextSecondary
                                  : AppTheme.textSecondary)
                          : null,
                    ),
                    AnimatedCrossFade(
                      firstChild: const SizedBox.shrink(),
                      secondChild: Container(
                        width: double.infinity,
                        padding: const EdgeInsets.fromLTRB(20, 0, 20, 16),
                        child: Container(
                          padding: const EdgeInsets.all(14),
                          decoration: BoxDecoration(
                            color: color.withValues(alpha: 0.06),
                            borderRadius: BorderRadius.circular(10),
                          ),
                          child: Text(
                            guide.toString(),
                            style: TextStyle(
                              fontSize: 14,
                              height: 1.6,
                              color: isDark
                                  ? AppTheme.darkTextPrimary
                                  : AppTheme.textPrimary,
                            ),
                          ),
                        ),
                      ),
                      crossFadeState: isExpanded
                          ? CrossFadeState.showSecond
                          : CrossFadeState.showFirst,
                      duration: const Duration(milliseconds: 250),
                    ),
                  ],
                ),
              ),
            );
          }),
      ],
    );
  }

  // 暖心资源区
  Widget _buildResourcesSection(bool isDark) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        _buildSectionTitle('暖心资源', Icons.menu_book, isDark),
        const SizedBox(height: 8),
        ..._resources.map((r) {
          final title = r['title'] ?? r['name'] ?? '资源';
          final desc = r['description'] ?? '';
          final url = r['url'] ?? '';
          return Card(
            color: isDark
                ? Colors.white.withValues(alpha: 0.1)
                : Colors.white.withValues(alpha: 0.92),
            shape:
                RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
            margin: const EdgeInsets.only(bottom: 8),
            child: ListTile(
              leading: Container(
                padding: const EdgeInsets.all(8),
                decoration: BoxDecoration(
                  color: AppTheme.primaryColor.withValues(alpha: 0.12),
                  borderRadius: BorderRadius.circular(10),
                ),
                child: const Icon(Icons.article_outlined,
                    color: AppTheme.primaryColor, size: 22),
              ),
              title: Text(title,
                  style: TextStyle(
                      fontWeight: FontWeight.w600,
                      color: isDark
                          ? AppTheme.darkTextPrimary
                          : AppTheme.textPrimary)),
              subtitle: desc.isNotEmpty
                  ? Text(desc,
                      maxLines: 2,
                      overflow: TextOverflow.ellipsis,
                      style: TextStyle(
                          fontSize: 13,
                          color: isDark
                              ? AppTheme.darkTextSecondary
                              : AppTheme.textSecondary))
                  : null,
              trailing: url.toString().isNotEmpty
                  ? Icon(Icons.open_in_new,
                      size: 18,
                      color: isDark
                          ? AppTheme.darkTextSecondary
                          : AppTheme.textSecondary)
                  : null,
              onTap: url.toString().isNotEmpty
                  ? () async {
                      final uri = Uri.tryParse(url);
                      if (uri != null && await canLaunchUrl(uri)) {
                        await launchUrl(uri,
                            mode: LaunchMode.externalApplication);
                      }
                    }
                  : null,
            ),
          );
        }),
      ],
    );
  }

  // 隐私声明
  Widget _buildPrivacyNote(bool isDark) {
    return Container(
      padding: const EdgeInsets.symmetric(vertical: 12, horizontal: 16),
      decoration: BoxDecoration(
        color: isDark
            ? Colors.white.withValues(alpha: 0.05)
            : Colors.white.withValues(alpha: 0.6),
        borderRadius: BorderRadius.circular(12),
      ),
      child: Row(
        children: [
          Icon(Icons.lock_outline,
              size: 16,
              color:
                  isDark ? AppTheme.darkTextSecondary : AppTheme.textTertiary),
          const SizedBox(width: 8),
          Expanded(
            child: Text(
              '你的访问记录完全匿名，我们不会追踪你的身份',
              style: TextStyle(
                fontSize: 12,
                color:
                    isDark ? AppTheme.darkTextSecondary : AppTheme.textTertiary,
              ),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildSectionTitle(String title, IconData icon, bool isDark) {
    return Padding(
      padding: const EdgeInsets.only(left: 4),
      child: Row(
        children: [
          Icon(icon, size: 20, color: Colors.white.withValues(alpha: 0.9)),
          const SizedBox(width: 6),
          Text(title,
              style: TextStyle(
                fontSize: 17,
                fontWeight: FontWeight.bold,
                color: Colors.white.withValues(alpha: 0.95),
                shadows: [
                  Shadow(
                      color: Colors.black.withValues(alpha: 0.3), blurRadius: 4)
                ],
              )),
        ],
      ),
    );
  }

  Widget _buildEmptyHint(String text, bool isDark) {
    return Card(
      color: isDark
          ? Colors.white.withValues(alpha: 0.1)
          : Colors.white.withValues(alpha: 0.92),
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
      child: Padding(
        padding: const EdgeInsets.all(24),
        child: Center(
          child: Text(text,
              style: TextStyle(
                  color: isDark
                      ? AppTheme.darkTextSecondary
                      : AppTheme.textTertiary)),
        ),
      ),
    );
  }
}
