<template>
  <section class="ops-dashboard-deck" :class="{ 'is-compact': compact }">
    <article class="ops-dashboard-card ops-dashboard-card--overview">
      <div class="card-heading">
        <div>
          <span class="card-caption">{{ eyebrow }}</span>
          <h2>{{ title }}</h2>
        </div>
        <span class="card-chip">{{ resolvedHeadingChip }}</span>
      </div>

      <div class="overview-total">
        <span>{{ metricLabel }}</span>
        <div class="overview-total__value" :class="{ 'is-textual': !isMetricNumeric }">
          <strong>{{ metricValue }}</strong>
          <small v-if="metricUnit">{{ metricUnit }}</small>
        </div>
        <p>{{ metricDescription }}</p>
      </div>

      <div v-if="$slots.actions" class="overview-actions">
        <slot name="actions" />
      </div>

      <div v-if="overviewHighlights.length && !compact" class="overview-highlights">
        <article
          v-for="item in overviewHighlights"
          :key="item.label"
          class="overview-highlight"
          :class="item.tone"
        >
          <span>{{ item.label }}</span>
          <strong>{{ item.value }}</strong>
          <small>{{ item.note }}</small>
        </article>
      </div>

      <div class="section-heading">
        <span>{{ sectionLabel }}</span>
        <small>{{ sectionNote }}</small>
      </div>

      <div class="overview-cards">
        <article
          v-for="item in overviewCards"
          :key="item.label"
          class="overview-mini-card"
          :class="item.tone"
        >
          <span>{{ item.label }}</span>
          <strong>{{ item.value }}</strong>
          <small>{{ item.note }}</small>
          <em v-if="!compact">{{ item.footer }}</em>
        </article>

        <article
          class="overview-shortcut overview-shortcut--data"
          :class="{ 'is-textual': !isFocusNumeric }"
        >
          <strong>{{ focusCard.value }}</strong>
          <small>{{ focusCard.label }}</small>
          <em v-if="!compact && focusCard.note">{{ focusCard.note }}</em>
        </article>
      </div>
    </article>

    <article class="ops-dashboard-card ops-dashboard-card--spending">
      <div class="card-heading">
        <div>
          <span class="card-caption">{{ rhythmEyebrow }}</span>
          <h3>{{ rhythmTitle }}</h3>
        </div>
        <span class="card-chip">{{ rhythmChip }}</span>
      </div>

      <div class="spending-stage">
        <span class="spending-badge">{{ rhythmBadge }}</span>
        <div
          class="spending-bars"
          :class="{ 'spending-bars--quad': normalizedRhythmItems.length <= 4 }"
        >
          <article
            v-for="item in normalizedRhythmItems"
            :key="item.label"
            class="spending-bar"
            :class="{ 'is-peak': item.isPeak }"
          >
            <div class="spending-bar__track">
              <span :style="{ height: `${item.height}%` }" />
            </div>
            <small>{{ item.label }}</small>
          </article>
        </div>
      </div>
    </article>

    <article class="ops-dashboard-card ops-dashboard-card--activity">
      <div class="card-heading">
        <div>
          <span class="card-caption">{{ activityEyebrow }}</span>
          <h3>{{ activityTitle }}</h3>
        </div>
        <span class="card-chip">{{ activityChip }}</span>
      </div>

      <div class="transaction-list">
        <article v-for="row in activityRows" :key="row.id" class="transaction-item">
          <div class="transaction-item__icon" :class="row.iconTone">
            {{ row.badge }}
          </div>
          <div class="transaction-item__copy">
            <strong>{{ row.title }}</strong>
            <span>{{ row.meta }}</span>
          </div>
          <div class="transaction-item__amount" :class="row.tone">
            {{ row.amount }}
          </div>
        </article>
      </div>
    </article>

    <article class="ops-dashboard-card ops-dashboard-card--guide">
      <div class="card-heading">
        <div>
          <span class="card-caption">{{ guideEyebrow }}</span>
          <h3>{{ guideTitle }}</h3>
        </div>
        <span class="card-chip">{{ guideChip }}</span>
      </div>

      <div class="guide-hero">
        <strong>{{ guideHeadline }}</strong>
        <span>{{ guideCopy }}</span>
      </div>

      <div class="guide-summary">
        <div class="guide-summary__metric">
          <span class="guide-summary__label">{{ guidePulseLabel }}</span>
          <strong>{{ guidePulseValue }}</strong>
          <small>{{ guidePulseNote }}</small>
        </div>
        <span class="guide-summary__chip">{{ guideChip }}</span>
      </div>

      <div v-if="resolvedGuideItems.length" class="guide-flags">
        <article v-for="item in resolvedGuideItems" :key="item.label" class="guide-flag">
          <small>{{ item.label }}</small>
          <strong>{{ item.value }}</strong>
        </article>
      </div>
    </article>
  </section>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import dayjs from 'dayjs'

