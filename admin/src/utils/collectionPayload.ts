type RecordLike = Record<string, unknown>

export interface NormalizedCollectionResponse<T = unknown> {
  data: RecordLike
  items: T[]
  total: number
}

function isRecordLike(value: unknown): value is RecordLike {
  return value !== null && typeof value === 'object' && !Array.isArray(value)
}

function unwrapPayload(payload: unknown): RecordLike {
  if (!isRecordLike(payload)) return {}
  const nested = payload.data
  return isRecordLike(nested) ? nested : payload
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

export function normalizeCollectionResponse<T = unknown>(
  payload: unknown,
  semanticKeys: readonly string[] = [],
): NormalizedCollectionResponse<T> {
  const data = unwrapPayload(payload)
  const items = pickArray<T>(data, [...semanticKeys, 'list', 'items'])
  const totalValue = data.total
  const total =
    typeof totalValue === 'number'
      ? totalValue
      : typeof totalValue === 'string' && totalValue.trim() !== ''
        ? Number(totalValue) || items.length
        : items.length

  return { data, items, total }
}
