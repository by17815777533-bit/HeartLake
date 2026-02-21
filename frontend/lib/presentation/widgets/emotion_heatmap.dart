// @file emotion_heatmap.dart
// @brief GitHub 风格情绪贡献热力图组件
// Created by AI Assistant

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import '../../utils/mood_colors.dart';
import '../../utils/app_theme.dart';

/// 情绪热力图 - GitHub 风格的年度情绪贡献图
class EmotionHeatmap extends StatefulWidget {
  /// 日期 -> {score: double, mood: String}
  final Map<String, Map<String, dynamic>> data;

  /// 点击某天的回调
  final Function(String date, Map<String, dynamic> data)? onDayTap;

  const EmotionHeatmap({
    super.key,
    required this.data,
    this.onDayTap,
  });

  @override
  State<EmotionHeatmap> createState() => _EmotionHeatmapState();
}

class _EmotionHeatmapState extends State<EmotionHeatmap>
    with SingleTickerProviderStateMixin {
  late AnimationController _animController;
  String? _hoveredDate;

  // 星期标签（周一到周日）
  static const _weekdayLabels = ['一', '二', '三', '四', '五', '六', '日'];

  // 月份标签
  static const _monthLabels = [
    '1月', '2月', '3月', '4月', '5月', '6月',
    '7月', '8月', '9月', '10月', '11月', '12月',
  ];

  @override
  void initState() {
    super.initState();
    _animController = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 1200),
    )..forward();
  }

  @override
  void dispose() {
    _animController.dispose();
    super.dispose();
  }

  /// 获取过去52周的日期范围
  List<DateTime> _generateYearDates() {
    final today = DateTime.now();
    // 找到本周日（一周的最后一天，以周一为起始）
    final endDate = today;
    // 回溯到52周前的周一
    final daysBack = 52 * 7 + (today.weekday - 1);
    final startDate = today.subtract(Duration(days: daysBack));

    final dates = <DateTime>[];
    var current = startDate;
    while (!current.isAfter(endDate)) {
      dates.add(current);
      current = current.add(const Duration(days: 1));
    }
    return dates;
  }

  /// 根据情绪数据获取单元格颜色
  Color _getCellColor(Map<String, dynamic>? dayData) {
    if (dayData == null) return Colors.grey.shade200;

    final score = (dayData['score'] ?? 0.5) as num;
    final mood = dayData['mood'] as String?;
    final moodType = mood != null
        ? MoodColors.fromString(mood)
        : MoodColors.fromSentimentScore(score.toDouble());

    final config = MoodColors.getConfig(moodType);
    // 用 score 控制饱和度
    final intensity = score.toDouble().clamp(0.2, 1.0);
    return Color.lerp(Colors.grey.shade100, config.primary, intensity)!;
  }

  /// 格式化日期为 YYYY-MM-DD
  String _formatDate(DateTime date) {
    final y = date.year.toString();
    final m = date.month.toString().padLeft(2, '0');
    final d = date.day.toString().padLeft(2, '0');
    return '$y-$m-$d';
  }

  /// 计算月份标签位置
  List<_MonthLabel> _computeMonthLabels(List<DateTime> dates) {
    final labels = <_MonthLabel>[];
    int? lastMonth;
    for (int i = 0; i < dates.length; i++) {
      final date = dates[i];
      if (date.month != lastMonth) {
        // 列号 = i / 7（每列7天）
        final col = i ~/ 7;
        labels.add(_MonthLabel(
          label: _monthLabels[date.month - 1],
          column: col,
        ));
        lastMonth = date.month;
      }
    }
    return labels;
  }

  @override
  Widget build(BuildContext context) {
    final dates = _generateYearDates();
    final monthLabels = _computeMonthLabels(dates);
    final totalColumns = (dates.length / 7).ceil();

    return _buildContent(dates, monthLabels, totalColumns);
  }

  Widget _buildContent(
    List<DateTime> dates,
    List<_MonthLabel> monthLabels,
    int totalColumns,
  ) {
    const double cellSize = 12.0;
    const double cellGap = 3.0;
    const double weekdayWidth = 24.0;

    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(16),
        boxShadow: [
          BoxShadow(
            color: Colors.black.withOpacity(0.05),
            blurRadius: 10,
            offset: const Offset(0, 4),
          ),
        ],
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          // 标题
          const Row(
            children: [
              Icon(Icons.grid_view_rounded,
                  size: 18, color: AppTheme.primaryColor),
              SizedBox(width: 8),
              Text(
                '情绪贡献图',
                style: TextStyle(
                  fontSize: 15,
                  fontWeight: FontWeight.w600,
                  color: AppTheme.textPrimary,
                ),
              ),
              Spacer(),
              Text(
                '过去一年',
                style: TextStyle(
                  fontSize: 12,
                  color: AppTheme.textTertiary,
                ),
              ),
            ],
          ),
          const SizedBox(height: 12),
          // 月份标签行
          _buildMonthLabelsRow(monthLabels, totalColumns, cellSize, cellGap, weekdayWidth),
          const SizedBox(height: 4),
          // 热力图主体
          _buildHeatmapBody(dates, totalColumns, cellSize, cellGap, weekdayWidth),
          const SizedBox(height: 12),
          // Tooltip 显示
          if (_hoveredDate != null) _buildTooltip(),
          // 图例
          _buildColorLegend(),
        ],
      ),
    );
  }

  Widget _buildMonthLabelsRow(
    List<_MonthLabel> monthLabels,
    int totalColumns,
    double cellSize,
    double cellGap,
    double weekdayWidth,
  ) {
    return SizedBox(
      height: 16,
      child: Row(
        children: [
          SizedBox(width: weekdayWidth),
          Expanded(
            child: SingleChildScrollView(
              scrollDirection: Axis.horizontal,
              physics: const NeverScrollableScrollPhysics(),
              child: SizedBox(
                width: totalColumns * (cellSize + cellGap),
                child: Stack(
                  children: monthLabels.map((ml) {
                    return Positioned(
                      left: ml.column * (cellSize + cellGap),
                      child: Text(
                        ml.label,
                        style: const TextStyle(
                          fontSize: 10,
                          color: AppTheme.textTertiary,
                        ),
                      ),
                    );
                  }).toList(),
                ),
              ),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildHeatmapBody(
    List<DateTime> dates,
    int totalColumns,
    double cellSize,
    double cellGap,
    double weekdayWidth,
  ) {
    return SizedBox(
      height: 7 * (cellSize + cellGap),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          // 星期标签列
          SizedBox(
            width: weekdayWidth,
            child: Column(
              children: List.generate(7, (row) {
                // 只显示 周一、周三、周五
                final showLabel = row == 0 || row == 2 || row == 4;
                return SizedBox(
                  height: cellSize + cellGap,
                  child: showLabel
                      ? Align(
                          alignment: Alignment.centerLeft,
                          child: Text(
                            _weekdayLabels[row],
                            style: const TextStyle(
                              fontSize: 9,
                              color: AppTheme.textTertiary,
                            ),
                          ),
                        )
                      : const SizedBox.shrink(),
                );
              }),
            ),
          ),
          // 网格区域
          Expanded(
            child: SingleChildScrollView(
              scrollDirection: Axis.horizontal,
              child: AnimatedBuilder(
                animation: _animController,
                builder: (context, _) {
                  return Row(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: List.generate(totalColumns, (col) {
                      return _buildColumn(dates, col, cellSize, cellGap, totalColumns);
                    }),
                  );
                },
              ),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildColumn(
    List<DateTime> dates,
    int col,
    double cellSize,
    double cellGap,
    int totalColumns,
  ) {
    return Column(
      children: List.generate(7, (row) {
        final index = col * 7 + row;
        if (index >= dates.length) {
          return SizedBox(
            width: cellSize + cellGap,
            height: cellSize + cellGap,
          );
        }

        final date = dates[index];
        final dateKey = _formatDate(date);
        final dayData = widget.data[dateKey];
        final color = _getCellColor(dayData);
        final isHovered = _hoveredDate == dateKey;

        // 渐入动画：按列逐步显示
        final animProgress = _animController.value;
        final colRatio = col / totalColumns;
        final cellOpacity = ((animProgress - colRatio * 0.6) / 0.4).clamp(0.0, 1.0);

        return GestureDetector(
          onTap: () {
            HapticFeedback.lightImpact();
            setState(() => _hoveredDate = dateKey);
            if (dayData != null) {
              widget.onDayTap?.call(dateKey, dayData);
            }
          },
          child: AnimatedContainer(
            duration: const Duration(milliseconds: 200),
            width: cellSize + cellGap,
            height: cellSize + cellGap,
            padding: EdgeInsets.all(cellGap / 2),
            child: Opacity(
              opacity: cellOpacity,
              child: Container(
                decoration: BoxDecoration(
                  color: color,
                  borderRadius: BorderRadius.circular(2.5),
                  border: isHovered
                      ? Border.all(color: AppTheme.primaryColor, width: 1.5)
                      : null,
                  boxShadow: isHovered
                      ? [BoxShadow(color: color.withOpacity(0.5), blurRadius: 4)]
                      : null,
                ),
              ),
            ),
          ),
        );
      }),
    );
  }

  Widget _buildTooltip() {
    final dayData = widget.data[_hoveredDate];
    if (dayData == null) {
      return Padding(
        padding: const EdgeInsets.only(bottom: 8),
        child: Text(
          '$_hoveredDate · 无记录',
          style: const TextStyle(fontSize: 12, color: AppTheme.textTertiary),
        ),
      );
    }

    final score = (dayData['score'] ?? 0.5) as num;
    final mood = dayData['mood'] as String?;
    final moodType = mood != null
        ? MoodColors.fromString(mood)
        : MoodColors.fromSentimentScore(score.toDouble());
    final config = MoodColors.getConfig(moodType);

    return Container(
      margin: const EdgeInsets.only(bottom: 8),
      padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
      decoration: BoxDecoration(
        color: config.cardColor,
        borderRadius: BorderRadius.circular(10),
        border: Border.all(color: config.primary.withOpacity(0.3)),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Icon(config.icon, size: 16, color: config.primary),
          const SizedBox(width: 8),
          Text(
            '$_hoveredDate',
            style: const TextStyle(fontSize: 12, color: AppTheme.textSecondary),
          ),
          const SizedBox(width: 8),
          Text(
            config.name,
            style: TextStyle(
              fontSize: 12,
              fontWeight: FontWeight.w600,
              color: config.primary,
            ),
          ),
          const SizedBox(width: 8),
          Text(
            '${(score * 100).toInt()}%',
            style: const TextStyle(fontSize: 12, color: AppTheme.textSecondary),
          ),
        ],
      ),
    );
  }

  Widget _buildColorLegend() {
    final legendItems = <_LegendItem>[
      _LegendItem('开心', MoodColors.getConfig(MoodType.happy).primary),
      _LegendItem('平静', MoodColors.getConfig(MoodType.calm).primary),
      _LegendItem('悲伤', MoodColors.getConfig(MoodType.sad).primary),
      _LegendItem('愤怒', MoodColors.getConfig(MoodType.angry).primary),
      _LegendItem('无记录', Colors.grey.shade200),
    ];

    return Row(
      mainAxisAlignment: MainAxisAlignment.center,
      children: [
        const Text('少', style: TextStyle(fontSize: 10, color: AppTheme.textTertiary)),
        const SizedBox(width: 4),
        // 渐变色块
        ...List.generate(5, (i) {
          final opacity = (i + 1) / 5;
          return Container(
            width: 12,
            height: 12,
            margin: const EdgeInsets.symmetric(horizontal: 1),
            decoration: BoxDecoration(
              color: AppTheme.secondaryColor.withOpacity(opacity),
              borderRadius: BorderRadius.circular(2),
            ),
          );
        }),
        const SizedBox(width: 4),
        const Text('多', style: TextStyle(fontSize: 10, color: AppTheme.textTertiary)),
        const SizedBox(width: 16),
        // 情绪颜色图例
        ...legendItems.map((item) => Padding(
              padding: const EdgeInsets.only(left: 8),
              child: Row(
                mainAxisSize: MainAxisSize.min,
                children: [
                  Container(
                    width: 10,
                    height: 10,
                    decoration: BoxDecoration(
                      color: item.color,
                      borderRadius: BorderRadius.circular(2),
                    ),
                  ),
                  const SizedBox(width: 3),
                  Text(
                    item.label,
                    style: const TextStyle(fontSize: 9, color: AppTheme.textTertiary),
                  ),
                ],
              ),
            )),
      ],
    );
  }
}

/// 月份标签位置信息
class _MonthLabel {
  final String label;
  final int column;
  const _MonthLabel({required this.label, required this.column});
}

/// 图例项
class _LegendItem {
  final String label;
  final Color color;
  const _LegendItem(this.label, this.color);
}
