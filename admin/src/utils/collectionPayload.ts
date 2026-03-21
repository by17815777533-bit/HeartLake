type RecordLike = Record<string, unknown>

export interface NormalizedCollectionResponse<T = unknown> {
  data: RecordLike
  items: T[]
  total: number
}

function isRecordLike(value: unknown): value is RecordLike {
  return value !== null && typeof value === 'object' && !Array.isArray(value)
}

function pickNumber(value: unknown): number | null {
  if (typeof value === 'number' && Number.isFinite(value)) {
    return value
  }
  if (typeof value === 'string' && value.trim() !== '') {
    const parsed = Number(value)
    return Number.isFinite(parsed) ? parsed : null
  }
  return null
}

function buildArrayPayload<T>(items: T[], fallback: RecordLike = {}): RecordLike {
  const pagination = isRecordLike(fallback.pagination) ? fallback.pagination : null
  return {
    ...fallback,
    data: items,
    list: items,
    items,
    total:
      pickNumber(fallback.total) ??
      pickNumber(fallback.count) ??
      pickNumber(fallback.total_count) ??
      (pagination ? pickNumber(pagination.total) : null) ??
      items.length,
    count: pickNumber(fallback.count) ?? items.length,
  }
}

function unwrapPayload(payload: unknown): RecordLike {
  if (Array.isArray(payload)) {
    return buildArrayPayload(payload)
  }
  if (!isRecordLike(payload)) return {}

  const nested = payload.data
  if (Array.isArray(nested)) {
    return buildArrayPayload(nested, payload)
  }
  if (isRecordLike(nested) && nested !== payload) {
    return { ...payload, ...unwrapPayload(nested) }
  }
  return payload
}

function pickArray<T>(data: RecordLike, keys: readonly string[]): T[] {
  for (const key of keys) {
    const candidate = data[key]
    if (Array.isArray(candidate)) {
      return candidate as T[]
    }
  }
  return []
}

export function normalizePayloadRecord(payload: unknown): RecordLike {
  return unwrapPayload(payload)
}

export function normalizeCollectionResponse<T = unknown>(
  payload: unknown,
  semanticKeys: readonly string[] = [],
): NormalizedCollectionResponse<T> {
  const data = normalizePayloadRecord(payload)
  const items = pickArray<T>(data, [...semanticKeys, 'list', 'items'])
  const pagination = isRecordLike(data.pagination) ? data.pagination : null
  const total =
    pickNumber(data.total) ??
    pickNumber(data.count) ??
    pickNumber(data.total_count) ??
    (pagination ? pickNumber(pagination.total) : null) ??
    items.length

  return { data, items, total }
}
