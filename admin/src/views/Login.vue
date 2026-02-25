<!--
  @file Login.vue
  @brief Login 组件 - 光遇(Sky)梦幻风格
  Created by 林子怡
-->

<template>
  <div class="login-page">
    <div class="login-card">
      <div class="logo-area">
        <img
          src="@/assets/logo.svg"
          alt="HeartLake"
          class="logo-icon"
        >
        <h1 class="title">
          心湖管理后台
        </h1>
        <p class="subtitle">
          HeartLake Admin Console
        </p>
      </div>

      <el-form
        ref="formRef"
        :model="form"
        :rules="rules"
        class="login-form"
        aria-label="管理员登录"
      >
        <el-form-item prop="username">
          <el-input
            v-model="form.username"
            placeholder="用户名"
            size="large"
            aria-label="用户名"
          />
        </el-form-item>

        <el-form-item prop="password">
          <el-input
            v-model="form.password"
            type="password"
            placeholder="密码"
            size="large"
            show-password
            @keyup.enter="handleLogin"
          />
        </el-form-item>

        <el-button
          type="primary"
          size="large"
          class="login-btn"
          :loading="loading"
          @click="handleLogin"
        >
          {{ loading ? '登录中...' : '登 录' }}
        </el-button>
      </el-form>

      <div class="footer">
        &copy; 2026 HeartLake
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
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

<style scoped lang="scss">
/* ===== 页面容器 ===== */
.login-page {
  position: relative;
  width: 100%;
  height: 100vh;
  display: flex;
  align-items: center;
  justify-content: center;
  overflow: hidden;
  background: var(--m3-surface);
}

/* ===== 登录卡片 ===== */
.login-card {
  position: relative;
  z-index: 10;
  width: 420px;
  padding: 48px 40px;
  background: var(--m3-surface-container);
  border: 1px solid var(--m3-outline-variant);
  border-radius: var(--m3-shape-xl);
  box-shadow: var(--m3-elevation-3);
}

/* ===== Logo 区域 ===== */
.logo-area {
  text-align: center;
  margin-bottom: 40px;

  .logo-icon {
    width: 72px;
    height: 72px;
    margin-bottom: 20px;
  }

  .title {
    font-size: 28px;
    font-weight: 600;
    color: var(--m3-primary);
    margin: 0 0 8px 0;
    letter-spacing: 0.5px;
  }

  .subtitle {
    font-size: 13px;
    color: var(--m3-on-surface-variant);
    margin: 0;
    letter-spacing: 0.5px;
  }
}

/* ===== 表单 ===== */
.login-form {
  :deep(.el-form-item) {
    margin-bottom: 24px;

    .el-input__wrapper {
      background: var(--m3-surface);
      border: 1px solid var(--m3-outline);
      box-shadow: none;
      transition: all 0.3s ease;

      &:hover {
        border-color: var(--m3-on-surface);
        background: var(--m3-surface-container-highest);
      }

      &.is-focus {
        border-color: var(--m3-primary);
        background: var(--m3-surface-container-highest);
        box-shadow: 0 0 0 1px var(--m3-primary);
      }
    }

    .el-input__inner {
      color: var(--m3-on-surface);
      font-size: 15px;

      &::placeholder {
        color: var(--m3-outline);
      }
    }

    .el-input__suffix {
      .el-icon {
        color: var(--m3-on-surface-variant);
      }
    }
  }

  :deep(.el-form-item__error) {
    color: var(--m3-error);
  }

  .login-btn {
    width: 100%;
    height: 48px;
    margin-top: 12px;
    font-size: 16px;
    font-weight: 500;
    letter-spacing: 0.5px;
    border: none;
    border-radius: var(--m3-shape-large);
    background: var(--m3-primary);
    color: var(--m3-on-primary);
    transition: all 0.3s ease;

    &:hover,
    &:focus {
      background: var(--m3-primary);
      box-shadow: var(--m3-elevation-2);
      transform: translateY(-1px);
      opacity: 0.92;
    }

    &:active {
      transform: translateY(0);
      box-shadow: var(--m3-elevation-1);
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
  color: var(--m3-outline);
  letter-spacing: 0.5px;
}

/* ===== 响应式 ===== */
@media (max-width: 480px) {
  .login-card {
    width: calc(100% - 32px);
    padding: 36px 28px;
    border-radius: var(--m3-shape-large);
  }
}
</style>
