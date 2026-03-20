<!--
  系统偏好（设置）页面 -- 仅 super_admin 角色可访问

  功能：
  - 系统配置：站点名称、描述、注册/匿名开关
  - AI 配置：模型提供商、API Key、情感分析/自动回复开关
  - 速率限制：投石/纸船/消息频率、内容最大长度
  - 全站广播：向所有在线用户推送消息（info/warning/danger 三级）
  - 配置读取和保存通过 GET/PUT /admin/config 接口
-->

<template>
  <div class="settings-page ops-page">
    <OpsWorkbench>
      <template #stage>
        <OpsSurfaceCard eyebrow="控制台" title="系统偏好" :chip="activeTab" tone="sky">
          <div class="ops-big-metric">
            <span class="ops-big-metric__label">当前工作区</span>
            <div class="ops-big-metric__value">
              {{ summaryItems[2]?.value || '系统' }}
            </div>
            <p class="ops-big-metric__note">
              集中管理站点开关、智能回复、速率限制与全站广播，高权限操作在这里完成统一配置。
            </p>
          </div>

          <div class="ops-mini-grid settings-summary-grid">
            <article
              v-for="item in summaryItems"
              :key="item.label"
              class="ops-mini-tile"
              :class="getWorkbenchTileTone(item.tone)"
            >
              <span>{{ item.label }}</span>
              <strong>{{ item.value }}</strong>
              <small>{{ item.note }}</small>
            </article>
          </div>
        </OpsSurfaceCard>
      </template>

      <template #support>
        <OpsSurfaceCard
          eyebrow="概览"
          title="当前开关"
          :chip="providerLabelMap[aiConfig.provider] || aiConfig.provider"
          tone="ice"
          compact
        >
          <OpsMiniBars :items="settingsVizBars" />
        </OpsSurfaceCard>
      </template>

      <template #rail>
        <OpsSurfaceCard
          eyebrow="广播"
          title="广播状态"
          :chip="levelLabelMap[broadcastForm.level] || broadcastForm.level"
          tone="mint"
        >
          <div class="ops-kv-grid">
            <article class="ops-kv-item">
              <span>消息上限</span>
              <strong>{{ rateConfig.maxContentLength }}</strong>
            </article>
            <article class="ops-kv-item">
              <span>分钟限额</span>
              <strong>{{ rateConfig.messagePerMinute }}</strong>
            </article>
            <article class="ops-kv-item">
              <span>广播字数</span>
              <strong>{{ broadcastForm.message.length }}/500</strong>
            </article>
          </div>
        </OpsSurfaceCard>
      </template>

      <el-card shadow="never" class="table-card ops-table-card">
        <div class="ops-soft-toolbar">
          <div class="settings-table-copy">
            <h3>高权限配置</h3>
            <p>系统、智能回复、限流和广播统一收进一张工作台，校验和保存逻辑保持原样。</p>
          </div>
        </div>

        <el-tabs v-model="activeTab">
          <el-tab-pane label="系统配置" name="system">
            <el-form
              ref="systemFormRef"
              :model="systemConfig"
              :rules="systemRules"
              label-width="150px"
              aria-label="系统配置"
            >
              <el-form-item label="系统名称" prop="name">
                <el-input v-model="systemConfig.name" placeholder="心湖" />
              </el-form-item>
              <el-form-item label="系统描述" prop="description">
                <el-input v-model="systemConfig.description" type="textarea" :rows="2" />
              </el-form-item>
              <el-form-item label="开启注册">
                <el-switch v-model="systemConfig.allowRegister" />
              </el-form-item>
              <el-form-item label="开启匿名登录">
                <el-switch v-model="systemConfig.allowAnonymous" />
              </el-form-item>
              <el-form-item>
                <el-button type="primary" :loading="saving" @click="saveConfig('system')">
                  保存配置
                </el-button>
              </el-form-item>
            </el-form>
          </el-tab-pane>

          <el-tab-pane label="智能回复" name="ai">
            <el-form
              ref="aiFormRef"
              :model="aiConfig"
              :rules="aiRules"
              label-width="150px"
              aria-label="智能回复设置"
            >
              <el-form-item label="回复服务提供商" prop="provider">
                <el-select v-model="aiConfig.provider">
                  <el-option label="DeepSeek" value="deepseek" />
                  <el-option label="OpenAI" value="openai" />
                </el-select>
              </el-form-item>
              <el-form-item label="访问密钥" prop="apiKey">
                <el-input
                  v-model="aiConfig.apiKey"
                  :type="apiKeyVisible ? 'text' : 'password'"
                  placeholder="sk-..."
                  @focus="onApiKeyFocus"
                  @input="onApiKeyInput"
                >
                  <template #suffix>
                    <el-icon style="cursor: pointer" @click="apiKeyVisible = !apiKeyVisible">
                      <View v-if="apiKeyVisible" />
                      <Hide v-else />
                    </el-icon>
                  </template>
                </el-input>
              </el-form-item>
              <el-form-item label="服务地址" prop="baseUrl">
                <el-input v-model="aiConfig.baseUrl" placeholder="https://api.deepseek.com" />
              </el-form-item>
              <el-form-item label="使用模型" prop="model">
                <el-input v-model="aiConfig.model" placeholder="deepseek-chat" />
              </el-form-item>
              <el-form-item label="开启情感分析">
                <el-switch v-model="aiConfig.enableSentiment" />
              </el-form-item>
              <el-form-item label="开启智能回复">
                <el-switch v-model="aiConfig.enableAutoReply" />
              </el-form-item>
              <el-form-item>
                <el-button type="primary" :loading="saving" @click="saveConfig('ai')">
                  保存配置
                </el-button>
                <el-button :loading="testing" @click="testAI"> 测试连接 </el-button>
              </el-form-item>
            </el-form>
          </el-tab-pane>

          <el-tab-pane label="限流配置" name="rate">
            <el-form
              ref="rateFormRef"
              :model="rateConfig"
              :rules="rateRules"
              label-width="180px"
              aria-label="限流配置"
            >
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
                <el-input-number
                  v-model="rateConfig.maxContentLength"
                  :min="100"
                  :max="5000"
                  :step="100"
                />
              </el-form-item>
              <el-form-item>
                <el-button type="primary" :loading="saving" @click="saveConfig('rate')">
                  保存配置
                </el-button>
              </el-form-item>
            </el-form>
          </el-tab-pane>

          <el-tab-pane label="广播" name="broadcast">
            <el-form :model="broadcastForm" label-width="120px" aria-label="广播消息">
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
                <el-button type="primary" :loading="broadcasting" @click="sendBroadcast">
                  发送广播
                </el-button>
              </el-form-item>
            </el-form>
          </el-tab-pane>
        </el-tabs>
      </el-card>
    </OpsWorkbench>
  </div>
