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
        <div class="overview-total__value">
          <strong>{{ metricValue }}</strong>
          <small v-if="metricUnit">{{ metricUnit }}</small>
        </div>
        <p>{{ metricDescription }}</p>
      </div>

      <div v-if="$slots.actions" class="overview-actions">
        <slot name="actions" />
      </div>

      <div v-if="overviewHighlights.length" class="overview-highlights">
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
          <div class="overview-mini-card__ornament" />
          <span>{{ item.label }}</span>
          <strong>{{ item.value }}</strong>
          <small>{{ item.note }}</small>
          <em>{{ item.footer }}</em>
        </article>

        <article class="overview-shortcut overview-shortcut--data">
          <strong>{{ focusCard.value }}</strong>
          <small>{{ focusCard.label }}</small>
          <em>{{ focusCard.note }}</em>
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

      <div class="guide-pulse">
        <span class="guide-pulse__label">{{ guidePulseLabel }}</span>
        <strong>{{ guidePulseValue }}</strong>
        <small>{{ guidePulseNote }}</small>
      </div>

      <div class="guide-list">
        <article v-for="item in guideItems" :key="item.label" class="guide-item">
          <div class="guide-item__badge">{{ item.value }}</div>
          <div class="guide-item__copy">
            <strong>{{ item.label }}</strong>
          </div>
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
    height: Math.max(26, Math.round((item.numericValue / max) * 100)),
  }))
})
</script>

<style scoped lang="scss">
.ops-dashboard-deck {
  --deck-gap: 18px;
  --deck-radius: 28px;

  display: grid;
  gap: var(--deck-gap);
  grid-template-columns: minmax(0, 1.48fr) minmax(0, 0.82fr) minmax(0, 0.94fr);
  grid-template-rows: minmax(182px, auto) minmax(138px, auto);
  grid-template-areas:
    'overview spending activity'
    'overview guide activity';
  margin-bottom: 18px;
}

.ops-dashboard-deck.is-compact {
  --deck-gap: 14px;
  --deck-radius: 24px;

  grid-template-columns: minmax(0, 1.42fr) minmax(0, 0.8fr) minmax(0, 0.9fr);
  grid-template-rows: minmax(162px, auto) minmax(124px, auto);
  margin-bottom: 14px;
}

.ops-dashboard-card {
  position: relative;
  overflow: hidden;
  display: flex;
  flex-direction: column;
  min-height: 0;
  padding: 24px;
  border-radius: var(--deck-radius);
  border: 1px solid rgba(163, 183, 224, 0.18);
  background: linear-gradient(180deg, rgba(251, 253, 255, 0.98), rgba(241, 247, 255, 0.98));
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.96),
    0 22px 42px rgba(105, 131, 183, 0.1);
}

.ops-dashboard-deck.is-compact .ops-dashboard-card {
  padding: 20px;
}

.ops-dashboard-card--overview {
  grid-area: overview;
}

.ops-dashboard-card--spending {
  grid-area: spending;
  background: linear-gradient(180deg, rgba(215, 227, 255, 0.98), rgba(204, 218, 255, 0.98));
}

.ops-dashboard-card--activity {
  grid-area: activity;
  background: linear-gradient(180deg, rgba(214, 237, 231, 0.98), rgba(204, 233, 226, 0.98));
}

.ops-dashboard-card--guide {
  grid-area: guide;
  background: linear-gradient(180deg, rgba(221, 242, 236, 0.98), rgba(211, 238, 232, 0.98));
}

.card-heading {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 12px;

  > div {
    display: grid;
    gap: 6px;
  }

  h2,
  h3 {
    margin: 0;
    color: #101828;
    font-size: 21px;
    font-weight: 680;
    line-height: 1.12;
  }

  h3 {
    font-size: 19px;
  }
}

.ops-dashboard-deck.is-compact .card-heading {
  gap: 10px;

  > div {
    gap: 4px;
  }

  h2,
  h3 {
    font-size: 18px;
  }

  h3 {
    font-size: 16px;
  }
}

.card-caption,
.card-chip {
  display: inline-flex;
  align-items: center;
  min-height: 32px;
  padding: 0 12px;
  border-radius: 999px;
  font-size: 10px;
  font-weight: 700;
  letter-spacing: 0.04em;
  text-transform: uppercase;
}

.card-caption {
  background: rgba(255, 255, 255, 0.66);
  color: var(--hl-ink-soft);
}

