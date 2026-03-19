<template>
  <section class="ops-signal-deck">
    <article
      v-for="item in items"
      :key="`${item.label}-${item.value}`"
      class="ops-signal-deck__card"
      :class="item.tone ? `is-${item.tone}` : 'is-lake'"
    >
      <div class="ops-signal-deck__top">
        <span class="ops-signal-deck__label">{{ item.label }}</span>
        <span
          v-if="item.badge"
          class="ops-signal-deck__badge"
        >
          {{ item.badge }}
        </span>
      </div>
      <strong>{{ item.value }}</strong>
      <p>{{ item.note }}</p>
    </article>
  </section>
</template>

<script setup lang="ts">
interface OpsSignalItem {
  label: string
  value: string | number
  note: string
  badge?: string
  tone?: 'lake' | 'amber' | 'sage' | 'rose'
}

defineProps<{
  items: OpsSignalItem[]
}>()
</script>

<style scoped lang="scss">
.ops-signal-deck {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
  gap: 14px;
  margin-bottom: 20px;
}

.ops-signal-deck__card {
  position: relative;
  min-height: 148px;
  padding: 20px 18px 18px;
  border-radius: 26px;
  border: 1px solid rgba(120, 146, 157, 0.16);
  background:
    radial-gradient(circle at 90% 12%, rgba(255, 255, 255, 0.42), transparent 28%),
    linear-gradient(165deg, rgba(251, 252, 252, 0.96), rgba(237, 244, 246, 0.94));
  box-shadow: 0 18px 38px rgba(10, 23, 31, 0.06);
  overflow: hidden;

  &::before {
    content: '';
    position: absolute;
    inset: auto 18px 0;
    height: 1px;
    background: linear-gradient(90deg, transparent, rgba(17, 62, 74, 0.16), transparent);
  }

  &::after {
    content: '';
    position: absolute;
    right: -16px;
    top: -12px;
    width: 96px;
    height: 96px;
    border-radius: 999px;
    background: var(--signal-glow, rgba(17, 62, 74, 0.08));
    filter: blur(1px);
    opacity: 0.9;
  }

  strong {
    position: relative;
    z-index: 1;
    display: block;
    margin-top: 18px;
    font-family: var(--hl-font-display);
    font-size: clamp(24px, 3vw, 34px);
    line-height: 1.05;
    color: var(--hl-ink);
    letter-spacing: 0.01em;
  }

  p {
    position: relative;
    z-index: 1;
    margin: 12px 0 0;
    color: var(--hl-ink-soft);
    font-size: 13px;
    line-height: 1.7;
  }
}

.ops-signal-deck__top {
  position: relative;
  z-index: 1;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
}

.ops-signal-deck__label,
.ops-signal-deck__badge {
  display: inline-flex;
  align-items: center;
  min-height: 28px;
  border-radius: 999px;
  font-family: var(--hl-font-mono);
  font-size: 10px;
  letter-spacing: 0.14em;
  text-transform: uppercase;
}

.ops-signal-deck__label {
  padding: 0 10px;
  background: var(--signal-chip, rgba(17, 62, 74, 0.08));
  color: var(--signal-accent, #113e4a);
}

.ops-signal-deck__badge {
  padding: 0 10px;
  border: 1px solid var(--signal-outline, rgba(17, 62, 74, 0.12));
  color: var(--hl-ink-soft);
  background: rgba(255, 255, 255, 0.58);
}

.is-lake {
  --signal-accent: #113e4a;
  --signal-chip: rgba(17, 62, 74, 0.08);
  --signal-outline: rgba(17, 62, 74, 0.12);
  --signal-glow: rgba(17, 62, 74, 0.08);
}

.is-amber {
  --signal-accent: #b67a42;
  --signal-chip: rgba(182, 122, 66, 0.12);
  --signal-outline: rgba(182, 122, 66, 0.14);
  --signal-glow: rgba(182, 122, 66, 0.1);
}

.is-sage {
  --signal-accent: #49735a;
  --signal-chip: rgba(73, 115, 90, 0.12);
  --signal-outline: rgba(73, 115, 90, 0.16);
  --signal-glow: rgba(73, 115, 90, 0.1);
}

.is-rose {
  --signal-accent: #9e5b5b;
  --signal-chip: rgba(158, 91, 91, 0.12);
  --signal-outline: rgba(158, 91, 91, 0.14);
  --signal-glow: rgba(158, 91, 91, 0.1);
}

@media (max-width: 640px) {
  .ops-signal-deck {
    grid-template-columns: 1fr;
  }
}
</style>