type OverviewCardTone = 'lake' | 'sage' | 'amber' | 'rose' | 'is-blue' | 'is-mint'
type ActivityRowTone = 'is-positive' | 'is-negative' | 'is-neutral'
type ActivityIconTone = 'is-blue' | 'is-mint'

interface OverviewCard {
  label: string
  value: string
  note: string
  footer: string
  tone: OverviewCardTone
}

interface FocusCard {
  label: string
  value: string
  note: string
}

interface OverviewHighlight {
  label: string
  value: string
  note: string
  tone?: OverviewCardTone
}

interface RhythmItem {
  label: string
  value: number | string
}

interface ActivityRow {
  id: string
  badge: string
  title: string
  meta: string
  amount: string
  tone: ActivityRowTone
  iconTone: ActivityIconTone
}

interface GuideItem {
  label: string
  value: string
  note?: string
}

const props = withDefaults(
  defineProps<{
    eyebrow: string
    title: string
    headingChip?: string
    metricLabel: string
    metricValue: string
    metricUnit?: string
    metricDescription: string
    sectionLabel?: string
    sectionNote?: string
    overviewCards: OverviewCard[]
    overviewHighlights?: OverviewHighlight[]
    focusCard: FocusCard
    rhythmEyebrow?: string
    rhythmTitle: string
    rhythmChip: string
    rhythmBadge: string
    rhythmItems: RhythmItem[]
    activityEyebrow?: string
    activityTitle: string
    activityChip: string
    activityRows: ActivityRow[]
    guideEyebrow?: string
    guideTitle: string
    guideChip: string
    guideHeadline: string
    guideCopy: string
    guidePulseLabel: string
    guidePulseValue: string
    guidePulseNote: string
    guideItems: GuideItem[]
    compact?: boolean
  }>(),
  {
    headingChip: '',
    metricUnit: '',
    sectionLabel: '常用入口',
    sectionNote: '桌面常用入口',
    rhythmEyebrow: '节律',
    activityEyebrow: '动态',
    guideEyebrow: '建议',
    overviewHighlights: () => [],
    compact: false,
  },
)

const resolvedHeadingChip = computed(
  () => props.headingChip || dayjs().format('YYYY年MM月DD日 HH:mm'),
)

const normalizedRhythmItems = computed(() => {
  const source = props.rhythmItems.map((item) => ({
    ...item,
    numericValue: Number(item.value) || 0,
  }))
  const max = Math.max(...source.map((item) => item.numericValue), 1)
  const peakValue = Math.max(...source.map((item) => item.numericValue), 0)
  const peakIndex = source.findIndex((item) => item.numericValue === peakValue)

  return source.map((item, index) => ({
    ...item,
    isPeak: index === peakIndex,
    height: Math.max(18, Math.round((item.numericValue / max) * 100)),
  }))
})