.card-chip {
  background: rgba(255, 255, 255, 0.72);
  color: var(--hl-ink);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.86);
}

.ops-dashboard-deck.is-compact .card-caption,
.ops-dashboard-deck.is-compact .card-chip {
  min-height: 28px;
  padding: 0 10px;
  font-size: 9px;
}

.overview-total {
  margin-top: 18px;

  > span {
    display: block;
    color: #202738;
    font-size: 13px;
    font-weight: 600;
    letter-spacing: 0.01em;
  }

  p {
    max-width: 22rem;
    margin: 10px 0 0;
    color: var(--hl-ink-soft);
    font-size: 11px;
    line-height: 1.5;
  }
}

.ops-dashboard-deck.is-compact .overview-total {
  margin-top: 12px;

  > span,
  p {
    font-size: 10px;
  }

  p {
    margin-top: 8px;
    line-height: 1.45;
  }
}

.overview-total__value {
  display: flex;
  align-items: flex-end;
  gap: 2px;
  margin-top: 12px;
  color: #12131a;
  line-height: 0.92;

  strong {
    font-size: clamp(48px, 4.8vw, 64px);
    font-weight: 800;
    letter-spacing: -0.08em;
  }

  small {
    margin-bottom: 6px;
    color: #8b98b3;
    font-size: 20px;
    font-weight: 700;
  }
}

.ops-dashboard-deck.is-compact .overview-total__value {
  margin-top: 8px;

  strong {
    font-size: clamp(40px, 4.2vw, 56px);
  }

  small {
    margin-bottom: 5px;
    font-size: 16px;
  }
}

.overview-actions {
  display: flex;
  gap: 10px;
  margin-top: 20px;
}

.ops-dashboard-deck.is-compact .overview-actions {
  gap: 8px;
  margin-top: 14px;
}

.overview-actions :deep(.overview-action) {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
  min-height: 42px;
  padding: 0 18px;
  border: 1px solid rgba(153, 175, 219, 0.24);
  border-radius: 999px;
  background: rgba(255, 255, 255, 0.78);
  color: #243555;
  font-size: 12px;
  font-weight: 700;
  cursor: pointer;
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.92),
    0 14px 22px rgba(116, 141, 191, 0.08);
  transition:
    transform 0.18s ease,
    box-shadow 0.18s ease,
    border-color 0.18s ease;
}

.ops-dashboard-deck.is-compact .overview-actions :deep(.overview-action) {
  min-height: 38px;
  padding: 0 14px;
  font-size: 11px;
}

.overview-actions :deep(.overview-action:hover) {
  transform: translateY(-1px);
  border-color: rgba(126, 154, 225, 0.28);
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.92),
    0 16px 26px rgba(116, 141, 191, 0.12);
}

.overview-actions :deep(.overview-action .el-icon) {
  width: 26px;
  height: 26px;
  display: grid;
  place-items: center;
  border-radius: 50%;
  background: rgba(233, 241, 255, 0.92);
}

.overview-highlights {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 10px;
  margin-top: 14px;
}

.ops-dashboard-deck.is-compact .overview-highlights {
  gap: 8px;
  margin-top: 10px;
}

.overview-highlight {
  display: grid;
  gap: 5px;
  min-height: 104px;
  padding: 14px 16px;
  border-radius: 24px;
  border: 1px solid rgba(152, 175, 222, 0.14);
  background: rgba(255, 255, 255, 0.68);
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.9),
    0 16px 28px rgba(108, 132, 182, 0.08);

  span,
  small {
    color: rgba(19, 32, 51, 0.62);
    font-size: 10px;
    line-height: 1.5;
  }

  strong {
    color: #132033;
    font-size: 24px;
    font-weight: 760;
    letter-spacing: -0.04em;
    line-height: 1.1;
  }
}

.ops-dashboard-deck.is-compact .overview-highlight {
  min-height: 90px;
  padding: 12px 14px;

  strong {
    font-size: 20px;
  }
}

.overview-highlight.lake,
.overview-highlight.is-blue {
  background: linear-gradient(180deg, rgba(245, 249, 255, 0.92), rgba(236, 243, 255, 0.92));
}

.overview-highlight.sage,
.overview-highlight.is-mint {
  background: linear-gradient(180deg, rgba(233, 247, 243, 0.94), rgba(223, 243, 237, 0.94));
}

.overview-highlight.amber {
  background: linear-gradient(180deg, rgba(248, 242, 225, 0.94), rgba(242, 234, 210, 0.94));
}

