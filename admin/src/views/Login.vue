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
    appStore.setToken(token)
    // PASETO token 不可客户端解码，直接存储登录响应中的用户信息
    const user = resData.user || resData.admin || { username: form.username }
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
  position: relative;
  width: 100%;
  min-height: 100vh;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 32px;
  overflow: hidden;
  background:
    radial-gradient(circle at top left, rgba(162, 190, 255, 0.34), transparent 26%),
    radial-gradient(circle at bottom right, rgba(157, 228, 217, 0.22), transparent 24%),
    linear-gradient(180deg, #d9e8ff 0%, #cddfff 100%);

  &::before,
  &::after {
    content: '';
    position: absolute;
    border-radius: 999px;
    pointer-events: none;
    filter: blur(12px);
  }

  &::before {
    width: 320px;
    height: 320px;
    top: -120px;
    right: -60px;
    background: rgba(148, 176, 255, 0.26);
  }

  &::after {
    width: 280px;
    height: 280px;
    bottom: -120px;
    left: -80px;
    background: rgba(160, 226, 214, 0.24);
  }
}

.login-shell {
  position: relative;
  z-index: 2;
  width: min(1180px, 100%);
  display: grid;
  grid-template-columns: minmax(0, 1.2fr) minmax(360px, 410px);
  gap: 20px;
  align-items: stretch;
  padding: 18px;
  border-radius: 38px;
  border: 9px solid rgba(255, 255, 255, 0.96);
  background: linear-gradient(180deg, rgba(234, 242, 255, 0.96), rgba(223, 235, 255, 0.98));
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.8),
    0 26px 54px rgba(91, 121, 178, 0.14);
}

.login-intro {
  position: relative;
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  min-height: 640px;
  padding: 34px;
  border-radius: 28px;
  border: 1px solid rgba(160, 179, 215, 0.16);
  background: linear-gradient(180deg, rgba(239, 245, 255, 0.98), rgba(229, 238, 255, 0.98));
  box-shadow: var(--hl-shadow-soft);
}

.intro-kicker {
  display: inline-flex;
  width: fit-content;
  padding: 7px 12px;
  border-radius: 999px;
  background: rgba(255, 255, 255, 0.82);
  color: #7f96cb;
  font-family: var(--hl-font-mono);
  font-size: 11px;
  letter-spacing: 0.16em;
  text-transform: uppercase;
}

.intro-title {
  margin: 20px 0 12px;
  font-family: var(--hl-font-display);
  max-width: 10ch;
  font-size: clamp(30px, 4vw, 44px);
  line-height: 1.08;
  color: var(--hl-ink);
  text-wrap: balance;
}

.intro-copy {
  max-width: 34rem;
  margin: 0 0 30px;
  font-size: 16px;
  line-height: 1.8;
  color: var(--hl-ink-soft);
}

.intro-grid {
  display: grid;
  grid-template-columns: minmax(0, 1.08fr) minmax(0, 0.92fr);
  grid-template-rows: repeat(2, minmax(0, 1fr));
  gap: 14px;
}

.intro-panel {
  min-height: 156px;
  padding: 18px;
  border-radius: 24px;
  border: 1px solid rgba(160, 179, 215, 0.14);
  background: rgba(255, 255, 255, 0.72);
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.9),
    0 12px 22px rgba(110, 136, 186, 0.08);

  &:nth-child(1) {
    grid-row: 1 / 3;
    background: linear-gradient(180deg, rgba(255, 255, 255, 0.9), rgba(235, 242, 255, 0.98));
  }

  &:nth-child(2) {
    background: linear-gradient(180deg, rgba(221, 246, 240, 0.92), rgba(212, 240, 233, 0.96));
  }

  &:nth-child(3) {
    background: linear-gradient(180deg, rgba(223, 235, 255, 0.96), rgba(214, 228, 255, 0.98));
  }

  strong {
    display: block;
    margin-top: 18px;
    font-size: 16px;
    color: var(--hl-ink);
  }

  p {
    margin-top: 8px;
    font-size: 13px;
    line-height: 1.7;
    color: var(--hl-ink-soft);
  }
}

