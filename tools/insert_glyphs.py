# -*- coding: utf-8 -*-
# Inserts generated CJK glyphs into cjk_font.h:
#  - appends bitmap bytes at the end of F12_BITS / F16_BITS (existing offsets stay valid)
#  - inserts glyph entries into F12_GLYPHS / F16_GLYPHS keeping cp sort order (binary-searched)
#  - updates glyph counts
import re, sys

ROOT = r"e:\Desktop\GitTemp\ESP32\CYD_DD"
FONT_H = ROOT + r"\src\ui\cjk_font.h"
GLYPHS_TXT = ROOT + r"\tools\glyphs.txt"

# (cp, size, hexlist)
glyphs = []
raw = open(GLYPHS_TXT, "rb").read()
if raw.startswith(b"\xff\xfe") or raw.startswith(b"\xfe\xff"):
    text = raw.decode("utf-16")
else:
    try:
        text = raw.decode("utf-8")
    except UnicodeDecodeError:
        text = raw.decode("gbk")
for line in text.splitlines():
    line = line.strip()
    if "|" not in line:
        continue
    parts = line.split("|")
    if len(parts) != 3:
        continue
    cp_s, size_s, hex_s = parts
    cp_s = cp_s.strip().lstrip("\ufeff")
    if not (cp_s.isdigit() and size_s.isdigit()):
        continue
    toks = re.findall(r"0x[0-9A-Fa-f]{2}", hex_s)
    size = int(size_s)
    want = size * 2
    if len(toks) != want:
        print("ERROR cp=%s size=%d: %d tokens, want %d" % (cp_s, size, len(toks), want))
        print("  raw:", hex_s)
        sys.exit(1)
    glyphs.append((int(cp_s), size, toks))

for size in (12, 16):
    n = sum(1 for g in glyphs if g[1] == size)
    print("parsed %d glyphs at %dpx" % (n, size))
if len(glyphs) != 10:
    print("ERROR: expected 10 glyph rows, got %d" % len(glyphs))
    sys.exit(1)

src = open(FONT_H, encoding="utf-8").read()

def array_span(text, decl):
    """Return (start,end) char offsets of the brace body of array declaration."""
    i = text.index(decl)
    j = text.index("{", i)
    k = text.index("};", j)
    return j + 1, k  # body between braces

def count_tokens(body):
    return len(re.findall(r"0x[0-9A-Fa-f]{2}", body))

# ---- 12px ----
b12s, b12e = array_span(src, "static const uint8_t F12_BITS[] PROGMEM")
base12 = count_tokens(src[b12s:b12e])
print("F12_BITS existing bytes:", base12)

# ---- 16px ----
b16s, b16e = array_span(src, "static const uint8_t F16_BITS[] PROGMEM")
base16 = count_tokens(src[b16s:b16e])
print("F16_BITS existing bytes:", base16)

def insert_sorted(text, table_decl, entry_re, new_entries):
    """Insert '{ cp, adv, FXX_BITS + off }' entries keeping cp order.
    new_entries: list of (cp, adv, offset) already formatted strings."""
    i = text.index(table_decl)
    j = text.index("{", i)
    k = text.index("};", j)
    body = text[j + 1:k]
    lines = body.split("\n")
    out = []
    inserted = 0
    for ln in lines:
        m = re.search(entry_re, ln)
        if m and inserted < len(new_entries):
            cp = int(m.group(1), 16)
            while inserted < len(new_entries) and new_entries[inserted][0] < cp:
                out.append("  " + new_entries[inserted][2] + ",\n")
                inserted += 1
        out.append(ln)
    while inserted < len(new_entries):
        out.append("  " + new_entries[inserted][2] + ",\n")
        inserted += 1
    return text[:j + 1] + "".join(out) + text[k:]

# build append payloads (sorted by cp for deterministic offsets)
g12 = sorted([g for g in glyphs if g[1] == 12], key=lambda t: t[0])
g16 = sorted([g for g in glyphs if g[1] == 16], key=lambda t: t[0])

add12_lines = []
for idx, (cp, _, toks) in enumerate(g12):
    add12_lines.append("  " + ",".join(toks) + ",")
add12 = "\n" + "\n".join(add12_lines) + "\n"

add16_lines = []
for idx, (cp, _, toks) in enumerate(g16):
    add16_lines.append("  " + ",".join(toks) + ",")
add16 = "\n" + "\n".join(add16_lines) + "\n"

# append bytes: insert just before the closing '};' of each bits array
src = src[:b12e] + add12 + src[b12e:]
# recompute 16px span after first insertion (offsets shifted)
b16s, b16e = array_span(src, "static const uint8_t F16_BITS[] PROGMEM")
src = src[:b16e] + add16 + src[b16e:]

# glyph entries
ent12 = []
for idx, (cp, _, _t) in enumerate(g12):
    ent12.append((cp, 12, "{ 0x%04X, 12, F12_BITS + %d }" % (cp, base12 + idx * 24)))
ent16 = []
for idx, (cp, _, _t) in enumerate(g16):
    ent16.append((cp, 16, "{ 0x%04X, 16, F16_BITS + %d }" % (cp, base16 + idx * 32)))

src = insert_sorted(src, "static const CjkGlyph F12_GLYPHS[]", r"\{\s*0x([0-9A-Fa-f]+),\s*12,", ent12)
src = insert_sorted(src, "static const CjkGlyph F16_GLYPHS[]", r"\{\s*0x([0-9A-Fa-f]+),\s*16,", ent16)

src = src.replace("CJK_FONT_12 = { F12_GLYPHS, 320,", "CJK_FONT_12 = { F12_GLYPHS, 325,")
src = src.replace("CJK_FONT_16 = { F16_GLYPHS, 320,", "CJK_FONT_16 = { F16_GLYPHS, 325,")
src = src.replace("// ---- 12 px (320 glyphs) ----", "// ---- 12 px (325 glyphs) ----")
src = src.replace("// ---- 16 px (320 glyphs) ----", "// ---- 16 px (325 glyphs) ----")

open(FONT_H, "w", encoding="utf-8", newline="\n").write(src)

# verification: sorted order + counts
chk = open(FONT_H, encoding="utf-8").read()
for decl, adv in (("F12_GLYPHS", 12), ("F16_GLYPHS", 16)):
    i = chk.index("static const CjkGlyph %s[]" % decl)
    j = chk.index("{", i)
    k = chk.index("};", j)
    cps = [int(m, 16) for m in re.findall(r"\{\s*0x([0-9A-Fa-f]+),\s*%d," % adv, chk[j:k])]
    assert cps == sorted(cps), "not sorted in %s" % decl
    print(decl, "entries:", len(cps), "sorted OK")
print("done")
