<!--
  @file Login.vue
  @brief Login 组件 - 光遇(Sky)梦幻风格
  Created by 林子怡
-->

<template>
  <div class="login-page">
    <!-- 星星层 -->
    <div class="stars"></div>
    <div class="stars stars--secondary"></div>

    <div class="login-card">
      <div class="logo-area">
        <img src="@/assets/logo.svg" alt="HeartLake" class="logo-icon" />
        <h1 class="title">心湖管理后台</h1>
        <p class="subtitle">Sky Admin Console</p>
      </div>

      <el-form ref="formRef" :model="form" :rules="rules" class="login-form">
        <el-form-item prop="username">
          <el-input v-model="form.username" placeholder="用户名" size="large" />
        </el-form-item>

        <el-form-item prop="password">
          <el-input v-model="form.password" type="password" placeholder="密码" size="large" show-password @keyup.enter="handleLogin" />
        </el-form-item>

        <el-button type="primary" size="large" class="login-btn" :loading="loading" @click="handleLogin">
          {{ loading ? '登录中...' : '登 录' }}
        </el-button>
      </el-form>

      <div class="footer">&copy; 2026 HeartLake</div>
    </div>
  </div>
</template>

<script setup>
import { ref, reactive } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import api from '@/api'
import { useAppStore } from '@/stores'
import { getErrorMessage } from '@/utils/errorHelper'

const router = useRouter()
const appStore = useAppStore()
const formRef = ref(null)
const loading = ref(false)

const form = reactive({
  username: '',
  password: '',
})

const rules = {
  username: [{ required: true, message: '请输入用户名', trigger: 'blur' }],
  password: [{ required: true, message: '请输入密码', trigger: 'blur' }],
}

const handleLogin = async () => {
  const valid = await formRef.value?.validate().catch(() => false)
  if (!valid) return

  loading.value = true
  try {
    const res = await api.login(form)
    const resData = res?.data?.data || res?.data || {}
    const token = resData.token || res?.token
    if (!token) {
      throw new Error('登录返回缺少token')
    }
    appStore.setToken(token)
    // PASETO token 不可客户端解码，直接存储登录响应中的用户信息
    const user = resData.user || resData.admin || { username: form.username }
    appStore.setUserInfo(user)
    ElMessage.success('登录成功')
    router.push('/dashboard')
  } catch (e) {
    console.error('登录失败:', e)
    // H-3: 使用统一错误处理，登录失败给出友好提示
    ElMessage.error(getErrorMessage(e, '登录失败，请检查用户名和密码'))
  } finally {
    loading.value = false
  }
}
</script>

<style lang="scss" scoped>
/* ===== 星星生成函数 ===== */
@function generate-stars($count) {
  $result: '';
  @for $i from 1 through $count {
    $x: random(2000);
    $y: random(2000);
    @if $i == 1 {
      $result: '#{$x}px #{$y}px #fff';
    } @else {
      $result: '#{$result}, #{$x}px #{$y}px #fff';
    }
  }
  @return unquote($result);
}

