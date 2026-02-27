<!--
  403 权限不足页面
-->

<template>
  <div class="forbidden-page">
    <div class="forbidden-card">
      <el-icon
        class="forbidden-icon"
        :size="72"
      >
        <Lock />
      </el-icon>
      <h1 class="forbidden-title">
        权限不足
      </h1>
      <p class="forbidden-desc">
        你没有访问该页面的权限，请联系管理员提升权限。
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
import { Lock } from '@element-plus/icons-vue'

const router = useRouter()
</script>

<style scoped lang="scss">
.forbidden-page {
  width: 100%;
  height: 100vh;
  display: flex;
  align-items: center;
  justify-content: center;
  background: var(--m3-surface);
}

.forbidden-card {
  text-align: center;
  padding: 48px 40px;
  background: var(--m3-surface-container);
  border: 1px solid var(--m3-outline-variant);
  border-radius: var(--m3-shape-xl);
  box-shadow: var(--m3-elevation-2);
  max-width: 420px;
}

.forbidden-icon {
  color: var(--m3-error);
  margin-bottom: 20px;
}

.forbidden-title {
  font-size: 28px;
  font-weight: 600;
  color: var(--m3-on-surface);
  margin: 0 0 12px 0;
}

.forbidden-desc {
  font-size: 14px;
  color: var(--m3-on-surface-variant);
  margin: 0 0 28px 0;
  line-height: 1.6;
}
</style>
