<template>
  <section class="ops-workbench">
    <div
      v-if="hasDeck"
      class="ops-workbench__deck"
      :class="{
        'has-support': hasSupport,
        'has-rail': hasRail,
        'has-footer': hasFooter,
      }"
    >
      <div v-if="hasStage" class="ops-workbench__stage">
        <slot name="stage" />
      </div>

      <div v-if="hasSupport" class="ops-workbench__support">
        <slot name="support" />
      </div>

      <div v-if="hasFooter" class="ops-workbench__footer">
        <slot name="footer" />
      </div>

      <div v-if="hasRail" class="ops-workbench__rail">
        <slot name="rail" />
      </div>
    </div>

    <div class="ops-workbench__main">
      <slot />
    </div>
  </section>
</template>

<script setup lang="ts">
import { computed, useSlots } from 'vue'

const slots = useSlots()

const hasStage = computed(() => Boolean(slots.stage))
const hasSupport = computed(() => Boolean(slots.support))
const hasRail = computed(() => Boolean(slots.rail))
const hasFooter = computed(() => Boolean(slots.footer))
const hasDeck = computed(
  () => hasStage.value || hasSupport.value || hasRail.value || hasFooter.value,
)
</script>

<style scoped lang="scss">
.ops-workbench {
  --ops-workbench-gap: 18px;

  position: relative;
  display: grid;
  gap: var(--ops-workbench-gap);
  padding: 10px;
  border-radius: 38px;
  background:
    radial-gradient(circle at 12% 8%, rgba(255, 255, 255, 0.78), transparent 28%),
    radial-gradient(circle at 88% 12%, rgba(214, 232, 255, 0.44), transparent 24%),
    linear-gradient(180deg, rgba(252, 254, 255, 0.62), rgba(236, 244, 255, 0.4));
  border: 1px solid rgba(168, 185, 223, 0.16);
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.9),
    0 28px 54px rgba(104, 128, 176, 0.08);
  isolation: isolate;
}

.ops-workbench::before,
.ops-workbench::after {
  content: '';
  position: absolute;
  pointer-events: none;
}

.ops-workbench::before {
  inset: 0;
  border-radius: inherit;
  background:
    linear-gradient(135deg, rgba(255, 255, 255, 0.22), transparent 40%),
    linear-gradient(180deg, rgba(139, 163, 214, 0.08), transparent 62%);
  opacity: 0.95;
}

.ops-workbench::after {
  top: 28px;
  right: 22px;
  width: 220px;
  height: 220px;
  border-radius: 50%;
  border: 1px solid rgba(145, 168, 218, 0.12);
  box-shadow:
    0 0 0 28px rgba(255, 255, 255, 0.08),
    0 0 0 56px rgba(215, 229, 255, 0.08);
  opacity: 0.68;
}

.ops-workbench__deck {
  position: relative;
  display: grid;
  grid-template-columns: minmax(0, 1.52fr) minmax(228px, 0.86fr) minmax(300px, 0.98fr);
  grid-template-areas:
    'stage support rail'
    'stage footer rail';
  gap: var(--ops-workbench-gap);
  align-items: start;
  z-index: 1;
}

.ops-workbench__stage {
  grid-area: stage;
}

.ops-workbench__rail {
  grid-area: rail;
}

.ops-workbench__support {
  grid-area: support;
}

.ops-workbench__footer {
  grid-area: footer;
}

.ops-workbench__deck:not(.has-support).has-footer {
  grid-template-columns: minmax(0, 1.52fr) minmax(228px, 0.86fr) minmax(300px, 0.98fr);
  grid-template-areas:
    'stage footer rail'
    'stage footer rail';
}

.ops-workbench__deck:not(.has-footer).has-support {
  grid-template-columns: minmax(0, 1.52fr) minmax(228px, 0.86fr) minmax(300px, 0.98fr);
  grid-template-areas: 'stage support rail';
}

.ops-workbench__deck:not(.has-rail) {
  grid-template-columns: minmax(0, 1.5fr) minmax(240px, 0.92fr);
  grid-template-areas:
    'stage support'
    'stage footer';
}

.ops-workbench__deck:not(.has-support):not(.has-footer) {
  grid-template-columns: minmax(0, 1.56fr) minmax(280px, 0.94fr);
  grid-template-areas: 'stage rail';
}

.ops-workbench__stage,
.ops-workbench__support,
.ops-workbench__rail,
.ops-workbench__main,
.ops-workbench__footer {
  min-width: 0;
  align-self: start;
}

.ops-workbench__main {
  min-width: 0;
  position: relative;
  z-index: 1;
}

@media (max-width: 1180px) and (min-width: 961px) {
  .ops-workbench {
    --ops-workbench-gap: 14px;
    padding: 8px;
    border-radius: 32px;
  }

  .ops-workbench__deck {
    grid-template-columns: minmax(0, 1.44fr) minmax(210px, 0.82fr) minmax(250px, 0.92fr);
    grid-template-areas:
      'stage support rail'
      'stage footer rail';
  }

  .ops-workbench__deck:not(.has-footer).has-support {
    grid-template-columns: minmax(0, 1.44fr) minmax(210px, 0.82fr) minmax(250px, 0.92fr);
    grid-template-areas: 'stage support rail';
  }

  .ops-workbench__deck:not(.has-support).has-footer {
    grid-template-columns: minmax(0, 1.44fr) minmax(210px, 0.82fr) minmax(250px, 0.92fr);
    grid-template-areas:
      'stage footer rail'
      'stage footer rail';
  }

  .ops-workbench__deck:not(.has-rail) {
    grid-template-columns: minmax(0, 1.34fr) minmax(198px, 0.86fr);
    grid-template-areas:
      'stage support'
      'stage footer';
  }

  .ops-workbench__deck:not(.has-support):not(.has-footer) {
    grid-template-columns: minmax(0, 1.48fr) minmax(216px, 0.9fr);
    grid-template-areas: 'stage rail';
  }
}

@media (max-width: 960px) {
  .ops-workbench {
    padding: 0;
    border: none;
    background: transparent;
    box-shadow: none;
  }

  .ops-workbench::before,
  .ops-workbench::after {
    display: none;
  }

  .ops-workbench__deck,
  .ops-workbench__deck:not(.has-rail),
  .ops-workbench__deck:not(.has-support),
  .ops-workbench__deck:not(.has-footer) {
    grid-template-columns: minmax(0, 1fr);
    grid-template-areas:
      'stage'
      'support'
      'footer'
      'rail';
  }
}
</style>
