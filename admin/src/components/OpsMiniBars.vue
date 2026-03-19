<template>
  <div class="ops-mini-bars">
    <div
      v-for="item in normalizedItems"
      :key="item.label"
      class="ops-mini-bars__item"
    >
      <div class="ops-mini-bars__track">
        <span :style="{ height: `${item.height}%` }" />
      </div>
      <strong>{{ item.display }}</strong>
      <small>{{ item.label }}</small>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'

interface OpsMiniBarItem {
  label: string
  value: number
  display?: string
}

const props = defineProps<{
  items: OpsMiniBarItem[]
}>()

const normalizedItems = computed(() => {
  const source = props.items.filter((item) => Number.isFinite(Number(item.value)))
  const max = Math.max(...source.map((item) => Number(item.value || 0)), 1)

  return source.map((item) => ({
    ...item,
    height: Math.max(18, Math.round((Number(item.value || 0) / max) * 100)),
    display: item.display || String(item.value),
  }))
})
</script>

<style scoped lang="scss">
.ops-mini-bars {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(52px, 1fr));
  gap: 10px;
  align-items: end;
  min-height: 182px;
}

.ops-mini-bars__item {
  display: grid;
  gap: 8px;
  justify-items: center;
}

.ops-mini-bars__track {
  width: 100%;
  height: 108px;
  display: flex;
  align-items: end;
  padding: 0 8px;

  span {
    width: 100%;
    border-radius: 999px;
    background: linear-gradient(180deg, rgba(144, 175, 255, 0.96), rgba(125, 157, 255, 0.96));
    box-shadow: 0 10px 22px rgba(125, 157, 255, 0.2);
  }
}

.ops-mini-bars__item strong {
  color: var(--hl-ink);
  font-size: 13px;
  font-weight: 700;
}

.ops-mini-bars__item small {
  color: var(--hl-ink-soft);
  font-size: 11px;
}
</style>
