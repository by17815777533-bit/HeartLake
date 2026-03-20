type SummaryTone = 'lake' | 'sage' | 'amber' | 'rose' | 'plain' | 'is-blue' | 'is-mint'

interface SummaryItem {
  label: string
  value: string
  note: string
  tone?: SummaryTone
}

interface SignalItem {
  label: string
  value: string
  note: string
  badge?: string
  tone?: SummaryTone
}

interface MetricItem {
  label: string
  value: string
  note?: string
}

interface BarItem {
  label: string
  value: number | string
}

const normalizeTone = (tone?: SummaryTone) => {
  if (tone === 'sage' || tone === 'amber' || tone === 'rose') return tone
  return 'lake'
}

export const createDeckOverviewCards = (items: SummaryItem[]) =>
  items.slice(0, 2).map((item) => ({
    label: item.label,
    value: item.value,
    note: item.note,
    footer: item.note,
    tone: normalizeTone(item.tone),
  }))

export const createDeckFocusCard = (item?: SummaryItem, fallbackNote = '') => ({
  label: item?.label || '重点指标',
  value: item?.value || '0',
  note: fallbackNote || item?.note || '保持当前巡看节奏。',
})

export const createDeckRhythmItems = (items: BarItem[]) =>
  items.map((item) => ({
    label: item.label,
    value: Number(item.value) || 0,
  }))

const resolveActivityTone = (tone?: SummaryTone) => {
  if (tone === 'sage') return 'is-positive'
  if (tone === 'rose') return 'is-negative'
  return 'is-neutral'
}

const resolveActivityIconTone = (tone?: SummaryTone) => {
  if (tone === 'sage') return 'is-mint'
  return 'is-blue'
}

export const createDeckActivityRows = (items: SignalItem[]) =>
  items.slice(0, 3).map((item, index) => ({
    id: `${item.label}-${index}`,
    badge: item.label.replace(/\s+/g, '').slice(0, 2) || String(index + 1),
    title: item.value,
    meta: item.note,
    amount: item.badge || '--',
    tone: resolveActivityTone(item.tone),
    iconTone: resolveActivityIconTone(item.tone),
  }))

export const createDeckGuideItems = (items: MetricItem[]) =>
  items.slice(0, 3).map((item) => ({
    label: item.label,
    value: item.value,
    note: item.note || '',
  }))
