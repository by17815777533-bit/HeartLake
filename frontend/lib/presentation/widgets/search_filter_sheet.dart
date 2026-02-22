// @file search_filter_sheet.dart
// @brief 搜索过滤器底部弹窗组件
// Created by 吴睿璐

library;
import 'package:flutter/material.dart';
import '../../utils/app_theme.dart';
import '../../utils/mood_colors.dart';

class SearchFilterSheet extends StatefulWidget {
  final List<String>? initialMoods;

  final List<String>? initialTags;

  final double? initialMinScore;

  final double? initialMaxScore;

  final DateTime? initialStartDate;

  final DateTime? initialEndDate;

  final Function(
    List<String>? moods,
    List<String>? tags,
    double? minScore,
    double? maxScore,
    DateTime? startDate,
    DateTime? endDate,
  ) onApply;

  const SearchFilterSheet({
    super.key,
    this.initialMoods,
    this.initialTags,
    this.initialMinScore,
    this.initialMaxScore,
    this.initialStartDate,
    this.initialEndDate,
    required this.onApply,
  });

  @override
  State<SearchFilterSheet> createState() => _SearchFilterSheetState();
}

class _SearchFilterSheetState extends State<SearchFilterSheet> {
  // 情绪选择
  final List<String> _availableMoods = [
    'happy',
    'calm',
    'sad',
    'anxious',
    'angry',
    'surprised',
    'confused',
  ];

  late Set<String> _selectedMoods;

  // 标签选择
  final List<String> _availableTags = [
    '工作',
    '学习',
    '生活',
    '感情',
    '家庭',
    '朋友',
    '健康',
    '梦想',
    '困惑',
    '成长',
    '孤独',
    '快乐',
    '压力',
    '希望',
    '回忆',
  ];

  late Set<String> _selectedTags;

  // 情绪分数范围
  late RangeValues _emotionScoreRange;

  // 日期范围
  DateTime? _startDate;
  DateTime? _endDate;

