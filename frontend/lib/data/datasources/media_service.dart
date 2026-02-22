// @file media_service.dart
// @brief 媒体服务 - 处理文件上传和媒体管理
// Created by 王璐瑶

import 'dart:io';
import 'package:dio/dio.dart';
import 'package:flutter/foundation.dart';
import 'package:http_parser/http_parser.dart';
import '../../utils/app_config.dart';
import 'base_service.dart';

class MediaService extends BaseService {
  @override
  String get serviceName => 'MediaService';

  // 上传超时时间（毫秒）
  static const int uploadTimeout = 60000; // 60秒

  // 根据文件扩展名获取 MIME 类型
  MediaType _getMimeType(String fileName) {
    final ext = fileName.split('.').last.toLowerCase();
    switch (ext) {
      case 'jpg':
      case 'jpeg':
        return MediaType('image', 'jpeg');
      case 'png':
        return MediaType('image', 'png');
      case 'gif':
        return MediaType('image', 'gif');
      case 'webp':
        return MediaType('image', 'webp');
      case 'mp4':
        return MediaType('video', 'mp4');
      case 'mov':
        return MediaType('video', 'quicktime');
      case 'avi':
        return MediaType('video', 'x-msvideo');
      case 'mkv':
        return MediaType('video', 'x-matroska');
      case 'mp3':
        return MediaType('audio', 'mpeg');
      case 'wav':
        return MediaType('audio', 'wav');
      case 'aac':
        return MediaType('audio', 'aac');
      case 'm4a':
        return MediaType('audio', 'mp4');
      default:
        return MediaType('application', 'octet-stream');
    }
  }

  // 上传单个媒体文件
  Future<Map<String, dynamic>> uploadMedia(File file) async {
    try {
      // 验证文件大小
      final fileLength = await file.length();
      final fileName = file.path.split('/').last;
      final ext = fileName.split('.').last.toLowerCase();

      // 图片最大10MB，视频最大100MB，音频最大20MB
      int maxSize;
      if (['jpg', 'jpeg', 'png', 'gif', 'webp'].contains(ext)) {
        maxSize = 10 * 1024 * 1024;
      } else if (['mp4', 'mov', 'avi', 'mkv'].contains(ext)) {
        maxSize = 100 * 1024 * 1024;
      } else if (['mp3', 'wav', 'aac', 'm4a'].contains(ext)) {
        maxSize = 20 * 1024 * 1024;
      } else {
        maxSize = 10 * 1024 * 1024;
      }

      if (fileLength > maxSize) {
        return {
          'success': false,
          'message': '文件太大，请选择较小的文件',
        };
      }

      final formData = FormData.fromMap({
        'file': await MultipartFile.fromFile(
          file.path,
          filename: fileName,
          contentType: _getMimeType(fileName),
        ),
      });

      debugPrint(
          '📤 上传媒体文件: $fileName (${(fileLength / 1024).toStringAsFixed(1)} KB)');

      final response = await post<Map<String, dynamic>>(
        '/media/upload',
        data: formData,
      );

      if (response.success) {
        debugPrint('✅ 媒体上传成功: ${response.data?['media_id']}');
        return {
          'success': true,
          'data': response.data,
        };
      } else {
        debugPrint('❌ 媒体上传失败: ${response.message}');
        return {
          'success': false,
          'message': response.message ?? '上传失败',
        };
      }
    } catch (e) {
      debugPrint('❌ 上传错误: $e');
      return {
        'success': false,
        'message': _uploadErrorMessage(e),
      };
    }
  }

  /// 统一上传错误消息
  String _uploadErrorMessage(dynamic e) {
    final msg = e.toString();
    if (msg.contains('timeout') || msg.contains('超时')) {
      return '上传超时，请检查网络连接';
    }
    if (msg.contains('Connection refused') || msg.contains('connection')) {
      return '无法连接服务器';
    }
    return '上传失败';
  }

  // 批量上传媒体文件（最多9个）
  Future<Map<String, dynamic>> uploadMultipleMedia(List<File> files) async {
    if (files.length > 9) {
      return {
        'success': false,
        'message': '最多只能上传9个文件',
      };
    }

    if (files.isEmpty) {
      return {
        'success': false,
        'message': '没有选择文件',
      };
    }

    try {
      final List<MultipartFile> multipartFiles = [];
      int totalSize = 0;

      for (var file in files) {
        final fileName = file.path.split('/').last;
        final fileLength = await file.length();
        totalSize += fileLength;

        multipartFiles.add(await MultipartFile.fromFile(
          file.path,
          filename: fileName,
          contentType: _getMimeType(fileName),
        ));
      }

      final formData = FormData();
      for (var mf in multipartFiles) {
        formData.files.add(MapEntry('files', mf));
      }

      debugPrint(
          '📤 批量上传 ${files.length} 个文件 (${(totalSize / 1024).toStringAsFixed(1)} KB)');

      final response = await post<Map<String, dynamic>>(
        '/media/upload/multiple',
        data: formData,
      );

      if (response.success) {
        debugPrint('✅ 批量上传成功');
        return {
          'success': true,
          'data': response.data,
        };
      } else {
        debugPrint('❌ 批量上传失败: ${response.message}');
        return {
          'success': false,
          'message': response.message ?? '批量上传失败',
        };
      }
    } catch (e) {
      debugPrint('❌ 批量上传错误: $e');
      return {
        'success': false,
        'message': _uploadErrorMessage(e),
      };
    }
  }

  // 上传媒体文件（带进度回调）
  Future<Map<String, dynamic>> uploadMediaWithProgress(
    File file, {
    void Function(double progress)? onProgress,
  }) async {
    try {
      final fileName = file.path.split(Platform.pathSeparator).last;

      final formData = FormData.fromMap({
        'file': await MultipartFile.fromFile(
          file.path,
          filename: fileName,
          contentType: _getMimeType(fileName),
        ),
      });

      // 使用 ApiClient 的 dio 实例直接调用以支持 onSendProgress
      final response = await client.dio.post(
        '/media/upload',
        data: formData,
        onSendProgress: (sent, total) {
          if (total > 0 && onProgress != null) {
            onProgress(sent / total);
          }
        },
      );

      if (response.statusCode == 200 && response.data['code'] == 0) {
        return {'success': true, 'data': response.data['data']};
      }
      return {'success': false, 'message': response.data['message'] ?? '上传失败'};
    } catch (e) {
      debugPrint('❌ 上传错误: $e');
      return {'success': false, 'message': _uploadErrorMessage(e)};
    }
  }

  // 获取媒体信息
  Future<Map<String, dynamic>> getMediaInfo(String mediaId) async {
    final response = await get<Map<String, dynamic>>('/media/$mediaId');
    return toMap(response);
  }

  // 删除媒体文件
  Future<Map<String, dynamic>> deleteMedia(String mediaId) async {
    final response = await delete('/media/$mediaId');
    return toMap(response);
  }

  // 获取媒体URL
  String getMediaUrl(String url) {
    if (url.startsWith('http')) {
      return url;
    }
    final baseUrlWithoutApi = AppConfig().apiBaseUrl.replaceFirst('/api', '');
    return '$baseUrlWithoutApi$url';
  }

  // 判断文件类型（委托给 _getMimeType 避免重复逻辑）
  MediaType getMediaType(File file) {
    return _getMimeType(file.path.split('.').last);
  }

  // 验证文件大小
  Future<bool> validateFileSize(File file, int maxSizeMB) async {
    int fileSize = await file.length();
    int maxSize = maxSizeMB * 1024 * 1024;
    return fileSize <= maxSize;
  }
}
