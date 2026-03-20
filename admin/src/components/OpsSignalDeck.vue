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
        <span v-if="item.badge" class="ops-signal-deck__badge">
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
  grid-template-columns: repeat(auto-fit, minmax(230px, 1fr));
  gap: 12px;
  margin-bottom: 18px;
}

.ops-signal-deck__card {
  position: relative;
  min-height: 136px;
  padding: 16px 18px;
  border-radius: 24px;
  border: 1px solid rgba(133, 156, 201, 0.12);
  background:
    radial-gradient(circle at 88% 18%, rgba(255, 255, 255, 0.92), rgba(255, 255, 255, 0) 30%),
    linear-gradient(180deg, rgba(255, 255, 255, 0.9), rgba(237, 245, 255, 0.96));
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.96),
    0 20px 40px rgba(94, 118, 166, 0.1);
  overflow: hidden;

  &::before {
    content: '';
    position: absolute;
    right: 18px;
    top: 16px;
    width: 84px;
    height: 46px;
    border-radius: 999px;
    background: linear-gradient(180deg, rgba(255, 255, 255, 0.4), rgba(255, 255, 255, 0));
    box-shadow: inset 0 0 0 1px var(--signal-outline, rgba(109, 136, 118, 0.12));
  }

  &::after {
    content: '';
    position: absolute;
    right: 26px;
    top: 30px;
    width: 58px;
    height: 18px;
    border-bottom: 3px solid var(--signal-accent, #6d8876);
    border-radius: 0 0 30px 30px;
    transform: skewX(-20deg);
    opacity: 0.56;
  }

  strong {
    position: relative;
    z-index: 1;
    display: block;
    margin-top: 14px;
    color: var(--hl-ink);
    font-family: var(--hl-font-display);
    font-size: clamp(22px, 2.6vw, 30px);
    font-weight: 700;
    line-height: 1.04;
    letter-spacing: -0.03em;
  }

  p {
    position: relative;
    z-index: 1;
    max-width: 24ch;
    margin: 8px 0 0;
    color: var(--hl-ink-soft);
    font-size: 12px;
    line-height: 1.6;
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
  font-size: 10px;
  font-weight: 700;
  letter-spacing: 0.12em;
  text-transform: uppercase;
}

.ops-signal-deck__label {
  padding: 0 10px;
  background: var(--signal-chip, rgba(109, 136, 118, 0.12));
  color: var(--signal-accent, #6d8876);
}

.ops-signal-deck__badge {
  padding: 0 10px;
  background: rgba(255, 255, 255, 0.72);
  border: 1px solid var(--signal-outline, rgba(109, 136, 118, 0.12));
  color: var(--hl-ink-soft);
}

.is-lake {
  --signal-accent: #87abff;
  --signal-chip: rgba(135, 171, 255, 0.14);
  --signal-outline: rgba(135, 171, 255, 0.14);
}

.is-amber {
  --signal-accent: #8ec6ff;
  --signal-chip: rgba(142, 198, 255, 0.14);
  --signal-outline: rgba(142, 198, 255, 0.14);
}

.is-sage {
  --signal-accent: #62b9ac;
  --signal-chip: rgba(98, 185, 172, 0.14);
  --signal-outline: rgba(98, 185, 172, 0.14);
}

.is-rose {
  --signal-accent: #ea8c93;
  --signal-chip: rgba(234, 140, 147, 0.14);
  --signal-outline: rgba(234, 140, 147, 0.14);
}

@media (max-width: 640px) {
  .ops-signal-deck {
    grid-template-columns: 1fr;
  }
}
</style>
