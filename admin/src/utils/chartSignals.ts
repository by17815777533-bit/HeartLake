/**
 * 图表信号辅助函数。
 *
 * 用于从真实序列中提炼更柔和的“陪伴基线”，
 * 让主趋势图保留真实数据的同时，拥有更接近参考图的双层波形语言。
 */
export function createSoftBaseline(values: number[]): number[] {
  if (!values.length) return []

  return values.map((value, index, source) => {
    const prev = source[Math.max(0, index - 1)] ?? value
    const next = source[Math.min(source.length - 1, index + 1)] ?? value
    return Math.round((prev + value * 2 + next) / 4)
  })
}