.overview-highlight.rose {
  background: linear-gradient(180deg, rgba(249, 235, 238, 0.94), rgba(245, 228, 232, 0.94));
}

.section-heading {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  margin-top: auto;
  padding-top: 18px;

  span {
    color: #111827;
    font-size: 15px;
    font-weight: 680;
  }

  small {
    color: var(--hl-ink-soft);
    font-size: 10px;
    line-height: 1.4;
  }
}

.ops-dashboard-deck.is-compact .section-heading {
  padding-top: 14px;

  span {
    font-size: 14px;
  }

  small {
    font-size: 9px;
  }
}

.overview-cards {
  display: grid;
  grid-template-columns: minmax(0, 1fr) minmax(0, 1fr) 68px;
  gap: 10px;
  margin-top: 14px;
  align-items: stretch;
}

.ops-dashboard-deck.is-compact .overview-cards {
  grid-template-columns: minmax(0, 1fr) minmax(0, 1fr) 58px;
  gap: 8px;
  margin-top: 10px;
}

.overview-mini-card,
.overview-shortcut {
  position: relative;
  border: none;
  border-radius: 24px;
  overflow: hidden;
}

.overview-mini-card {
  min-height: 132px;
  padding: 16px;
  text-align: left;
  box-shadow: 0 20px 34px rgba(115, 142, 195, 0.12);

  span,
  small,
  em {
    display: block;
    position: relative;
    z-index: 1;
  }

  span {
    color: rgba(28, 33, 48, 0.72);
    font-size: 12px;
    font-weight: 650;
  }

  strong {
    position: relative;
    z-index: 1;
    display: block;
    margin-top: 26px;
    color: #121827;
    font-size: 24px;
    font-weight: 800;
    letter-spacing: -0.04em;
  }

  small {
    margin-top: 6px;
    color: rgba(27, 33, 48, 0.62);
    font-size: 10px;
    line-height: 1.45;
  }

  em {
    position: absolute;
    left: 18px;
    bottom: 16px;
    font-style: normal;
    color: rgba(27, 33, 48, 0.78);
    font-size: 10px;
    font-weight: 600;
  }
}

.ops-dashboard-deck.is-compact .overview-mini-card {
  min-height: 112px;
  padding: 14px;

  span,
  small,
  em {
    font-size: 9px;
  }

  strong {
    margin-top: 18px;
    font-size: 21px;
  }

  em {
    left: 14px;
    bottom: 12px;
  }
}

.overview-mini-card__ornament {
  position: absolute;
  top: 14px;
  right: 14px;
  width: 88px;
  height: 88px;
  border-radius: 50%;
  border: 1.5px solid rgba(42, 52, 74, 0.18);

  &::before,
  &::after {
    content: '';
    position: absolute;
    border-radius: 50%;
    border: 1.5px solid rgba(42, 52, 74, 0.18);
  }

  &::before {
    inset: 10px;
  }

  &::after {
    inset: -18px 26px 26px -18px;
  }
}

.ops-dashboard-deck.is-compact .overview-mini-card__ornament {
  top: 10px;
  right: 10px;
  width: 66px;
  height: 66px;

  &::before {
    inset: 8px;
  }

  &::after {
    inset: -14px 18px 18px -14px;
  }
}

.overview-mini-card.is-blue,
.overview-mini-card.lake {
  background: linear-gradient(180deg, rgba(177, 204, 255, 0.96), rgba(163, 192, 255, 0.98));
}

.overview-mini-card.is-mint,
.overview-mini-card.sage {
  background: linear-gradient(180deg, rgba(210, 239, 235, 0.96), rgba(196, 234, 227, 0.98));
}

.overview-mini-card.amber {
  background: linear-gradient(180deg, rgba(244, 236, 212, 0.96), rgba(236, 226, 196, 0.98));
}

.overview-mini-card.rose {
  background: linear-gradient(180deg, rgba(246, 225, 228, 0.96), rgba(240, 214, 219, 0.98));
}

.overview-shortcut {
  display: grid;
  place-items: center;
  min-height: 132px;
  background: #232326;
  color: #ffffff;
  box-shadow: 0 20px 30px rgba(35, 35, 38, 0.2);
}

.ops-dashboard-deck.is-compact .overview-shortcut {
  min-height: 112px;
}

