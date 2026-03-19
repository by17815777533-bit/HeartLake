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
  grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
  gap: 14px;
  margin-bottom: 20px;
}

.ops-metric-strip__card {
  position: relative;
  min-height: 142px;
  padding: 20px 18px 18px;
  border-radius: 24px;
  border: 1px solid rgba(115, 141, 151, 0.14);
  background:
    radial-gradient(circle at right top, rgba(255, 255, 255, 0.32), transparent 34%),
    linear-gradient(160deg, rgba(250, 252, 252, 0.96), rgba(239, 244, 246, 0.96));
  box-shadow: 0 20px 42px rgba(10, 23, 31, 0.06);
  overflow: hidden;

  &::before {
    content: '';
    position: absolute;
    inset: 0 auto auto 0;
    width: 100%;
    height: 4px;
    background: var(--metric-accent, #113e4a);
  }
}

.ops-metric-strip__label {
  display: inline-flex;
  align-items: center;
  min-height: 28px;
  padding: 0 10px;
  border-radius: 999px;
  background: var(--metric-chip, rgba(17, 62, 74, 0.08));
  color: var(--metric-accent, #113e4a);
  font-family: var(--hl-font-mono);
  font-size: 10px;
  letter-spacing: 0.14em;
  text-transform: uppercase;
}

.ops-metric-strip__value-row {
  margin-top: 18px;

  strong {
    display: block;
    font-family: var(--hl-font-display);
    font-size: clamp(28px, 3vw, 36px);
    line-height: 1;
    color: var(--hl-ink);
  }
}

.ops-metric-strip__note {
  margin: 12px 0 0;
  color: var(--hl-ink-soft);
  font-size: 13px;
  line-height: 1.7;
}

.is-lake {
  --metric-accent: #113e4a;
  --metric-chip: rgba(17, 62, 74, 0.08);
}

.is-amber {
  --metric-accent: #b67a42;
  --metric-chip: rgba(182, 122, 66, 0.12);
}

.is-sage {
  --metric-accent: #49735a;
  --metric-chip: rgba(73, 115, 90, 0.12);
}

.is-rose {
  --metric-accent: #9e5b5b;
  --metric-chip: rgba(158, 91, 91, 0.12);
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

  .ops-metric-strip__card {
    min-height: 128px;
  }
}
</style>
