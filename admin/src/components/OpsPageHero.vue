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
  gap: 18px;
  padding: 28px 30px;
  margin-bottom: 22px;
  border-radius: 30px;
  border: 1px solid rgba(140, 161, 171, 0.18);
  background:
    radial-gradient(circle at right top, rgba(123, 160, 173, 0.14), transparent 28%),
    linear-gradient(145deg, rgba(249, 251, 252, 0.88), rgba(239, 244, 246, 0.96));
  box-shadow: 0 24px 56px rgba(11, 25, 33, 0.08);
  overflow: hidden;

  &::after {
    content: '';
    position: absolute;
    right: -42px;
    top: -68px;
    width: 220px;
    height: 220px;
    border-radius: 999px;
    border: 1px solid rgba(17, 62, 74, 0.08);
    box-shadow:
      0 0 0 24px rgba(17, 62, 74, 0.03),
      0 0 0 48px rgba(17, 62, 74, 0.02);
    pointer-events: none;
  }
}

.ops-page-hero__main {
  position: relative;
  z-index: 1;
}

.ops-page-hero__eyebrow {
  display: inline-flex;
  width: fit-content;
  min-height: 30px;
  align-items: center;
  padding: 0 12px;
  border-radius: 999px;
  background: rgba(17, 62, 74, 0.08);
  color: var(--m3-primary);
  font-family: var(--hl-font-mono);
  font-size: 11px;
  letter-spacing: 0.16em;
  text-transform: uppercase;
}

.ops-page-hero__title-row {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 14px;
  margin-top: 14px;

  h1 {
    margin: 0;
    font-family: var(--hl-font-display);
    font-size: clamp(30px, 4vw, 42px);
    line-height: 1.02;
    color: var(--hl-ink);
    letter-spacing: 0.02em;
  }
}

.ops-page-hero__status {
  display: inline-flex;
  align-items: center;
  gap: 8px;
  min-height: 34px;
  padding: 0 14px;
  border-radius: 999px;
  background: rgba(255, 255, 255, 0.72);
  border: 1px solid rgba(17, 62, 74, 0.1);
  color: var(--hl-ink-soft);
  font-family: var(--hl-font-mono);
  font-size: 11px;
  letter-spacing: 0.12em;
  text-transform: uppercase;

  i {
    width: 8px;
    height: 8px;
    border-radius: 999px;
    background: #4d8f6b;
    box-shadow: 0 0 0 6px rgba(77, 143, 107, 0.12);
  }
}

.ops-page-hero__description {
  max-width: 56rem;
  margin: 12px 0 0;
  color: var(--hl-ink-soft);
  font-size: 15px;
  line-height: 1.8;
}

.ops-page-hero__chips {
  position: relative;
  z-index: 1;
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
}

.ops-page-hero__chip {
  display: inline-flex;
  align-items: center;
  min-height: 34px;
  padding: 0 14px;
  border-radius: 999px;
  background: rgba(255, 255, 255, 0.68);
  border: 1px solid rgba(140, 161, 171, 0.16);
  color: var(--hl-ink);
  font-size: 12px;
  font-weight: 600;
}

.ops-page-hero__actions {
  position: relative;
  z-index: 1;
  display: flex;
  flex-wrap: wrap;
  gap: 12px;
}

@media (max-width: 720px) {
  .ops-page-hero {
    padding: 22px 18px;
  }

  .ops-page-hero__title-row h1 {
    font-size: 30px;
  }
}
</style>
