#!/usr/bin/env python3
"""
verify_payload.py - static check that a Slinky payload jar really speaks the namespace it claims.

Parses every dev/slinky/**.class constant pool (no JVM needed) and reports references to Minecraft
classes/members that still use the WRONG naming scheme for the target namespace.

usage: python verify_payload.py <jar> <named|srg|official> <mappings-file>
"""
import sys, zipfile, struct, collections

def parse_cp(data):
    pos = 8
    n = struct.unpack('>H', data[pos:pos+2])[0]; pos += 2
    cp = [None]*n; i = 1
    while i < n:
        t = data[pos]; pos += 1
        if t == 1:
            l = struct.unpack('>H', data[pos:pos+2])[0]; pos += 2
            cp[i] = ('utf8', data[pos:pos+l].decode('utf-8', 'replace')); pos += l
        elif t in (3, 4): cp[i] = ('n', data[pos:pos+4]); pos += 4
        elif t in (5, 6): cp[i] = ('n', data[pos:pos+8]); pos += 8; i += 1
        elif t in (7, 8, 16, 19, 20): cp[i] = ('ref1', struct.unpack('>H', data[pos:pos+2])[0]); pos += 2
        elif t in (9, 10, 11, 12, 17, 18): cp[i] = ('ref2', t, *struct.unpack('>HH', data[pos:pos+4])); pos += 4
        elif t == 15: cp[i] = ('mh',); pos += 3
        else: raise ValueError('bad cp tag %d' % t)
        i += 1
    return cp

def utf(cp, idx): return cp[idx][1]

def refs(data):
    cp = parse_cp(data)
    out = []
    for e in cp:
        if not e: continue
        if e[0] == 'ref2' and e[1] in (9, 10, 11):
            owner = utf(cp, cp[e[2]][1]); nt = cp[e[3]]
            out.append(('F' if e[1] == 9 else 'M', owner, utf(cp, nt[2]), utf(cp, nt[3])))
    classes = [utf(cp, e[1]) for e in cp if e and e[0] == 'ref1' and cp[e[1]] and cp[e[1]][0] == 'utf8' and False]
    return out

def main(jar, ns, mapfile):
    cl, fd, md = {}, {}, {}
    for line in open(mapfile, encoding='utf-8'):
        c = line.rstrip('\n').split('\t')
        if c[0] == 'CL': cl[c[1]] = c[2]
        elif c[0] == 'FD': fd[(c[1], c[2])] = c[3]
        elif c[0] == 'MD': md[(c[1], c[2], c[3])] = c[4]
    target_classes = set(cl.values()); named_classes = set(cl.keys())
    target_names = set(fd.values()) | set(md.values())
    named_only = {n for (o, n), t in fd.items() if t != n} | {n for (o, n, d), t in md.items() if t != n}
    named_only -= target_names
    bad = collections.Counter(); samples = {}
    total = 0
    z = zipfile.ZipFile(jar)
    for name in z.namelist():
        if not (name.startswith('dev/slinky/') and name.endswith('.class')) or name.startswith('dev/slinky/shaded/'):
            continue
        for kind, owner, mname, desc in refs(z.read(name)):
            total += 1
            problem = None
            if ns == 'named': continue
            if ns == 'official':
                if owner.startswith('net/minecraft/') and owner in named_classes and cl[owner] != owner:
                    problem = 'class-not-remapped'
                elif owner in target_classes and mname in named_only and len(mname) > 2:
                    problem = 'member-not-remapped'
            elif ns == 'srg':
                if owner in named_classes and mname in named_only:
                    problem = 'member-not-remapped'
            if problem:
                key = (problem, owner, mname)
                bad[key] += 1; samples.setdefault(key, name)
    print('%s [%s]: %d member refs scanned, %d distinct unmapped refs' % (jar, ns, total, len(bad)))
    for (p, o, m), n in bad.most_common(8):
        print('   ', p, o, m, 'x%d' % n, 'e.g.', samples[(p, o, m)])
    return len(bad)

if __name__ == '__main__':
    sys.exit(1 if main(*sys.argv[1:4]) else 0)
