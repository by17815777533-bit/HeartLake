<!--
  @file Settings.vue
  @brief Settings 组件
  Created by 林子怡
-->

<template>
  <div class="settings-page">
    <el-tabs v-model="activeTab">
      <!-- 系统配置 -->
      <el-tab-pane label="系统配置" name="system">
        <el-card shadow="never">
          <el-form :model="systemConfig" label-width="150px">
            <el-form-item label="系统名称">
              <el-input v-model="systemConfig.name" placeholder="心湖" />
            </el-form-item>
            <el-form-item label="系统描述">
              <el-input v-model="systemConfig.description" type="textarea" :rows="2" />
            </el-form-item>
            <el-form-item label="开启注册">
              <el-switch v-model="systemConfig.allowRegister" />
            </el-form-item>
            <el-form-item label="开启匿名登录">
              <el-switch v-model="systemConfig.allowAnonymous" />
            </el-form-item>
            <el-form-item>
              <el-button type="primary" @click="saveConfig('system')" :loading="saving">
                保存配置
              </el-button>
            </el-form-item>
          </el-form>
        </el-card>
      </el-tab-pane>

      <!-- AI配置 -->
      <el-tab-pane label="AI配置" name="ai">
        <el-card shadow="never">
          <!-- M-4: AI 配置添加表单验证 -->
          <el-form ref="aiFormRef" :model="aiConfig" :rules="aiRules" label-width="150px">
            <el-form-item label="AI服务提供商" prop="provider">
              <el-select v-model="aiConfig.provider">
                <el-option label="DeepSeek" value="deepseek" />
                <el-option label="OpenAI" value="openai" />
              </el-select>
            </el-form-item>
            <el-form-item label="API Key" prop="apiKey">
              <el-input v-model="aiConfig.apiKey" type="password" show-password placeholder="sk-..." />
            </el-form-item>
            <el-form-item label="Base URL" prop="baseUrl">
              <el-input v-model="aiConfig.baseUrl" placeholder="https://api.deepseek.com" />
            </el-form-item>
            <el-form-item label="模型名称" prop="model">
              <el-input v-model="aiConfig.model" placeholder="deepseek-chat" />
            </el-form-item>
            <el-form-item label="开启情感分析">
              <el-switch v-model="aiConfig.enableSentiment" />
            </el-form-item>
            <el-form-item label="开启AI回复">
              <el-switch v-model="aiConfig.enableAutoReply" />
            </el-form-item>
            <el-form-item>
              <el-button type="primary" @click="saveConfig('ai')" :loading="saving">
                保存配置
              </el-button>
              <el-button @click="testAI" :loading="testing">测试连接</el-button>
            </el-form-item>
          </el-form>
        </el-card>
      </el-tab-pane>

      <!-- 限流配置 -->
      <el-tab-pane label="限流配置" name="rate">
        <el-card shadow="never">
          <!-- M-4: 限流配置添加表单验证 -->
          <el-form ref="rateFormRef" :model="rateConfig" :rules="rateRules" label-width="180px">
            <el-form-item label="每小时投石限制" prop="stonePerHour">
              <el-input-number v-model="rateConfig.stonePerHour" :min="1" :max="100" />
            </el-form-item>
            <el-form-item label="每小时纸船限制" prop="boatPerHour">
              <el-input-number v-model="rateConfig.boatPerHour" :min="1" :max="100" />
            </el-form-item>
            <el-form-item label="每分钟消息限制" prop="messagePerMinute">
              <el-input-number v-model="rateConfig.messagePerMinute" :min="1" :max="120" />
            </el-form-item>
            <el-form-item label="内容最大长度" prop="maxContentLength">
              <el-input-number v-model="rateConfig.maxContentLength" :min="100" :max="5000" :step="100" />
            </el-form-item>
            <el-form-item>
              <el-button type="primary" @click="saveConfig('rate')" :loading="saving">
                保存配置
              </el-button>
            </el-form-item>
          </el-form>
        </el-card>
      </el-tab-pane>

      <!-- 广播配置 -->
      <el-tab-pane label="广播" name="broadcast">
        <el-card shadow="never">
          <el-form :model="broadcastForm" label-width="120px">
            <el-form-item label="广播内容">
              <el-input
                v-model="broadcastForm.message"
                type="textarea"
                :rows="3"
                placeholder="请输入要广播的内容"
                :maxlength="500"
                show-word-limit
              />
            </el-form-item>
            <el-form-item label="级别">
              <el-select v-model="broadcastForm.level">
                <el-option label="信息" value="info" />
                <el-option label="成功" value="success" />
                <el-option label="警告" value="warning" />
                <el-option label="错误" value="error" />
              </el-select>
            </el-form-item>
            <el-form-item>
              <el-button type="primary" @click="sendBroadcast" :loading="broadcasting">
                发送广播
              </el-button>
            </el-form-item>
          </el-form>
        </el-card>
      </el-tab-pane>
    </el-tabs>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import api from '@/api'
