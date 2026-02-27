// 主页框架 - 底部导航和页面切换

import 'package:flutter/material.dart';
import 'lake_screen.dart';
import 'discover_screen.dart';
import 'publish_screen.dart';
import 'friends_screen.dart';
import 'profile_screen.dart';

class HomeScreen extends StatefulWidget {
  const HomeScreen({super.key});

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  int _selectedIndex = 0;
  final GlobalKey<LakeScreenState> _lakeScreenKey =
      GlobalKey<LakeScreenState>();

  // 记录哪些 Tab 曾经被访问过，按需创建页面
  final Set<int> _initializedTabs = {0};
  // 缓存已创建的 Tab 页面，避免重复创建
  final Map<int, Widget> _cachedTabs = {};

  void _onTabTapped(int index) {
    // 切换到观湖页面时，刷新石头列表
    if (index == 0 && _selectedIndex != 0) {
      _lakeScreenKey.currentState?.refreshStones();
    }

    setState(() {
      _selectedIndex = index;
      _initializedTabs.add(index);
    });
  }

  Widget _buildTab(int index) {
    return _cachedTabs.putIfAbsent(index, () {
      switch (index) {
        case 0:
          return LakeScreen(key: _lakeScreenKey);
        case 1:
          return const DiscoverScreen();
        case 2:
          return PublishScreen(
            onPublished: () {
              if (!mounted) return;
              setState(() {
                _selectedIndex = 0;
              });
              _lakeScreenKey.currentState?.refreshStones();
            },
          );
        case 3:
          return const FriendsScreen();
        case 4:
          return const ProfileScreen();
        default:
          return const SizedBox.shrink();
      }
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: IndexedStack(
        index: _selectedIndex,
        children: List.generate(5, (index) {
          if (_initializedTabs.contains(index)) {
            return _buildTab(index);
          }
          return const SizedBox.shrink();
        }),
      ),
      bottomNavigationBar: NavigationBar(
        selectedIndex: _selectedIndex,
        onDestinationSelected: _onTabTapped,
        backgroundColor: Theme.of(context).brightness == Brightness.dark
            ? const Color(0xFF303134)
            : const Color(0xFFF5F9FC),
        indicatorColor: const Color(0xFF4285F4).withAlpha(30),
        destinations: const [
          NavigationDestination(
              icon: Icon(Icons.water_outlined),
              selectedIcon: Icon(Icons.water),
              label: '观湖'),
          NavigationDestination(
              icon: Icon(Icons.explore_outlined),
              selectedIcon: Icon(Icons.explore),
              label: '共鸣'),
          NavigationDestination(
              icon: Icon(Icons.add_circle_outline),
              selectedIcon: Icon(Icons.add_circle),
              label: '投石'),
          NavigationDestination(
              icon: Icon(Icons.people_outline),
              selectedIcon: Icon(Icons.people),
              label: '好友'),
          NavigationDestination(
              icon: Icon(Icons.blur_on_outlined),
              selectedIcon: Icon(Icons.blur_on),
              label: '倒影'),
        ],
      ),
    );
  }
}