.overview-shortcut--data {
  padding: 16px 10px;
  gap: 8px;
  align-content: center;
  justify-items: center;
  text-align: center;

  strong {
    font-size: 36px;
    font-weight: 800;
    line-height: 1;
    letter-spacing: -0.06em;
  }

  small {
    color: rgba(255, 255, 255, 0.82);
    font-size: 10px;
    font-weight: 700;
  }

  em {
    color: rgba(255, 255, 255, 0.62);
    font-size: 10px;
    font-style: normal;
    line-height: 1.4;
  }
}

.ops-dashboard-deck.is-compact .overview-shortcut--data strong {
  font-size: 30px;
}

.spending-stage {
  flex: 1 1 auto;
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  gap: 14px;
  margin-top: 12px;
}

.ops-dashboard-deck.is-compact .spending-stage {
  gap: 10px;
  margin-top: 10px;
}

.spending-badge {
  align-self: flex-start;
  padding: 6px 12px;
  border-radius: 999px;
  background: rgba(255, 255, 255, 0.7);
  color: #1a2540;
  font-size: 11px;
  font-weight: 700;
}

.ops-dashboard-deck.is-compact .spending-badge {
  padding: 5px 10px;
  font-size: 10px;
}

.spending-bars {
  display: grid;
  grid-template-columns: repeat(7, minmax(0, 1fr));
  gap: 8px;
  align-items: end;
  min-height: 120px;
}

.ops-dashboard-deck.is-compact .spending-bars {
  gap: 6px;
  min-height: 92px;
}

.spending-bars--quad {
  grid-template-columns: repeat(4, minmax(0, 1fr));
}

.spending-bar {
  display: grid;
  justify-items: center;
  gap: 10px;
}

.ops-dashboard-deck.is-compact .spending-bar {
  gap: 7px;
}

.spending-bar__track {
  width: 26px;
  height: 98px;
  display: flex;
  align-items: flex-end;
  justify-content: center;
  padding: 4px 0;
  border-radius: 999px;
  background: rgba(255, 255, 255, 0.32);

  span {
    width: 18px;
    border-radius: 999px;
    background: linear-gradient(180deg, rgba(255, 255, 255, 0.96), rgba(241, 246, 255, 0.96));
    box-shadow:
      inset 0 -1px 0 rgba(144, 163, 214, 0.18),
      0 10px 18px rgba(255, 255, 255, 0.28);
  }
}

.ops-dashboard-deck.is-compact .spending-bar__track {
  width: 22px;
  height: 78px;

  span {
    width: 14px;
  }
}