import { getErrorMessage } from '@/utils/errorHelper'

const activeTab = ref('system')
const saving = ref(false)
const testing = ref(false)
const broadcasting = ref(false)
const aiFormRef = ref(null)
const rateFormRef = ref(null)

// 系统配置
const systemConfig = reactive({
  name: '心湖',
  description: '一个温暖治愈的情感交流社区',
  allowRegister: true,
  allowAnonymous: true,
})

// AI配置
const aiConfig = reactive({
  provider: 'deepseek',
  apiKey: '',
  baseUrl: 'https://api.deepseek.com',
  model: 'deepseek-chat',
  enableSentiment: true,
  enableAutoReply: true,
})

// M-4: AI 配置验证规则
const aiRules = {
  provider: [{ required: true, message: '请选择 AI 服务提供商', trigger: 'change' }],
  apiKey: [
    { required: true, message: '请输入 API Key', trigger: 'blur' },
    { pattern: /^sk-/, message: 'API Key 应以 sk- 开头', trigger: 'blur' },
  ],
  baseUrl: [
    { required: true, message: '请输入 Base URL', trigger: 'blur' },
    { type: 'url', message: '请输入有效的 URL 地址', trigger: 'blur' },
  ],
  model: [{ required: true, message: '请输入模型名称', trigger: 'blur' }],
}

// 限流配置
const rateConfig = reactive({
  stonePerHour: 15,
  boatPerHour: 50,
  messagePerMinute: 60,
  maxContentLength: 2000,
})

// M-4: 限流配置验证规则
const rateRules = {
  stonePerHour: [{ required: true, message: '请设置每小时投石限制', trigger: 'change' }],
  boatPerHour: [{ required: true, message: '请设置每小时纸船限制', trigger: 'change' }],
  messagePerMinute: [{ required: true, message: '请设置每分钟消息限制', trigger: 'change' }],
  maxContentLength: [{ required: true, message: '请设置内容最大长度', trigger: 'change' }],
}

const broadcastForm = reactive({
  message: '',
  level: 'info',
})

// 加载配置（snake_case → camelCase 转换）
const loadConfig = async () => {
  try {
    const res = await api.getSystemConfig()
    if (res.data) {
      const sys = res.data.system || {}
      systemConfig.name = sys.name ?? sys.site_name ?? '心湖'
      systemConfig.description = sys.description ?? '一个温暖治愈的情感交流社区'
      systemConfig.allowRegister = sys.allow_register ?? sys.allowRegister ?? true
      systemConfig.allowAnonymous = sys.allow_anonymous ?? sys.allowAnonymous ?? true

      const ai = res.data.ai || {}
      aiConfig.provider = ai.provider ?? 'deepseek'
      aiConfig.apiKey = ai.api_key ?? ai.apiKey ?? ''
      aiConfig.baseUrl = ai.base_url ?? ai.baseUrl ?? 'https://api.deepseek.com'
      aiConfig.model = ai.model ?? 'deepseek-chat'
      aiConfig.enableSentiment = ai.enable_sentiment ?? ai.enableSentiment ?? true
      aiConfig.enableAutoReply = ai.enable_auto_reply ?? ai.enableAutoReply ?? true

      const rate = res.data.rate || {}
      rateConfig.stonePerHour = rate.stone_per_hour ?? rate.stonePerHour ?? 15
      rateConfig.boatPerHour = rate.boat_per_hour ?? rate.boatPerHour ?? 50
      rateConfig.messagePerMinute = rate.message_per_minute ?? rate.messagePerMinute ?? 60
      rateConfig.maxContentLength = rate.max_content_length ?? rate.maxContentLength ?? 2000
    }
  } catch (e) {
    console.error('加载配置失败:', e)
    ElMessage.error(getErrorMessage(e, '加载配置失败'))
  }
}

