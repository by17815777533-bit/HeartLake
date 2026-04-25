<!--
  管理员登录页面

  安全机制：
  - 连续登录失败 3 次后启动 30 秒客户端冷却，配合后端 429 速率限制双重防护
  - 非 HTTPS 环境弹出安全警告
  - PASETO token 不可客户端解码，用户信息从登录响应中提取并存入 sessionStorage
  - 密码字段支持 Enter 键提交，提升操作效率
-->

<template>
  <div class="login-page">
    <div class="login-shell">
      <section class="login-intro">
        <span class="intro-kicker">心湖 / 后台入口</span>
        <h1 class="intro-title">面向值守与处置的后台入口</h1>
        <p class="intro-copy">
          这里不是展示页，而是处理旅人内容、求助工单、实时告警和社区秩序的工作台。
        </p>

        <div class="intro-grid">
          <article class="intro-panel">
            <span class="panel-index">01</span>
            <strong>实时巡检</strong>
            <p>在线人数、投石动态与异常反馈统一归档。</p>
          </article>
          <article class="intro-panel">
            <span class="panel-index">02</span>
            <strong>内容处置</strong>
            <p>举报、风险词与审核结果串成可追溯流程。</p>
          </article>
          <article class="intro-panel">
            <span class="panel-index">03</span>
            <strong>操作留痕</strong>
            <p>后台动作可回放，可核查，也方便交接班。</p>
          </article>
        </div>

        <div class="intro-note">
          <span>内部入口</span>
          <span>分级权限</span>
          <span>操作留痕</span>
        </div>
      </section>

      <section class="login-card">
        <div class="logo-area">
          <img src="@/assets/logo.svg" alt="HeartLake" class="logo-icon" />
          <h1 class="title">心湖管理后台</h1>
          <p class="subtitle">值守与处置工作台</p>
        </div>

        <div class="trust-strip">
          <span>分级权限</span>
          <span>操作留痕</span>
          <span>实时看板</span>
        </div>

        <el-alert
          v-if="connectionWarning"
          class="login-alert"
          type="warning"
          :closable="false"
          show-icon
          :title="connectionWarning"
        />

        <el-alert
          v-if="loginError"
          class="login-alert"
          type="error"
          :closable="false"
          show-icon
          :title="loginError"
        />

        <el-alert
          v-if="cooldownSeconds > 0"
          class="login-alert"
          type="warning"
          :closable="false"
          show-icon
          :title="`连续登录失败过多，请 ${cooldownSeconds} 秒后再试`"
        />

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
              aria-label="密码"
              @keyup.enter="handleLogin"
            />
          </el-form-item>

          <el-button
            type="primary"
            size="large"
            class="login-btn"
            :loading="loading"
            aria-label="登录"
            @click="handleLogin"
          >
            {{ loading ? '登录中...' : '登 录' }}
          </el-button>
        </el-form>

        <p class="security-note">建议仅在可信网络中登录，敏感操作请在完成后主动退出会话。</p>

        <div class="footer">&copy; 2026 HeartLake</div>
      </section>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import api from '@/api'
import { useAppStore } from '@/stores'
import { getErrorMessage } from '@/utils/errorHelper'
import { normalizePayloadRecord } from '@/utils/collectionPayload'
import type { FormInstance } from 'element-plus'

const router = useRouter()
const appStore = useAppStore()
const formRef = ref<FormInstance | null>(null)
const loading = ref(false)
const loginError = ref('')
const connectionWarning = ref('')

const form = reactive({
  username: '',
  password: '',
})

const rules = {
  username: [{ required: true, message: '请输入用户名', trigger: 'blur' }],
  password: [{ required: true, message: '请输入密码', trigger: 'blur' }],
}

// 连续登录失败计数与冷却倒计时（防暴力破解的客户端侧限制）
const failCount = ref(0)
const cooldownSeconds = ref(0)
let cooldownTimer: ReturnType<typeof setInterval> | null = null

