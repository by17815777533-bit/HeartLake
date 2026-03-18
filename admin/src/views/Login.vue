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
        <h1 class="intro-title">
          面向值守与处置的后台入口
        </h1>
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
          <span>低配服务器已部署</span>
          <span>公网 HTTP 可访问</span>
        </div>
      </section>

      <section class="login-card">
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

        <div class="trust-strip">
          <span>分级权限</span>
          <span>操作留痕</span>
          <span>实时看板</span>
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

        <p class="security-note">
          建议仅在可信网络中登录，敏感操作请在完成后主动退出会话。
        </p>

        <div class="footer">
          &copy; 2026 HeartLake
        </div>
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
    ElMessage.warning(`操作过于频繁，请 ${cooldownSeconds.value} 秒后重试`)
    return
  }

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
    failCount.value = 0
    ElMessage.success('登录成功')
    router.push('/dashboard')
  } catch (e) {
    console.error('登录失败:', e)
    failCount.value++
    // 连续失败3次后启动30秒冷却，配合后端429速率限制
    if (failCount.value >= 3 && cooldownSeconds.value <= 0) {
      startCooldown(30)
      ElMessage.warning('连续登录失败多次，请 30 秒后重试')
    }
    // 统一错误处理
    ElMessage.error(getErrorMessage(e, '登录失败，请检查用户名和密码'))
  } finally {
    loading.value = false
  }
}

// 非 HTTPS 环境安全警告
onMounted(() => {
  if (location.protocol !== 'https:' && location.hostname !== 'localhost' && location.hostname !== '127.0.0.1') {
    ElMessage.warning({
      message: '当前非 HTTPS 连接，数据传输可能存在安全风险',
      duration: 6000,
    })
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
    radial-gradient(circle at top left, rgba(154, 106, 53, 0.16), transparent 26%),
    linear-gradient(180deg, #f7f1e7 0%, #efe5d7 100%);

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
    background: rgba(35, 73, 99, 0.12);
  }

  &::after {
    width: 280px;
    height: 280px;
    bottom: -120px;
    left: -80px;
    background: rgba(154, 106, 53, 0.14);
  }
}

.login-shell {
  position: relative;
  z-index: 2;
  width: min(1120px, 100%);
  display: grid;
  grid-template-columns: minmax(0, 1.15fr) minmax(360px, 420px);
  gap: 28px;
  align-items: stretch;
}

.login-intro {
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  min-height: 620px;
  padding: 38px;
  border-radius: 32px;
  border: 1px solid rgba(24, 36, 47, 0.08);
  background:
    linear-gradient(180deg, rgba(255, 250, 242, 0.9), rgba(244, 236, 223, 0.95));
  box-shadow: var(--hl-shadow-medium);
}

.intro-kicker {
  display: inline-flex;
  width: fit-content;
  padding: 7px 12px;
  border-radius: 999px;
  background: rgba(35, 73, 99, 0.08);
  color: var(--m3-primary);
  font-family: var(--hl-font-mono);
  font-size: 11px;
  letter-spacing: 0.16em;
  text-transform: uppercase;
}

.intro-title {
  margin: 24px 0 14px;
  font-family: var(--hl-font-display);
  max-width: 10ch;
  font-size: clamp(30px, 4.3vw, 48px);
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
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 14px;
}

.intro-panel {
  min-height: 156px;
  padding: 18px;
  border-radius: 22px;
  border: 1px solid rgba(24, 36, 47, 0.08);
  background: rgba(255, 255, 255, 0.52);

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
  background: rgba(154, 106, 53, 0.08);
  color: var(--m3-secondary);
  font-family: var(--hl-font-mono);
  font-size: 11px;
  letter-spacing: 0.12em;
}

.intro-note {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
  margin-top: 26px;

  span {
    display: inline-flex;
    align-items: center;
    height: 32px;
    padding: 0 14px;
    border-radius: 999px;
    border: 1px solid rgba(24, 36, 47, 0.08);
    background: rgba(255, 255, 255, 0.56);
    font-size: 12px;
    color: var(--hl-ink-soft);
  }
}

.login-card {
  position: relative;
  width: 100%;
  padding: 42px 36px;
  border-radius: 32px;
  border: 1px solid rgba(24, 36, 47, 0.08);
  background: rgba(255, 250, 242, 0.92);
  box-shadow: var(--hl-shadow-medium);
}

.logo-area {
  text-align: left;
  margin-bottom: 26px;

  .logo-icon {
    width: 64px;
    height: 64px;
    margin-bottom: 18px;
    padding: 14px;
    border-radius: 18px;
    background: linear-gradient(135deg, rgba(35, 73, 99, 0.08), rgba(154, 106, 53, 0.12));
  }

  .title {
    margin: 0 0 8px;
    font-family: var(--hl-font-display);
    font-size: 34px;
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
    background: rgba(35, 73, 99, 0.07);
    color: var(--m3-primary);
    font-size: 12px;
    font-weight: 600;
  }
}

.login-form {
  :deep(.el-form-item) {
    margin-bottom: 20px;

    .el-input__wrapper {
      min-height: 52px;
      background: rgba(255, 255, 255, 0.82);
      border: 1px solid rgba(24, 36, 47, 0.12);
      box-shadow: none !important;
      border-radius: 16px;
      transition: var(--m3-transition);

      &:hover {
        border-color: rgba(24, 36, 47, 0.2);
        background: rgba(255, 255, 255, 0.96);
      }

      &.is-focus {
        border-color: var(--m3-primary);
        background: rgba(255, 255, 255, 0.98);
        box-shadow: 0 0 0 3px rgba(35, 73, 99, 0.1) !important;
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
    letter-spacing: 0.12em;
    border: none;
    border-radius: var(--m3-shape-large);
    background: linear-gradient(135deg, var(--m3-primary), #345f7d);
    color: var(--m3-on-primary);
    transition: var(--m3-transition);

    &:hover,
    &:focus {
      background: linear-gradient(135deg, #1c3f56, var(--m3-primary));
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
  color: rgba(24, 36, 47, 0.42);
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
