#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Genera las fuentes LVGL con los caracteres chinos usados por el proyecto.

Recorre main/**/*.{c,cpp,h}, recoge todos los caracteres CJK (y puntuacion CJK)
que aparecen como literales y llama a lv_font_conv (via npx) para emitir:

  main/ui/digifont.c       24 px  -> interfaz principal
  main/ui/digifont_small.c 16 px  -> textos secundarios
  main/ui/digifont_big.c   64 px  -> digitos grandes (bateria / contador)
  main/ui/digifont_pet.c   96 px  -> caracter grande que hace de mascota

Re-ejecutar cuando se anadan textos nuevos:
  python tools/gen_font.py
"""
import os
import re
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, 'main')
OUTDIR = os.path.join(SRC, 'ui')
FONT = os.environ.get('PET_FONT', r'C:\Windows\Fonts\simhei.ttf')

# Respaldo si no se puede leer la config: primeros caracteres de las especies.
PET_SYMBOLS = '黑球兽滚亚古暴龙机械战斗丧尸'
DIGITS = '0123456789:% '

CJK_SET = 'cjk'
JOBS = [
    (30, 1, 'digifont.c',       CJK_SET),
    (20, 1, 'digifont_small.c', CJK_SET),
    (64, 1, 'digifont_big.c',   DIGITS),
    (96, 1, 'digifont_pet.c',   'pet'),
]


def collect():
    chars = set()
    for root, _dirs, files in os.walk(SRC):
        for fn in files:
            if not fn.lower().endswith(('.c', '.cpp', '.h')):
                continue
            if fn.startswith('digifont'):
                continue
            try:
                txt = open(os.path.join(root, fn), encoding='utf-8-sig').read()
            except UnicodeDecodeError:
                continue
            for ch in txt:
                if '\u4e00' <= ch <= '\u9fff' or '\u3000' <= ch <= '\u303f' or '\uff00' <= ch <= '\uffef':
                    chars.add(ch)
    return chars


def pet_symbols():
    """Primer caracter de cada especie de digimon_cfg.cpp (avatar de la mascota)."""
    path = os.path.join(SRC, 'app', 'digimon_cfg.cpp')
    try:
        txt = open(path, encoding='utf-8-sig').read()
    except OSError:
        return PET_SYMBOLS
    chars = {'数', '蛋'}   # respaldo "数码蛋" y avatar de huevo
    for m in re.finditer(r'"name"\s*:\s*"([^"]+)"', txt):
        if m.group(1):
            chars.add(m.group(1)[0])
    return ''.join(sorted(chars)) if chars else PET_SYMBOLS


def build(size, bpp, outfile, symbols):
    env = dict(os.environ)
    env.setdefault('npm_config_registry', 'https://registry.npmmirror.com')
    cache = os.path.join(tempfile.gettempdir(), 'opencode', 'npmcache')
    os.makedirs(cache, exist_ok=True)
    env['npm_config_cache'] = cache
    out = os.path.join(OUTDIR, outfile)
    cmd = [
        'npx', '--yes', 'lv_font_conv',
        '--font', FONT,
        '--size', str(size), '--bpp', str(bpp),
        '--range', '0x20-0x7f',
        '--symbols', symbols,
        '--format', 'lvgl',
        '--lv-include', 'lvgl.h',
        '--force-fast-kern-format',
        '-o', out,
    ]
    print('generando %s (%d px, bpp %d)' % (out, size, bpp))
    r = subprocess.run(cmd, env=env, shell=(os.name == 'nt'))
    if r.returncode != 0:
        sys.exit('lv_font_conv fallo para ' + outfile)
    txt = open(out, encoding='utf-8').read()
    m = re.search(r'const lv_font_t (\w+)', txt)
    print('  simbolo generado:', m.group(1) if m else '?')


def main():
    if not os.path.exists(FONT):
        sys.exit('no encuentro la fuente TTF: ' + FONT)
    chars = collect()
    if not chars:
        print('sin caracteres CJK; nada que hacer')
        return
    symbols = ''.join(sorted(chars))
    pet = pet_symbols()
    print('CJK unicos: %d -> %s' % (len(chars), symbols[:60] + ('...' if len(symbols) > 60 else '')))
    print('avatar de mascota: %s' % pet)
    for size, bpp, fn, symset in JOBS:
        if symset == CJK_SET:
            sym = symbols
        elif symset == 'pet':
            sym = pet
        else:
            sym = symset
        build(size, bpp, fn, sym)


if __name__ == '__main__':
    main()