.spending-bar.is-peak .spending-bar__track span {
  width: 20px;
  background: linear-gradient(180deg, #8fb3ff, #7d9ff3);
  box-shadow: 0 16px 26px rgba(126, 156, 241, 0.24);
}

.ops-dashboard-deck.is-compact .spending-bar.is-peak .spending-bar__track span {
  width: 16px;
}

.spending-bar small {
  color: rgba(27, 39, 65, 0.72);
  font-size: 10px;
  font-weight: 600;
}

.ops-dashboard-deck.is-compact .spending-bar small {
  font-size: 9px;
}

.transaction-list {
  display: flex;
  flex-direction: column;
  gap: 0;
  margin-top: 8px;
}

.ops-dashboard-deck.is-compact .transaction-list {
  margin-top: 6px;
}

.transaction-item {
  display: grid;
  grid-template-columns: 46px minmax(0, 1fr) auto;
  gap: 12px;
  align-items: center;
  padding: 10px 0;
  border-bottom: 1px solid rgba(80, 115, 113, 0.12);
}

.ops-dashboard-deck.is-compact .transaction-item {
  grid-template-columns: 42px minmax(0, 1fr) auto;
  gap: 10px;
  padding: 8px 0;
}

.transaction-item:last-child {
  border-bottom: none;
}

.transaction-item__icon {
  width: 46px;
  height: 46px;
  display: grid;
  place-items: center;
  border-radius: 50%;
  border: 1px solid rgba(28, 34, 45, 0.28);
  color: #1a2435;
  font-size: 13px;
  font-weight: 700;

  &.is-blue {
    background: rgba(235, 243, 255, 0.72);
  }

  &.is-mint {
    background: rgba(236, 248, 244, 0.78);
  }
}

.ops-dashboard-deck.is-compact .transaction-item__icon {
  width: 42px;
  height: 42px;
  font-size: 11px;
}

.transaction-item__copy {
  min-width: 0;

  strong {
    display: block;
    color: #132033;
    font-size: 14px;
    font-weight: 680;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  span {
    display: block;
    margin-top: 3px;
    color: rgba(19, 32, 51, 0.62);
    font-size: 10px;
  }
}

.ops-dashboard-deck.is-compact .transaction-item__copy {
  strong {
    font-size: 13px;
  }

  span {
    margin-top: 2px;
    font-size: 9px;
  }
}

.transaction-item__amount {
  font-size: 14px;
  font-weight: 720;
  letter-spacing: -0.02em;
}

.ops-dashboard-deck.is-compact .transaction-item__amount {
  font-size: 13px;
}

.transaction-item__amount.is-positive {
  color: #0f6e55;
}

.transaction-item__amount.is-negative {
  color: #1c2433;
}

.transaction-item__amount.is-neutral {
  color: #51627e;
}

.ops-dashboard-card--guide::before,
.ops-dashboard-card--guide::after {
  content: '';
  position: absolute;
  border-radius: 50%;
  border: 1.5px solid rgba(84, 121, 188, 0.18);
}

.ops-dashboard-card--guide::before {
  right: 26px;
  bottom: 26px;
  width: 94px;
  height: 94px;
}

.ops-dashboard-card--guide::after {
  right: 70px;
  bottom: 14px;
  width: 54px;
  height: 54px;
}

.guide-hero {
  display: grid;
  gap: 6px;
  margin-top: 14px;

  strong {
    color: #132033;
    font-size: 18px;
    font-weight: 720;
    letter-spacing: -0.04em;
    line-height: 1.2;
    display: -webkit-box;
    overflow: hidden;
    -webkit-box-orient: vertical;
    -webkit-line-clamp: 2;
  }

  span {
    color: rgba(19, 32, 51, 0.68);
    font-size: 12px;
    line-height: 1.65;
    display: -webkit-box;
    overflow: hidden;
    -webkit-box-orient: vertical;
    -webkit-line-clamp: 3;
  }
}

.ops-dashboard-deck.is-compact .guide-hero {
  gap: 4px;
  margin-top: 10px;

  strong {
    font-size: 16px;
  }

  span {
    font-size: 11px;
    line-height: 1.52;
  }
}

.guide-pulse {
  display: grid;
  gap: 4px;
  margin-top: 14px;
  padding: 12px 14px;
  border-radius: 22px;
  background: linear-gradient(180deg, rgba(248, 251, 255, 0.86), rgba(237, 244, 255, 0.9));
  border: 1px solid rgba(143, 166, 228, 0.12);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.9);
}

.ops-dashboard-deck.is-compact .guide-pulse {
  margin-top: 10px;
  padding: 10px 12px;
  border-radius: 18px;
}

.guide-pulse__label {
  color: rgba(19, 32, 51, 0.62);
  font-size: 11px;
  font-weight: 700;
  letter-spacing: 0.02em;
}

.guide-pulse strong {
  color: #132033;
  font-size: 26px;
  font-weight: 780;
  letter-spacing: -0.05em;
}

.ops-dashboard-deck.is-compact .guide-pulse strong {
  font-size: 22px;
}

.guide-pulse small {
  color: rgba(19, 32, 51, 0.62);
  font-size: 11px;
  line-height: 1.5;
}

.guide-list {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 10px;
  margin-top: 14px;
}

.ops-dashboard-deck.is-compact .guide-list {
  gap: 8px;
  margin-top: 10px;
}

.guide-item {
  display: grid;
  gap: 10px;
  align-content: space-between;
  min-height: 92px;
  padding: 12px 14px;
  border-radius: 20px;
  background: rgba(255, 255, 255, 0.7);
  border: 1px solid rgba(143, 166, 228, 0.12);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.88);
}

.ops-dashboard-deck.is-compact .guide-item {
  min-height: 82px;
  gap: 8px;
  padding: 10px 12px;
}

.guide-item__badge {
  width: fit-content;
  min-width: 44px;
  min-height: 30px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  padding: 0 9px;
  border-radius: 999px;
  background: rgba(231, 241, 255, 0.92);
  color: #3b5e9f;
  font-size: 10px;
  font-weight: 700;
  letter-spacing: 0.02em;
}

.guide-item__copy strong {
  display: block;
  color: #132033;
  font-size: 12px;
  font-weight: 700;
  line-height: 1.45;
}

