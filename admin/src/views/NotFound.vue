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
        @click="router.push(ADMIN_HOME_PATH)"
      >
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
  background:
    radial-gradient(circle at top left, rgba(162, 190, 255, 0.26), transparent 22%),
    radial-gradient(circle at bottom right, rgba(157, 228, 217, 0.2), transparent 24%),
    linear-gradient(180deg, #d9e8ff, #cfe0ff);
}

.not-found-card {
  text-align: center;
  padding: 54px 44px;
  background:
    radial-gradient(circle at top right, rgba(197, 238, 229, 0.26), transparent 26%),
    linear-gradient(180deg, rgba(255, 255, 255, 0.94), rgba(240, 247, 255, 0.98));
  border: 1px solid rgba(137, 165, 207, 0.14);
  border-radius: 34px;
  box-shadow: 0 28px 56px rgba(91, 121, 178, 0.12);
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
