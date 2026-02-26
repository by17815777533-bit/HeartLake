// @file report_dialog.dart
// @brief 举报对话框组件
// Created by 林子怡

import 'package:flutter/material.dart';
import '../../data/datasources/report_service.dart';
import '../../di/service_locator.dart';

/// 举报对话框组件
class ReportDialog extends StatefulWidget {
  final String targetType; // 'stone', 'boat', 'user'
  final String targetId;

  const ReportDialog({
    super.key,
    required this.targetType,
    required this.targetId,
  });

  @override
  State<ReportDialog> createState() => _ReportDialogState();
}

class _ReportDialogState extends State<ReportDialog> {
  final ReportService _reportService = sl<ReportService>();
  final TextEditingController _descriptionController = TextEditingController();
  String _selectedReason = 'spam';
  bool _isSubmitting = false;

  final Map<String, String> _reportReasons = {
    'spam': '垃圾信息',
    'harassment': '骚扰辱骂',
    'inappropriate': '不当内容',
    'violence': '暴力内容',
    'other': '其他',
  };

  @override
  void dispose() {
    _descriptionController.dispose();
    super.dispose();
  }

  Future<void> _submitReport() async {
    if (_isSubmitting) return;

    setState(() => _isSubmitting = true);

    try {
      final result = await _reportService.createReport(
        targetType: widget.targetType,
        targetId: widget.targetId,
        reason: _selectedReason,
        description: _descriptionController.text.trim(),
      );

      if (mounted) {
        if (result['success'] == true) {
          Navigator.of(context).pop(true);
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(
              content: Text('举报已提交，我们会尽快处理'),
              backgroundColor: Colors.green,
            ),
          );
        } else {
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(
              content: Text(result['message'] ?? '举报失败'),
              backgroundColor: Colors.red,
            ),
          );
        }
      }
    } finally {
      if (mounted) {
        setState(() => _isSubmitting = false);
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return AlertDialog(
      title: const Text('举报内容'),
      content: SingleChildScrollView(
        child: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text(
              '请选择举报原因：',
              style: TextStyle(fontWeight: FontWeight.w600),
            ),
            const SizedBox(height: 12),
            ..._reportReasons.entries.map((entry) {
              // ignore: deprecated_member_use
              return RadioListTile<String>(
                title: Text(entry.value),
                value: entry.key,
                // ignore: deprecated_member_use
                groupValue: _selectedReason,
                // ignore: deprecated_member_use
                onChanged: (value) {
                  if (value != null) {
                    setState(() => _selectedReason = value);
                  }
                },
                contentPadding: EdgeInsets.zero,
                dense: true,
              );
            }),
            const SizedBox(height: 16),
            TextField(
              controller: _descriptionController,
              maxLines: 3,
              maxLength: 200,
              decoration: const InputDecoration(
                labelText: '详细描述（可选）',
                hintText: '请简要描述问题...',
                border: OutlineInputBorder(),
              ),
            ),
          ],
        ),
      ),
      actions: [
        TextButton(
          onPressed: _isSubmitting ? null : () => Navigator.of(context).pop(),
          child: const Text('取消'),
        ),
        ElevatedButton(
          onPressed: _isSubmitting ? null : _submitReport,
          style: ElevatedButton.styleFrom(
            backgroundColor: Colors.red,
            foregroundColor: Colors.white,
          ),
          child: _isSubmitting
              ? const SizedBox(
                  width: 20,
                  height: 20,
                  child: CircularProgressIndicator(strokeWidth: 2),
                )
              : const Text('提交举报'),
        ),
      ],
    );
  }
}