.panel-index {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 40px;
  height: 24px;
  border-radius: 999px;
  background: rgba(143, 170, 255, 0.12);
  color: #7994e6;
  font-family: var(--hl-font-mono);
  font-size: 11px;
  letter-spacing: 0.12em;
}

.intro-note {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
  margin-top: 22px;

  span {
    display: inline-flex;
    align-items: center;
    height: 32px;
    padding: 0 14px;
    border-radius: 999px;
    border: 1px solid rgba(160, 179, 215, 0.14);
    background: rgba(255, 255, 255, 0.82);
    font-size: 12px;
    color: var(--hl-ink-soft);
  }
}

.login-card {
  position: relative;
  width: 100%;
  padding: 38px 34px;
  border-radius: 28px;
  border: 1px solid rgba(160, 179, 215, 0.16);
  background: linear-gradient(180deg, rgba(255, 255, 255, 0.94), rgba(243, 247, 255, 0.98));
  box-shadow: var(--hl-shadow-soft);
}

.logo-area {
  text-align: left;
  margin-bottom: 26px;

  .logo-icon {
    width: 56px;
    height: 56px;
    margin-bottom: 18px;
    padding: 14px;
    border-radius: 18px;
    background: linear-gradient(135deg, rgba(142, 174, 252, 0.16), rgba(127, 207, 192, 0.16));
  }

  .title {
    margin: 0 0 8px;
    font-family: var(--hl-font-display);
    font-size: 30px;
    font-weight: 600;
    color: var(--hl-ink);
    letter-spacing: 0.04em;
  }

  .subtitle {
    font-family: var(--hl-font-mono);
    font-size: 11px;
    color: var(--hl-ink-soft);
    margin: 0;
    letter-spacing: 0.16em;
    text-transform: uppercase;
  }
}

.trust-strip {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
  margin-bottom: 26px;

  span {
    display: inline-flex;
    align-items: center;
    height: 30px;
    padding: 0 12px;
    border-radius: 999px;
    background: rgba(238, 244, 255, 0.92);
    color: var(--hl-ink);
    font-size: 12px;
    font-weight: 600;
  }
}

.login-alert {
  margin-bottom: 16px;
}

.login-form {
  :deep(.el-form-item) {
    margin-bottom: 20px;

    .el-input__wrapper {
      min-height: 52px;
      background: rgba(240, 246, 255, 0.96);
      border: 1px solid rgba(160, 179, 215, 0.14);
      box-shadow: none !important;
      border-radius: 15px;
      transition: var(--m3-transition);

      &:hover {
        border-color: rgba(137, 165, 207, 0.18);
        background: rgba(255, 255, 255, 0.98);
      }

      &.is-focus {
        border-color: var(--m3-primary);
        background: rgba(255, 255, 255, 0.98);
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
    height: 52px;
    margin-top: 8px;
    font-size: 16px;
    font-weight: 600;
    letter-spacing: 0.08em;
    border: none;
    border-radius: 15px;
    background: linear-gradient(180deg, #8cb2ff, #789cf2);
    color: var(--m3-on-primary);
    transition: var(--m3-transition);

    &:hover,
    &:focus {
      background: linear-gradient(180deg, #82a9ff, #6f95eb);
      box-shadow: var(--hl-shadow-soft);
      transform: translateY(-1px);
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

.security-note {
  margin-top: 18px;
  font-size: 12px;
  line-height: 1.7;
  color: var(--hl-ink-soft);
}

.footer {
  margin-top: 18px;
  font-size: 12px;
  color: rgba(33, 42, 58, 0.42);
  letter-spacing: 0.08em;
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
    grid-template-rows: none;
  }

  .intro-panel:nth-child(1) {
    grid-row: auto;
  }
}

@media (max-width: 560px) {
  .login-page {
    padding: 16px;
  }

  .login-card {
    padding: 32px 24px;
    border-radius: 24px;
  }

  .login-intro {
    padding: 24px;
    border-radius: 24px;
  }

  .logo-area .title {
    font-size: 28px;
  }
}
</style>
