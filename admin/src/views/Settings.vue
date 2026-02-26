<!--
  @file Settings.vue
  @brief Settings 组件
  Created by 林子怡
-->

<template>
  <div class="settings-page">
    <el-tabs v-model="activeTab">
      <!-- 系统配置 -->
      <el-tab-pane
        label="系统配置"
        name="system"
      >
        <el-card shadow="never">
          <el-form
            ref="systemFormRef"
            :model="systemConfig"
            :rules="systemRules"
            label-width="150px"
            aria-label="系统配置"
          >
            <el-form-item
              label="系统名称"
              prop="name"
            >
              <el-input
                v-model="systemConfig.name"
                placeholder="心湖"
              />
            </el-form-item>
            <el-form-item
              label="系统描述"
              prop="description"
            >
              <el-input
                v-model="systemConfig.description"
                type="textarea"
                :rows="2"
              />
            </el-form-item>
            <el-form-item label="开启注册">
              <el-switch v-model="systemConfig.allowRegister" />
            </el-form-item>
            <el-form-item label="开启匿名登录">
              <el-switch v-model="systemConfig.allowAnonymous" />
            </el-form-item>
            <el-form-item>
              <el-button
                type="primary"
                :loading="saving"
                @click="saveConfig('system')"
              >
                保存配置
              </el-button>
            </el-form-item>
          </el-form>
        </el-card>
      </el-tab-pane>

      <!-- AI配置 -->
      <el-tab-pane
        label="AI配置"
        name="ai"
      >
        <el-card shadow="never">
          <!-- M-4: AI 配置添加表单验证 -->
          <el-form
            ref="aiFormRef"
            :model="aiConfig"
            :rules="aiRules"
            label-width="150px"
            aria-label="AI服务配置"
          >
            <el-form-item
              label="AI服务提供商"
              prop="provider"
            >
              <el-select v-model="aiConfig.provider">
                <el-option
                  label="DeepSeek"
                  value="deepseek"
                />
                <el-option
                  label="OpenAI"
                  value="openai"
                />
              </el-select>
            </el-form-item>
            <el-form-item
              label="API Key"
              prop="apiKey"
            >
              <el-input
                v-model="aiConfig.apiKey"
                :type="apiKeyVisible ? 'text' : 'password'"
                placeholder="sk-..."
                @focus="onApiKeyFocus"
                @input="onApiKeyInput"
              >
                <template #suffix>
                  <el-icon
                    style="cursor: pointer"
                    @click="apiKeyVisible = !apiKeyVisible"
                  >
                    <View v-if="apiKeyVisible" />
                    <Hide v-else />
                  </el-icon>
                </template>
              </el-input>
            </el-form-item>
            <el-form-item
              label="Base URL"
              prop="baseUrl"
            >
              <el-input
                v-model="aiConfig.baseUrl"
                placeholder="https://api.deepseek.com"
              />
            </el-form-item>
            <el-form-item
              label="模型名称"
              prop="model"
            >
              <el-input
                v-model="aiConfig.model"
                placeholder="deepseek-chat"
              />
            </el-form-item>
            <el-form-item label="开启情感分析">
              <el-switch v-model="aiConfig.enableSentiment" />
            </el-form-item>
            <el-form-item label="开启AI回复">
              <el-switch v-model="aiConfig.enableAutoReply" />
            </el-form-item>
            <el-form-item>
              <el-button
                type="primary"
                :loading="saving"
                @click="saveConfig('ai')"
              >
                保存配置
              </el-button>
              <el-button
                :loading="testing"
                @click="testAI"
              >
                测试连接
              </el-button>
            </el-form-item>
          </el-form>
        </el-card>
      </el-tab-pane>

      <!-- 限流配置 -->
      <el-tab-pane
        label="限流配置"
        name="rate"
      >
        <el-card shadow="never">
          <!-- M-4: 限流配置添加表单验证 -->
          <el-form
            ref="rateFormRef"
            :model="rateConfig"
            :rules="rateRules"
            label-width="180px"
            aria-label="限流配置"
          >
            <el-form-item
              label="每小时投石限制"
              prop="stonePerHour"
            >
              <el-input-number
                v-model="rateConfig.stonePerHour"
                :min="1"
                :max="100"
              />
            </el-form-item>
            <el-form-item
              label="每小时纸船限制"
              prop="boatPerHour"
            >
              <el-input-number
                v-model="rateConfig.boatPerHour"
                :min="1"
                :max="100"
              />
            </el-form-item>
            <el-form-item
              label="每分钟消息限制"
              prop="messagePerMinute"
            >
              <el-input-number
                v-model="rateConfig.messagePerMinute"
                :min="1"
                :max="120"
              />
            </el-form-item>
            <el-form-item
              label="内容最大长度"
              prop="maxContentLength"
            >
              <el-input-number
                v-model="rateConfig.maxContentLength"
                :min="100"
                :max="5000"
                :step="100"
              />
            </el-form-item>
            <el-form-item>
              <el-button
                type="primary"
                :loading="saving"
                @click="saveConfig('rate')"
              >
                保存配置
              </el-button>
            </el-form-item>
          </el-form>
        </el-card>
      </el-tab-pane>

      <!-- 广播配置 -->
      <el-tab-pane
        label="广播"
        name="broadcast"
      >
        <el-card shadow="never">
          <el-form
            :model="broadcastForm"
            label-width="120px"
            aria-label="广播消息"
          >
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
                <el-option
                  label="信息"
                  value="info"
                />
                <el-option
                  label="成功"
                  value="success"
                />
                <el-option
                  label="警告"
                  value="warning"
                />
                <el-option
                  label="错误"
                  value="error"
                />
              </el-select>
            </el-form-item>
            <el-form-item>
              <el-button
                type="primary"
                :loading="broadcasting"
                @click="sendBroadcast"
              >
                发送广播
              </el-button>
            </el-form-item>
          </el-form>
        </el-card>
      </el-tab-pane>
    </el-tabs>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, onMounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import type { FormInstance } from 'element-plus'