const isMetricNumeric = computed(() => /^[\d.,+-]+$/.test(props.metricValue.trim()))
const isFocusNumeric = computed(() => /^[\d.,+\-/%]+$/.test(props.focusCard.value.trim()))
const resolvedGuideItems = computed(() => props.guideItems.slice(0, 3))
</script>

<style scoped lang="scss">
.ops-dashboard-deck {
  display: grid;
  grid-template-columns:
    minmax(300px, 1.28fr) minmax(220px, 0.84fr) minmax(260px, 1fr)
    minmax(230px, 0.88fr);
  grid-template-rows: auto;
  grid-template-areas: 'overview spending activity guide';
  gap: 10px;
  margin-bottom: 10px;
}

.ops-dashboard-deck.is-compact {
  min-height: 0;
}

.ops-dashboard-card {
  min-height: 0;
  display: flex;
  flex-direction: column;
  padding: 12px;
  border: 1px solid #d8dee8;
  border-radius: 6px;
  background: #ffffff;
  box-shadow: none;
}

.ops-dashboard-card--overview {
  grid-area: overview;
  border-top: 3px solid #2563eb;
}

.ops-dashboard-card--spending {
  grid-area: spending;
  border-top: 3px solid #0f766e;
}

.ops-dashboard-card--activity {
  grid-area: activity;
  border-top: 3px solid #7c3aed;
}

.ops-dashboard-card--guide {
  grid-area: guide;
  border-top: 3px solid #d97706;
}

.card-heading {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 12px;

  > div {
    min-width: 0;
    display: grid;
    gap: 4px;
  }

  h2,
  h3 {
    margin: 0;
    color: #111827;
    font-size: 15px;
    font-weight: 760;
    line-height: 1.25;
    letter-spacing: 0;
  }
}

.card-caption,
.card-chip {
  display: inline-flex;
  align-items: center;
  width: fit-content;
  min-height: 20px;
  padding: 0 8px;
  border-radius: 4px;
  background: #f1f5f9;
  color: #475569;
  font-size: 10px;
  font-weight: 700;
  letter-spacing: 0;
}

.card-chip {
  flex: 0 0 auto;
}

.overview-total {
  margin-top: 10px;

  > span {
    display: block;
    color: #475569;
    font-size: 12px;
    font-weight: 650;
  }

  p {
    max-width: 28rem;
    margin: 6px 0 0;
    color: #64748b;
    font-size: 12px;
    line-height: 1.55;
  }
}

.overview-total__value {
  display: flex;
  align-items: flex-end;
  gap: 4px;
  margin-top: 6px;
  line-height: 1;

  strong {
    color: #111827;
    font-size: 30px;
    font-weight: 780;
    letter-spacing: 0;
  }

  small {
    margin-bottom: 5px;
    color: #64748b;
    font-size: 15px;
    font-weight: 700;
  }
}

.overview-total__value.is-textual {
  align-items: center;

  strong {
    max-width: 15rem;
    font-size: 20px;
    line-height: 1.16;
    word-break: keep-all;
    overflow-wrap: anywhere;
  }
}

.overview-actions {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  margin-top: 10px;
}

.overview-actions :deep(.overview-action) {
  min-height: 30px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  padding: 0 12px;
  border: 1px solid #d8dee8;
  border-radius: 4px;
  background: #ffffff;
  color: #1f2937;
  font-size: 11px;
  font-weight: 700;
  letter-spacing: 0;
  cursor: pointer;
}

.overview-actions :deep(.overview-action:hover) {
  border-color: #2563eb;
  color: #2563eb;
}

.overview-highlights {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 8px;
  margin-top: 10px;
}

.overview-highlight,
.overview-mini-card,
.overview-shortcut,
.guide-summary,
.guide-flag {
  border: 1px solid #d8dee8;
  border-radius: 6px;
  background: #f8fafc;
}

.overview-highlight {
  min-height: 68px;
  display: grid;
  gap: 4px;
  padding: 10px;

  span,
  small {
    color: #64748b;
    font-size: 11px;
  }

  strong {
    color: #111827;
    font-size: 20px;
    font-weight: 760;
    letter-spacing: 0;
  }
}

