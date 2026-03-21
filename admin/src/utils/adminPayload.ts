import { normalizeCollectionResponse, normalizePayloadRecord } from '@/utils/collectionPayload'
import type { ContentItem } from '@/types'

export type AdminRecord = Record<string, unknown>

type ContentType = 'stone' | 'boat'

function isRecord(value: unknown): value is AdminRecord {
  return value !== null && typeof value === 'object' && !Array.isArray(value)
}

function hasValue(value: unknown): boolean {
  if (value == null) return false
  if (typeof value === 'string') return value.trim() !== ''
  return true
}

export function normalizeAdminPayload(payload: unknown): AdminRecord {
  return normalizePayloadRecord(payload)
}

export function pickRecord(
  source: AdminRecord | null | undefined,
  keys: readonly string[],
): AdminRecord | undefined {
  if (!source) return undefined
  for (const key of keys) {
    const candidate = source[key]
    if (isRecord(candidate)) return candidate
  }
  return undefined
}

export function pickArray<T = unknown>(
  source: AdminRecord | null | undefined,
  keys: readonly string[],
): T[] {
  if (!source) return []
  for (const key of keys) {
    const candidate = source[key]
    if (Array.isArray(candidate)) return candidate as T[]
  }
  return []
}

export function pickString(
  source: AdminRecord | null | undefined,
  keys: readonly string[],
): string | undefined {
  if (!source) return undefined
  for (const key of keys) {
    const candidate = source[key]
    if (typeof candidate === 'string' && candidate.trim() !== '') return candidate
  }
  return undefined
}

export function pickNumber(
  source: AdminRecord | null | undefined,
  keys: readonly string[],
): number | undefined {
  if (!source) return undefined
  for (const key of keys) {
    const candidate = source[key]
    if (typeof candidate === 'number' && Number.isFinite(candidate)) return candidate
    if (typeof candidate === 'string' && candidate.trim() !== '') {
      const parsed = Number(candidate)
      if (Number.isFinite(parsed)) return parsed
    }
  }
  return undefined
}

export function pickBoolean(
  source: AdminRecord | null | undefined,
  keys: readonly string[],
): boolean | undefined {
  if (!source) return undefined
  for (const key of keys) {
    const candidate = source[key]
    if (typeof candidate === 'boolean') return candidate
    if (candidate === 1 || candidate === '1' || candidate === 'true') return true
    if (candidate === 0 || candidate === '0' || candidate === 'false') return false
  }
  return undefined
}

export function pickIdentifier(
  source: AdminRecord | null | undefined,
  keys: readonly string[],
): string | number | undefined {
  if (!source) return undefined
  for (const key of keys) {
    const candidate = source[key]
    if (typeof candidate === 'string' && candidate.trim() !== '') return candidate
    if (typeof candidate === 'number' && Number.isFinite(candidate)) return candidate
  }
  return undefined
}

export function toTimestamp(value: unknown): number {
  if (typeof value === 'number' && Number.isFinite(value)) return value
  if (typeof value === 'string' && value.trim() !== '') {
    const parsed = new Date(value).getTime()
    return Number.isFinite(parsed) ? parsed : 0
  }
  return 0
}

export function sortByCreatedAtDesc<T extends { created_at?: string }>(items: T[]): T[] {
  return [...items].sort((left, right) => toTimestamp(right.created_at) - toTimestamp(left.created_at))
}

export function toStringList(value: unknown): string[] {
  if (Array.isArray(value)) {
    return value
      .filter((item): item is string | number => typeof item === 'string' || typeof item === 'number')
      .map((item) => String(item))
      .filter(Boolean)
  }

  if (isRecord(value)) {
    return Object.entries(value)
      .filter(([, score]) => {
        if (typeof score === 'number') return score > 0
        if (typeof score === 'string') return score.trim() !== '' && Number(score) > 0
        return Boolean(score)
      })
      .sort((left, right) => Number(right[1]) - Number(left[1]))
      .map(([label]) => label)
  }

  return []
}

export function normalizeContentItem(payload: unknown, typeHint?: ContentType): ContentItem {
  const source = normalizeAdminPayload(payload)
  const inferredType =
    typeHint
    ?? (hasValue(source.boat_id) ? 'boat' : hasValue(source.stone_id) ? 'stone' : undefined)
    ?? (pickString(source, ['type', 'content_type', 'target_type']) === 'boat' ? 'boat' : 'stone')
  const contentType = inferredType as ContentType
  const nickname = pickString(source, ['author_nickname', 'nickname', 'user_nickname'])
  const rawStatus = pickString(source, ['status']) ?? 'published'

  return {
    id: pickIdentifier(source, contentType === 'boat' ? ['boat_id', 'id'] : ['stone_id', 'id']) ?? '',
    user_id: pickString(source, ['user_id', 'author_id', 'sender_id']) ?? '',
    nickname,
    user: nickname ? { nickname } : undefined,
    type: contentType,
    content: pickString(source, ['content', 'text', 'body']) ?? '',
    mood: pickString(source, ['mood', 'mood_type']),
    status: contentType === 'boat' && rawStatus === 'active' ? 'published' : rawStatus,
    created_at: pickString(source, ['created_at', 'timestamp']) ?? '',
    likes_count: pickNumber(source, ['likes_count', 'like_count', 'likes']),
    replies_count: pickNumber(source, ['replies_count', 'reply_count', 'replies', 'boat_count']),
  }
}

export function normalizeContentCollection(
  payload: unknown,
  type: ContentType | undefined,
  semanticKeys: readonly string[],
): { items: ContentItem[]; total: number } {
  const { items, total } = normalizeCollectionResponse<AdminRecord>(payload, semanticKeys)
  return {
    items: items.map((item) => normalizeContentItem(item, type)),
    total,
  }
}
