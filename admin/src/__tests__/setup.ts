// Vitest setup: 提供 localStorage / sessionStorage mock（jsdom 环境下可能不完整）

function createStorageMock(): Storage {
  const store: Record<string, string> = {}
  return {
    getItem: (key: string): string | null => store[key] ?? null,
    setItem: (key: string, value: string): void => { store[key] = String(value) },
    removeItem: (key: string): void => { delete store[key] },
    clear: (): void => { Object.keys(store).forEach(k => delete store[k]) },
    get length(): number { return Object.keys(store).length },
    key: (i: number): string | null => Object.keys(store)[i] ?? null,
  }
}

Object.defineProperty(globalThis, 'localStorage', { value: createStorageMock(), writable: true })
Object.defineProperty(globalThis, 'sessionStorage', { value: createStorageMock(), writable: true })
