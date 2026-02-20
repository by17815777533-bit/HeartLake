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

<script setup>
import zhCn from 'element-plus/dist/locale/zh-cn.mjs'
</script>

<style>
#app {
  width: 100%;
  height: 100vh;
}
</style>
