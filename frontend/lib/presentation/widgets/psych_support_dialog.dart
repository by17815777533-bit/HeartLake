// 心理支持弹窗 - 温暖关怀的心理援助入口

import 'package:flutter/material.dart';
import 'package:url_launcher/url_launcher.dart';
import '../../data/datasources/psych_support_service.dart';
import '../../data/datasources/social_payload_normalizer.dart';
import '../../di/service_locator.dart';

class PsychSupportDialog extends StatefulWidget {
  final String? helpTip;

  const PsychSupportDialog({super.key, this.helpTip});

  static Future<void> show(BuildContext context, {String? helpTip}) {
    return showDialog(
      context: context,
      barrierDismissible: false,
      builder: (_) => PsychSupportDialog(helpTip: helpTip),
    );
  }

  @override
  State<PsychSupportDialog> createState() => _PsychSupportDialogState();
}

class _PsychSupportDialogState extends State<PsychSupportDialog> {
  final PsychSupportService _service = sl<PsychSupportService>();
  List<Map<String, dynamic>> _hotlines = [];
  String _prompt = '';
  bool _loading = true;

  @override
  void initState() {
    super.initState();
    _loadData();
  }

  List<Map<String, dynamic>> _extractHotlines(Map<String, dynamic> payload) {
    return extractNormalizedList(
      payload,
      itemNormalizer: (item) => Map<String, dynamic>.from(item),
      listKeys: const ['hotlines'],
    );
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

  Future<void> _loadData() async {
    try {
      final results =
          await Future.wait([_service.getHotlines(), _service.getPrompt()]);
      if (!mounted) return;
      final hotlinesSuccess =
          results[0]['success'] == true && results[0]['data'] != null;
      final promptSuccess =
          results[1]['success'] == true && results[1]['data'] != null;
      final failures = <String>[
        if (!hotlinesSuccess) results[0]['message']?.toString() ?? '心理热线加载失败',
        if (!promptSuccess) results[1]['message']?.toString() ?? '关怀提示加载失败',
      ];
      final promptData = promptSuccess && results[1]['data'] is Map
          ? Map<String, dynamic>.from(
              (results[1]['data'] as Map).cast<String, dynamic>(),
            )
          : const <String, dynamic>{};
      if (mounted) {
        setState(() {
          _hotlines =
              hotlinesSuccess ? _extractHotlines(results[0]) : _hotlines;
          _prompt = promptData['prompt']?.toString() ??
              (_prompt.isNotEmpty ? _prompt : null) ??
              widget.helpTip ??
              '需要有人陪伴吗？我们在这里倾听你。';
          _loading = false;
        });
        if (failures.isNotEmpty) {
          _showMessage(failures.join('；'));
        }
      }
    } catch (error, stackTrace) {
      _reportUiError(error, stackTrace, 'PsychSupportDialog._loadData');
      if (mounted) {
        setState(() {
          _prompt = _prompt.isNotEmpty
              ? _prompt
              : (widget.helpTip ?? '需要有人陪伴吗？我们在这里倾听你。');
          _loading = false;
        });
        _showMessage('心理支持信息加载失败，请稍后重试');
      }
    }
  }

  Future<void> _callHotline(String phone) async {
    if (phone.trim().isEmpty) {
      _showMessage('热线号码缺失');
      return;
    }
    final uri = Uri.parse('tel:$phone');
    try {
      if (await canLaunchUrl(uri)) {
        await launchUrl(uri);
      } else {
        _showMessage('无法拨打 $phone');
      }
    } catch (error, stackTrace) {
      _reportUiError(error, stackTrace, 'PsychSupportDialog._callHotline');
      _showMessage('拨号失败，请稍后重试');
    }
  }

  @override
  Widget build(BuildContext context) {
    return AlertDialog(
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(20)),
      backgroundColor: const Color(0xFFFFF8E7),
      title: Row(
        children: [
          Container(
            padding: const EdgeInsets.all(8),
            decoration: BoxDecoration(
                color: const Color(0xFFFFE0B2),
                borderRadius: BorderRadius.circular(12)),
            child:
                const Icon(Icons.favorite, color: Color(0xFFFF8A65), size: 24),
          ),
          const SizedBox(width: 12),
          const Text('需要有人陪伴吗？',
              style: TextStyle(
                  fontSize: 18,
                  fontWeight: FontWeight.w600,
                  color: Color(0xFF5D4037))),
        ],
      ),
      content: _loading
          ? const SizedBox(
              height: 100, child: Center(child: CircularProgressIndicator()))
          : Column(
              mainAxisSize: MainAxisSize.min,
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(_prompt,
                    style: const TextStyle(
                        fontSize: 15, height: 1.6, color: Color(0xFF6D4C41))),
                const SizedBox(height: 20),
                ..._hotlines.take(2).map((h) => Padding(
                      padding: const EdgeInsets.only(bottom: 12),
                      child: _buildResourceCard(
                        icon: Icons.phone_in_talk,
                        title: h['name'] ?? '心理援助热线',
                        subtitle: h['phone'] ?? '',
                        color: const Color(0xFF81C784),
                        onTap: () => _callHotline(h['phone'] ?? ''),
                      ),
                    )),
              ],
            ),
      actions: [
        TextButton(
          onPressed: () => Navigator.of(context).pop(),
          child: const Text('我知道了',
              style: TextStyle(color: Color(0xFF8D6E63), fontSize: 16)),
        ),
        ElevatedButton(
          onPressed: _hotlines.isNotEmpty
              ? () => _callHotline(_hotlines.first['phone'] ?? '')
              : null,
          style: ElevatedButton.styleFrom(
              backgroundColor: const Color(0xFF81C784),
              foregroundColor: Colors.white),
          child: const Text('寻求专业帮助'),
        ),
      ],
    );
  }

  Widget _buildResourceCard(
      {required IconData icon,
      required String title,
      required String subtitle,
      required Color color,
      required VoidCallback onTap}) {
    return Material(
      color: Colors.white,
      borderRadius: BorderRadius.circular(12),
      child: InkWell(
        onTap: onTap,
        borderRadius: BorderRadius.circular(12),
        child: Padding(
          padding: const EdgeInsets.all(12),
          child: Row(
            children: [
              Container(
                padding: const EdgeInsets.all(10),
                decoration: BoxDecoration(
                    color: color.withValues(alpha: 0.2),
                    borderRadius: BorderRadius.circular(10)),
                child: Icon(icon, color: color, size: 24),
              ),
              const SizedBox(width: 12),
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(title,
                        style: const TextStyle(
                            fontWeight: FontWeight.w600,
                            fontSize: 14,
                            color: Color(0xFF5D4037))),
                    const SizedBox(height: 2),
                    Text(subtitle,
                        style:
                            TextStyle(fontSize: 12, color: Colors.grey[600])),
                  ],
                ),
              ),
              Icon(Icons.chevron_right, color: Colors.grey[400]),
            ],
          ),
        ),
      ),
    );
  }
}