@media (max-width: 1180px) and (min-width: 961px) {
  .ops-dashboard-deck {
    --deck-gap: 14px;
    --deck-radius: 24px;
    grid-template-columns: minmax(0, 1.4fr) minmax(0, 0.76fr) minmax(0, 0.88fr);
    grid-template-rows: minmax(148px, auto) minmax(116px, auto);
  }

  .ops-dashboard-card {
    padding: 18px;
  }

  .card-heading {
    h2,
    h3 {
      font-size: 18px;
    }

    h3 {
      font-size: 16px;
    }
  }

  .card-caption,
  .card-chip {
    min-height: 28px;
    padding: 0 10px;
    font-size: 9px;
  }

  .overview-total {
    margin-top: 12px;

    > span,
    p {
      font-size: 10px;
    }
  }

  .overview-total__value {
    margin-top: 8px;

    strong {
      font-size: clamp(40px, 5vw, 52px);
    }

    small {
      margin-bottom: 5px;
      font-size: 16px;
    }
  }

  .overview-actions {
    margin-top: 14px;
  }

  .overview-highlights {
    gap: 8px;
    margin-top: 10px;
  }

  .overview-highlight {
    min-height: 88px;
    padding: 12px;

    strong {
      font-size: 18px;
    }
  }

  .section-heading {
    margin-top: 14px;

    span {
      font-size: 14px;
    }

    small {
      display: none;
    }
  }

  .overview-cards {
    grid-template-columns: minmax(0, 1fr) minmax(0, 0.94fr) 58px;
    margin-top: 10px;
  }

  .overview-mini-card {
    min-height: 104px;
    padding: 12px;

    span,
    small,
    em {
      font-size: 9px;
    }

    strong {
      margin-top: 16px;
      font-size: 19px;
    }

    em {
      left: 14px;
      bottom: 12px;
    }
  }

  .overview-mini-card__ornament {
    top: 10px;
    right: 10px;
    width: 64px;
    height: 64px;

    &::before {
      inset: 8px;
    }

    &::after {
      inset: -14px 18px 18px -14px;
    }
  }

  .overview-shortcut {
    min-height: 104px;
  }

  .overview-shortcut--data strong {
    font-size: 28px;
  }

  .spending-stage {
    gap: 10px;
    margin-top: 10px;
  }

  .spending-badge {
    font-size: 9px;
  }

  .spending-bars {
    min-height: 82px;
    gap: 6px;
  }

  .spending-bar {
    gap: 6px;
  }

  .spending-bar__track {
    width: 20px;
    height: 66px;

    span {
      width: 12px;
    }
  }

  .spending-bar.is-peak .spending-bar__track span {
    width: 14px;
  }

  .spending-bar small {
    font-size: 8px;
  }

  .transaction-item {
    grid-template-columns: 34px minmax(0, 1fr) auto;
    gap: 8px;
    padding: 6px 0;
  }

  .transaction-item__icon {
    width: 34px;
    height: 34px;
    font-size: 10px;
  }

  .transaction-item__copy {
    strong {
      font-size: 12px;
    }

    span {
      margin-top: 2px;
      font-size: 9px;
    }
  }

  .transaction-item__amount {
    font-size: 12px;
  }

  .guide-hero {
    margin-top: 10px;

    strong {
      font-size: 15px;
    }

    span {
      font-size: 10px;
    }
  }

  .guide-pulse {
    margin-top: 10px;
    padding: 12px;
    border-radius: 18px;
  }

  .guide-pulse strong {
    font-size: 22px;
  }

  .guide-list {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }
}

@media (max-width: 960px) {
  .ops-dashboard-deck {
    grid-template-columns: minmax(0, 1fr);
    grid-template-rows: none;
    grid-template-areas:
      'overview'
      'spending'
      'guide'
      'activity';
  }
}

@media (max-width: 640px) {
  .ops-dashboard-card {
    padding: 18px;
    border-radius: 24px;
  }

  .overview-total__value strong {
    font-size: 40px;
  }

  .overview-total__value small {
    font-size: 16px;
  }

  .overview-actions {
    flex-direction: column;
  }

  .overview-highlights {
    grid-template-columns: 1fr;
  }

  .overview-cards {
    grid-template-columns: 1fr;
  }

  .overview-shortcut {
    min-height: 92px;
  }

  .spending-bars,
  .spending-bars--quad {
    grid-template-columns: repeat(4, minmax(0, 1fr));
  }

  .guide-list {
    grid-template-columns: 1fr;
  }
}
</style>
