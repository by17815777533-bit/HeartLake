// @file home_screen.dart
// @brief 主页框架 - 底部导航和页面切换
// Created by 林子怡

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

  late final List<Widget> _screens;

  @override
  void initState() {
    super.initState();
    _screens = [
      LakeScreen(key: _lakeScreenKey),
      const DiscoverScreen(),
      PublishScreen(
        onPublished: () {
          if (!mounted) return;
          setState(() {
            _selectedIndex = 0;
          });
          _lakeScreenKey.currentState?.refreshStones();
        },
      ),
      const FriendsScreen(),
      const ProfileScreen(),
    ];
  }

  void _onTabTapped(int index) {
    // 切换到观湖页面时，刷新石头列表
    if (index == 0 && _selectedIndex != 0) {
      _lakeScreenKey.currentState?.refreshStones();
    }

    setState(() {
      _selectedIndex = index;
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: IndexedStack(
        index: _selectedIndex,
        children: _screens,
      ),
      bottomNavigationBar: NavigationBar(
        selectedIndex: _selectedIndex,
        onDestinationSelected: _onTabTapped,
        backgroundColor: Theme.of(context).brightness == Brightness.dark
            ? const Color(0xFF303134)
            : const Color(0xFFF5F9FC),
        indicatorColor: const Color(0xFF4285F4).withAlpha(30),
        destinations: const [
          NavigationDestination(icon: Icon(Icons.water_outlined), selectedIcon: Icon(Icons.water), label: '观湖'),
          NavigationDestination(icon: Icon(Icons.explore_outlined), selectedIcon: Icon(Icons.explore), label: '湖底'),
          NavigationDestination(icon: Icon(Icons.add_circle_outline), selectedIcon: Icon(Icons.add_circle), label: '投石'),
          NavigationDestination(icon: Icon(Icons.people_outline), selectedIcon: Icon(Icons.people), label: '好友'),
          NavigationDestination(icon: Icon(Icons.blur_on_outlined), selectedIcon: Icon(Icons.blur_on), label: '倒影'),
        ],
      ),
    );
  }
}
