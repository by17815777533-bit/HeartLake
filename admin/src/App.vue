<!--
  @file App.vue
  @brief App 组件
  Created by 林子怡
-->

<template>
  <el-config-provider :locale="zhCn">
    <router-view />
  </el-config-provider>
</template>

<script setup lang="ts">
import zhCn from 'element-plus/es/locale/lang/zh-cn'
</script>

<style lang="scss">
#app {
  width: 100%;
  height: 100vh;
  font-family: 'Inter', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Noto Sans SC', sans-serif;
  background: var(--m3-surface);
  color: var(--m3-on-surface);
}
</style>
