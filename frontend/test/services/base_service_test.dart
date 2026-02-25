import 'package:flutter_test/flutter_test.dart';
import 'package:heart_lake/data/datasources/base_service.dart';

/// BaseService 的 ServiceResponse 和 toMap 逻辑的全面测试。

void main() {
  // ==================== ServiceResponse 泛型测试 ====================
  group('ServiceResponse with String type', () {
    test('should store string data', () {
      final resp = ServiceResponse<String>.success('hello');
      expect(resp.data, 'hello');
      expect(resp.success, true);
    });

    test('should handle null string', () {
      final resp = ServiceResponse<String>(success: true, data: null);
      expect(resp.data, isNull);
    });
  });

  group('ServiceResponse with int type', () {
    test('should store int data', () {
      final resp = ServiceResponse<int>.success(42);
      expect(resp.data, 42);
    });

    test('should store zero', () {
      final resp = ServiceResponse<int>.success(0);
      expect(resp.data, 0);
    });

    test('should store negative', () {
      final resp = ServiceResponse<int>.success(-1);
      expect(resp.data, -1);
    });
  });

  group('ServiceResponse with List type', () {
    test('should store list data', () {
      final resp = ServiceResponse<List<String>>.success(['a', 'b', 'c']);
      expect(resp.data, ['a', 'b', 'c']);
    });

    test('should store empty list', () {
      final resp = ServiceResponse<List>.success([]);
      expect(resp.data, isEmpty);
    });
  });

  group('ServiceResponse with Map type', () {
    test('should store map data', () {
      final resp = ServiceResponse<Map<String, dynamic>>.success({'key': 'value'});
      expect(resp.data!['key'], 'value');
    });

    test('should store nested map', () {
      final resp = ServiceResponse<Map<String, dynamic>>.success({
        'user': {'name': '小明', 'age': 25}
      });
      expect(resp.data!['user']['name'], '小明');
    });

    test('should store empty map', () {
      final resp = ServiceResponse<Map>.success({});
      expect(resp.data, isEmpty);
    });
  });

  group('ServiceResponse with bool type', () {
    test('should store true', () {
      final resp = ServiceResponse<bool>.success(true);
      expect(resp.data, true);
    });

    test('should store false', () {
      final resp = ServiceResponse<bool>.success(false);
      expect(resp.data, false);
    });
  });

  group('ServiceResponse with double type', () {
    test('should store double', () {
      final resp = ServiceResponse<double>.success(3.14);
      expect(resp.data, closeTo(3.14, 0.001));
    });

    test('should store zero', () {
      final resp = ServiceResponse<double>.success(0.0);
      expect(resp.data, 0.0);
    });

    test('should store negative', () {
      final resp = ServiceResponse<double>.success(-1.5);
      expect(resp.data, -1.5);
    });
  });

  // ==================== ServiceResponse.error 详细测试 ====================
  group('ServiceResponse.error detailed', () {
    test('should set success to false', () {
      final resp = ServiceResponse.error('错误');
      expect(resp.success, false);
    });

    test('should store message', () {
      final resp = ServiceResponse.error('网络超时');
      expect(resp.message, '网络超时');
    });

    test('should default code to 500', () {
      final resp = ServiceResponse.error('错误');
      expect(resp.code, 500);
    });

    test('should allow custom code 400', () {
      final resp = ServiceResponse.error('参数错误', code: 400);
      expect(resp.code, 400);
    });

    test('should allow custom code 401', () {
      final resp = ServiceResponse.error('未授权', code: 401);
      expect(resp.code, 401);
    });

    test('should allow custom code 403', () {
      final resp = ServiceResponse.error('禁止访问', code: 403);
      expect(resp.code, 403);
    });

    test('should allow custom code 404', () {
      final resp = ServiceResponse.error('未找到', code: 404);
      expect(resp.code, 404);
    });

    test('should allow custom code 429', () {
      final resp = ServiceResponse.error('请求过多', code: 429);
      expect(resp.code, 429);
    });

    test('should allow custom code 502', () {
      final resp = ServiceResponse.error('网关错误', code: 502);
      expect(resp.code, 502);
    });

    test('should allow custom code 503', () {
      final resp = ServiceResponse.error('服务不可用', code: 503);
      expect(resp.code, 503);
    });

    test('should have null data', () {
      final resp = ServiceResponse.error('错误');
      expect(resp.data, isNull);
    });

    test('should allow data in error', () {
      final resp = ServiceResponse.error('错误', data: {'detail': '详情'});
      expect(resp.data, isNotNull);
    });

    test('should handle empty message', () {
      final resp = ServiceResponse.error('');
      expect(resp.message, '');
    });

    test('should handle Chinese message', () {
      final resp = ServiceResponse.error('服务器内部错误，请稍后重试');
      expect(resp.message, contains('服务器'));
    });
  });

  // ==================== ServiceResponse.success 详细测试 ====================
  group('ServiceResponse.success detailed', () {
    test('should set success to true', () {
      final resp = ServiceResponse.success(null);
      expect(resp.success, true);
    });

    test('should set code to 200', () {
      final resp = ServiceResponse.success(null);
      expect(resp.code, 200);
    });

    test('should store message', () {
      final resp = ServiceResponse.success(null, message: '创建成功');
      expect(resp.message, '创建成功');
    });

    test('should allow null message', () {
      final resp = ServiceResponse.success(null);
      expect(resp.message, isNull);
    });

    test('should store complex data', () {
      final data = {
        'users': [
          {'id': 1, 'name': '小明'},
          {'id': 2, 'name': '小红'},
        ],
        'total': 2,
        'page': 1,
      };
      final resp = ServiceResponse<Map<String, dynamic>>.success(data);
      expect((resp.data!['users'] as List).length, 2);
    });
  });

  // ==================== ServiceResponse 构造函数 ====================
  group('ServiceResponse constructor edge cases', () {
    test('should allow all null optional fields', () {
      final resp = ServiceResponse(success: true);
      expect(resp.data, isNull);
      expect(resp.message, isNull);
      expect(resp.code, isNull);
    });

    test('should allow false success with data', () {
      final resp = ServiceResponse(success: false, data: 'error_data');
      expect(resp.success, false);
      expect(resp.data, 'error_data');
    });

    test('should allow code 0', () {
      final resp = ServiceResponse(success: true, code: 0);
      expect(resp.code, 0);
    });

    test('should allow negative code', () {
      final resp = ServiceResponse(success: false, code: -1);
      expect(resp.code, -1);
    });

    test('should allow very large code', () {
      final resp = ServiceResponse(success: false, code: 999999);
      expect(resp.code, 999999);
    });
  });

  // ==================== toMap 逻辑测试 ====================
  group('toMap conversion', () {
    // 模拟 BaseService.toMap
    Map<String, dynamic> toMap<T>(ServiceResponse<T> response) {
      return {
        'success': response.success,
        'code': response.code,
        'message': response.message,
        'data': response.data,
      };
    }

    test('should convert success response', () {
      final resp = ServiceResponse.success({'id': 1});
      final map = toMap(resp);
      expect(map['success'], true);
      expect(map['code'], 200);
      expect(map['data'], {'id': 1});
    });

    test('should convert error response', () {
      final resp = ServiceResponse.error('失败', code: 400);
      final map = toMap(resp);
      expect(map['success'], false);
      expect(map['code'], 400);
      expect(map['message'], '失败');
      expect(map['data'], isNull);
    });

    test('should convert response with null data', () {
      final resp = ServiceResponse.success(null);
      final map = toMap(resp);
      expect(map['data'], isNull);
    });

    test('should convert response with all fields', () {
      final resp = ServiceResponse(
        success: true,
        data: [1, 2, 3],
        message: '成功',
        code: 200,
      );
      final map = toMap(resp);
      expect(map.length, 4);
      expect(map.containsKey('success'), true);
      expect(map.containsKey('code'), true);
      expect(map.containsKey('message'), true);
      expect(map.containsKey('data'), true);
    });

    test('should preserve list data type', () {
      final resp = ServiceResponse.success([1, 2, 3]);
      final map = toMap(resp);
      expect(map['data'], isA<List>());
    });

    test('should preserve map data type', () {
      final resp = ServiceResponse.success({'key': 'value'});
      final map = toMap(resp);
      expect(map['data'], isA<Map>());
    });
  });

  // ==================== 分页信息构建 ====================
  group('Pagination building (StoneService pattern)', () {
    Map<String, dynamic> buildPagination(Map<String, dynamic>? data) {
      final page = data?['page'] ?? 1;
      final totalPages = data?['total_pages'] ?? 1;
      final total = data?['total'] ?? 0;
      final pageSize = data?['page_size'] ?? 20;
      return {
        'page': page,
        'total_pages': totalPages,
        'total': total,
        'page_size': pageSize,
        'has_more': page < totalPages,
      };
    }

    test('should build from complete data', () {
      final p = buildPagination({'page': 1, 'total_pages': 5, 'total': 100, 'page_size': 20});
      expect(p['page'], 1);
      expect(p['total_pages'], 5);
      expect(p['total'], 100);
      expect(p['page_size'], 20);
      expect(p['has_more'], true);
    });

    test('should have has_more false on last page', () {
      final p = buildPagination({'page': 5, 'total_pages': 5, 'total': 100, 'page_size': 20});
      expect(p['has_more'], false);
    });

    test('should have has_more false when page exceeds total', () {
      final p = buildPagination({'page': 6, 'total_pages': 5});
      expect(p['has_more'], false);
    });

    test('should default page to 1', () {
      final p = buildPagination({});
      expect(p['page'], 1);
    });

    test('should default total_pages to 1', () {
      final p = buildPagination({});
      expect(p['total_pages'], 1);
    });

    test('should default total to 0', () {
      final p = buildPagination({});
      expect(p['total'], 0);
    });

    test('should default page_size to 20', () {
      final p = buildPagination({});
      expect(p['page_size'], 20);
    });

    test('should handle null data', () {
      final p = buildPagination(null);
      expect(p['page'], 1);
      expect(p['has_more'], false);
    });

    test('single page should have has_more false', () {
      final p = buildPagination({'page': 1, 'total_pages': 1});
      expect(p['has_more'], false);
    });

    test('first of two pages should have has_more true', () {
      final p = buildPagination({'page': 1, 'total_pages': 2});
      expect(p['has_more'], true);
    });
  });
}