/** 启动冷却倒计时，每秒递减直到归零 */
function startCooldown(seconds: number) {
  cooldownSeconds.value = seconds
  cooldownTimer = setInterval(() => {
    cooldownSeconds.value--
    if (cooldownSeconds.value <= 0) {
      clearInterval(cooldownTimer!)
      cooldownTimer = null
    }
  }, 1000)
}

const handleLogin = async () => {
  if (cooldownSeconds.value > 0) {
    loginError.value = `操作过于频繁，请 ${cooldownSeconds.value} 秒后重试`
    return
  }

  loginError.value = ''
  const formInstance = formRef.value
  if (!formInstance) return
  const valid = await formInstance.validate().then(
    () => true,
    () => false,
  )
  if (!valid) return

  loading.value = true
  try {
    const res = await api.login(form)
    const resData = normalizePayloadRecord(res?.data)
    const token = resData.token || res?.token
    if (!token) {
      throw new Error('登录返回缺少token')
    }
    const user = resData.user || resData.admin
    if (!user || typeof user !== 'object') {
      throw new Error('登录返回缺少管理员信息')
    }
    appStore.setToken(token)
    // PASETO token 不可客户端解码，直接存储登录响应中的用户信息
    appStore.setUserInfo(user)
    failCount.value = 0
    loginError.value = ''
    ElMessage.success('登录成功')
    router.push('/dashboard')
  } catch (e) {
    failCount.value++
    const message = getErrorMessage(e, '登录失败，请检查用户名和密码')
    // 连续失败3次后启动30秒冷却，配合后端429速率限制
    if (failCount.value >= 3 && cooldownSeconds.value <= 0) {
      startCooldown(30)
      loginError.value = '连续登录失败多次，请 30 秒后重试'
    } else {
      loginError.value = message
    }
  } finally {
    loading.value = false
  }
}

// 非 HTTPS 环境安全警告
onMounted(() => {
  if (
    location.protocol !== 'https:' &&
    location.hostname !== 'localhost' &&
    location.hostname !== '127.0.0.1'
  ) {
    connectionWarning.value =
      '当前不是 HTTPS 连接，登录凭证传输存在风险，请尽快切换到受信任的加密入口。'
  }
})
</script>

<style scoped lang="scss">
.login-page {
  width: 100%;
  min-height: 100vh;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 24px;
  background: #eef2f7;
}

.login-shell {
  width: min(1060px, 100%);
  display: grid;
  grid-template-columns: minmax(0, 1fr) 380px;
  gap: 12px;
  align-items: stretch;
  padding: 12px;
  border: 1px solid #d8dee8;
  border-radius: 8px;
  background: #ffffff;
  box-shadow: 0 10px 24px rgba(15, 23, 42, 0.08);
}

.login-intro {
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  min-height: 520px;
  padding: 24px;
  border: 1px solid #d8dee8;
  border-radius: 6px;
  background: #f8fafc;
}

.intro-kicker {
  display: inline-flex;
  width: fit-content;
  min-height: 22px;
  align-items: center;
  padding: 0 8px;
  border-radius: 4px;
  background: #dbeafe;
  color: #1d4ed8;
  font-family: var(--hl-font-mono);
  font-size: 11px;
  letter-spacing: 0;
}

.intro-title {
  margin: 18px 0 10px;
  font-family: var(--hl-font-display);
  max-width: 14ch;
  font-size: 32px;
  line-height: 1.14;
  color: var(--hl-ink);
}

.intro-copy {
  max-width: 34rem;
  margin: 0 0 24px;
  font-size: 14px;
  line-height: 1.7;
  color: var(--hl-ink-soft);
}

.intro-grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 10px;
}

.intro-panel {
  min-height: 132px;
  padding: 14px;
  border-radius: 6px;
  border: 1px solid #d8dee8;
  background: #ffffff;
  border-top: 3px solid #2563eb;

  strong {
    display: block;
    margin-top: 14px;
    font-size: 14px;
    color: var(--hl-ink);
  }

  p {
    margin: 6px 0 0;
    font-size: 12px;
    line-height: 1.55;
    color: var(--hl-ink-soft);
  }
}

