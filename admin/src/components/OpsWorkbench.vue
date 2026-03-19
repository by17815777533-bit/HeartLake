<template>
  <section
    class="ops-workbench"
    :class="{
      'has-support': hasSupport,
      'has-rail': hasRail,
      'has-footer': hasFooter,
    }"
  >
    <div
      v-if="hasStage"
      class="ops-workbench__stage"
    >
      <slot name="stage" />
    </div>

    <div
      v-if="hasSupport"
      class="ops-workbench__support"
    >
      <slot name="support" />
    </div>

    <div
      v-if="hasRail"
      class="ops-workbench__rail"
    >
      <slot name="rail" />
    </div>

    <div class="ops-workbench__main">
      <slot />
    </div>

    <div
      v-if="hasFooter"
      class="ops-workbench__footer"
    >
      <slot name="footer" />
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
</script>

<style scoped lang="scss">
.ops-workbench {
  display: grid;
  grid-template-columns: minmax(0, 1.55fr) minmax(240px, 0.88fr) minmax(280px, 1.08fr);
  gap: 18px;
  grid-template-areas:
    "stage support rail"
    "main main footer";
}

.ops-workbench.has-support {
  grid-template-columns: minmax(0, 1.55fr) minmax(240px, 0.88fr) minmax(280px, 1.08fr);
}

.ops-workbench:not(.has-rail) {
  grid-template-columns: minmax(0, 1fr);
  grid-template-areas:
    "stage"
    "support"
    "main"
    "footer";
}

.ops-workbench:not(.has-support) {
  grid-template-columns: minmax(0, 1.72fr) minmax(280px, 1fr);
  grid-template-areas:
    "stage rail"
    "main footer";
}

.ops-workbench:not(.has-footer) {
  grid-template-areas:
    "stage support rail"
    "main main rail";
}

.ops-workbench:not(.has-support):not(.has-footer) {
  grid-template-areas:
    "stage rail"
    "main rail";
}

.ops-workbench__stage { grid-area: stage; }
.ops-workbench__support { grid-area: support; }
.ops-workbench__rail { grid-area: rail; }
.ops-workbench__main { grid-area: main; min-width: 0; }
.ops-workbench__footer { grid-area: footer; }

.ops-workbench__stage,
.ops-workbench__support,
.ops-workbench__rail,
.ops-workbench__main,
.ops-workbench__footer {
  min-width: 0;
}

@media (max-width: 1180px) {
  .ops-workbench,
  .ops-workbench.has-support,
  .ops-workbench:not(.has-rail),
  .ops-workbench:not(.has-support),
  .ops-workbench:not(.has-footer) {
    grid-template-columns: minmax(0, 1fr);
    grid-template-areas:
      "stage"
      "support"
      "rail"
      "main"
      "footer";
  }
}
</style>
