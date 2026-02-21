#!/usr/bin/env python3
"""
M3 迁移脚本 v2 - 处理 const 上下文中的 AppTheme.* 引用
策略:
1. 替换 AppTheme.* -> colorScheme.* (按 MAPPING)
2. 移除包含 colorScheme 的表达式中的 const 关键字
3. 确保每个使用 colorScheme 的 build 方法中有定义
4. 跳过 KEEP 集合中的业务颜色
5. 处理默认参数值等特殊情况
"""

import re
import os
import glob

MAPPING = {
    'AppTheme.primaryColor': 'colorScheme.primary',
    'AppTheme.primaryLightColor': 'colorScheme.primary',
    'AppTheme.primaryDarkColor': 'colorScheme.primaryContainer',
    'AppTheme.secondaryColor': 'colorScheme.secondary',
    'AppTheme.tertiaryColor': 'colorScheme.tertiary',
    'AppTheme.accentColor': 'colorScheme.tertiary',
    'AppTheme.errorColor': 'colorScheme.error',
    'AppTheme.warmPink': 'colorScheme.error',
    'AppTheme.backgroundColor': 'colorScheme.surface',
    'AppTheme.nightDeep': 'colorScheme.surface',
    'AppTheme.nightSurface': 'colorScheme.surfaceContainerHighest',
    'AppTheme.textPrimary': 'colorScheme.onSurface',
    'AppTheme.textSecondary': 'colorScheme.onSurfaceVariant',
    'AppTheme.textTertiary': 'colorScheme.outline',
    'AppTheme.darkTextPrimary': 'colorScheme.onSurface',
    'AppTheme.darkTextSecondary': 'colorScheme.onSurfaceVariant',
    'AppTheme.warmOrange': 'colorScheme.primary',
    'AppTheme.peachPink': 'colorScheme.primaryContainer',
    'AppTheme.candleGlow': 'colorScheme.primaryContainer',
    'AppTheme.purpleColor': 'colorScheme.secondary',
    'AppTheme.spiritBlue': 'colorScheme.secondary',
    'AppTheme.borderCyan': 'colorScheme.outline',
    'AppTheme.cloudPink': 'colorScheme.surfaceContainerHighest',
    'AppTheme.skyBlue': 'colorScheme.surfaceContainerHigh',
    'AppTheme.starPurple': 'colorScheme.inverseSurface',
    'AppTheme.darkBlue': 'colorScheme.surface',
}

# 业务颜色，不迁移
KEEP = {
    'AppTheme.lakeSurface', 'AppTheme.lakeMiddle', 'AppTheme.lakeDeep',
    'AppTheme.lakeBackground', 'AppTheme.lightStone', 'AppTheme.mediumStone',
    'AppTheme.heavyStone', 'AppTheme.successColor', 'AppTheme.warningColor',
    'AppTheme.rainbowColors', 'AppTheme.warmGradient', 'AppTheme.sunsetGradient',
    'AppTheme.nightGradient', 'AppTheme.seedColor',
    'AppTheme.lightTheme', 'AppTheme.darkTheme', 'AppTheme.fallbackColorScheme',
}

# 排除的文件
EXCLUDE_FILES = {'app_theme.dart', 'mood_colors.dart', 'stone_card.dart'}


def find_dart_files():
    """查找所有需要迁移的 dart 文件"""
    base = os.path.join(os.path.dirname(__file__), 'lib')
    files = glob.glob(os.path.join(base, '**', '*.dart'), recursive=True)
    return [f for f in files if os.path.basename(f) not in EXCLUDE_FILES]


def has_mappable_ref(line):
    """检查行中是否有可映射的 AppTheme 引用"""
    for key in MAPPING:
        if key in line:
            # 确保不是 KEEP 中的
            is_keep = False
            for k in KEEP:
                if k in line:
                    is_keep = True
                    break
            if not is_keep:
                return True
    return False


def replace_apptheme(line):
    """替换行中的 AppTheme.* 引用"""
    changed = False
    for old, new in sorted(MAPPING.items(), key=lambda x: -len(x[0])):
        if old in line:
            # 确认不是 KEEP 中更长的匹配
            skip = False
            for k in KEEP:
                if k in line and old in k:
                    skip = True
                    break
            if not skip:
                line = line.replace(old, new)
                changed = True
    return line, changed


def remove_const_from_line(line):
    """移除行中的 const 关键字（如果该行包含 colorScheme）"""
    if 'colorScheme.' not in line:
        return line

    # 移除行首的 const（如 "const TextStyle(...)" -> "TextStyle(...)"）
    # 以及 "const [" -> "["
    # 但保留 "const EdgeInsets" 等不含 colorScheme 的

    # 策略：移除所有 const 关键字，因为该行已包含运行时值
    result = re.sub(r'\bconst\s+', '', line)
    return result