// 保存配置（带表单验证）
const saveConfig = async (type) => {
  // AI 和限流配置需要先校验表单
  if (type === 'ai' && aiFormRef.value) {
    const valid = await aiFormRef.value.validate().catch(() => false)
    if (!valid) return
  }
  if (type === 'rate' && rateFormRef.value) {
    const valid = await rateFormRef.value.validate().catch(() => false)
    if (!valid) return
  }

  saving.value = true
  try {
    // camelCase → snake_case 转换
    const configMap = {
      system: {
        name: systemConfig.name,
        description: systemConfig.description,
        allow_register: systemConfig.allowRegister,
        allow_anonymous: systemConfig.allowAnonymous,
      },
      ai: {
        provider: aiConfig.provider,
        api_key: aiConfig.apiKey,
        base_url: aiConfig.baseUrl,
        model: aiConfig.model,
        enable_sentiment: aiConfig.enableSentiment,
        enable_auto_reply: aiConfig.enableAutoReply,
      },
      rate: {
        stone_per_hour: rateConfig.stonePerHour,
        boat_per_hour: rateConfig.boatPerHour,
        message_per_minute: rateConfig.messagePerMinute,
        max_content_length: rateConfig.maxContentLength,
      },
    }
    await api.updateSystemConfig({ [type]: configMap[type] })
    ElMessage.success('配置保存成功')
  } catch (e) {
    console.error('配置保存失败:', e)
    ElMessage.error(getErrorMessage(e, '配置保存失败'))
  } finally {
    saving.value = false
  }
}

// 测试AI连接
const testAI = async () => {
  testing.value = true
  try {
    await api.testAIConnection()
    ElMessage.success('AI服务连接正常')
  } catch (e) {
    console.error('AI服务连接失败:', e)
    ElMessage.error(getErrorMessage(e, 'AI服务连接失败'))
  } finally {
    testing.value = false
  }
}

onMounted(() => {
  loadConfig()
})

// M-5: 发送广播添加二次确认和内容长度限制
const sendBroadcast = async () => {
  const msg = broadcastForm.message.trim()
  if (!msg) {
    ElMessage.warning('请输入广播内容')
    return
  }
  if (msg.length > 500) {
    ElMessage.warning('广播内容不能超过 500 字')
    return
  }

  // 二次确认
  try {
    await ElMessageBox.confirm(
      `确定要向所有在线用户发送以下广播吗？\n\n"${msg.substring(0, 100)}${msg.length > 100 ? '...' : ''}"`,
      '发送广播确认',
      {
        confirmButtonText: '确定发送',
        cancelButtonText: '取消',
        type: 'warning',
      }
    )
  } catch {
    return // 用户取消
  }

  broadcasting.value = true
  try {
    await api.broadcastMessage({
      message: msg,
      level: broadcastForm.level,
    })
    ElMessage.success('广播已发送')
    broadcastForm.message = ''
  } catch (e) {
    console.error('广播发送失败:', e)
    ElMessage.error(getErrorMessage(e, '广播发送失败'))
  } finally {
    broadcasting.value = false
  }
}
</script>