/* ===== 动画 ===== */
@keyframes fadeInUp {
  from {
    opacity: 0;
    transform: translateY(40px);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}

@keyframes twinkle {
  0%, 100% { opacity: 0.3; }
  50% { opacity: 1; }
}

@keyframes twinkle-alt {
  0%, 100% { opacity: 0.6; }
  50% { opacity: 0.2; }
}

@keyframes glow-pulse {
  0%, 100% { box-shadow: 0 0 20px rgba(242, 204, 143, 0.15); }
  50% { box-shadow: 0 0 40px rgba(242, 204, 143, 0.25); }
}

/* ===== 页面背景 ===== */
.login-page {
  width: 100%;
  height: 100vh;
  display: flex;
  align-items: center;
  justify-content: center;
  background: linear-gradient(180deg, #0A0A1A 0%, #2D1B69 40%, #E07A5F 75%, #F2CC8F 100%);
  position: relative;
  overflow: hidden;
}

/* ===== 星星层 ===== */
.stars {
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  pointer-events: none;
  z-index: 0;

  &::before,
  &::after {
    content: '';
    position: absolute;
    top: 0;
    left: 0;
    border-radius: 50%;
  }

  &::before {
    width: 1px;
    height: 1px;
    box-shadow: generate-stars(120);
    animation: twinkle 4s ease-in-out infinite;
  }

  &::after {
    width: 2px;
    height: 2px;
    box-shadow: generate-stars(60);
    animation: twinkle 6s ease-in-out infinite 1s;
  }
}

.stars--secondary {
  &::before {
    width: 1.5px;
    height: 1.5px;
    box-shadow: generate-stars(80);
    animation: twinkle-alt 5s ease-in-out infinite 0.5s;
  }

  &::after {
    width: 1px;
    height: 1px;
    box-shadow: generate-stars(100);
    animation: twinkle-alt 7s ease-in-out infinite 2s;
  }
}

/* ===== 登录卡片 ===== */
.login-card {
  width: 420px;
  padding: 48px;
  background: rgba(20, 20, 50, 0.7);
  backdrop-filter: blur(24px);
  -webkit-backdrop-filter: blur(24px);
  border: 1px solid rgba(242, 204, 143, 0.15);
  border-radius: 24px;
  box-shadow:
    0 8px 32px rgba(0, 0, 0, 0.5),
    0 0 60px rgba(242, 204, 143, 0.08);
  position: relative;
  z-index: 1;
  animation: fadeInUp 0.8s ease-out, glow-pulse 4s ease-in-out infinite 0.8s;
}

/* ===== Logo 区域 ===== */
.logo-area {
  text-align: center;
  margin-bottom: 36px;
}

.logo-icon {
  width: 56px;
  height: 56px;
  margin-bottom: 16px;
  filter: drop-shadow(0 0 12px rgba(242, 204, 143, 0.4));
}

.title {
  font-size: 26px;
  font-weight: 600;
  letter-spacing: 2px;
  margin: 0 0 8px;
  background: linear-gradient(135deg, #F2CC8F 0%, #E8A87C 50%, #F2CC8F 100%);
  background-clip: text;
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
}

.subtitle {
  font-size: 13px;
  color: rgba(242, 204, 143, 0.5);
  letter-spacing: 3px;
  margin: 0;
  font-weight: 300;
}

/* ===== 表单 ===== */
.login-form {
  :deep(.el-form-item) {
    margin-bottom: 20px;
  }

  :deep(.el-input__wrapper) {
    background: rgba(10, 10, 26, 0.5);
    border-radius: 12px;
    box-shadow: none;
    border: 1px solid rgba(242, 204, 143, 0.15);
    padding: 4px 16px;
    transition: all 0.3s ease;

    &:hover {
      border-color: rgba(242, 204, 143, 0.3);
    }

    &.is-focus {
      border-color: rgba(242, 204, 143, 0.5);
      box-shadow: 0 0 16px rgba(242, 204, 143, 0.12);
    }
  }

  :deep(.el-input__inner) {
    color: #F0E6D3;
    font-size: 15px;
    height: 40px;

    &::placeholder {
      color: #7A6F63;
    }
  }

  // 密码眼睛图标
  :deep(.el-input__suffix) {
    color: #7A6F63;

    .el-input__icon {
      color: #7A6F63;
    }
  }

  // 表单验证错误信息
  :deep(.el-form-item__error) {
    color: #E07A5F;
  }

  .login-btn {
    width: 100%;
    margin-top: 8px;
    height: 48px;
    border: none;
    border-radius: 12px;
    font-size: 16px;
    font-weight: 600;
    letter-spacing: 4px;
    color: #1A0A00;
    background: linear-gradient(135deg, #F2CC8F 0%, #E8A87C 50%, #E07A5F 100%);
    transition: all 0.3s ease;

    &:hover,
    &:focus {
      background: linear-gradient(135deg, #F5D9A8 0%, #EDBA93 50%, #E8907A 100%);
      box-shadow: 0 4px 24px rgba(242, 204, 143, 0.35);
      transform: translateY(-1px);
    }

    &:active {
      transform: translateY(0);
      box-shadow: 0 2px 12px rgba(242, 204, 143, 0.25);
    }

    &.is-loading {
      opacity: 0.85;
    }
  }
}

/* ===== 底部 ===== */
.footer {
  text-align: center;
  margin-top: 28px;
  font-size: 12px;
  color: #7A6F63;
  letter-spacing: 1px;
}

/* ===== 响应式 ===== */
@media (max-width: 480px) {
  .login-card {
    width: calc(100% - 32px);
    padding: 36px 28px;
    border-radius: 20px;
  }
}
</style>
