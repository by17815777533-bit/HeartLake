<template>
  <article
    class="ops-surface-card"
    :class="[`is-${tone}`, { 'is-compact': compact, 'has-media': Boolean($slots.media) }]"
  >
    <div v-if="eyebrow || title || chip || $slots.actions" class="ops-surface-card__header">
      <div class="ops-surface-card__titles">
        <span v-if="eyebrow" class="ops-surface-card__eyebrow">
          {{ eyebrow }}
        </span>
        <h2 v-if="title" class="ops-surface-card__title">
          {{ title }}
        </h2>
      </div>

      <div class="ops-surface-card__tools">
        <span v-if="chip" class="ops-surface-card__chip">
          {{ chip }}
        </span>
        <div v-if="$slots.actions" class="ops-surface-card__actions">
          <slot name="actions" />
        </div>
      </div>
    </div>

    <div class="ops-surface-card__content">
      <div class="ops-surface-card__body">
        <slot />
      </div>
      <div v-if="$slots.media" class="ops-surface-card__media">
        <slot name="media" />
      </div>
    </div>
  </article>
</template>

<script setup lang="ts">
withDefaults(
  defineProps<{
    eyebrow?: string
    title?: string
    chip?: string
    tone?: 'sky' | 'mint' | 'ice' | 'plain' | 'ink'
    compact?: boolean
  }>(),
  {
    eyebrow: '',
    title: '',
    chip: '',
    tone: 'sky',
    compact: false,
  },
)
</script>

<style scoped lang="scss">
.ops-surface-card {
  --surface-accent: #8eaefc;
  --surface-accent-soft: rgba(142, 174, 252, 0.18);
  --surface-accent-strong: #6f93ee;
  --surface-secondary: rgba(157, 228, 217, 0.18);
  --surface-radius: 28px;

  position: relative;
  overflow: hidden;
  min-height: 100%;
  padding: 22px 22px 20px;
  border-radius: var(--surface-radius);
  border: 1px solid rgba(166, 184, 220, 0.18);
  background: linear-gradient(180deg, rgba(251, 253, 255, 0.98), rgba(241, 247, 255, 0.98));
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.96),
    0 22px 40px rgba(107, 130, 173, 0.1);
  transition:
    transform 180ms ease,
    box-shadow 180ms ease;
  font-family: var(--hl-font-body);

  &::before,
  &::after {
    content: '';
    position: absolute;
    pointer-events: none;
  }

  &::before {
    inset: 0 auto auto 0;
    width: 44%;
    height: 56%;
    background: radial-gradient(circle at top left, rgba(255, 255, 255, 0.58), transparent 74%);
  }

  &::after {
    inset: auto 0 0 auto;
    width: 42%;
    height: 48%;
    background: radial-gradient(circle at bottom right, rgba(214, 228, 255, 0.2), transparent 70%);
  }
}

@media (hover: hover) and (pointer: fine) {
  .ops-surface-card:hover {
    transform: translateY(-3px);
    box-shadow:
      inset 0 1px 0 rgba(255, 255, 255, 0.96),
      0 24px 42px rgba(107, 130, 173, 0.14);
  }
}

.ops-surface-card.is-compact {
  --surface-radius: 24px;
  padding: 18px 18px 16px;
}

.ops-surface-card.is-mint {
  --surface-accent: #7fcfc0;
  --surface-accent-soft: rgba(127, 207, 192, 0.16);
  --surface-accent-strong: #5ea99c;
  --surface-secondary: rgba(182, 239, 228, 0.28);
  background: linear-gradient(180deg, rgba(214, 237, 231, 0.98), rgba(204, 233, 226, 0.98));
}

.ops-surface-card.is-ice {
  --surface-accent: #adc2ff;
  --surface-accent-soft: rgba(173, 194, 255, 0.16);
  --surface-accent-strong: #90a9ef;
  --surface-secondary: rgba(214, 232, 255, 0.24);
  background: linear-gradient(180deg, rgba(215, 227, 255, 0.98), rgba(204, 218, 255, 0.98));
}

.ops-surface-card.is-plain {
  --surface-accent: #9eb0d9;
  --surface-accent-soft: rgba(158, 176, 217, 0.12);
  --surface-accent-strong: #8ea1c8;
  --surface-secondary: rgba(238, 244, 255, 0.14);
  background: linear-gradient(180deg, rgba(250, 252, 255, 0.98), rgba(241, 246, 255, 0.98));
}

.ops-surface-card.is-ink {
  --surface-accent: #2a3144;
  --surface-accent-soft: rgba(42, 49, 68, 0.16);
  --surface-accent-strong: #20263a;
  --surface-secondary: rgba(109, 126, 179, 0.18);
  color: #ffffff;
  background: linear-gradient(180deg, rgba(47, 55, 76, 0.96), rgba(33, 39, 56, 0.98));

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
  gap: 14px;
}

.ops-surface-card__titles {
  min-width: 0;
}

.ops-surface-card__eyebrow,
.ops-surface-card__chip {
  display: inline-flex;
  align-items: center;
  min-height: 28px;
  padding: 0 11px;
  border-radius: 999px;
  font-size: 11px;
  font-weight: 700;
  letter-spacing: 0.04em;
}

.ops-surface-card__eyebrow {
  background: rgba(255, 255, 255, 0.66);
  color: color-mix(in srgb, var(--surface-accent-strong) 78%, #42506d 22%);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.88);
}

.ops-surface-card__title {
  margin: 12px 0 0;
  color: var(--hl-ink);
  font-size: clamp(20px, 1.92vw, 24px);
  font-weight: 680;
  letter-spacing: -0.05em;
  line-height: 1.08;
}

.ops-surface-card__tools {
  display: flex;
  align-items: center;
  gap: 8px 10px;
  flex-wrap: wrap;
  justify-content: flex-end;
}

.ops-surface-card__chip {
  color: var(--hl-ink);
  background: rgba(255, 255, 255, 0.78);
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
  gap: 18px;
  margin-top: 18px;
}

.ops-surface-card.has-media .ops-surface-card__content {
  grid-template-columns: minmax(0, 1fr) minmax(148px, 0.78fr);
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
  align-self: stretch;
}

@media (max-width: 1180px) and (min-width: 961px) {
  .ops-surface-card {
    --surface-radius: 24px;
    padding: 18px 18px 16px;
  }

  .ops-surface-card__header {
    gap: 10px;
  }

  .ops-surface-card__eyebrow,
  .ops-surface-card__chip {
    min-height: 28px;
    padding: 0 10px;
    font-size: 10px;
  }

  .ops-surface-card__title {
    margin-top: 8px;
    font-size: clamp(17px, 1.9vw, 20px);
  }

  .ops-surface-card__content {
    gap: 14px;
    margin-top: 14px;
  }

  .ops-surface-card.has-media .ops-surface-card__content {
    grid-template-columns: minmax(0, 1fr) minmax(124px, 0.74fr);
  }
}

@media (max-width: 720px) {
  .ops-surface-card {
    padding: 18px;
    --surface-radius: 24px;
  }

  .ops-surface-card.has-media .ops-surface-card__content {
    grid-template-columns: minmax(0, 1fr);
  }
}
</style>
