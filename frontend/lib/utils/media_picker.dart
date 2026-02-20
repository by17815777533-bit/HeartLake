// @file media_picker.dart
// @brief 媒体选择工具
// Created by 王璐瑶

import 'package:flutter/material.dart';
import 'dart:io';
import 'package:image_picker/image_picker.dart';
import 'package:file_picker/file_picker.dart';

class MediaPicker {
  final ImagePicker _imagePicker = ImagePicker();

  // 从相册选择图片（支持多选）
  Future<List<File>> pickImages({int maxCount = 9}) async {
    try {
      final List<XFile> images = await _imagePicker.pickMultiImage();

      if (images.isEmpty) {
        return [];
      }

      // 限制数量
      final limitedImages = images.take(maxCount).toList();
      return limitedImages.map((xfile) => File(xfile.path)).toList();
    } catch (e) {
      debugPrint('选择图片失败: $e');
      return [];
    }
  }

  // 拍照
  Future<File?> takePhoto() async {
    try {
      final XFile? photo = await _imagePicker.pickImage(
        source: ImageSource.camera,
        imageQuality: 85,
      );

      if (photo == null) return null;
      return File(photo.path);
    } catch (e) {
      debugPrint('拍照失败: $e');
      return null;
    }
  }

  // 选择视频
  Future<File?> pickVideo() async {
    try {
      final XFile? video = await _imagePicker.pickVideo(
        source: ImageSource.gallery,
        maxDuration: const Duration(minutes: 5),
      );

      if (video == null) return null;
      return File(video.path);
    } catch (e) {
      debugPrint('选择视频失败: $e');
      return null;
    }
  }

  // 录制视频
  Future<File?> recordVideo() async {
    try {
      final XFile? video = await _imagePicker.pickVideo(
        source: ImageSource.camera,
        maxDuration: const Duration(minutes: 5),
      );

      if (video == null) return null;
      return File(video.path);
    } catch (e) {
      debugPrint('录制视频失败: $e');
      return null;
    }
  }

  // 选择音频文件
  Future<File?> pickAudio() async {
    try {
      FilePickerResult? result = await FilePicker.platform.pickFiles(
        type: FileType.audio,
        allowMultiple: false,
      );

      if (result == null || result.files.isEmpty) return null;

      final path = result.files.single.path;
      if (path == null) return null;

      return File(path);
    } catch (e) {
      debugPrint('选择音频失败: $e');
      return null;
    }
  }

  // 显示媒体选择器对话框
  Future<List<File>?> showMediaPicker(
    BuildContext context, {
    bool allowImages = true,
    bool allowVideo = true,
    bool allowAudio = true,
    int maxImageCount = 9,
  }) async {
    return showModalBottomSheet<List<File>>(
      context: context,
      builder: (context) => SafeArea(
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            if (allowImages) ...[
              ListTile(
                leading: const Icon(Icons.photo_library),
                title: const Text('从相册选择'),
                onTap: () async {
                  final files = await pickImages(maxCount: maxImageCount);
                  if (context.mounted) {
                    Navigator.pop(context, files);
                  }
                },
              ),
              ListTile(
                leading: const Icon(Icons.camera_alt),
                title: const Text('拍照'),
                onTap: () async {
                  final file = await takePhoto();
                  if (context.mounted) {
                    Navigator.pop(context, file != null ? [file] : null);
                  }
                },
              ),
            ],
            if (allowVideo) ...[
              ListTile(
                leading: const Icon(Icons.videocam),
                title: const Text('录制视频'),
                onTap: () async {
                  final file = await recordVideo();
                  if (context.mounted) {
                    Navigator.pop(context, file != null ? [file] : null);
                  }
                },
              ),
              ListTile(
                leading: const Icon(Icons.video_library),
                title: const Text('选择视频'),
                onTap: () async {
                  final file = await pickVideo();
                  if (context.mounted) {
                    Navigator.pop(context, file != null ? [file] : null);
                  }
                },
              ),
            ],
            if (allowAudio)
              ListTile(
                leading: const Icon(Icons.audiotrack),
                title: const Text('选择音频'),
                onTap: () async {
                  final file = await pickAudio();
                  if (context.mounted) {
                    Navigator.pop(context, file != null ? [file] : null);
                  }
                },
              ),
            ListTile(
              leading: const Icon(Icons.cancel),
              title: const Text('取消'),
              onTap: () => Navigator.pop(context),
            ),
          ],
        ),
      ),
    );
  }

  // 获取文件大小（MB）
  Future<double> getFileSizeMB(File file) async {
    int bytes = await file.length();
    return bytes / (1024 * 1024);
  }

  // 验证文件
  Future<Map<String, dynamic>> validateFile(
    File file, {
    double maxSizeMB = 100,
    List<String> allowedExtensions = const [],
  }) async {
    // 检查文件大小
    double sizeMB = await getFileSizeMB(file);
    if (sizeMB > maxSizeMB) {
      return {
        'valid': false,
        'message': '文件大小不能超过 ${maxSizeMB}MB',
      };
    }

    // 检查文件扩展名
    if (allowedExtensions.isNotEmpty) {
      String extension = file.path.split('.').last.toLowerCase();
      if (!allowedExtensions.contains(extension)) {
        return {
          'valid': false,
          'message': '不支持的文件格式',
        };
      }
    }

    return {
      'valid': true,
      'size_mb': sizeMB,
    };
  }
}
