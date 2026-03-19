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
  background:
    radial-gradient(circle at top left, rgba(162, 190, 255, 0.26), transparent 22%),
    radial-gradient(circle at bottom right, rgba(157, 228, 217, 0.2), transparent 24%),
    linear-gradient(180deg, #d9e8ff, #cfe0ff);
}

.forbidden-card {
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

.forbidden-icon {
  color: var(--m3-error);
  margin-bottom: 20px;
}

.forbidden-title {
  font-family: var(--hl-font-display);
  font-size: 34px;
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