def process_file(filepath):
    """处理单个文件"""
    with open(filepath, 'r') as f:
        lines = f.readlines()

    original = lines[:]
    new_lines = []
    replacements = 0
    needs_colorscheme = set()  # 需要 colorScheme 定义的方法起始行号

    # === Pass 1: 替换 AppTheme.* 引用 ===
    for i, line in enumerate(lines):
        new_line, changed = replace_apptheme(line)
        if changed:
            replacements += 1
            # 移除该行的 const
            new_line = remove_const_from_line(new_line)
        new_lines.append(new_line)

    if replacements == 0:
        return 0

    lines = new_lines

    # === Pass 2: 处理包含 colorScheme 的 const 上下文 ===
    # 有些 const 在上面的行，需要向上查找并移除
    new_lines = []
    for i, line in enumerate(lines):
        new_lines.append(line)

    # 向上扫描：如果某行有 colorScheme.，检查上面的行是否有对应的 const
    lines = new_lines
    new_lines = list(lines)

    for i in range(len(lines)):
        if 'colorScheme.' in lines[i]:
            # 向上查找最近的 const（在同一个表达式中）
            # 检查当前行是否在一个 const 构造器/列表中
            _remove_enclosing_const(new_lines, i)

    # === Pass 3: 确保 colorScheme 定义存在 ===
    lines = new_lines
    new_lines = _ensure_colorscheme_defined(lines, filepath)

    # === Pass 4: 处理默认参数值 ===
    new_lines = _handle_default_params(new_lines)

    if new_lines == original:
        return 0

    with open(filepath, 'w') as f:
        f.writelines(new_lines)

    return replacements


def _remove_enclosing_const(lines, target_idx):
    """移除包围 target_idx 行的 const 关键字"""
    # 向上扫描，找到包含 const 的行
    # 追踪括号深度
    depth = 0
    for i in range(target_idx, -1, -1):
        line = lines[i]
        # 计算该行的括号
        for ch in reversed(line):
            if ch in (')', ']', '}'):
                depth += 1
            elif ch in ('(', '[', '{'):
                depth -= 1

        # 如果找到了 const 关键字且在合理范围内
        if 'const ' in line and i != target_idx:
            # 检查这个 const 是否是包围 target 的
            # 简单启发式：如果 depth <= 0，说明我们到了表达式的开始
            stripped = line.lstrip()
            if stripped.startswith('const ') or re.search(r':\s*const\s', line) or re.search(r'=\s*const\s', line) or re.search(r',\s*const\s', line) or re.search(r'\(\s*const\s', line) or re.search(r'\[\s*const\s', line):
                lines[i] = re.sub(r'\bconst\s+', '', line, count=1)
                break

            if depth <= 0:
                break
        if depth <= 0 and i < target_idx:
            break


def _ensure_colorscheme_defined(lines, filepath):
    """确保每个使用 colorScheme 的 build/方法中有定义"""
    result = list(lines)

    # 找到所有使用 colorScheme. 的行
    colorscheme_lines = set()
    for i, line in enumerate(lines):
        if 'colorScheme.' in line and 'final colorScheme' not in line and 'Theme.of(context).colorScheme' not in line:
            colorscheme_lines.add(i)

    if not colorscheme_lines:
        return result

    # 找到所有方法/函数的起始位置
    # 查找 "Widget build(BuildContext context)" 和其他含 BuildContext 的方法
    method_ranges = []
    i = 0
    while i < len(lines):
        line = lines[i]
        # 匹配方法签名（含 BuildContext）
        if 'BuildContext' in line and ('{' in line or (i + 1 < len(lines) and '{' in lines[i + 1])):
            # 找到方法体的开始 {
            start = i
            brace_line = i
            if '{' not in line:
                brace_line = i + 1

            # 找到方法体的结束 }
            depth = 0
            for j in range(brace_line, len(lines)):
                for ch in lines[j]:
                    if ch == '{':
                        depth += 1
                    elif ch == '}':
                        depth -= 1
                if depth == 0 and j > brace_line:
                    method_ranges.append((start, brace_line, j))
                    break
        i += 1

    # 对每个方法，检查是否需要插入 colorScheme 定义
    insertions = []
    for start, brace_line, end in method_ranges:
        # 检查该方法范围内是否有 colorScheme 使用
        has_usage = any(ln in colorscheme_lines for ln in range(start, end + 1))
        if not has_usage:
            continue

        # 检查是否已有定义
        has_def = False
        for j in range(start, end + 1):
            if 'final colorScheme' in lines[j] or 'var colorScheme' in lines[j]:
                has_def = True
                break

        if not has_def:
            # 在 { 后面插入定义
            # 找到 { 所在行
            insert_after = brace_line
            # 获取缩进
            indent = '    '
            if insert_after + 1 < len(lines):
                next_line = lines[insert_after + 1]
                match = re.match(r'^(\s+)', next_line)
                if match:
                    indent = match.group(1)

            insertions.append((insert_after + 1, f'{indent}final colorScheme = Theme.of(context).colorScheme;\n'))

    # 从后往前插入，避免行号偏移
    for pos, text in sorted(insertions, reverse=True):
        result.insert(pos, text)

    return result


def _handle_default_params(lines):
    """处理默认参数值中的 colorScheme 引用"""
    result = []
    for line in lines:
        # 默认参数值不能用运行时值
        # 如 "this.color = colorScheme.primary" -> "this.color" (移除默认值，在构造函数体中设置)
        # 更简单的方案：将默认值改为 null，然后在使用处用 ?? colorScheme.primary
        # 但这改动太大，暂时用 Color 字面量替代
        if re.search(r'this\.\w+\s*=\s*colorScheme\.', line):
            # 替换为 null 默认值
            line = re.sub(r'(this\.\w+)\s*=\s*colorScheme\.\w+', r'\1', line)
        result.append(line)
    return result


def main():
    files = find_dart_files()
    print(f'扫描 {len(files)} 个文件...')

    total = 0
    modified = []
    for f in sorted(files):
        count = process_file(f)
        if count > 0:
            rel = os.path.relpath(f)
            modified.append((rel, count))
            total += count
            print(f'  ✅ {rel}: {count} 处替换')

    print(f'\n总计: {len(modified)} 个文件, {total} 处替换')

    if not modified:
        print('没有需要迁移的引用。')


if __name__ == '__main__':
    main()