.section-heading {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
  margin-top: auto;
  padding-top: 10px;

  span {
    color: #111827;
    font-size: 13px;
    font-weight: 720;
  }

  small {
    color: #64748b;
    font-size: 11px;
  }
}

.overview-cards {
  display: grid;
  grid-template-columns: minmax(0, 1fr) minmax(0, 1fr) minmax(88px, 0.55fr);
  gap: 8px;
  margin-top: 8px;
}

.overview-mini-card {
  min-height: 70px;
  padding: 10px;
  position: relative;
  overflow: hidden;

  span,
  small,
  em {
    display: block;
    color: #64748b;
    font-size: 10px;
    line-height: 1.45;
  }

  span {
    color: #475569;
    font-size: 11px;
    font-weight: 650;
  }

  strong {
    display: block;
    margin-top: 8px;
    color: #111827;
    font-size: 18px;
    font-weight: 780;
    letter-spacing: 0;
  }

  em {
    position: absolute;
    left: 12px;
    right: 12px;
    bottom: 10px;
    overflow: hidden;
    font-style: normal;
    text-overflow: ellipsis;
    white-space: nowrap;
  }
}

.overview-mini-card.lake,
.overview-mini-card.is-blue,
.overview-highlight.lake,
.overview-highlight.is-blue {
  background: #eff6ff;
}

.overview-mini-card.sage,
.overview-mini-card.is-mint,
.overview-highlight.sage,
.overview-highlight.is-mint {
  background: #ecfdf5;
}

.overview-mini-card.amber,
.overview-highlight.amber {
  background: #fffbeb;
}

.overview-mini-card.rose,
.overview-highlight.rose {
  background: #fef2f2;
}

.overview-shortcut {
  min-height: 70px;
  display: grid;
  place-items: center;
  align-content: center;
  gap: 6px;
  padding: 10px;
  background: #eef6ff;
  color: #1d4ed8;
  text-align: center;

  strong {
    font-size: 24px;
    font-weight: 780;
    letter-spacing: 0;
    line-height: 1;
  }

  small,
  em {
    color: #64748b;
    font-size: 10px;
    font-style: normal;
    line-height: 1.3;
  }
}

.overview-shortcut.is-textual strong {
  max-width: 100%;
  font-size: 13px;
  line-height: 1.16;
  overflow-wrap: anywhere;
}

.spending-stage {
  flex: 1 1 auto;
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  gap: 8px;
  margin-top: 8px;
}

.spending-badge {
  align-self: flex-start;
  padding: 4px 8px;
  border-radius: 4px;
  background: #f1f5f9;
  color: #475569;
  font-size: 11px;
  font-weight: 700;
}

.spending-bars {
  min-height: 64px;
  display: grid;
  grid-template-columns: repeat(7, minmax(0, 1fr));
  gap: 6px;
  align-items: end;
}

.spending-bars--quad {
  grid-template-columns: repeat(4, minmax(0, 1fr));
}

.spending-bar {
  display: grid;
  justify-items: center;
  gap: 6px;

  small {
    color: #64748b;
    font-size: 10px;
    font-weight: 650;
  }
}

.spending-bar__track {
  width: 20px;
  height: 60px;
  display: flex;
  align-items: flex-end;
  justify-content: center;
  padding: 4px 0;
  border-radius: 6px;
  background: #e2e8f0;

  span {
    width: 14px;
    border-radius: 4px;
    background: #94a3b8;
  }
}

.spending-bar.is-peak .spending-bar__track span {
  background: #2563eb;
}

.transaction-list {
  display: flex;
  flex-direction: column;
  margin-top: 6px;
}

.transaction-item {
  display: grid;
  grid-template-columns: 30px minmax(0, 1fr) auto;
  gap: 8px;
  align-items: center;
  padding: 6px 0;
  border-bottom: 1px solid #e2e8f0;
}

