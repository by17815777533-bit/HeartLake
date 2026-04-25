<template>
  <div class="ops-gauge-meter">
    <div class="ops-gauge-meter__dial">
      <div class="ops-gauge-meter__track" />
      <div class="ops-gauge-meter__arc" />
      <span class="ops-gauge-meter__marker" :style="markerStyle" />
      <div class="ops-gauge-meter__mask" />
      <div class="ops-gauge-meter__content">
        <strong>{{ displayValue }}</strong>
        <span :style="{ color: labelColor }">{{ label }}</span>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'

const props = withDefaults(
  defineProps<{
    value: number
    max?: number
    label: string
    formatter?: (value: number) => string
  }>(),
  {
    max: 100,
    formatter: undefined,
  },
)

const ratio = computed(() => {
  if (!props.max) return 0
  return Math.max(0, Math.min(1, props.value / props.max))
})

const displayValue = computed(() =>
  props.formatter ? props.formatter(props.value) : String(props.value),
)
const labelColor = computed(() => {
  if (ratio.value < 0.34) return '#f08283'
  if (ratio.value < 0.67) return '#e0a74f'
  return '#61baaa'
})

const markerStyle = computed(() => {
  const angle = Math.PI * (1 - ratio.value)
  const radius = 80
  const centerX = 104
  const centerY = 106

  return {
    left: `${centerX + radius * Math.cos(angle)}px`,
    top: `${centerY - radius * Math.sin(angle)}px`,
    '--marker-color': labelColor.value,
  }
})
</script>

<style scoped lang="scss">
.ops-gauge-meter {
  display: grid;
  place-items: center;
}

.ops-gauge-meter__dial {
  position: relative;
  width: min(214px, 100%);
  aspect-ratio: 1 / 0.72;
  overflow: hidden;
  animation: gauge-settle 460ms ease-out both;
}

.ops-gauge-meter__track,
.ops-gauge-meter__arc {
  position: absolute;
  left: 50%;
  bottom: 0;
  width: 198px;
  height: 198px;
  transform: translateX(-50%);
  border-radius: 50%;
  -webkit-mask: radial-gradient(
    circle at 50% 50%,
    transparent 0 56%,
    #000 57% 69%,
    transparent 70% 100%
  );
  mask: radial-gradient(circle at 50% 50%, transparent 0 56%, #000 57% 69%, transparent 70% 100%);
}

.ops-gauge-meter__track {
  background: conic-gradient(
    from 180deg at 50% 50%,
    rgba(220, 228, 243, 0.96) 0 180deg,
    transparent 180deg 360deg
  );
}

.ops-gauge-meter__arc {
  background: conic-gradient(
    from 180deg at 50% 50%,
    #f07b7d 0 38deg,
    #efad59 38deg 90deg,
    #f1ce67 90deg 118deg,
    #8fd7a5 118deg 148deg,
    #79d2c3 148deg 180deg,
    transparent 180deg 360deg
  );
}

.ops-gauge-meter__mask {
  position: absolute;
  left: 50%;
  bottom: 12px;
  width: 142px;
  height: 142px;
  transform: translateX(-50%);
  border-radius: 50%;
  background: #ffffff;
  box-shadow: 0 0 0 1px #e2e8f0;
}

.ops-gauge-meter__marker {
  position: absolute;
  z-index: 2;
  width: 16px;
  height: 16px;
  border-radius: 50%;
  background: #ffffff;
  border: 4px solid var(--marker-color);
  box-shadow: 0 10px 20px rgba(119, 142, 191, 0.2);
  transform: translate(-50%, -50%);
  transition:
    left 260ms ease,
    top 260ms ease,
    border-color 220ms ease;
}

.ops-gauge-meter__content {
  position: absolute;
  inset: auto 0 10px;
  z-index: 2;
  display: grid;
  gap: 4px;
  justify-items: center;

  strong {
    color: var(--hl-ink);
    font-size: 36px;
    font-weight: 800;
    line-height: 1;
    letter-spacing: 0;
  }

  span {
    font-size: 14px;
    font-weight: 700;
  }
}

@keyframes gauge-settle {
  from {
    opacity: 0;
    transform: translateY(8px);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}
</style>
