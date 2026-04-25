<!--
  404 页面 - 访问不存在的路径时展示
-->

<template>
  <div class="not-found-page">
    <div class="not-found-card">
      <el-icon class="not-found-icon" :size="72">
        <Warning />
      </el-icon>
      <h1 class="not-found-title">404</h1>
      <p class="not-found-desc">你访问的页面不存在或已被移除。</p>
      <el-button type="primary" size="large" @click="router.push(ADMIN_HOME_PATH)">
        返回首页
      </el-button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { useRouter } from 'vue-router'
import { Warning } from '@element-plus/icons-vue'
import { ADMIN_HOME_PATH } from '@/utils/adminRoutes'

const router = useRouter()
</script>

<style scoped lang="scss">
.not-found-page {
  width: 100%;
  height: 100vh;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 24px;
  background: #eef2f7;
}

.not-found-card {
  text-align: center;
  padding: 34px 32px;
  background: #ffffff;
  border: 1px solid #d8dee8;
  border-radius: 8px;
  box-shadow: 0 10px 24px rgba(15, 23, 42, 0.08);
  max-width: 420px;
}

.not-found-icon {
  color: var(--m3-tertiary);
  margin-bottom: 20px;
}

.not-found-title {
  font-family: var(--hl-font-display);
  font-size: 44px;
  font-weight: 700;
  color: var(--m3-on-surface);
  margin: 0 0 12px 0;
}

.not-found-desc {
  font-size: 14px;
  color: var(--m3-on-surface-variant);
  margin: 0 0 28px 0;
  line-height: 1.6;
}
</style>
