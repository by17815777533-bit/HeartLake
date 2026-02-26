<!--
  @file NotFound.vue
  @brief 404 页面 - 访问不存在的路径时展示
  Created by 林子怡
-->

<template>
  <div class="not-found-page">
    <el-empty
      description="页面不存在"
      :image-size="200"
    >
      <template #description>
        <p class="not-found-desc">你访问的页面不存在或已被移除</p>
      </template>
      <el-button
        type="primary"
        @click="goHome"
      >
        返回首页
      </el-button>
    </el-empty>
  </div>
</template>

<script setup lang="ts">
import { useRouter } from 'vue-router'

const router = useRouter()

const goHome = () => {
  router.push('/dashboard')
}
</script>

<style lang="scss" scoped>
.not-found-page {
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 100vh;
  background: var(--m3-surface-dim, #1a1a3e);

  .not-found-desc {
    color: var(--m3-on-surface-variant, #b8a99a);
    font-size: 16px;
    margin: 12px 0;
  }
}
</style>
