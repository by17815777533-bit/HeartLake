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
  font-family: 'Inter', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Noto Sans SC', sans-serif;
}

/* 全局 Element Plus 暗色模式下的光遇风格覆盖 */
html.dark .el-button--primary {
  --el-button-bg-color: #F2CC8F;
  --el-button-border-color: #F2CC8F;
  --el-button-text-color: #1A0A00;
  --el-button-hover-bg-color: #F7DDB4;
  --el-button-hover-border-color: #F7DDB4;
  --el-button-hover-text-color: #1A0A00;
  --el-button-active-bg-color: #D4A55A;
  --el-button-active-border-color: #D4A55A;
}

html.dark .el-button--success {
  --el-button-bg-color: #81B29A;
  --el-button-border-color: #81B29A;
  --el-button-text-color: #0A1A12;
  --el-button-hover-bg-color: #A8D5BA;
  --el-button-hover-border-color: #A8D5BA;
}

html.dark .el-button--danger {
  --el-button-bg-color: #E07A5F;
  --el-button-border-color: #E07A5F;
  --el-button-text-color: #1A0500;
  --el-button-hover-bg-color: #F4A594;
  --el-button-hover-border-color: #F4A594;
}

html.dark .el-button--warning {
  --el-button-bg-color: #E8A87C;
  --el-button-border-color: #E8A87C;
  --el-button-text-color: #1A0A00;
  --el-button-hover-bg-color: #F0C4A3;
  --el-button-hover-border-color: #F0C4A3;
}

html.dark .el-button--info {
  --el-button-bg-color: #7B68AE;
  --el-button-border-color: #7B68AE;
  --el-button-text-color: #F0E6D3;
  --el-button-hover-bg-color: #A594D0;
  --el-button-hover-border-color: #A594D0;
}

/* 暗色模式 Dialog */
html.dark .el-dialog {
  background: rgba(26, 26, 62, 0.95);
  backdrop-filter: blur(20px);
  border: 1px solid rgba(242, 204, 143, 0.1);
  border-radius: 16px;
}

html.dark .el-dialog__header {
  border-bottom: 1px solid rgba(242, 204, 143, 0.08);
}

html.dark .el-dialog__title {
  color: #F2CC8F;
}

/* 暗色模式 Message */
html.dark .el-message {
  background: rgba(26, 26, 62, 0.9);
  backdrop-filter: blur(16px);
  border: 1px solid rgba(242, 204, 143, 0.12);
  border-radius: 12px;
}

/* 暗色模式 Input */
html.dark .el-input__wrapper {
  background: rgba(10, 10, 26, 0.5);
  border-radius: 10px;
  box-shadow: 0 0 0 1px rgba(242, 204, 143, 0.12) inset;
  transition: all 0.3s ease;
}

html.dark .el-input__wrapper:hover {
  box-shadow: 0 0 0 1px rgba(242, 204, 143, 0.25) inset;
}

html.dark .el-input__wrapper.is-focus {
  box-shadow: 0 0 0 1px rgba(242, 204, 143, 0.5) inset, 0 0 12px rgba(242, 204, 143, 0.1);
}

html.dark .el-input__inner {
  color: #F0E6D3;
}

html.dark .el-input__inner::placeholder {
  color: #7A6F63;
}

/* 暗色模式 Select */
html.dark .el-select-dropdown {
  background: rgba(26, 26, 62, 0.95);
  backdrop-filter: blur(16px);
  border: 1px solid rgba(242, 204, 143, 0.1);
  border-radius: 12px;
}

html.dark .el-select-dropdown__item.is-hovering {
  background: rgba(242, 204, 143, 0.08);
}

html.dark .el-select-dropdown__item.is-selected {
  color: #F2CC8F;
}

/* 暗色模式 Tabs */
html.dark .el-tabs__item {
  color: #B8A99A;
}

html.dark .el-tabs__item.is-active {
  color: #F2CC8F;
}

html.dark .el-tabs__active-bar {
  background: linear-gradient(90deg, #F2CC8F, #E8A87C);
}

html.dark .el-tabs__nav-wrap::after {
  background: rgba(242, 204, 143, 0.06);
}

/* 暗色模式 Tag */
html.dark .el-tag {
  border-radius: 8px;
}

/* 暗色模式 Pagination */
html.dark .el-pagination {
  --el-pagination-bg-color: transparent;
  --el-pagination-button-bg-color: rgba(26, 26, 62, 0.6);
  --el-pagination-hover-color: #F2CC8F;
}

html.dark .el-pager li.is-active {
  background: rgba(242, 204, 143, 0.15);
  color: #F2CC8F;
}

/* 暗色模式 Switch */
html.dark .el-switch.is-checked .el-switch__core {
  background-color: #F2CC8F;
  border-color: #F2CC8F;
}

/* 暗色模式 Progress */
html.dark .el-progress-bar__outer {
  background: rgba(242, 204, 143, 0.08);
}

/* 暗色模式 Form */
html.dark .el-form-item__label {
  color: #B8A99A;
}

/* 暗色模式 Breadcrumb */
html.dark .el-breadcrumb__inner {
  color: #7A6F63;
}

html.dark .el-breadcrumb__inner.is-link:hover {
  color: #F2CC8F;
}

html.dark .el-breadcrumb__separator {
  color: #5A5048;
}

/* 暗色模式 MessageBox */
html.dark .el-message-box {
  background: rgba(26, 26, 62, 0.95);
  backdrop-filter: blur(20px);
  border: 1px solid rgba(242, 204, 143, 0.1);
  border-radius: 16px;
}

/* 暗色模式 Popconfirm */
html.dark .el-popconfirm {
  background: rgba(26, 26, 62, 0.95);
}

/* 暗色模式 DatePicker */
html.dark .el-date-editor {
  --el-input-bg-color: rgba(10, 10, 26, 0.5);
}

/* 暗色模式 Tooltip */
html.dark .el-tooltip__trigger:focus-visible {
  outline: 2px solid rgba(242, 204, 143, 0.5);
}

/* 暗色模式 Loading */
html.dark .el-loading-mask {
  background: rgba(10, 10, 26, 0.7);
}

html.dark .el-loading-spinner .circular .path {
  stroke: #F2CC8F;
}
</style>
