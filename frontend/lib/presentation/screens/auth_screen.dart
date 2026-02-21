// @file auth_screen.dart
// @brief 登录注册界面
// Created by 林子怡

import 'dart:async';
import 'package:flutter/material.dart';
import 'package:heart_lake/data/datasources/auth_service.dart';
import 'package:heart_lake/presentation/widgets/sky_scaffold.dart';
import 'package:heart_lake/presentation/widgets/sky_input.dart';
import 'package:heart_lake/presentation/widgets/sky_button.dart';
import 'package:heart_lake/presentation/widgets/sky_glass_card.dart';
import 'package:heart_lake/utils/app_theme.dart';
import 'home_screen.dart';

class AuthScreen extends StatefulWidget {
  const AuthScreen({super.key});

  @override
  State<AuthScreen> createState() => _AuthScreenState();
}

class _AuthScreenState extends State<AuthScreen>
    with SingleTickerProviderStateMixin {
  final AuthService _authService = AuthService();
  late TabController _tabController;

  // 登录表单
  final _loginUsernameController = TextEditingController();
  final _loginPasswordController = TextEditingController();

  // 注册表单
  final _registerUsernameController = TextEditingController();
  final _registerPasswordController = TextEditingController();
  final _registerConfirmPasswordController = TextEditingController();
  final _registerNicknameController = TextEditingController();
  final _registerVerificationCodeController = TextEditingController();
  final _registerEmailController = TextEditingController();

  bool _isLoading = false;
  bool _obscurePassword = true;
  bool _obscureConfirmPassword = true;
  int _countdown = 0;
  bool _useEmailLogin = false; // 切换登录方式
  // P2-6 修复：使用 Timer.periodic 替代 for 循环 + Future.delayed
  Timer? _countdownTimer;

  @override
  void initState() {
    super.initState();
    _tabController = TabController(length: 2, vsync: this);
  }

  @override
  void dispose() {
    // P2-6: 取消倒计时定时器
    _countdownTimer?.cancel();
    _tabController.dispose();
    _loginUsernameController.dispose();
    _loginPasswordController.dispose();
    _registerUsernameController.dispose();
    _registerPasswordController.dispose();
    _registerConfirmPasswordController.dispose();
    _registerNicknameController.dispose();
    _registerVerificationCodeController.dispose();
    _registerEmailController.dispose();
    super.dispose();
  }

  /// P2-6: 启动验证码倒计时
  void _startCountdown() {
    _countdownTimer?.cancel();
    setState(() => _countdown = 60);
    _countdownTimer = Timer.periodic(const Duration(seconds: 1), (timer) {
      if (!mounted) {
        timer.cancel();
        return;
      }
      setState(() {
        _countdown--;
        if (_countdown <= 0) {
          timer.cancel();
        }
      });
    });
  }

  Future<void> _handleLogin() async {
    if (_useEmailLogin) {
      // 邮箱登录
      final email = _loginUsernameController.text.trim();
      final password = _loginPasswordController.text;

      // 严格验证邮箱格式
      final emailRegex = RegExp(r'^[\w-\.]+@([\w-]+\.)+[\w-]{2,4}$');
      if (email.isEmpty || !emailRegex.hasMatch(email)) {
        _showSnackBar('请输入有效的邮箱地址');
        return;
      }
      if (password.isEmpty) {
        _showSnackBar('请填写密码');
        return;
      }

      setState(() => _isLoading = true);

      final result = await _authService.login(email: email, password: password);

      setState(() => _isLoading = false);

      if (result['success']) {
        _navigateToHome();
      } else {
        _showSnackBar(result['message'] ?? '登录失败');
      }
    } else {
      // 用户名登录
      final username = _loginUsernameController.text.trim();
      final password = _loginPasswordController.text;

      if (username.isEmpty) {
        _showSnackBar('请填写用户名');
        return;
      }
      if (password.isEmpty) {
        _showSnackBar('请填写密码');
        return;
      }

      setState(() => _isLoading = true);

      final result = await _authService.loginWithUsername(
        username: username,
        password: password,
      );

      setState(() => _isLoading = false);

      if (result['success']) {
        _navigateToHome();
      } else {
        _showSnackBar(result['message'] ?? '登录失败');
      }
    }
  }

  Future<void> _handleRegister() async {
    final password = _registerPasswordController.text;
    final confirmPassword = _registerConfirmPasswordController.text;
    final nickname = _registerNicknameController.text.trim();
    final verificationCode = _registerVerificationCodeController.text.trim();
    final email = _registerEmailController.text.trim();

    // 严格验证邮箱格式
    final emailRegex = RegExp(r'^[\w-\.]+@([\w-]+\.)+[\w-]{2,4}$');
    if (email.isEmpty || !emailRegex.hasMatch(email)) {
      _showSnackBar('请输入有效的邮箱地址');
      return;
    }

    // 密码强度验证
    if (password.length < 6) {
      _showSnackBar('密码至少6个字符');
      return;
    }

    if (password != confirmPassword) {
      _showSnackBar('两次密码输入不一致');
      return;
    }

    if (verificationCode.isEmpty) {
      _showSnackBar('请输入验证码');
      return;
    }

    setState(() => _isLoading = true);

    // 邮箱注册
    final result = await _authService.registerWithEmail(
      email: email,
      password: password,
      verificationCode: verificationCode,
      nickname: nickname.isEmpty ? null : nickname,
    );

    setState(() => _isLoading = false);

    if (result['success']) {
      _showSnackBar('注册成功！你的账号是: ${result['username']}');
      _navigateToHome();
    } else {
      _showSnackBar(result['message'] ?? '注册失败');
    }
  }

  Future<void> _handleAnonymousLogin() async {
    setState(() => _isLoading = true);

    final result = await _authService.anonymousLogin();

    setState(() => _isLoading = false);

    if (result['success']) {
      _navigateToHome();
    } else {
      _showSnackBar(result['message'] ?? '匿名登录失败');
    }
  }

  void _navigateToHome() {
    Navigator.of(context).pushReplacement(
      MaterialPageRoute(builder: (context) => const HomeScreen()),
    );
  }

  void _showSnackBar(String message) {
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text(message), backgroundColor: AppTheme.warmOrange),
    );
  }

  @override
  Widget build(BuildContext context) {
    return SkyScaffold(
      showParticles: true,
      showWater: true,
      body: Stack(
        children: [
          SafeArea(
            child: SingleChildScrollView(
              padding: const EdgeInsets.all(24),
              child: Column(
                children: [
                  const SizedBox(height: 40),
                  // Logo - 暖色光晕
                  Container(
                    width: 100,
                    height: 100,
                    decoration: BoxDecoration(
                      color: AppTheme.warmOrange.withValues(alpha: 0.15),
                      shape: BoxShape.circle,
                      border: Border.all(
                          color: Colors.white.withValues(alpha: 0.5), width: 2),
                      boxShadow: [
                        BoxShadow(
                          color: AppTheme.candleGlow.withValues(alpha: 0.3),
                          blurRadius: 20,
                          spreadRadius: 2,
                        ),
                      ],
                    ),
                    child: const Icon(Icons.water_drop,
                        size: 50, color: Colors.white),
                  ),
                  const SizedBox(height: 16),
                  const Text(
                    '心湖',
                    style: TextStyle(
                      fontSize: 36,
                      fontWeight: FontWeight.bold,
                      color: Colors.white,
                      letterSpacing: 4,
                    ),
                  ),
                  const SizedBox(height: 8),
                  Text(
                    '把石头投入湖中',
                    style: TextStyle(
                      fontSize: 14,
                      color: Colors.white.withValues(alpha: 0.8),
                    ),
                  ),
                  const SizedBox(height: 40),

                  // 表单卡片 - 毛玻璃
                  SkyGlassCard(
                    enableGlow: false,
                    borderRadius: 24,
                    padding: EdgeInsets.zero,
                    child: Column(
                      children: [
                        // Tab 切换
                        TabBar(
                          controller: _tabController,
                          indicatorColor: AppTheme.warmOrange,
                          labelColor: AppTheme.warmOrange,
                          unselectedLabelColor: AppTheme.textSecondary,
                          tabs: const [
                            Tab(text: '登录'),
                            Tab(text: '注册'),
                          ],
                        ),
                        ConstrainedBox(
                          constraints: const BoxConstraints(minHeight: 380, maxHeight: 420),
                          child: TabBarView(
                            controller: _tabController,
                            children: [
                              _buildLoginForm(),
                              _buildRegisterForm(),
                            ],
                          ),
                        ),
                      ],
                    ),
                  ),

                  const SizedBox(height: 24),

                  // 匿名登录按钮
                  TextButton.icon(
                    onPressed: _isLoading ? null : _handleAnonymousLogin,
                    icon:
                        const Icon(Icons.visibility_off, color: Colors.white70),
                    label: const Text(
                      '匿名漫游心湖',
                      style: TextStyle(color: Colors.white70, fontSize: 16),
                    ),
                  ),

                  const SizedBox(height: 16),
                  Text(
                    '匿名用户数据将在24小时后消失',
                    style: TextStyle(
                      fontSize: 12,
                      color: Colors.white.withValues(alpha: 0.5),
                    ),
                  ),
                ],
              ),
            ),
          ),

          // Loading 遮罩
          if (_isLoading)
            Container(
              color: Colors.black.withValues(alpha: 0.3),
              child: const Center(
                child: CircularProgressIndicator(color: Colors.white),
              ),
            ),
        ],
      ),
    );
  }

  Widget _buildLoginForm() {
    return Padding(
      padding: const EdgeInsets.all(20),
      child: Column(
        children: [
          // 登录方式切换
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              ChoiceChip(
                label: const Text('用户名登录'),
                selected: !_useEmailLogin,
                onSelected: (selected) {
                  if (selected) setState(() => _useEmailLogin = false);
                },
                selectedColor: AppTheme.warmOrange.withValues(alpha: 0.3),
              ),
              const SizedBox(width: 12),
              ChoiceChip(
                label: const Text('邮箱登录'),
                selected: _useEmailLogin,
                onSelected: (selected) {
                  if (selected) setState(() => _useEmailLogin = true);
                },
                selectedColor: AppTheme.warmOrange.withValues(alpha: 0.3),
              ),
            ],
          ),
          const SizedBox(height: 16),
          SkyInput(
            controller: _loginUsernameController,
            labelText: _useEmailLogin ? '邮箱地址' : '用户名 (8位数字)',
            hintText: _useEmailLogin ? '请输入有效的邮箱地址' : '请输入8位数字用户名',
            prefixIcon: Icon(
                _useEmailLogin ? Icons.email_outlined : Icons.person_outline),
            keyboardType: _useEmailLogin
                ? TextInputType.emailAddress
                : TextInputType.number,
          ),
          const SizedBox(height: 16),
          SkyInput(
            controller: _loginPasswordController,
            obscureText: _obscurePassword,
            labelText: '密码',
            prefixIcon: const Icon(Icons.lock_outline),
            suffixIcon: IconButton(
              icon: Icon(
                  _obscurePassword ? Icons.visibility_off : Icons.visibility),
              onPressed: () =>
                  setState(() => _obscurePassword = !_obscurePassword),
            ),
          ),
          const SizedBox(height: 24),
          SkyButton(
            label: '登录',
            onPressed: _isLoading ? null : _handleLogin,
            isLoading: _isLoading,
            width: double.infinity,
          ),
        ],
      ),
    );
  }

  Widget _buildRegisterForm() {
    return Padding(
      padding: const EdgeInsets.all(20),
      child: SingleChildScrollView(
        child: Column(
          children: [
            // 注册模式提示
            const Text(
              '请使用邮箱注册账号',
              style: TextStyle(
                fontSize: 14,
                color: AppTheme.textSecondary,
              ),
            ),
            const SizedBox(height: 16),

            // 邮箱输入框（必填）
            SkyInput(
              controller: _registerEmailController,
              labelText: '邮箱地址 *',
              hintText: '请输入有效的邮箱地址',
              prefixIcon: const Icon(Icons.email_outlined),
              keyboardType: TextInputType.emailAddress,
            ),
            const SizedBox(height: 12),

            SkyInput(
              controller: _registerNicknameController,
              labelText: '昵称 (选填)',
              prefixIcon: const Icon(Icons.badge_outlined),
            ),
            const SizedBox(height: 12),
            Row(
              children: [
                Expanded(
                  flex: 2,
                  child: SkyInput(
                    controller: _registerVerificationCodeController,
                    labelText: '验证码',
                    prefixIcon: const Icon(Icons.verified_outlined),
                    keyboardType: TextInputType.number,
                    maxLength: 6,
                  ),
                ),
                const SizedBox(width: 8),
                Expanded(
                  flex: 1,
                  child: ElevatedButton(
                    onPressed: _countdown > 0
                        ? null
                        : () async {
                            // 邮箱验证码
                            final email = _registerEmailController.text.trim();

                            // 严格验证邮箱格式
                            final emailRegex =
                                RegExp(r'^[\w-\.]+@([\w-]+\.)+[\w-]{2,4}$');
                            if (email.isEmpty || !emailRegex.hasMatch(email)) {
                              _showSnackBar('请输入有效的邮箱地址');
                              return;
                            }

                            try {
                              final result = await _authService
                                  .sendEmailVerificationCode(email);
                              if (result['success']) {
                                _showSnackBar('验证码已发送到您的邮箱');
                                // P2-6: 使用 Timer.periodic 倒计时
                                _startCountdown();
                              } else {
                                _showSnackBar(result['message'] ?? '发送失败');
                                return;
                              }
                            } catch (e) {
                              _showSnackBar('发送验证码失败: $e');
                              return;
                            }
                          },
                    style: ElevatedButton.styleFrom(
                      backgroundColor: AppTheme.warmOrange,
                      foregroundColor: Colors.white,
                      padding: const EdgeInsets.symmetric(vertical: 14),
                      shape: RoundedRectangleBorder(
                          borderRadius: BorderRadius.circular(12)),
                    ),
                    child: Text(
                      _countdown > 0 ? '$_countdown秒' : '发送',
                      style: const TextStyle(fontSize: 13),
                    ),
                  ),
                ),
              ],
            ),
            const SizedBox(height: 12),
            SkyInput(
              controller: _registerPasswordController,
              obscureText: _obscurePassword,
              labelText: '密码 (至少6个字符)',
              prefixIcon: const Icon(Icons.lock_outline),
              suffixIcon: IconButton(
                icon: Icon(_obscurePassword
                    ? Icons.visibility_off
                    : Icons.visibility),
                onPressed: () =>
                    setState(() => _obscurePassword = !_obscurePassword),
              ),
            ),
            const SizedBox(height: 12),
            SkyInput(
              controller: _registerConfirmPasswordController,
              obscureText: _obscureConfirmPassword,
              labelText: '确认密码',
              prefixIcon: const Icon(Icons.lock_outline),
              suffixIcon: IconButton(
                icon: Icon(_obscureConfirmPassword
                    ? Icons.visibility_off
                    : Icons.visibility),
                onPressed: () => setState(
                    () => _obscureConfirmPassword = !_obscureConfirmPassword),
              ),
            ),
            const SizedBox(height: 16),
            SkyButton(
              label: '注册',
              onPressed: _isLoading ? null : _handleRegister,
              isLoading: _isLoading,
              width: double.infinity,
            ),
          ],
        ),
      ),
    );
  }
}