<style lang="scss" scoped>
.settings-page {
  padding: 24px;

  :deep(.el-form) {
    max-width: 600px;
  }

  // ── 光遇主题：Tabs 标签页 ──
  :deep(.el-tabs) {
    .el-tabs__header {
      border-bottom: 1px solid rgba(242, 204, 143, 0.1);
    }

    .el-tabs__nav-wrap::after {
      background: rgba(242, 204, 143, 0.1);
    }

    .el-tabs__active-bar {
      background: linear-gradient(90deg, #F2CC8F, #E8A87C);
      height: 3px;
      border-radius: 2px;
    }

    .el-tabs__item {
      color: #7A6F63;
      font-size: 15px;

      &:hover {
        color: #B8A99A;
      }

      &.is-active {
        color: #F2CC8F;
        font-weight: 600;
      }
    }
  }

  // ── 毛玻璃卡片 ──
  :deep(.el-card) {
    background: rgba(26, 26, 62, 0.6);
    backdrop-filter: blur(16px);
    border: 1px solid rgba(242, 204, 143, 0.12);
    border-radius: 16px;
    color: #F0E6D3;

    .el-card__body {
      color: #F0E6D3;
    }
  }

  // ── 表单标签 ──
  :deep(.el-form-item__label) {
    color: #B8A99A;
  }

  // ── 输入框 ──
  :deep(.el-input) {
    .el-input__wrapper {
      background: rgba(20, 20, 50, 0.6);
      border: 1px solid rgba(242, 204, 143, 0.15);
      border-radius: 8px;
      box-shadow: none;

      .el-input__inner {
        color: #F0E6D3;

        &::placeholder {
          color: #7A6F63;
        }
      }

      &:hover {
        border-color: rgba(242, 204, 143, 0.3);
      }

      &.is-focus {
        border-color: #F2CC8F;
        box-shadow: 0 0 8px rgba(242, 204, 143, 0.15);
      }
    }
  }

  // ── 文本域 ──
  :deep(.el-textarea) {
    .el-textarea__inner {
      background: rgba(20, 20, 50, 0.6);
      border: 1px solid rgba(242, 204, 143, 0.15);
      border-radius: 8px;
      box-shadow: none;
      color: #F0E6D3;

      &::placeholder {
        color: #7A6F63;
      }

      &:hover {
        border-color: rgba(242, 204, 143, 0.3);
      }

      &:focus {
        border-color: #F2CC8F;
        box-shadow: 0 0 8px rgba(242, 204, 143, 0.15);
      }
    }

    // 字数统计
    .el-input__count {
      background: transparent;
      color: #7A6F63;
    }
  }

  // ── 下拉选择器 ──
  :deep(.el-select) {
    .el-select__wrapper {
      background: rgba(20, 20, 50, 0.6);
      border: 1px solid rgba(242, 204, 143, 0.15);
      border-radius: 8px;
      box-shadow: none;
      color: #F0E6D3;

      &:hover {
        border-color: rgba(242, 204, 143, 0.3);
      }

      &.is-focused {
        border-color: #F2CC8F;
        box-shadow: 0 0 8px rgba(242, 204, 143, 0.15);
      }

      .el-select__selected-item {
        color: #F0E6D3;
      }

      .el-select__placeholder {
        color: #7A6F63;
      }

      .el-select__suffix {
        color: #B8A99A;
      }
    }
  }

  :deep(.el-select__popper) {
    background: rgba(26, 26, 62, 0.95);
    backdrop-filter: blur(16px);
    border: 1px solid rgba(242, 204, 143, 0.12);
    border-radius: 12px;

    .el-select-dropdown__item {
      color: #B8A99A;

      &.is-hovering {
        background: rgba(242, 204, 143, 0.08);
        color: #F2CC8F;
      }

      &.is-selected {
        color: #F2CC8F;
        font-weight: 600;
      }
    }
  }

  // ── 开关 ──
  :deep(.el-switch) {
    --el-switch-off-color: rgba(122, 111, 99, 0.4);

    &.is-checked {
      .el-switch__core {
        background: linear-gradient(135deg, #F2CC8F, #E8A87C);
        border-color: transparent;
      }
    }

    .el-switch__core {
      border: 1px solid rgba(242, 204, 143, 0.2);
    }
  }

  // ── 数字输入框 ──
  :deep(.el-input-number) {
    .el-input__wrapper {
      background: rgba(20, 20, 50, 0.6);
      border: 1px solid rgba(242, 204, 143, 0.15);
      border-radius: 8px;
      box-shadow: none;

      .el-input__inner {
        color: #F0E6D3;
      }

      &:hover {
        border-color: rgba(242, 204, 143, 0.3);
      }

      &.is-focus {
        border-color: #F2CC8F;
        box-shadow: 0 0 8px rgba(242, 204, 143, 0.15);
      }
    }

    .el-input-number__decrease,
    .el-input-number__increase {
      background: rgba(20, 20, 50, 0.4);
      border-color: rgba(242, 204, 143, 0.15);
      color: #B8A99A;

      &:hover {
        color: #F2CC8F;
      }
    }
  }

  // ── 按钮 ──
  :deep(.el-button--primary:not(.is-link)) {
    background: linear-gradient(135deg, #F2CC8F, #E8A87C);
    border: none;
    color: #141432;
    font-weight: 600;
    border-radius: 8px;

    &:hover {
      background: linear-gradient(135deg, #E8A87C, #E07A5F);
      box-shadow: 0 4px 12px rgba(242, 204, 143, 0.3);
    }
  }

  :deep(.el-button:not(.el-button--primary):not(.el-button--success):not(.el-button--danger):not(.el-button--warning):not(.el-button--info):not(.is-link)) {
    background: rgba(20, 20, 50, 0.6);
    border: 1px solid rgba(242, 204, 143, 0.2);
    color: #F0E6D3;
    border-radius: 8px;

    &:hover {
      border-color: #F2CC8F;
      color: #F2CC8F;
    }
  }

  // ── 确认弹窗 ──
  :deep(.el-message-box) {
    background: rgba(26, 26, 62, 0.95);
    backdrop-filter: blur(20px);
    border: 1px solid rgba(242, 204, 143, 0.12);
    border-radius: 16px;

    .el-message-box__title {
      color: #F0E6D3;
    }

    .el-message-box__content {
      color: #B8A99A;
    }
  }

  // ── Loading 遮罩 ──
  :deep(.el-loading-mask) {
    background: rgba(20, 20, 50, 0.7);
    backdrop-filter: blur(4px);
  }
}
</style>
