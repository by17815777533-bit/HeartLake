// Provider压力测试 - ChangeNotifier通知机制、并发listener、内存泄漏检测
import 'package:flutter_test/flutter_test.dart';
import 'package:flutter/foundation.dart';

/// 用于压测的简单Provider
class StressTestProvider extends ChangeNotifier {
  int _counter = 0;
  int get counter => _counter;

  void increment() {
    _counter++;
    notifyListeners();
  }

  void reset() {
    _counter = 0;
    notifyListeners();
  }

  void setValue(int value) {
    _counter = value;
    notifyListeners();
  }

  void silentIncrement() {
    _counter++;
  }
}

/// 带历史记录的Provider
class HistoryProvider extends ChangeNotifier {
  final List<String> _history = [];
  List<String> get history => List.unmodifiable(_history);

  void addEntry(String entry) {
    _history.add(entry);
    notifyListeners();
  }

  void clear() {
    _history.clear();
    notifyListeners();
  }
}

/// 嵌套通知Provider
class NestedProvider extends ChangeNotifier {
  int _depth = 0;
  int get depth => _depth;
  int _notifyCount = 0;
  int get notifyCount => _notifyCount;

  void setDepth(int d) {
    _depth = d;
    _notifyCount++;
    notifyListeners();
  }
}