</template>

<script setup lang="ts">
import { computed, ref, reactive, onMounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import type { FormInstance } from 'element-plus'
import { View, Hide } from '@element-plus/icons-vue'
import api, { isRequestCanceled } from '@/api'
import OpsWorkbench from '@/components/OpsWorkbench.vue'
import OpsSurfaceCard from '@/components/OpsSurfaceCard.vue'
import OpsMiniBars from '@/components/OpsMiniBars.vue'
import { getErrorMessage } from '@/utils/errorHelper'
import { getWorkbenchTileTone } from '@/utils/workbenchTone'

const activeTab = ref('system')
const saving = ref(false)
const testing = ref(false)
const broadcasting = ref(false)
const aiFormRef = ref<FormInstance | null>(null)
const rateFormRef = ref<FormInstance | null>(null)
const systemFormRef = ref<FormInstance | null>(null)
const apiKeyEdited = ref(false)
const apiKeyVisible = ref(false)

// 原始 API Key 存储在非响应式变量中，避免 Vue DevTools 泄漏明文
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
  description: [{ max: 200, message: '系统描述不超过 200 个字符', trigger: 'blur' }],
}

// 系统配置
const systemConfig = reactive({
  name: '心湖',
  description: '一个温暖治愈的情感交流社区',
  allowRegister: true,
  allowAnonymous: true,
})

// 智能回复
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

// 输入时同步到非响应式变量，避免 DevTools 长期持有明文
const onApiKeyInput = (val: string) => {
  if (apiKeyEdited.value) {
    rawApiKey = val
  }
}

