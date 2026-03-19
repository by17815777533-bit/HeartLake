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
  grid-template-columns: repeat(auto-fit, minmax(230px, 1fr));
  gap: 14px;
  margin-bottom: 20px;
}

.ops-signal-deck__card {
  position: relative;
  min-height: 156px;
  padding: 18px 20px;
  border-radius: 28px;
  border: 1px solid rgba(121, 110, 95, 0.08);
  background:
    radial-gradient(circle at 88% 18%, rgba(255, 255, 255, 0.92), rgba(255, 255, 255, 0) 30%),
    linear-gradient(180deg, rgba(255, 255, 255, 0.88), rgba(247, 243, 236, 0.96));
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.96),
    0 20px 40px rgba(83, 71, 56, 0.06);
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
    margin-top: 18px;
    color: var(--hl-ink);
    font-family: var(--hl-font-display);
    font-size: clamp(24px, 3vw, 34px);
    font-weight: 700;
    line-height: 1.04;
    letter-spacing: -0.03em;
  }

  p {
    position: relative;
    z-index: 1;
    max-width: 22ch;
    margin: 10px 0 0;
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
  --signal-accent: #6d8876;
  --signal-chip: rgba(109, 136, 118, 0.12);
  --signal-outline: rgba(109, 136, 118, 0.14);
}

.is-amber {
  --signal-accent: #b1906e;
  --signal-chip: rgba(177, 144, 110, 0.14);
  --signal-outline: rgba(177, 144, 110, 0.14);
}

.is-sage {
  --signal-accent: #5b7665;
  --signal-chip: rgba(91, 118, 101, 0.14);
  --signal-outline: rgba(91, 118, 101, 0.14);
}

.is-rose {
  --signal-accent: #a37a72;
  --signal-chip: rgba(163, 122, 114, 0.14);
  --signal-outline: rgba(163, 122, 114, 0.14);
}

@media (max-width: 640px) {
  .ops-signal-deck {
    grid-template-columns: 1fr;
  }
}
</style>
