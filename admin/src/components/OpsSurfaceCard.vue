<template>
  <article
    class="ops-surface-card"
    :class="[
      `is-${tone}`,
      { 'is-compact': compact, 'has-media': Boolean($slots.media) },
    ]"
  >
    <div
      v-if="eyebrow || title || chip || $slots.actions"
      class="ops-surface-card__header"
    >
      <div class="ops-surface-card__titles">
        <span
          v-if="eyebrow"
          class="ops-surface-card__eyebrow"
        >
          {{ eyebrow }}
        </span>
        <h2
          v-if="title"
          class="ops-surface-card__title"
        >
          {{ title }}
        </h2>
      </div>

      <div class="ops-surface-card__tools">
        <span
          v-if="chip"
          class="ops-surface-card__chip"
        >
          {{ chip }}
        </span>
        <div
          v-if="$slots.actions"
          class="ops-surface-card__actions"
        >
          <slot name="actions" />
        </div>
      </div>
    </div>

    <div class="ops-surface-card__content">
      <div class="ops-surface-card__body">
        <slot />
      </div>
      <div
        v-if="$slots.media"
        class="ops-surface-card__media"
      >
        <slot name="media" />
      </div>
    </div>
  </article>
</template>

<script setup lang="ts">
withDefaults(defineProps<{
  eyebrow?: string
  title?: string
  chip?: string
  tone?: 'sky' | 'mint' | 'ice' | 'plain' | 'ink'
  compact?: boolean
}>(), {
  eyebrow: '',
  title: '',
  chip: '',
  tone: 'sky',
  compact: false,
})
</script>

<style scoped lang="scss">
.ops-surface-card {
  --surface-accent: #8eaefc;
  --surface-accent-soft: rgba(142, 174, 252, 0.18);
  --surface-accent-strong: #6f93ee;
  --surface-secondary: rgba(157, 228, 217, 0.18);

  position: relative;
  overflow: hidden;
  min-height: 100%;
  padding: 18px 18px 16px;
  border-radius: 22px;
  border: 1px solid rgba(166, 184, 220, 0.18);
  background:
    linear-gradient(180deg, rgba(255, 255, 255, 0.94), rgba(243, 247, 255, 0.98));
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.96),
    0 18px 34px rgba(107, 130, 173, 0.1);
  transition: transform 180ms ease, box-shadow 180ms ease;

  &::after {
    content: '';
    position: absolute;
    inset: 0 auto auto 0;
    width: 42%;
    height: 54%;
    background: radial-gradient(circle at top left, rgba(255, 255, 255, 0.52), transparent 72%);
    pointer-events: none;
  }

  &:hover {
    transform: translateY(-3px);
    box-shadow:
      inset 0 1px 0 rgba(255, 255, 255, 0.96),
      0 22px 38px rgba(107, 130, 173, 0.14);
  }
}

.ops-surface-card.is-compact {
  padding: 16px;
  border-radius: 20px;
}

.ops-surface-card.is-mint {
  --surface-accent: #7fcfc0;
  --surface-accent-soft: rgba(127, 207, 192, 0.16);
  --surface-accent-strong: #5ea99c;
  --surface-secondary: rgba(182, 239, 228, 0.28);
  background:
    linear-gradient(180deg, rgba(228, 247, 242, 0.96), rgba(219, 243, 237, 0.98));
}

.ops-surface-card.is-ice {
  --surface-accent: #adc2ff;
  --surface-accent-soft: rgba(173, 194, 255, 0.16);
  --surface-accent-strong: #90a9ef;
  --surface-secondary: rgba(214, 232, 255, 0.24);
  background:
    linear-gradient(180deg, rgba(238, 244, 255, 0.98), rgba(229, 238, 255, 0.98));
}

.ops-surface-card.is-plain {
  --surface-accent: #9eb0d9;
  --surface-accent-soft: rgba(158, 176, 217, 0.12);
  --surface-accent-strong: #8ea1c8;
  --surface-secondary: rgba(238, 244, 255, 0.14);
  background:
    linear-gradient(180deg, rgba(248, 250, 255, 0.98), rgba(238, 243, 255, 0.98));
}

.ops-surface-card.is-ink {
  --surface-accent: #2a3144;
  --surface-accent-soft: rgba(42, 49, 68, 0.16);
  --surface-accent-strong: #20263a;
  --surface-secondary: rgba(109, 126, 179, 0.18);
  color: #ffffff;
  background:
    linear-gradient(180deg, rgba(47, 55, 76, 0.96), rgba(33, 39, 56, 0.98));

  .ops-surface-card__title,
  .ops-surface-card__chip,
  .ops-surface-card__eyebrow {
    color: #ffffff;
  }

  .ops-surface-card__chip {
    background: rgba(255, 255, 255, 0.14);
    border-color: rgba(255, 255, 255, 0.18);
  }
}

.ops-surface-card__header {
  position: relative;
  z-index: 1;
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 12px;
}

.ops-surface-card__titles {
  min-width: 0;
}

.ops-surface-card__eyebrow,
.ops-surface-card__chip {
  display: inline-flex;
  align-items: center;
  min-height: 26px;
  padding: 0 10px;
  border-radius: 999px;
  font-size: 10px;
  font-weight: 700;
  letter-spacing: 0.14em;
}

.ops-surface-card__eyebrow {
  background: rgba(255, 255, 255, 0.74);
  color: var(--surface-accent-strong);
  text-transform: uppercase;
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.88);
}

.ops-surface-card__title {
  margin: 8px 0 0;
  color: var(--hl-ink);
  font-size: clamp(17px, 2.1vw, 24px);
  font-weight: 700;
  letter-spacing: -0.04em;
  line-height: 1.04;
}

.ops-surface-card__tools {
  display: flex;
  align-items: center;
  gap: 10px;
  flex-wrap: wrap;
  justify-content: flex-end;
}

.ops-surface-card__chip {
  color: var(--hl-ink);
  background: rgba(255, 255, 255, 0.82);
  border: 1px solid rgba(166, 184, 220, 0.18);
}

.ops-surface-card__actions {
  display: flex;
  align-items: center;
  gap: 8px;
  flex-wrap: wrap;
}

.ops-surface-card__content {
  position: relative;
  z-index: 1;
  display: grid;
  gap: 14px;
  margin-top: 14px;
}

.ops-surface-card.has-media .ops-surface-card__content {
  grid-template-columns: minmax(0, 1fr) minmax(140px, 0.8fr);
  align-items: end;
}

.ops-surface-card__body {
  min-width: 0;
}

.ops-surface-card__media {
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 100%;
}

@media (max-width: 720px) {
  .ops-surface-card {
    padding: 16px;
    border-radius: 22px;
  }

  .ops-surface-card.has-media .ops-surface-card__content {
    grid-template-columns: minmax(0, 1fr);
  }
}
</style>