// 智能回复配置验证规则
const aiRules = {
  provider: [{ required: true, message: '请选择回复服务提供商', trigger: 'change' }],
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

// 限流配置验证规则
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

const providerLabelMap: Record<string, string> = {
  deepseek: 'DeepSeek',
  openai: 'OpenAI',
}

const levelLabelMap: Record<string, string> = {
  info: '信息',
  success: '成功',
  warning: '警告',
  error: '错误',
}

const summaryItems = computed(() => [
  {
    label: '开放注册',
    value: systemConfig.allowRegister ? '开启' : '关闭',
    note: systemConfig.allowRegister ? '新旅人可以自行加入' : '当前仅允许内部导入或邀请',
    tone: systemConfig.allowRegister ? ('sage' as const) : ('rose' as const),
  },
  {
    label: '匿名进入',
    value: systemConfig.allowAnonymous ? '允许' : '关闭',
    note: systemConfig.allowAnonymous ? '保持低门槛表达入口' : '当前要求更明确的身份绑定',
    tone: systemConfig.allowAnonymous ? ('lake' as const) : ('amber' as const),
  },
  {
    label: '回复方案',
    value: providerLabelMap[aiConfig.provider] || aiConfig.provider,
    note: `模型 ${aiConfig.model || '未设置'} · 当前页签 ${activeTab.value}`,
    tone: 'amber' as const,
  },
  {
    label: '内容上限',
    value: `${rateConfig.maxContentLength}`,
    note: `广播级别 ${levelLabelMap[broadcastForm.level] || broadcastForm.level} · 每分钟消息 ${rateConfig.messagePerMinute}`,
    tone: 'lake' as const,
  },
])

const settingsVizBars = computed(() => [
  { label: '投石', value: rateConfig.stonePerHour, display: String(rateConfig.stonePerHour) },
  { label: '纸船', value: rateConfig.boatPerHour, display: String(rateConfig.boatPerHour) },
  {
    label: '消息',
    value: rateConfig.messagePerMinute,
    display: String(rateConfig.messagePerMinute),
  },
  {
    label: '长度',
    value: rateConfig.maxContentLength,
    display: String(rateConfig.maxContentLength),
  },
])

const settingsScore = computed(() => {
  let score = 54
  if (systemConfig.allowRegister) score += 10
  if (systemConfig.allowAnonymous) score += 8
  if (aiConfig.enableAutoReply) score += 10
  if (aiConfig.enableSentiment) score += 8
  if (broadcastForm.message.length > 0) score += 5
  return Math.max(30, Math.min(95, score))
})

const settingsLabel = computed(() => {
  if (settingsScore.value >= 82) return '完整'
  if (settingsScore.value >= 60) return '已配置'
  return '待完善'
})

// 加载配置（snake_case → camelCase 转换）
const loadConfig = async () => {
  try {
    const res = await api.getSystemConfig()
    const data = res.data?.data || res.data
    if (data) {
      const sys = data.system || {}
      systemConfig.name = sys.name ?? sys.site_name ?? '心湖'
      systemConfig.description = sys.description ?? '一个温暖治愈的情感交流社区'
      systemConfig.allowRegister = sys.allow_register ?? sys.allowRegister ?? true
      systemConfig.allowAnonymous = sys.allow_anonymous ?? sys.allowAnonymous ?? true

      const ai = data.ai || {}
      aiConfig.provider = ai.provider ?? 'deepseek'
      aiConfig.apiKey = maskApiKey(ai.api_key ?? ai.apiKey ?? '')
      rawApiKey = '' // 加载时不保留原始 key，仅展示掩码
      apiKeyEdited.value = false
      apiKeyVisible.value = false
      aiConfig.baseUrl = ai.base_url ?? ai.baseUrl ?? 'https://api.deepseek.com'
      aiConfig.model = ai.model ?? 'deepseek-chat'
      aiConfig.enableSentiment = ai.enable_sentiment ?? ai.enableSentiment ?? true
      aiConfig.enableAutoReply = ai.enable_auto_reply ?? ai.enableAutoReply ?? true

      const rate = data.rate || {}
      rateConfig.stonePerHour = rate.stone_per_hour ?? rate.stonePerHour ?? 15
      rateConfig.boatPerHour = rate.boat_per_hour ?? rate.boatPerHour ?? 50
      rateConfig.messagePerMinute = rate.message_per_minute ?? rate.messagePerMinute ?? 60
      rateConfig.maxContentLength = rate.max_content_length ?? rate.maxContentLength ?? 2000
    }
  } catch (e) {
    if (isRequestCanceled(e)) return
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

// 测试智能回复连接
const testAI = async () => {
  testing.value = true
  try {
    await api.getEdgeAIStatus()
    ElMessage.success('回复服务连接正常')
  } catch (e) {
    console.error('回复服务连接失败:', e)
    ElMessage.error(getErrorMessage(e, '回复服务连接失败'))
  } finally {
    testing.value = false
  }
}

onMounted(() => {
  loadConfig()
})

// 发送广播前二次确认并限制内容长度
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
      },
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
  .settings-summary-grid {
    margin-top: 18px;
  }

  .settings-table-copy {
    h3 {
      color: var(--hl-ink);
      font-size: 24px;
      font-weight: 700;
      letter-spacing: -0.03em;
    }

    p {
      margin-top: 8px;
      color: var(--hl-ink-soft);
      font-size: 13px;
      line-height: 1.7;
    }
  }

  :deep(.el-form) {
    max-width: 680px;
  }
}
</style>
