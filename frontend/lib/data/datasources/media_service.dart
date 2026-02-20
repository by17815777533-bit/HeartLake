// @file media_service.dart
// @brief 媒体服务 - 处理文件上传和媒体管理
// Created by 王璐瑶

import 'dart:io';
import 'package:http/http.dart' as http;
import 'package:dio/dio.dart' as dio;
import 'dart:convert';
import 'package:flutter/foundation.dart';
import 'package:http_parser/http_parser.dart';
import '../../utils/app_config.dart';
import '../../utils/storage_util.dart';

class MediaService {
  final appConfig = AppConfig();
  String get baseUrl => appConfig.apiBaseUrl;

  // 上传超时时间（毫秒）
  static const int uploadTimeout = 60000; // 60秒

  // 获取认证token
  Future<String?> _getToken() async {
    return await StorageUtil.getToken();
  }

  // 获取用户ID
  Future<String?> _getUserId() async {
    return await StorageUtil.getUserId();
  }

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
      final token = await _getToken();
      final userId = await _getUserId();

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

      var request = http.MultipartRequest(
        'POST',
        Uri.parse('$baseUrl/media/upload'),
      );

      // 添加认证headers
      if (token != null) {
        request.headers['Authorization'] = 'Bearer $token';
      }
      if (userId != null) {
        request.headers['X-User-Id'] = userId;
      }

      // 添加文件 - 使用正确的 MIME 类型
      var stream = http.ByteStream(file.openRead());
      var multipartFile = http.MultipartFile(
        'file',
        stream,
        fileLength,
        filename: fileName,
        contentType: _getMimeType(fileName),
      );
      request.files.add(multipartFile);

      debugPrint(
          '📤 上传媒体文件: $fileName (${(fileLength / 1024).toStringAsFixed(1)} KB)');

      // 发送请求，带超时
      var streamedResponse = await request.send().timeout(
        const Duration(milliseconds: uploadTimeout),
        onTimeout: () {
          throw Exception('上传超时，请检查网络连接');
        },
      );

      var responseData = await streamedResponse.stream.bytesToString();

      if (responseData.isEmpty) {
        return {
          'success': false,
          'message': '服务器响应为空',
        };
      }

      var jsonResponse = json.decode(responseData);

