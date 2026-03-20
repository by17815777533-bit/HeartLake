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
      <p v-if="item.note" class="ops-metric-strip__note">
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
  gap: 12px;
  margin-bottom: 18px;
}

.ops-metric-strip__card {
  position: relative;
  min-height: 112px;
  padding: 16px 18px;
  border-radius: 24px;
  border: 1px solid rgba(133, 156, 201, 0.12);
  background:
    radial-gradient(circle at 88% 22%, rgba(255, 255, 255, 0.92), rgba(255, 255, 255, 0) 32%),
    linear-gradient(180deg, rgba(255, 255, 255, 0.92), rgba(237, 245, 255, 0.96));
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.96),
    0 18px 36px rgba(94, 118, 166, 0.1);
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
    background: linear-gradient(180deg, transparent 0 50%, var(--metric-accent, #6d8876) 50% 100%);
    mask: linear-gradient(
      90deg,
      #000 0 8px,
      transparent 8px 13px,
      #000 13px 21px,
      transparent 21px 26px,
      #000 26px 34px,
      transparent 34px 39px,
      #000 39px 47px,
      transparent 47px 52px
    );
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
  margin-top: 12px;

  strong {
    display: block;
    color: var(--hl-ink);
    font-family: var(--hl-font-display);
    font-size: clamp(28px, 2.7vw, 34px);
    font-weight: 700;
    line-height: 1;
    letter-spacing: -0.03em;
  }
}

.ops-metric-strip__note {
  max-width: 20ch;
  margin: 8px 0 0;
  color: var(--hl-ink-soft);
  font-size: 12px;
  line-height: 1.55;
}

.is-lake {
  --metric-accent: #87abff;
  --metric-chip: rgba(135, 171, 255, 0.14);
  --metric-outline: rgba(135, 171, 255, 0.12);
}

.is-amber {
  --metric-accent: #8cc5ff;
  --metric-chip: rgba(140, 197, 255, 0.14);
  --metric-outline: rgba(140, 197, 255, 0.12);
}

.is-sage {
  --metric-accent: #62b9ac;
  --metric-chip: rgba(98, 185, 172, 0.14);
  --metric-outline: rgba(98, 185, 172, 0.12);
}

.is-rose {
  --metric-accent: #ea8c93;
  --metric-chip: rgba(234, 140, 147, 0.14);
  --metric-outline: rgba(234, 140, 147, 0.12);
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
