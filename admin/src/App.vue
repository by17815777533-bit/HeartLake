<!--
  应用根组件 -- 注入 Element Plus 中文语言包，挂载全局路由出口

  el-config-provider 确保所有 Element Plus 组件（分页、日期选择器等）
  默认使用中文文案，无需在每个组件单独配置。
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
  font-family: var(--hl-font-body);
  background: transparent;
  color: var(--m3-on-surface);
}
</style>