      if (streamedResponse.statusCode == 200 && jsonResponse['code'] == 0) {
        debugPrint('✅ 媒体上传成功: ${jsonResponse['data']?['media_id']}');
        return {
          'success': true,
          'data': jsonResponse['data'],
        };
      } else {
        debugPrint('❌ 媒体上传失败: ${jsonResponse['message']}');
        return {
          'success': false,
          'message': jsonResponse['message'] ?? '上传失败',
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
      final token = await _getToken();
      final userId = await _getUserId();

      var request = http.MultipartRequest(
        'POST',
        Uri.parse('$baseUrl/media/upload/multiple'),
      );

      // 添加认证headers
      if (token != null) {
        request.headers['Authorization'] = 'Bearer $token';
      }
      if (userId != null) {
        request.headers['X-User-Id'] = userId;
      }

      // 添加所有文件 - 使用正确的 MIME 类型
      int totalSize = 0;
      for (var file in files) {
        final fileName = file.path.split('/').last;
        final fileLength = await file.length();
        totalSize += fileLength;

        var stream = http.ByteStream(file.openRead());
        var multipartFile = http.MultipartFile(
          'files',
          stream,
          fileLength,
          filename: fileName,
          contentType: _getMimeType(fileName),
        );
        request.files.add(multipartFile);
      }

      debugPrint(
          '📤 批量上传 ${files.length} 个文件 (${(totalSize / 1024).toStringAsFixed(1)} KB)');

      // 发送请求，带超时（批量上传时间更长）
      var streamedResponse = await request.send().timeout(
        Duration(milliseconds: uploadTimeout * files.length),
        onTimeout: () {
          throw Exception('上传超时，请检查网络连接');
        },
      );
      var responseData = await streamedResponse.stream.bytesToString();

      if (responseData.isEmpty) {
        return {
          'success': false,
          'message': '服务器响应为空',
        };
      }

      var jsonResponse = json.decode(responseData);

      if (streamedResponse.statusCode == 200 && jsonResponse['code'] == 0) {
        debugPrint('✅ 批量上传成功');
        return {
          'success': true,
          'data': jsonResponse['data'],
        };
      } else {
        debugPrint('❌ 批量上传失败: ${jsonResponse['message']}');
        return {
          'success': false,
          'message': jsonResponse['message'] ?? '批量上传失败',
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
      final token = await _getToken();
      final userId = await _getUserId();
      final fileName = file.path.split(Platform.pathSeparator).last;

      final formData = dio.FormData.fromMap({
        'file': await dio.MultipartFile.fromFile(
          file.path,
          filename: fileName,
          contentType: _getMimeType(fileName),
        ),
      });

      final dioClient = dio.Dio();
      final response = await dioClient.post(
        '$baseUrl/media/upload',
        data: formData,
        options: dio.Options(
          headers: {
            if (token != null) 'Authorization': 'Bearer $token',
            if (userId != null) 'X-User-Id': userId,
          },
        ),
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
      return {'success': false, 'message': '上传失败'};
    }
  }

  // 获取媒体信息
  Future<Map<String, dynamic>> getMediaInfo(String mediaId) async {
    try {
      final token = await _getToken();
      final response = await http.get(
        Uri.parse('$baseUrl/media/$mediaId'),
        headers: token != null ? {'Authorization': 'Bearer $token'} : null,
      );

      if (response.statusCode == 200) {
        var jsonResponse = json.decode(response.body);
        if (jsonResponse['code'] == 0) {
          return {'success': true, 'data': jsonResponse['data']};
        }
      }
      return {'success': false, 'message': '获取媒体信息失败'};
    } catch (e) {
      return {'success': false, 'message': '获取媒体信息失败: $e'};
    }
  }

  // 删除媒体文件
  Future<Map<String, dynamic>> deleteMedia(String mediaId) async {
    try {
      final token = await _getToken();
      final response = await http.delete(
        Uri.parse('$baseUrl/media/$mediaId'),
        headers: token != null ? {'Authorization': 'Bearer $token'} : null,
      );

      if (response.statusCode == 200) {
        return {'success': true, 'message': '删除成功'};
      }
      return {'success': false, 'message': '删除失败'};
    } catch (e) {
      return {'success': false, 'message': '删除失败: $e'};
    }
  }

  // 获取媒体URL
  String getMediaUrl(String url) {
    if (url.startsWith('http')) {
      return url;
    }
    // 使用配置的基础URL替换localhost
    final baseUrlWithoutApi = appConfig.apiBaseUrl.replaceFirst('/api', '');
    return '$baseUrlWithoutApi$url';
  }

  // 判断文件类型
  MediaType getMediaType(File file) {
    String extension = file.path.split('.').last.toLowerCase();

    // 图片
    if (['jpg', 'jpeg', 'png', 'gif', 'webp'].contains(extension)) {
      return MediaType('image', extension == 'jpg' ? 'jpeg' : extension);
    }

    // 视频
    if (['mp4', 'mov', 'avi'].contains(extension)) {
      return MediaType('video', extension);
    }

    // 音频
    if (['mp3', 'wav', 'ogg', 'm4a'].contains(extension)) {
      return MediaType('audio', extension);
    }

    return MediaType('application', 'octet-stream');
  }

  // 验证文件大小
  Future<bool> validateFileSize(File file, int maxSizeMB) async {
    int fileSize = await file.length();
    int maxSize = maxSizeMB * 1024 * 1024;
    return fileSize <= maxSize;
  }
}
