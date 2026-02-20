<!--
  @file Login.vue
  @brief Login 组件
  Created by 林子怡
-->

<template>
  <div class="login-page">
    <div class="login-card">
      <h1>心湖管理后台</h1>

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
.login-page {
  width: 100%;
  height: 100vh;
  display: flex;
  align-items: center;
  justify-content: center;
  background: #f1f3f4;
}

.login-card {
  width: 400px;
  padding: 48px 40px;
  background: white;
  border-radius: 8px;
  box-shadow: 0 1px 3px rgba(60, 64, 67, 0.15);

  h1 {
    text-align: center;
    font-size: 24px;
    font-weight: 400;
    color: #202124;
    margin: 0 0 32px;
  }

  .login-form {
    :deep(.el-input__wrapper) {
      border-radius: 4px;
      box-shadow: inset 0 0 0 1px #dadce0;
      &:hover { box-shadow: inset 0 0 0 1px #202124; }
      &.is-focus { box-shadow: inset 0 0 0 2px #1A73E8; }
    }

    .login-btn {
      width: 100%;
      margin-top: 16px;
      background: #1A73E8;
      border-color: #1A73E8;
      border-radius: 4px;
      height: 40px;
      font-weight: 500;

      &:hover {
        background: #1557b0;
        border-color: #1557b0;
      }
    }
  }

  .footer {
    text-align: center;
    margin-top: 24px;
    font-size: 12px;
    color: #5f6368;
  }
}
</style>
