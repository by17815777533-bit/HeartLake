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

// 加载配置
const loadConfig = async () => {
  try {
    const res = await api.getSystemConfig()
    if (res.data) {
      Object.assign(systemConfig, res.data.system || {})
      Object.assign(aiConfig, res.data.ai || {})
      Object.assign(rateConfig, res.data.rate || {})
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
    const configMap = {
      system: systemConfig,
      ai: aiConfig,
      rate: rateConfig,
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
  .el-card {
    margin-bottom: 20px;
    border-radius: 8px;
  }

  .el-form {
    max-width: 600px;
  }
}
</style>