.transaction-item:last-child {
  border-bottom: none;
}

.transaction-item__icon {
  width: 30px;
  height: 30px;
  display: grid;
  place-items: center;
  border: 1px solid #d8dee8;
  border-radius: 8px;
  background: #f8fafc;
  color: #111827;
  font-size: 11px;
  font-weight: 760;
}

.transaction-item__icon.is-blue {
  color: #2563eb;
}

.transaction-item__icon.is-mint {
  color: #0f766e;
}

.transaction-item__copy {
  min-width: 0;

  strong,
  span {
    display: block;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  strong {
    color: #111827;
    font-size: 12px;
    font-weight: 720;
  }

  span {
    margin-top: 2px;
    color: #64748b;
    font-size: 11px;
  }
}

.transaction-item__amount {
  color: #475569;
  font-size: 12px;
  font-weight: 760;
  letter-spacing: 0;
}

.transaction-item__amount.is-positive {
  color: #16a34a;
}

.transaction-item__amount.is-negative {
  color: #dc2626;
}

.guide-hero {
  display: grid;
  gap: 4px;
  margin-top: 8px;

  strong {
    color: #111827;
    font-size: 13px;
    font-weight: 760;
    line-height: 1.35;
  }

  span {
    color: #64748b;
    font-size: 12px;
    line-height: 1.55;
  }
}

.guide-summary {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 10px;
  margin-top: 8px;
  padding: 9px;
}

.guide-summary__metric {
  display: grid;
  gap: 3px;

  span,
  small {
    color: #64748b;
    font-size: 11px;
  }

  strong {
    color: #111827;
    font-size: 18px;
    font-weight: 780;
    letter-spacing: 0;
  }
}

.guide-summary__chip {
  flex: 0 0 auto;
  padding: 4px 8px;
  border-radius: 4px;
  background: #ffffff;
  color: #475569;
  font-size: 11px;
  font-weight: 700;
}

.guide-flags {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 8px;
  margin-top: 8px;
}

.guide-flag {
  min-height: 40px;
  display: grid;
  align-content: center;
  gap: 2px;
  padding: 7px;
  background: #ffffff;

  small,
  strong {
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  small {
    color: #64748b;
    font-size: 10px;
  }

  strong {
    color: #111827;
    font-size: 12px;
    font-weight: 760;
  }
}

.ops-dashboard-deck.is-compact .overview-total p,
.ops-dashboard-deck.is-compact .section-heading {
  display: none;
}

.ops-dashboard-deck.is-compact .overview-cards {
  margin-top: 10px;
}

.ops-dashboard-deck.is-compact .overview-mini-card,
.ops-dashboard-deck.is-compact .overview-shortcut {
  min-height: 58px;
  padding: 8px;
}

.ops-dashboard-deck.is-compact .overview-mini-card strong {
  margin-top: 6px;
  font-size: 16px;
}

.ops-dashboard-deck.is-compact .spending-stage {
  justify-content: flex-start;
}

.ops-dashboard-deck.is-compact .spending-bars {
  margin-top: 10px;
}

.ops-dashboard-deck.is-compact .guide-flags {
  display: none;
}

@media (max-width: 1320px) {
  .ops-dashboard-deck,
  .ops-dashboard-deck.is-compact {
    grid-template-columns: minmax(0, 1fr) minmax(300px, 0.84fr);
    grid-template-areas:
      'overview spending'
      'activity guide';
  }
}

@media (max-width: 900px) {
  .ops-dashboard-deck,
  .ops-dashboard-deck.is-compact {
    grid-template-columns: minmax(0, 1fr);
    grid-template-rows: none;
    grid-template-areas:
      'overview'
      'spending'
      'guide'
      'activity';
  }

  .overview-cards,
  .guide-flags {
    grid-template-columns: 1fr;
  }

  .overview-highlights {
    grid-template-columns: 1fr;
  }
}
</style>
