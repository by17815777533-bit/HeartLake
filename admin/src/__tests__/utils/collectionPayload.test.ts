import { describe, expect, it } from 'vitest'
import { normalizeCollectionResponse } from '@/utils/collectionPayload'

describe('normalizeCollectionResponse', () => {
  it('prefers semantic keys from nested data payloads', () => {
    const result = normalizeCollectionResponse(
      {
        code: 0,
        data: {
          users: [{ user_id: 'u1' }],
          list: [{ user_id: 'legacy' }],
          total: 3,
        },
      },
      ['users'],
    )

    expect(result.items).toEqual([{ user_id: 'u1' }])
    expect(result.total).toBe(3)
  })

  it('falls back to list and items when semantic key is absent', () => {
    expect(normalizeCollectionResponse({ data: { list: [{ id: 1 }] } }, ['reports']).items).toEqual(
      [{ id: 1 }],
    )
    expect(
      normalizeCollectionResponse({ data: { items: [{ id: 2 }] } }, ['reports']).items,
    ).toEqual([{ id: 2 }])
  })

  it('derives total from item count when total is missing or invalid', () => {
    expect(
      normalizeCollectionResponse({ data: { words: [{ id: 1 }, { id: 2 }] } }, ['words']).total,
    ).toBe(2)
    expect(
      normalizeCollectionResponse({ data: { words: [{ id: 1 }], total: '' } }, ['words']).total,
    ).toBe(1)
  })

  it('returns an empty collection for malformed payloads', () => {
    expect(normalizeCollectionResponse(null, ['logs'])).toEqual({
      data: {},
      items: [],
      total: 0,
    })
    expect(normalizeCollectionResponse({ data: [] }, ['logs'])).toEqual({
      data: {
        data: [],
        list: [],
        items: [],
        total: 0,
        count: 0,
      },
      items: [],
      total: 0,
    })
  })
})
