<template>
  <section class="ops-page-hero">
    <div class="ops-page-hero__main">
      <span class="ops-page-hero__eyebrow">{{ eyebrow }}</span>
      <div class="ops-page-hero__title-row">
        <h1>{{ title }}</h1>
        <span
          v-if="status"
          class="ops-page-hero__status"
        >
          <i />
          {{ status }}
        </span>
      </div>
      <p class="ops-page-hero__description">
        {{ description }}
      </p>
    </div>

    <div
      v-if="chips?.length || $slots.actions"
      class="ops-page-hero__side"
    >
      <div
        v-if="chips?.length"
        class="ops-page-hero__chips"
      >
        <span
          v-for="chip in chips"
          :key="chip"
          class="ops-page-hero__chip"
        >
          {{ chip }}
        </span>
      </div>

      <div
        v-if="$slots.actions"
        class="ops-page-hero__actions"
      >
        <slot name="actions" />
      </div>
    </div>
  </section>
</template>

<script setup lang="ts">
defineProps<{
  eyebrow: string
  title: string
  description: string
  status?: string
  chips?: string[]
}>()
</script>

<style scoped lang="scss">
.ops-page-hero {
  position: relative;
  display: grid;
  grid-template-columns: minmax(0, 1fr) auto;
  gap: 18px 22px;
  align-items: start;
  padding: 28px;
  margin-bottom: 20px;
  border-radius: 30px;
  border: 1px solid rgba(133, 156, 201, 0.12);
  background:
    radial-gradient(circle at 92% 16%, rgba(147, 221, 209, 0.16), transparent 18%),
    linear-gradient(180deg, rgba(255, 255, 255, 0.88), rgba(237, 245, 255, 0.96));
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.96),
    0 20px 46px rgba(94, 118, 166, 0.1);
}

.ops-page-hero__main {
  min-width: 0;
}

.ops-page-hero__eyebrow {
  display: inline-flex;
  width: fit-content;
  min-height: 30px;
  align-items: center;
  padding: 0 12px;
  border-radius: 999px;
  background: rgba(237, 245, 255, 0.94);
  color: var(--hl-ink-soft);
  font-size: 11px;
  letter-spacing: 0.14em;
  text-transform: uppercase;
}

.ops-page-hero__title-row {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 12px;
  margin-top: 12px;

  h1 {
    margin: 0;
    color: var(--hl-ink);
    font-size: clamp(30px, 4vw, 40px);
    line-height: 1.04;
    font-weight: 700;
    letter-spacing: -0.03em;
  }
}

.ops-page-hero__status {
  display: inline-flex;
  align-items: center;
  gap: 8px;
  min-height: 34px;
  padding: 0 14px;
  border-radius: 999px;
  background: rgba(147, 221, 209, 0.16);
  color: #4d8f86;
  font-size: 11px;
  font-weight: 700;
  letter-spacing: 0.14em;
  text-transform: uppercase;

  i {
    width: 8px;
    height: 8px;
    border-radius: 999px;
    background: #62b9ac;
    box-shadow: 0 0 0 6px rgba(98, 185, 172, 0.12);
  }
}

.ops-page-hero__description {
  max-width: 58rem;
  margin: 12px 0 0;
  color: var(--hl-ink-soft);
  font-size: 15px;
  line-height: 1.8;
}

.ops-page-hero__side {
  display: grid;
  gap: 14px;
  justify-items: end;
  min-width: min(360px, 100%);
}

.ops-page-hero__chips {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
  justify-content: flex-end;
}

.ops-page-hero__chip {
  display: inline-flex;
  align-items: center;
  min-height: 34px;
  padding: 0 14px;
  border-radius: 999px;
  background: rgba(237, 245, 255, 0.94);
  color: var(--hl-ink);
  font-size: 12px;
  font-weight: 600;
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.9);
}

.ops-page-hero__actions {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
  justify-content: flex-end;
}

@media (max-width: 980px) {
  .ops-page-hero {
    grid-template-columns: 1fr;
  }

  .ops-page-hero__side,
  .ops-page-hero__chips,
  .ops-page-hero__actions {
    justify-items: start;
    justify-content: flex-start;
  }
}

@media (max-width: 640px) {
  .ops-page-hero {
    padding: 22px 18px;
    border-radius: 26px;
  }

  .ops-page-hero__title-row h1 {
    font-size: 28px;
  }
}
</style>
