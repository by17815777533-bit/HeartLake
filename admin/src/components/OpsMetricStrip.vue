<template>
  <section class="ops-metric-strip">
    <article
      v-for="item in items"
      :key="item.label"
      class="ops-metric-strip__card"
      :class="item.tone ? `is-${item.tone}` : 'is-lake'"
    >
      <span class="ops-metric-strip__label">{{ item.label }}</span>
      <div class="ops-metric-strip__value-row">
        <strong>{{ item.value }}</strong>
      </div>
      <p
        v-if="item.note"
        class="ops-metric-strip__note"
      >
        {{ item.note }}
      </p>
    </article>
  </section>
</template>

<script setup lang="ts">
interface OpsMetricItem {
  label: string
  value: string | number
  note?: string
  tone?: 'lake' | 'amber' | 'sage' | 'rose'
}

defineProps<{
  items: OpsMetricItem[]
}>()
</script>

<style scoped lang="scss">
.ops-metric-strip {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(210px, 1fr));
  gap: 14px;
  margin-bottom: 20px;
}

.ops-metric-strip__card {
  position: relative;
  min-height: 126px;
  padding: 18px 20px;
  border-radius: 26px;
  border: 1px solid rgba(121, 110, 95, 0.08);
  background:
    radial-gradient(circle at 88% 22%, rgba(255, 255, 255, 0.92), rgba(255, 255, 255, 0) 32%),
    linear-gradient(180deg, rgba(255, 255, 255, 0.9), rgba(247, 243, 236, 0.96));
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.96),
    0 18px 36px rgba(83, 71, 56, 0.06);
  overflow: hidden;

  &::before {
    content: '';
    position: absolute;
    inset: 16px 16px auto auto;
    width: 70px;
    height: 36px;
    border-radius: 999px;
    background:
      linear-gradient(90deg, transparent 0 12%, rgba(255, 255, 255, 0.06) 12% 100%),
      linear-gradient(180deg, rgba(255, 255, 255, 0.1), rgba(255, 255, 255, 0));
    box-shadow: inset 0 0 0 1px var(--metric-outline, rgba(109, 136, 118, 0.12));
    opacity: 0.9;
  }

  &::after {
    content: '';
    position: absolute;
    right: 22px;
    top: 25px;
    width: 52px;
    height: 18px;
    background:
      linear-gradient(180deg, transparent 0 50%, var(--metric-accent, #6d8876) 50% 100%);
    mask: linear-gradient(90deg, #000 0 8px, transparent 8px 13px, #000 13px 21px, transparent 21px 26px, #000 26px 34px, transparent 34px 39px, #000 39px 47px, transparent 47px 52px);
    opacity: 0.65;
  }
}

.ops-metric-strip__label {
  display: inline-flex;
  align-items: center;
  min-height: 28px;
  padding: 0 10px;
  border-radius: 999px;
  background: var(--metric-chip, rgba(111, 136, 118, 0.12));
  color: var(--metric-accent, #6d8876);
  font-size: 10px;
  font-weight: 700;
  letter-spacing: 0.12em;
  text-transform: uppercase;
}

.ops-metric-strip__value-row {
  margin-top: 14px;

  strong {
    display: block;
    color: var(--hl-ink);
    font-family: var(--hl-font-display);
    font-size: clamp(30px, 3vw, 38px);
    font-weight: 700;
    line-height: 1;
    letter-spacing: -0.03em;
  }
}

.ops-metric-strip__note {
  max-width: 20ch;
  margin: 10px 0 0;
  color: var(--hl-ink-soft);
  font-size: 13px;
  line-height: 1.6;
}

.is-lake {
  --metric-accent: #6d8876;
  --metric-chip: rgba(109, 136, 118, 0.12);
  --metric-outline: rgba(109, 136, 118, 0.12);
}

.is-amber {
  --metric-accent: #b1906e;
  --metric-chip: rgba(177, 144, 110, 0.14);
  --metric-outline: rgba(177, 144, 110, 0.12);
}

.is-sage {
  --metric-accent: #5b7665;
  --metric-chip: rgba(91, 118, 101, 0.14);
  --metric-outline: rgba(91, 118, 101, 0.12);
}

.is-rose {
  --metric-accent: #a37a72;
  --metric-chip: rgba(163, 122, 114, 0.14);
  --metric-outline: rgba(163, 122, 114, 0.12);
}

@media (max-width: 720px) {
  .ops-metric-strip {
    grid-template-columns: 1fr 1fr;
  }
}

@media (max-width: 520px) {
  .ops-metric-strip {
    grid-template-columns: 1fr;
  }
}
</style>