import { View, Hide } from '@element-plus/icons-vue'
import api from '@/api'
import { getErrorMessage } from '@/utils/errorHelper'

const activeTab = ref('system')
const saving = ref(false)
const testing = ref(false)
const broadcasting = ref(false)
const aiFormRef = ref<FormInstance | null>(null)
const rateFormRef = ref<FormInstance | null>(null)
const systemFormRef = ref<FormInstance | null>(null)
const apiKeyEdited = ref(false)
const apiKeyVisible = ref(false)

// L-6: 原始 API Key 存储在非响应式变量中，避免 Vue DevTools 泄漏明文
let rawApiKey = ''

// API Key 脱敏：仅显示最后4位，其余用 * 替代
function maskApiKey(key: string): string {
  if (!key || key.length <= 4) return '****'
  return '****' + key.slice(-4)
}

// 系统配置验证规则
const systemRules = {
  name: [
    { required: true, message: '请输入系统名称', trigger: 'blur' },
    { min: 1, max: 20, message: '系统名称长度为 1-20 个字符', trigger: 'blur' },
  ],
  description: [
    { max: 200, message: '系统描述不超过 200 个字符', trigger: 'blur' },
  ],
}

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

// API Key 聚焦时清空脱敏值，让用户输入新 Key
const onApiKeyFocus = () => {
  if (!apiKeyEdited.value) {
    aiConfig.apiKey = ''
    rawApiKey = ''
    apiKeyEdited.value = true
    apiKeyVisible.value = true
  }
}

// L-6: 输入时同步到非响应式变量，避免 DevTools 长期持有明文
const onApiKeyInput = (val: string) => {
  if (apiKeyEdited.value) {
    rawApiKey = val
  }
}

// M-4: AI 配置验证规则
const aiRules = {
  provider: [{ required: true, message: '请选择 AI 服务提供商', trigger: 'change' }],
  apiKey: [
    {
      validator: (_rule: unknown, value: string, callback: (error?: Error) => void) => {
        // 未编辑时跳过校验（显示的是脱敏值）
        if (!apiKeyEdited.value) return callback()
        if (!value) return callback(new Error('请输入 API Key'))
        if (!/^sk-/.test(value)) return callback(new Error('API Key 应以 sk- 开头'))
        callback()
      },
      trigger: 'blur',
    },
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
      aiConfig.apiKey = maskApiKey(ai.api_key ?? ai.apiKey ?? '')
      rawApiKey = '' // 加载时不保留原始 key，仅展示掩码
      apiKeyEdited.value = false
      apiKeyVisible.value = false
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
const saveConfig = async (type: 'system' | 'ai' | 'rate') => {
  // 系统、AI、限流配置都需要先校验表单
  if (type === 'system' && systemFormRef.value) {
    const valid = await systemFormRef.value.validate().catch(() => false)
    if (!valid) return
  }
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
        // 未编辑 API Key 时不发送，避免把脱敏值写回后端
        ...(apiKeyEdited.value ? { api_key: rawApiKey || aiConfig.apiKey } : {}),
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
    await api.getEdgeAIStatus()
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
}
</style>
