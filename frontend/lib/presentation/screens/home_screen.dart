// @file home_screen.dart
// @brief 主页框架 - 底部导航和页面切换
// Created by 林子怡

import 'package:flutter/material.dart';
import 'lake_screen.dart';
import 'discover_screen.dart';
import 'publish_screen.dart';
import 'friends_screen.dart';
import 'profile_screen.dart';
import '../widgets/sky_nav_bar.dart';

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
      const PublishScreen(),
      const FriendsScreen(),
      const ProfileScreen(),
    ];
  }

  void _onTabTapped(int index) {
    // 如果从投石页面切换到观湖页面，刷新列表
    if (_selectedIndex == 2 && index == 0) {
      _lakeScreenKey.currentState?.refreshStones();
    }

    setState(() {
      _selectedIndex = index;
    });
  }

  @override
  Widget build(BuildContext context) {
    final colorScheme = Theme.of(context).colorScheme;
    return Scaffold(
      backgroundColor: colorScheme.surfaceContainerHighest,
      body: AnimatedSwitcher(
        duration: const Duration(milliseconds: 300),
        transitionBuilder: (child, animation) => FadeTransition(opacity: animation, child: child),
        child: KeyedSubtree(key: ValueKey(_selectedIndex), child: _screens[_selectedIndex]),
      ),
      bottomNavigationBar: SkyNavBar(
        selectedIndex: _selectedIndex,
        onDestinationSelected: _onTabTapped,
      ),
    );
  }
}