void main() {
  // ============================================================
  // 快速连续状态变更
  // ============================================================
  group('ChangeNotifier - 快速连续状态变更', () {
    late StressTestProvider provider;

    setUp(() {
      provider = StressTestProvider();
    });

    tearDown(() {
      provider.dispose();
    });

    test('连续increment 10000次', () {
      for (var i = 0; i < 10000; i++) {
        provider.increment();
      }
      expect(provider.counter, 10000);
    });

    test('连续increment 50000次', () {
      for (var i = 0; i < 50000; i++) {
        provider.increment();
      }
      expect(provider.counter, 50000);
    });

    test('交替increment和reset 5000次', () {
      for (var i = 0; i < 5000; i++) {
        provider.increment();
        provider.reset();
      }
      expect(provider.counter, 0);
    });

    test('setValue快速切换', () {
      for (var i = 0; i < 10000; i++) {
        provider.setValue(i);
      }
      expect(provider.counter, 9999);
    });

    test('silentIncrement不触发通知', () {
      var notifyCount = 0;
      provider.addListener(() => notifyCount++);
      for (var i = 0; i < 10000; i++) {
        provider.silentIncrement();
      }
      expect(provider.counter, 10000);
      expect(notifyCount, 0);
    });

    test('混合silent和notify操作', () {
      var notifyCount = 0;
      provider.addListener(() => notifyCount++);
      for (var i = 0; i < 1000; i++) {
        provider.silentIncrement();
        provider.silentIncrement();
        provider.increment(); // 只有这个触发通知
      }
      expect(provider.counter, 3000);
      expect(notifyCount, 1000);
    });
  });

  // ============================================================
  // 多listener并发
  // ============================================================
  group('ChangeNotifier - 多listener并发', () {
    late StressTestProvider provider;

    setUp(() {
      provider = StressTestProvider();
    });

    tearDown(() {
      provider.dispose();
    });

    test('100个listener同时监听', () {
      final counts = List.filled(100, 0);
      for (var i = 0; i < 100; i++) {
        final idx = i;
        provider.addListener(() => counts[idx]++);
      }
      provider.increment();
      for (final c in counts) {
        expect(c, 1);
      }
    });

    test('100个listener监听100次变更', () {
      final counts = List.filled(100, 0);
      for (var i = 0; i < 100; i++) {
        final idx = i;
        provider.addListener(() => counts[idx]++);
      }
      for (var i = 0; i < 100; i++) {
        provider.increment();
      }
      for (final c in counts) {
        expect(c, 100);
      }
    });

    test('500个listener同时监听', () {
      final counts = List.filled(500, 0);
      for (var i = 0; i < 500; i++) {
        final idx = i;
        provider.addListener(() => counts[idx]++);
      }
      provider.increment();
      for (final c in counts) {
        expect(c, 1);
      }
    });

    test('listener中读取最新状态', () {
      final values = <int>[];
      provider.addListener(() => values.add(provider.counter));
      for (var i = 0; i < 100; i++) {
        provider.increment();
      }
      expect(values.length, 100);
      for (var i = 0; i < 100; i++) {
        expect(values[i], i + 1);
      }
    });

    test('多个listener记录不同数据', () {
      final sums = <double>[];
      final products = <int>[];
      provider.addListener(() => sums.add(provider.counter * 1.5));
      provider.addListener(() => products.add(provider.counter * 2));
      for (var i = 0; i < 50; i++) {
        provider.increment();
      }
      expect(sums.length, 50);
      expect(products.length, 50);
      expect(sums.last, 50 * 1.5);
      expect(products.last, 50 * 2);
    });
  });

  // ============================================================
  // 添加/移除listener压测
  // ============================================================
  group('ChangeNotifier - 添加/移除listener', () {
    late StressTestProvider provider;

    setUp(() {
      provider = StressTestProvider();
    });

    tearDown(() {
      provider.dispose();
    });

    test('添加/移除listener 1000次', () {
      for (var i = 0; i < 1000; i++) {
        void listener() {}
        provider.addListener(listener);
        provider.removeListener(listener);
      }
      // 所有listener已移除，increment不应有副作用
      provider.increment();
      expect(provider.counter, 1);
    });

    test('添加1000个listener后全部移除', () {
      final listeners = <VoidCallback>[];
      for (var i = 0; i < 1000; i++) {
        void listener() {}
        listeners.add(listener);
        provider.addListener(listener);
      }
      for (final l in listeners) {
        provider.removeListener(l);
      }
      var notified = false;
      provider.addListener(() => notified = true);
      provider.increment();
      expect(notified, isTrue);
    });

    test('交替添加不同listener并移除', () {
      var countA = 0;
      var countB = 0;
      void listenerA() => countA++;
      void listenerB() => countB++;

      for (var i = 0; i < 500; i++) {
        provider.addListener(listenerA);
        provider.increment();
        provider.removeListener(listenerA);
        provider.addListener(listenerB);
        provider.increment();
        provider.removeListener(listenerB);
      }
      expect(countA, 500);
      expect(countB, 500);
    });

    test('重复添加同一个listener', () {
      var count = 0;
      void listener() => count++;
      // 添加同一个listener多次
      for (var i = 0; i < 100; i++) {
        provider.addListener(listener);
      }
      provider.increment();
      // ChangeNotifier允许重复添加，每次都会被调用
      expect(count, 100);
      // 移除一次只移除一个
      provider.removeListener(listener);
      count = 0;
      provider.increment();
      expect(count, 99);
    });

    test('移除不存在的listener不报错', () {
      void listener() {}
      // 不应抛出异常
      provider.removeListener(listener);
      provider.increment();
      expect(provider.counter, 1);
    });

    test('大量添加后dispose', () {
      final p = StressTestProvider();
      for (var i = 0; i < 1000; i++) {
        p.addListener(() {});
      }
      // dispose不应崩溃
      p.dispose();
    });
  });

  // ============================================================
  // 通知计数精确性
  // ============================================================
  group('ChangeNotifier - 通知计数精确性', () {
    test('每次notifyListeners精确触发一次', () {
      final provider = StressTestProvider();
      var count = 0;
      provider.addListener(() => count++);
      for (var i = 0; i < 5000; i++) {
        provider.increment();
      }
      expect(count, 5000);
      provider.dispose();
    });

    test('reset也触发通知', () {
      final provider = StressTestProvider();
      var count = 0;
      provider.addListener(() => count++);
      provider.increment();
      provider.reset();
      expect(count, 2);
      provider.dispose();
    });

    test('setValue每次都触发通知即使值相同', () {
      final provider = StressTestProvider();
      var count = 0;
      provider.addListener(() => count++);
      for (var i = 0; i < 100; i++) {
        provider.setValue(42);
      }
      // 即使值相同，notifyListeners仍然触发
      expect(count, 100);
      provider.dispose();
    });
  });

  // ============================================================
  // HistoryProvider 压测
  // ============================================================
  group('HistoryProvider - 大量数据', () {
    late HistoryProvider provider;

    setUp(() {
      provider = HistoryProvider();
    });

    tearDown(() {
      provider.dispose();
    });

    test('添加10000条历史记录', () {
      for (var i = 0; i < 10000; i++) {
        provider.addEntry('entry_$i');
      }
      expect(provider.history.length, 10000);
      expect(provider.history.first, 'entry_0');
      expect(provider.history.last, 'entry_9999');
    });

    test('添加后清空循环1000次', () {
      for (var i = 0; i < 1000; i++) {
        provider.addEntry('item_$i');
        provider.clear();
      }
      expect(provider.history.length, 0);
    });

    test('history返回不可变列表', () {
      provider.addEntry('test');
      final h = provider.history;
      expect(() => (h as List).add('hack'), throwsA(isA<UnsupportedError>()));
    });

    test('大量通知与历史记录同步', () {
      var notifyCount = 0;
      provider.addListener(() => notifyCount++);
      for (var i = 0; i < 500; i++) {
        provider.addEntry('e$i');
      }
      expect(notifyCount, 500);
      expect(provider.history.length, 500);
    });
  });

  // ============================================================
  // NestedProvider 压测
  // ============================================================
  group('NestedProvider - 通知追踪', () {
    late NestedProvider provider;

    setUp(() {
      provider = NestedProvider();
    });

    tearDown(() {
      provider.dispose();
    });

    test('通知计数准确', () {
      for (var i = 0; i < 1000; i++) {
        provider.setDepth(i);
      }
      expect(provider.notifyCount, 1000);
      expect(provider.depth, 999);
    });

    test('多listener都收到通知', () {
      final received = List.filled(10, 0);
      for (var i = 0; i < 10; i++) {
        final idx = i;
        provider.addListener(() => received[idx]++);
      }
      for (var i = 0; i < 100; i++) {
        provider.setDepth(i);
      }
      for (final r in received) {
        expect(r, 100);
      }
    });
  });

  // ============================================================
  // 内存泄漏检测模式
  // ============================================================
  group('ChangeNotifier - 内存泄漏检测', () {
    test('创建和dispose 1000个provider', () {
      for (var i = 0; i < 1000; i++) {
        final p = StressTestProvider();
        p.addListener(() {});
        p.increment();
        p.dispose();
      }
    });

    test('大量listener添加移除循环', () {
      final provider = StressTestProvider();
      for (var cycle = 0; cycle < 100; cycle++) {
        final listeners = <VoidCallback>[];
        for (var i = 0; i < 100; i++) {
          void l() {}
          listeners.add(l);
          provider.addListener(l);
        }
        provider.increment();
        for (final l in listeners) {
          provider.removeListener(l);
        }
      }
      // 所有listener已清理
      var finalCount = 0;
      provider.addListener(() => finalCount++);
      provider.increment();
      expect(finalCount, 1);
      provider.dispose();
    });

    test('provider链式创建dispose', () {
      StressTestProvider? current;
      for (var i = 0; i < 500; i++) {
        current?.dispose();
        current = StressTestProvider();
        current.addListener(() {});
        current.increment();
      }
      current?.dispose();
    });

    test('HistoryProvider大量数据后dispose', () {
      final p = HistoryProvider();
      for (var i = 0; i < 5000; i++) {
        p.addEntry('long_entry_with_data_$i' * 10);
      }
      p.dispose();
    });
  });

  // ============================================================
  // 边界条件
  // ============================================================
  group('ChangeNotifier - 边界条件', () {
    test('无listener时notifyListeners不报错', () {
      final provider = StressTestProvider();
      for (var i = 0; i < 1000; i++) {
        provider.increment();
      }
      expect(provider.counter, 1000);
      provider.dispose();
    });

    test('单个listener高频通知', () {
      final provider = StressTestProvider();
      var count = 0;
      provider.addListener(() => count++);
      for (var i = 0; i < 100000; i++) {
        provider.increment();
      }
      expect(count, 100000);
      provider.dispose();
    });

    test('listener中修改状态(非递归通知)', () {
      final provider = StressTestProvider();
      var outerCount = 0;
      provider.addListener(() {
        outerCount++;
        // 在listener中静默修改，不触发递归通知
        provider.silentIncrement();
      });
      provider.increment();
      expect(outerCount, 1);
      // counter = 1(increment) + 1(silentIncrement in listener)
      expect(provider.counter, 2);
      provider.dispose();
    });
  });
}
