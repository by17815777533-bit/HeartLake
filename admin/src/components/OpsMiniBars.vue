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
  grid-template-columns: repeat(auto-fit, minmax(42px, 1fr));
  gap: 12px;
  align-items: end;
  min-height: 118px;
}

.ops-mini-bars__item {
  display: grid;
  grid-template-rows: auto 1fr auto;
  gap: 8px;
  justify-items: center;
  min-width: 0;
  transition: transform 180ms ease;

  &:hover {
    transform: translateY(-2px);
  }
}

.ops-mini-bars__track {
  position: relative;
  width: 30px;
  height: 74px;
  display: flex;
  align-items: end;
  justify-content: center;
  padding: 4px 0;
  border-radius: 999px;
  background: linear-gradient(180deg, rgba(255, 255, 255, 0.76), rgba(236, 242, 255, 0.5));
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.94),
    inset 0 -1px 0 rgba(152, 174, 224, 0.08);

  span {
    position: relative;
    z-index: 1;
    width: 22px;
    border-radius: 999px;
    background: linear-gradient(180deg, #8fb1ff, #7d9ff4);
    box-shadow: 0 14px 22px rgba(126, 156, 241, 0.24);
    transform-origin: bottom center;
    animation: bar-rise 420ms ease-out both;
  }
}

.ops-mini-bars__item strong {
  min-width: 34px;
  padding: 5px 8px;
  border-radius: 999px;
  background: rgba(255, 255, 255, 0.78);
  color: #1f2d44;
  font-size: 10px;
  font-weight: 700;
  line-height: 1;
  text-align: center;
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.9);
}

.ops-mini-bars__item small {
  color: var(--hl-ink-soft);
  font-size: 10px;
  text-align: center;
}

.ops-mini-bars__item:nth-child(even) .ops-mini-bars__track span {
  background: linear-gradient(180deg, rgba(255, 255, 255, 0.98), rgba(238, 243, 255, 0.96));
  box-shadow:
    inset 0 -1px 0 rgba(133, 158, 220, 0.22),
    0 12px 18px rgba(255, 255, 255, 0.4);
}

.ops-mini-bars__item.is-peak {
  transform: translateY(-2px);
}

.ops-mini-bars__item.is-peak .ops-mini-bars__track span {
  width: 24px;
  background: linear-gradient(180deg, #86adff, #7097f2);
}

@keyframes bar-rise {
  from {
    opacity: 0;
    transform: scaleY(0.45);
  }
  to {
    opacity: 1;
    transform: scaleY(1);
  }
}
</style>
