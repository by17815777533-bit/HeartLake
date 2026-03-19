export function getWorkbenchTileTone(tone?: string): string {
  const map: Record<string, string> = {
    lake: 'is-blue',
    sage: 'is-mint',
    amber: 'is-plain',
    rose: 'is-blue',
  }

  return map[tone || ''] || 'is-plain'
}
