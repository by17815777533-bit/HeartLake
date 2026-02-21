// @file psych_support_service.dart
// @brief 心理支持服务 - 获取心理援助资源
// Created by frontend-artist

import 'base_service.dart';

class PsychSupportService extends BaseService {
  @override
  String get serviceName => 'PsychSupport';

  Future<Map<String, dynamic>> getHotlines() async {
    final response = await get('/safe-harbor/hotlines');
    return toMap(response);
  }

  Future<Map<String, dynamic>> getTools() async {
    final response = await get('/safe-harbor/tools');
    return toMap(response);
  }

  Future<Map<String, dynamic>> getPrompt() async {
    final response = await get('/safe-harbor/prompt');
    return toMap(response);
  }
}
