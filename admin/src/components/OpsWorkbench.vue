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

      <div v-if="hasSupport || hasFooter" class="ops-workbench__stack">
        <div v-if="hasSupport" class="ops-workbench__support">
          <slot name="support" />
        </div>

        <div v-if="hasFooter" class="ops-workbench__footer">
          <slot name="footer" />
        </div>
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
}

.ops-workbench__deck {
  display: grid;
  grid-template-columns: minmax(0, 1.48fr) minmax(168px, 0.74fr) minmax(242px, 0.98fr);
  gap: var(--ops-workbench-gap);
  align-items: start;
}

.ops-workbench__deck:not(.has-rail) {
  grid-template-columns: minmax(0, 1.42fr) minmax(212px, 0.9fr);
}

.ops-workbench__deck:not(.has-support):not(.has-footer) {
  grid-template-columns: minmax(0, 1.56fr) minmax(228px, 0.94fr);
}

.ops-workbench__deck:not(.has-support).has-footer:not(.has-rail) {
  grid-template-columns: minmax(0, 1.42fr) minmax(212px, 0.9fr);
}

.ops-workbench__deck:not(.has-support).has-footer {
  grid-template-columns: minmax(0, 1.56fr) minmax(228px, 0.94fr);
}

.ops-workbench__stack {
  display: grid;
  gap: var(--ops-workbench-gap);
  align-self: start;
}

.ops-workbench__stage,
.ops-workbench__stack,
.ops-workbench__support,
.ops-workbench__rail,
.ops-workbench__main,
.ops-workbench__footer {
  min-width: 0;
  align-self: start;
}

.ops-workbench__main {
  min-width: 0;
}

@media (max-width: 1180px) and (min-width: 961px) {
  .ops-workbench {
    --ops-workbench-gap: 14px;
  }

  .ops-workbench__deck {
    grid-template-columns: minmax(0, 1.4fr) minmax(148px, 0.7fr) minmax(214px, 0.9fr);
  }

  .ops-workbench__deck:not(.has-rail) {
    grid-template-columns: minmax(0, 1.34fr) minmax(184px, 0.86fr);
  }

  .ops-workbench__deck:not(.has-support) {
    grid-template-columns: minmax(0, 1.48fr) minmax(202px, 0.9fr);
  }
}

@media (max-width: 960px) {
  .ops-workbench__deck,
  .ops-workbench__deck:not(.has-rail),
  .ops-workbench__deck:not(.has-support),
  .ops-workbench__deck:not(.has-footer) {
    grid-template-columns: minmax(0, 1fr);
  }
}
</style>