.intro-panel:nth-child(2) {
  border-top-color: #0f766e;
}

.intro-panel:nth-child(3) {
  border-top-color: #d97706;
}

.panel-index {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 34px;
  height: 22px;
  border-radius: 4px;
  background: #f1f5f9;
  color: #475569;
  font-family: var(--hl-font-mono);
  font-size: 11px;
  letter-spacing: 0;
}

.intro-note {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  margin-top: 18px;

  span {
    display: inline-flex;
    align-items: center;
    height: 26px;
    padding: 0 10px;
    border-radius: 4px;
    border: 1px solid #d8dee8;
    background: #ffffff;
    font-size: 11px;
    color: var(--hl-ink-soft);
  }
}

.login-card {
  width: 100%;
  padding: 28px;
  border-radius: 6px;
  border: 1px solid #d8dee8;
  background: #ffffff;
}

.logo-area {
  text-align: left;
  margin-bottom: 20px;

  .logo-icon {
    width: 44px;
    height: 44px;
    margin-bottom: 14px;
    padding: 10px;
    border-radius: 6px;
    border: 1px solid #dbeafe;
    background: #eff6ff;
  }

  .title {
    margin: 0 0 6px;
    font-family: var(--hl-font-display);
    font-size: 24px;
    font-weight: 760;
    color: var(--hl-ink);
    letter-spacing: 0;
  }

  .subtitle {
    font-family: var(--hl-font-mono);
    font-size: 11px;
    color: var(--hl-ink-soft);
    margin: 0;
    letter-spacing: 0;
  }
}

.trust-strip {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  margin-bottom: 20px;

  span {
    display: inline-flex;
    align-items: center;
    height: 24px;
    padding: 0 8px;
    border-radius: 4px;
    background: #f1f5f9;
    color: var(--hl-ink);
    font-size: 12px;
    font-weight: 600;
  }
}

.login-alert {
  margin-bottom: 10px;
}

.login-form {
  :deep(.el-form-item) {
    margin-bottom: 14px;

    .el-input__wrapper {
      min-height: 42px;
      background: #ffffff;
      border: 1px solid #d8dee8;
      box-shadow: none !important;
      border-radius: 6px;
      transition: var(--m3-transition);

      &:hover {
        border-color: #bfdbfe;
      }

      &.is-focus {
        border-color: var(--m3-primary);
        box-shadow: 0 0 0 3px rgba(140, 171, 255, 0.12) !important;
      }
    }

    .el-input__inner {
      color: var(--hl-ink);
      font-size: 15px;

      &::placeholder {
        color: rgba(24, 36, 47, 0.4);
      }
    }

    .el-input__suffix {
      .el-icon {
        color: var(--hl-ink-soft);
      }
    }
  }

  :deep(.el-form-item__error) {
    color: var(--m3-error);
  }

  .login-btn {
    width: 100%;
    height: 42px;
    margin-top: 4px;
    font-size: 14px;
    font-weight: 700;
    letter-spacing: 0;
    border: 1px solid #2563eb;
    border-radius: 6px;
    background: #2563eb;
    color: var(--m3-on-primary);
    transition: var(--m3-transition);

    &:hover,
    &:focus {
      background: #1d4ed8;
    }

    &:active {
      background: #1e40af;
    }

    &.is-loading {
      opacity: 0.85;
    }
  }
}

.security-note {
  margin-top: 14px;
  font-size: 12px;
  line-height: 1.6;
  color: var(--hl-ink-soft);
}

.footer {
  margin-top: 14px;
  font-size: 12px;
  color: #94a3b8;
  letter-spacing: 0;
}

@media (max-width: 980px) {
  .login-shell {
    grid-template-columns: 1fr;
  }

  .login-intro {
    min-height: auto;
  }

  .intro-grid {
    grid-template-columns: 1fr;
  }
}

@media (max-width: 560px) {
  .login-page {
    padding: 16px;
  }

  .login-card {
    padding: 22px;
  }

  .login-intro {
    padding: 18px;
  }

  .logo-area .title {
    font-size: 28px;
  }
}
</style>
