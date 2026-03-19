<template>
  <div class="ops-mini-bars">
    <div
      v-for="item in normalizedItems"
      :key="item.label"
      class="ops-mini-bars__item"
      :class="{ 'is-peak': item.isPeak }"
    >
      <strong>{{ item.display }}</strong>
      <div class="ops-mini-bars__track">
        <span :style="{ height: `${item.height}%` }" />
      </div>
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
    isPeak: Number(item.value || 0) === max,
  }))
})
</script>

<style scoped lang="scss">
.ops-mini-bars {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(58px, 1fr));
  gap: 12px;
  align-items: stretch;
  min-height: 182px;
}

.ops-mini-bars__item {
  display: grid;
  grid-template-rows: auto 1fr auto;
  gap: 8px;
  justify-items: center;
  min-width: 0;
  padding: 12px 8px 14px;
  border-radius: 24px;
  border: 1px solid rgba(151, 171, 219, 0.16);
  background: linear-gradient(180deg, rgba(239, 245, 255, 0.96), rgba(229, 239, 255, 0.98));
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.88),
    0 14px 24px rgba(134, 160, 220, 0.1);
}

.ops-mini-bars__track {
  position: relative;
  width: 100%;
  height: 108px;
  display: flex;
  align-items: end;
  justify-content: center;
  padding: 10px 0 4px;

  &::before {
    content: '';
    position: absolute;
    inset: 0;
    margin: auto;
    width: 30px;
    height: 100%;
    border-radius: 999px;
    background: linear-gradient(180deg, rgba(255, 255, 255, 0.72), rgba(236, 242, 255, 0.44));
    box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.92);
  }

  span {
    position: relative;
    z-index: 1;
    width: 22px;
    border-radius: 999px;
    background: linear-gradient(180deg, #8fb1ff, #7d9ff4);
    box-shadow: 0 14px 22px rgba(126, 156, 241, 0.24);
  }
}

.ops-mini-bars__item strong {
  min-width: 100%;
  padding: 7px 8px;
  border-radius: 999px;
  background: rgba(255, 255, 255, 0.78);
  color: #1f2d44;
  font-size: 12px;
  font-weight: 700;
  line-height: 1;
  text-align: center;
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.9);
}

.ops-mini-bars__item small {
  color: var(--hl-ink-soft);
  font-size: 11px;
  text-align: center;
}

.ops-mini-bars__item:nth-child(even) .ops-mini-bars__track span {
  background: linear-gradient(180deg, rgba(255, 255, 255, 0.98), rgba(238, 243, 255, 0.96));
  box-shadow:
    inset 0 -1px 0 rgba(133, 158, 220, 0.22),
    0 12px 18px rgba(255, 255, 255, 0.4);
}

.ops-mini-bars__item.is-peak {
  background: linear-gradient(180deg, rgba(228, 238, 255, 0.98), rgba(214, 229, 255, 0.98));
}

.ops-mini-bars__item.is-peak .ops-mini-bars__track span {
  width: 24px;
  background: linear-gradient(180deg, #86adff, #7097f2);
}
</style>
