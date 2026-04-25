<!--
  403 权限不足页面
-->

<template>
  <div class="forbidden-page">
    <div class="forbidden-card">
      <el-icon class="forbidden-icon" :size="72">
        <Lock />
      </el-icon>
      <h1 class="forbidden-title">权限不足</h1>
      <p class="forbidden-desc">你没有访问该页面的权限，请联系管理员提升权限。</p>
      <el-button type="primary" size="large" @click="router.push(ADMIN_HOME_PATH)">
        返回首页
      </el-button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { useRouter } from 'vue-router'
import { Lock } from '@element-plus/icons-vue'
import { ADMIN_HOME_PATH } from '@/utils/adminRoutes'

const router = useRouter()
</script>

<style scoped lang="scss">
.forbidden-page {
  width: 100%;
  height: 100vh;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 24px;
  background: #eef2f7;
}

.forbidden-card {
  text-align: center;
  padding: 34px 32px;
  background: #ffffff;
  border: 1px solid #d8dee8;
  border-radius: 8px;
  box-shadow: 0 10px 24px rgba(15, 23, 42, 0.08);
  max-width: 420px;
}

.forbidden-icon {
  color: var(--m3-error);
  margin-bottom: 20px;
}

.forbidden-title {
  font-family: var(--hl-font-display);
  font-size: 26px;
  font-weight: 760;
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
