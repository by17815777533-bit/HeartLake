<template>
  <div class="ops-gauge-meter">
    <div
      class="ops-gauge-meter__dial"
      :style="{ '--meter-angle': `${angle}deg` }"
    >
      <div class="ops-gauge-meter__mask" />
      <div class="ops-gauge-meter__content">
        <strong>{{ displayValue }}</strong>
        <span>{{ label }}</span>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'

const props = withDefaults(defineProps<{
  value: number
  max?: number
  label: string
  formatter?: (value: number) => string
}>(), {
  max: 100,
  formatter: undefined,
})

const ratio = computed(() => {
  if (!props.max) return 0
  return Math.max(0, Math.min(1, props.value / props.max))
})

const angle = computed(() => 180 * ratio.value)
const displayValue = computed(() => props.formatter ? props.formatter(props.value) : String(props.value))
</script>

<style scoped lang="scss">
.ops-gauge-meter {
  display: grid;
  place-items: center;
}

.ops-gauge-meter__dial {
  position: relative;
  width: min(240px, 100%);
  aspect-ratio: 1 / 0.68;
  overflow: hidden;
}

.ops-gauge-meter__dial::before {
  content: '';
  position: absolute;
  inset: 0;
  border-radius: 320px 320px 0 0;
  background:
    conic-gradient(
      from 180deg at 50% 100%,
      #ef6b6b 0 50deg,
      #f0b84d 50deg 110deg,
      #74cbb9 110deg var(--meter-angle),
      rgba(214, 226, 248, 0.9) var(--meter-angle) 180deg
    );
}

.ops-gauge-meter__mask {
  position: absolute;
  left: 50%;
  bottom: -6%;
  width: 72%;
  aspect-ratio: 1 / 1;
  transform: translateX(-50%);
  border-radius: 50%;
  background: rgba(239, 246, 255, 0.98);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.9);
}

.ops-gauge-meter__content {
  position: absolute;
  left: 50%;
  bottom: 10px;
  transform: translateX(-50%);
  display: grid;
  gap: 8px;
  justify-items: center;

  strong {
    color: var(--hl-ink);
    font-size: clamp(34px, 4vw, 48px);
    font-weight: 700;
    line-height: 1;
    letter-spacing: -0.04em;
  }

  span {
    color: var(--hl-ink-soft);
    font-size: 14px;
    font-weight: 600;
  }
}
</style>