  @override
  void initState() {
    super.initState();

    _selectedMoods = Set<String>.from(widget.initialMoods ?? []);
    _selectedTags = Set<String>.from(widget.initialTags ?? []);

    _emotionScoreRange = RangeValues(
      widget.initialMinScore ?? -1.0,
      widget.initialMaxScore ?? 1.0,
    );

    _startDate = widget.initialStartDate;
    _endDate = widget.initialEndDate;
  }

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: EdgeInsets.only(
        bottom: MediaQuery.of(context).viewInsets.bottom,
      ),
      decoration: const BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.vertical(top: Radius.circular(24)),
      ),
      child: SafeArea(
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            _buildHeader(),
            Flexible(
              child: SingleChildScrollView(
                padding: const EdgeInsets.all(20),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    _buildMoodFilter(),
                    const SizedBox(height: 24),
                    _buildTagFilter(),
                    const SizedBox(height: 24),
                    _buildEmotionScoreFilter(),
                    const SizedBox(height: 24),
                    _buildDateRangeFilter(),
                    const SizedBox(height: 32),
                  ],
                ),
              ),
            ),
            _buildActionButtons(),
          ],
        ),
      ),
    );
  }

  Widget _buildHeader() {
    return Container(
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        border: Border(
          bottom: BorderSide(color: Colors.grey.shade200),
        ),
      ),
      child: Row(
        children: [
          const Icon(
            Icons.filter_list_rounded,
            color: AppTheme.primaryColor,
            size: 24,
          ),
          const SizedBox(width: 12),
          Text(
            '筛选条件',
            style: Theme.of(context).textTheme.titleLarge?.copyWith(
                  fontWeight: FontWeight.bold,
                ),
          ),
          const Spacer(),
          TextButton(
            onPressed: _resetFilters,
            child: const Text(
              '重置',
              style: TextStyle(
                color: AppTheme.textSecondary,
                fontSize: 14,
              ),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildMoodFilter() {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Row(
          children: [
            const Icon(
              Icons.sentiment_satisfied_alt,
              color: AppTheme.primaryColor,
              size: 20,
            ),
            const SizedBox(width: 8),
            Text(
              '情绪类型',
              style: Theme.of(context).textTheme.titleMedium?.copyWith(
                    fontWeight: FontWeight.bold,
                  ),
            ),
          ],
        ),
        const SizedBox(height: 12),
        Wrap(
          spacing: 8,
          runSpacing: 8,
          children: _availableMoods.map((mood) {
            final moodConfig = MoodColors.getConfig(
              MoodColors.fromString(mood),
            );
            final isSelected = _selectedMoods.contains(mood);

            return InkWell(
              onTap: () {
                setState(() {
                  if (isSelected) {
                    _selectedMoods.remove(mood);
                  } else {
                    _selectedMoods.add(mood);
                  }
                });
              },
              borderRadius: BorderRadius.circular(20),
              child: Container(
                padding: const EdgeInsets.symmetric(
                  horizontal: 16,
                  vertical: 8,
                ),
                decoration: BoxDecoration(
                  color: isSelected
                      ? moodConfig.primary
                      : moodConfig.primary.withValues(alpha: 0.1),
                  borderRadius: BorderRadius.circular(20),
                  border: Border.all(
                    color: moodConfig.primary,
                    width: 1.5,
                  ),
                ),
                child: Row(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    Icon(
                      moodConfig.icon,
                      size: 16,
                      color: isSelected ? Colors.white : moodConfig.primary,
                    ),
                    const SizedBox(width: 6),
                    Text(
                      moodConfig.name,
                      style: TextStyle(
                        fontSize: 13,
                        fontWeight: FontWeight.w600,
                        color: isSelected ? Colors.white : moodConfig.primary,
                      ),
                    ),
                  ],
                ),
              ),
            );
          }).toList(),
        ),
      ],
    );
  }

  Widget _buildTagFilter() {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Row(
          children: [
            const Icon(
              Icons.local_offer_outlined,
              color: AppTheme.secondaryColor,
              size: 20,
            ),
            const SizedBox(width: 8),
            Text(
              '标签',
              style: Theme.of(context).textTheme.titleMedium?.copyWith(
                    fontWeight: FontWeight.bold,
                  ),
            ),
          ],
        ),
        const SizedBox(height: 12),
        Wrap(
          spacing: 8,
          runSpacing: 8,
          children: _availableTags.map((tag) {
            final isSelected = _selectedTags.contains(tag);

            return InkWell(
              onTap: () {
                setState(() {
                  if (isSelected) {
                    _selectedTags.remove(tag);
                  } else {
                    _selectedTags.add(tag);
                  }
                });
              },
              borderRadius: BorderRadius.circular(16),
              child: Container(
                padding: const EdgeInsets.symmetric(
                  horizontal: 12,
                  vertical: 6,
                ),
                decoration: BoxDecoration(
                  color: isSelected
                      ? AppTheme.secondaryColor
                      : AppTheme.secondaryColor.withValues(alpha: 0.1),
                  borderRadius: BorderRadius.circular(16),
                  border: Border.all(
                    color: AppTheme.secondaryColor,
                    width: 1,
                  ),
                ),
                child: Text(
                  '# $tag',
                  style: TextStyle(
                    fontSize: 12,
                    fontWeight: FontWeight.w600,
                    color: isSelected ? Colors.white : AppTheme.secondaryColor,
                  ),
                ),
              ),
            );
          }).toList(),
        ),
      ],
    );
  }

  Widget _buildEmotionScoreFilter() {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Row(
          children: [
            const Icon(
              Icons.tune,
              color: AppTheme.primaryColor,
              size: 20,
            ),
            const SizedBox(width: 8),
            Text(
              '情绪分数范围',
              style: Theme.of(context).textTheme.titleMedium?.copyWith(
                    fontWeight: FontWeight.bold,
                  ),
            ),
          ],
        ),
        const SizedBox(height: 12),
        Container(
          padding: const EdgeInsets.all(16),
          decoration: BoxDecoration(
            color: AppTheme.primaryColor.withValues(alpha: 0.05),
            borderRadius: BorderRadius.circular(12),
            border: Border.all(
              color: AppTheme.primaryColor.withValues(alpha: 0.2),
            ),
          ),
          child: Column(
            children: [
              const Row(
                mainAxisAlignment: MainAxisAlignment.spaceBetween,
                children: [
                  Text(
                    '很差',
                    style: TextStyle(
                      fontSize: 12,
                      color: AppTheme.textSecondary,
                    ),
                  ),
                  Text(
                    '一般',
                    style: TextStyle(
                      fontSize: 12,
                      color: AppTheme.textSecondary,
                    ),
                  ),
                  Text(
                    '很好',
                    style: TextStyle(
                      fontSize: 12,
                      color: AppTheme.textSecondary,
                    ),
                  ),
                ],
              ),
              const SizedBox(height: 8),
              RangeSlider(
                values: _emotionScoreRange,
                min: -1.0,
                max: 1.0,
                divisions: 20,
                activeColor: AppTheme.primaryColor,
                inactiveColor: AppTheme.primaryColor.withValues(alpha: 0.3),
                labels: RangeLabels(
                  _getScoreLabel(_emotionScoreRange.start),
                  _getScoreLabel(_emotionScoreRange.end),
                ),
                onChanged: (RangeValues values) {
                  setState(() {
                    _emotionScoreRange = values;
                  });
                },
              ),
              const SizedBox(height: 8),
              Row(
                mainAxisAlignment: MainAxisAlignment.spaceBetween,
                children: [
                  Text(
                    '最低: ${_getScoreLabel(_emotionScoreRange.start)}',
                    style: const TextStyle(
                      fontSize: 13,
                      fontWeight: FontWeight.w600,
                      color: AppTheme.primaryColor,
                    ),
                  ),
                  Text(
                    '最高: ${_getScoreLabel(_emotionScoreRange.end)}',
                    style: const TextStyle(
                      fontSize: 13,
                      fontWeight: FontWeight.w600,
                      color: AppTheme.primaryColor,
                    ),
                  ),
                ],
              ),
            ],
          ),
        ),
      ],
    );
  }

  Widget _buildDateRangeFilter() {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Row(
          children: [
            const Icon(
              Icons.date_range,
              color: AppTheme.secondaryColor,
              size: 20,
            ),
            const SizedBox(width: 8),
            Text(
              '时间范围',
              style: Theme.of(context).textTheme.titleMedium?.copyWith(
                    fontWeight: FontWeight.bold,
                  ),
            ),
          ],
        ),
        const SizedBox(height: 12),
        Row(
          children: [
            Expanded(
              child: _buildDateButton(
                label: '开始日期',
                date: _startDate,
                onTap: () => _selectStartDate(),
              ),
            ),
            const SizedBox(width: 12),
            const Icon(
              Icons.arrow_forward,
              color: AppTheme.textSecondary,
              size: 16,
            ),
            const SizedBox(width: 12),
            Expanded(
              child: _buildDateButton(
                label: '结束日期',
                date: _endDate,
                onTap: () => _selectEndDate(),
              ),
            ),
          ],
        ),
      ],
    );
  }

  Widget _buildDateButton({
    required String label,
    DateTime? date,
    required VoidCallback onTap,
  }) {
    return InkWell(
      onTap: onTap,
      borderRadius: BorderRadius.circular(12),
      child: Container(
        padding: const EdgeInsets.all(12),
        decoration: BoxDecoration(
          border: Border.all(color: Colors.grey.shade300),
          borderRadius: BorderRadius.circular(12),
        ),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              label,
              style: const TextStyle(
                fontSize: 11,
                color: AppTheme.textSecondary,
              ),
            ),
            const SizedBox(height: 4),
            Text(
              date != null
                  ? '${date.month}月${date.day}日'
                  : '选择日期',
              style: TextStyle(
                fontSize: 14,
                fontWeight: FontWeight.w600,
                color: date != null ? AppTheme.textPrimary : AppTheme.textSecondary,
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildActionButtons() {
    return Container(
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        border: Border(
          top: BorderSide(color: Colors.grey.shade200),
        ),
      ),
      child: Row(
        children: [
          Expanded(
            child: OutlinedButton(
              onPressed: () => Navigator.pop(context),
              style: OutlinedButton.styleFrom(
                padding: const EdgeInsets.symmetric(vertical: 16),
                side: const BorderSide(color: AppTheme.textSecondary),
              ),
              child: const Text(
                '取消',
                style: TextStyle(
                  color: AppTheme.textSecondary,
                  fontWeight: FontWeight.w600,
                ),
              ),
            ),
          ),
          const SizedBox(width: 16),
          Expanded(
            flex: 2,
            child: ElevatedButton(
              onPressed: _applyFilters,
              style: ElevatedButton.styleFrom(
                backgroundColor: AppTheme.primaryColor,
                padding: const EdgeInsets.symmetric(vertical: 16),
              ),
              child: const Text(
                '应用筛选',
                style: TextStyle(
                  color: Colors.white,
                  fontWeight: FontWeight.w600,
                ),
              ),
            ),
          ),
        ],
      ),
    );
  }

  String _getScoreLabel(double score) {
    if (score >= 0.6) return '很好';
    if (score >= 0.2) return '较好';
    if (score >= -0.2) return '一般';
    if (score >= -0.6) return '较差';
    return '很差';
  }

  Future<void> _selectStartDate() async {
    final DateTime? picked = await showDatePicker(
      context: context,
      initialDate: _startDate ?? DateTime.now().subtract(const Duration(days: 30)),
      firstDate: DateTime.now().subtract(const Duration(days: 365)),
      lastDate: _endDate ?? DateTime.now(),
      builder: (context, child) {
        return Theme(
          data: Theme.of(context).copyWith(
            colorScheme: Theme.of(context).colorScheme.copyWith(
                  primary: AppTheme.primaryColor,
                ),
          ),
          child: child!,
        );
      },
    );

    if (picked != null) {
      setState(() {
        _startDate = picked;
      });
    }
  }

  Future<void> _selectEndDate() async {
    final DateTime? picked = await showDatePicker(
      context: context,
      initialDate: _endDate ?? DateTime.now(),
      firstDate: _startDate ?? DateTime.now().subtract(const Duration(days: 365)),
      lastDate: DateTime.now(),
      builder: (context, child) {
        return Theme(
          data: Theme.of(context).copyWith(
            colorScheme: Theme.of(context).colorScheme.copyWith(
                  primary: AppTheme.primaryColor,
                ),
          ),
          child: child!,
        );
      },
    );

    if (picked != null) {
      setState(() {
        _endDate = picked;
      });
    }
  }

  void _resetFilters() {
    setState(() {
      _selectedMoods.clear();
      _selectedTags.clear();
      _emotionScoreRange = const RangeValues(-1.0, 1.0);
      _startDate = null;
      _endDate = null;
    });
  }

  void _applyFilters() {
    widget.onApply(
      _selectedMoods.isEmpty ? null : _selectedMoods.toList(),
      _selectedTags.isEmpty ? null : _selectedTags.toList(),
      _emotionScoreRange.start == -1.0 ? null : _emotionScoreRange.start,
      _emotionScoreRange.end == 1.0 ? null : _emotionScoreRange.end,
      _startDate,
      _endDate,
    );
    Navigator.pop(context);
  }
}