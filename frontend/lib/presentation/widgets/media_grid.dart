// @file media_grid.dart
// @brief 多媒体网格展示组件
// Created by 吴睿璐

import 'package:flutter/material.dart';
import 'dart:io';
import 'image_preview.dart';

/// 多媒体网格展示组件
/// 支持本地文件预览和网络图片显示
class MediaGrid extends StatelessWidget {
  final List<dynamic> mediaItems; // File 或 String (URL)
  final Function(int)? onRemove;
  final bool editable;

  const MediaGrid({
    super.key,
    required this.mediaItems,
    this.onRemove,
    this.editable = false,
  });

  bool _isImage(dynamic item) {
    final path = item is File ? item.path.toLowerCase() : item.toString().toLowerCase();
    return path.contains('.jpg') || path.contains('.jpeg') ||
           path.contains('.png') || path.contains('.gif');
  }

  void _openPreview(BuildContext context, int index) {
    final images = mediaItems.where(_isImage).toList();
    final imageIndex = images.indexOf(mediaItems[index]);
    if (imageIndex >= 0) {
      Navigator.push(
        context,
        MaterialPageRoute(
          builder: (_) => ImagePreviewScreen(images: images, initialIndex: imageIndex),
        ),
      );
    }
  }

  @override
  Widget build(BuildContext context) {
    if (mediaItems.isEmpty) return const SizedBox.shrink();

    return GridView.builder(
      shrinkWrap: true,
      physics: const NeverScrollableScrollPhysics(),
      gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
        crossAxisCount: 3,
        crossAxisSpacing: 8,
        mainAxisSpacing: 8,
      ),
      itemCount: mediaItems.length,
      itemBuilder: (context, index) {
        final item = mediaItems[index];

        return Stack(
          children: [
            GestureDetector(
              onTap: _isImage(item) ? () => _openPreview(context, index) : null,
              child: ClipRRect(
                borderRadius: BorderRadius.circular(8),
                child: _buildMediaPreview(item),
              ),
            ),
            if (editable && onRemove != null)
              Positioned(
                top: 4,
                right: 4,
                child: GestureDetector(
                  onTap: () => onRemove!(index),
                  child: Container(
                    padding: const EdgeInsets.all(4),
                    decoration: const BoxDecoration(
                      color: Colors.black54,
                      shape: BoxShape.circle,
                    ),
                    child: const Icon(
                      Icons.close,
                      size: 16,
                      color: Colors.white,
                    ),
                  ),
                ),
              ),
          ],
        );
      },
    );
  }

  Widget _buildMediaPreview(dynamic item) {
    if (item is File) {
      // 本地文件预览
      final path = item.path.toLowerCase();
      if (path.endsWith('.jpg') ||
          path.endsWith('.jpeg') ||
          path.endsWith('.png') ||
          path.endsWith('.gif')) {
        return Image.file(
          item,
          fit: BoxFit.cover,
          width: double.infinity,
          height: double.infinity,
        );
      } else if (path.endsWith('.mp4') || path.endsWith('.mov')) {
        return Container(
          color: Colors.black,
          child: const Center(
            child:
                Icon(Icons.play_circle_outline, size: 40, color: Colors.white),
          ),
        );
      } else if (path.endsWith('.mp3') || path.endsWith('.wav')) {
        return Container(
          color: Colors.grey[300],
          child: const Center(
            child: Icon(Icons.audiotrack, size: 40, color: Colors.grey),
          ),
        );
      }
    } else if (item is String) {
      // 网络URL
      if (item.contains('.jpg') ||
          item.contains('.jpeg') ||
          item.contains('.png') ||
          item.contains('.gif')) {
        return Image.network(
          item,
          fit: BoxFit.cover,
          width: double.infinity,
          height: double.infinity,
          loadingBuilder: (context, child, loadingProgress) {
            if (loadingProgress == null) return child;
            return const Center(child: CircularProgressIndicator());
          },
          errorBuilder: (context, error, stackTrace) {
            return Container(
              color: Colors.grey[300],
              child: const Icon(Icons.broken_image, color: Colors.grey),
            );
          },
        );
      } else if (item.contains('.mp4') || item.contains('.mov')) {
        return Container(
          color: Colors.black,
          child: const Center(
            child:
                Icon(Icons.play_circle_outline, size: 40, color: Colors.white),
          ),
        );
      } else if (item.contains('.mp3') || item.contains('.wav')) {
        return Container(
          color: Colors.grey[300],
          child: const Center(
            child: Icon(Icons.audiotrack, size: 40, color: Colors.grey),
          ),
        );
      }
    }

    return Container(
      color: Colors.grey[300],
      child: const Center(
        child: Icon(Icons.insert_drive_file, color: Colors.grey),
      ),
    );
  }
}
