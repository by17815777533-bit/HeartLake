<!--
  404 页面 - 访问不存在的路径时展示
-->

<template>
  <div class="not-found-page">
    <div class="not-found-card">
      <el-icon
        class="not-found-icon"
        :size="72"
      >
        <Warning />
      </el-icon>
      <h1 class="not-found-title">
        404
      </h1>
      <p class="not-found-desc">
        你访问的页面不存在或已被移除。
      </p>
      <el-button
        type="primary"
        size="large"
        @click="router.push('/dashboard')"
      >
        返回首页
      </el-button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { useRouter } from 'vue-router'
import { Warning } from '@element-plus/icons-vue'

const router = useRouter()
</script>

<style scoped lang="scss">
.not-found-page {
  width: 100%;
  height: 100vh;
  display: flex;
  align-items: center;
  justify-content: center;
  background:
    radial-gradient(circle at top left, rgba(182, 122, 66, 0.12), transparent 22%),
    radial-gradient(circle at bottom right, rgba(17, 62, 74, 0.12), transparent 24%),
    linear-gradient(180deg, var(--hl-paper-strong), var(--hl-paper));
}

.not-found-card {
  text-align: center;
  padding: 54px 44px;
  background: linear-gradient(180deg, rgba(249, 252, 252, 0.9), rgba(235, 242, 245, 0.96));
  border: 1px solid rgba(115, 141, 151, 0.14);
  border-radius: 30px;
  box-shadow: 0 28px 56px rgba(10, 23, 31, 0.08);
  max-width: 460px;
}

.not-found-icon {
  color: var(--m3-tertiary);
  margin-bottom: 20px;
}

.not-found-title {
  font-family: var(--hl-font-display);
  font-size: 56px;
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
