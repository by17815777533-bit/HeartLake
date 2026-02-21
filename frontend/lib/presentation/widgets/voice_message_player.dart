// @file voice_message_player.dart
// @brief 语音消息播放器组件
// Created by 林子怡

import 'dart:async';
import 'package:flutter/material.dart';
import 'package:audioplayers/audioplayers.dart';

class VoiceMessagePlayer extends StatefulWidget {
  final String audioUrl;
  final int duration; // 秒
  final bool isSelf;

  const VoiceMessagePlayer({
    super.key,
    required this.audioUrl,
    required this.duration,
    this.isSelf = false,
  });

  @override
  State<VoiceMessagePlayer> createState() => _VoiceMessagePlayerState();
}

class _VoiceMessagePlayerState extends State<VoiceMessagePlayer>
    with SingleTickerProviderStateMixin {
  final AudioPlayer _audioPlayer = AudioPlayer();
  bool _isPlaying = false;
  int _currentPosition = 0;
  late AnimationController _animationController;

  // 存储 StreamSubscription 以便在 dispose 时取消
  StreamSubscription<PlayerState>? _playerStateSubscription;
  StreamSubscription<Duration>? _positionSubscription;
  StreamSubscription<void>? _playerCompleteSubscription;

  @override
  void initState() {
    super.initState();
    _animationController = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 500),
    );

    _playerStateSubscription = _audioPlayer.onPlayerStateChanged.listen((state) {
      if (mounted) {
        setState(() {
          _isPlaying = state == PlayerState.playing;
        });
        if (state == PlayerState.playing) {
          _animationController.repeat();
        } else {
          _animationController.stop();
        }
      }
    });

    _positionSubscription = _audioPlayer.onPositionChanged.listen((position) {
      if (mounted) {
        setState(() {
          _currentPosition = position.inSeconds;
        });
      }
    });

    _playerCompleteSubscription = _audioPlayer.onPlayerComplete.listen((_) {
      if (mounted) {
        setState(() {
          _isPlaying = false;
          _currentPosition = 0;
        });
        _animationController.stop();
      }
    });
  }

  Future<void> _togglePlayPause() async {
    if (_isPlaying) {
      await _audioPlayer.pause();
    } else {
      await _audioPlayer.play(UrlSource(widget.audioUrl));
    }
  }

  @override
  void dispose() {
    // 取消所有 StreamSubscription，防止内存泄漏
    _playerStateSubscription?.cancel();
    _positionSubscription?.cancel();
    _playerCompleteSubscription?.cancel();
    _audioPlayer.dispose();
    _animationController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final displayDuration = _isPlaying ? _currentPosition : widget.duration;

    return GestureDetector(
      onTap: _togglePlayPause,
      child: Container(
        constraints: const BoxConstraints(
          minWidth: 100,
          maxWidth: 200,
        ),
        padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
        decoration: BoxDecoration(
          color: widget.isSelf ? Colors.blue[50] : Colors.grey[100],
          borderRadius: BorderRadius.circular(18),
          border: Border.all(
            color: widget.isSelf
                ? Colors.blue.withValues(alpha: 0.3)
                : Colors.grey.withValues(alpha: 0.3),
          ),
        ),
        child: Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            // 播放/暂停按钮
            Icon(
              _isPlaying ? Icons.pause : Icons.play_arrow,
              color: widget.isSelf ? Colors.blue : Colors.black87,
              size: 24,
            ),
            const SizedBox(width: 8),
            // 波形动画
            Expanded(
              child: Row(
                mainAxisSize: MainAxisSize.min,
                children: List.generate(3, (index) {
                  return AnimatedBuilder(
                    animation: _animationController,
                    builder: (context, child) {
                      return Container(
                        width: 2,
                        height: _isPlaying
                            ? 12 +
                                ((index + _animationController.value * 3)
                                            .toInt() %
                                        3) *
                                    4
                            : 12,
                        margin: const EdgeInsets.symmetric(horizontal: 2),
                        decoration: BoxDecoration(
                          color: widget.isSelf ? Colors.blue : Colors.black54,
                          borderRadius: BorderRadius.circular(1),
                        ),
                      );
                    },
                  );
                }),
              ),
            ),
            const SizedBox(width: 8),
            // 时长
            Text(
              '$displayDuration"',
              style: TextStyle(
                fontSize: 13,
                color: widget.isSelf ? Colors.blue[700] : Colors.black87,
              ),
            ),
          ],
        ),
      ),
    );
  }
}
