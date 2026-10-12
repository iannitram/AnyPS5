import subprocess, sys
ours_ref, base_ref, theirs_ref = sys.argv[1:4]
def show(ref):
    return subprocess.run(['git','show',f'{ref}:docs/dev/TechnicalDebt.md'],capture_output=True,text=True,check=True).stdout.split('\n')
def key(line):
    return line.split('](')[0] if line.startswith('- [') and '](' in line else None
ours, base, theirs = show(ours_ref), show(base_ref), show(theirs_ref)
base_keys = {key(l): l for l in base if key(l)}
ours_keys = {key(l): l for l in ours if key(l)}
changed = {k: v for k, v in ours_keys.items() if k in base_keys and base_keys[k] != v}
added = [l for l in ours if key(l) and key(l) not in base_keys]
out, applied = [], set()
theirs_keys = {key(l) for l in theirs if key(l)}
for l in theirs:
    k = key(l)
    if k in changed:
        if l != base_keys[k]:
            sys.exit(f'both sides changed entry {k}')
        out.append(changed[k]); applied.add(k)
    else:
        out.append(l)
missing = set(changed) - applied
if missing: sys.exit(f'entries not found on the other side: {missing}')
for l in added:
    if key(l) in theirs_keys:
        continue
    index = ours.index(l)
    anchor = next((key(p) for p in reversed(ours[:index]) if key(p) and key(p) in {key(o) for o in out}), None)
    if anchor is None:
        sys.exit(f'new entry needs placement: {key(l)}')
    position = next(i for i, o in enumerate(out) if key(o) == anchor)
    out.insert(position + 1, l)
open('docs/dev/TechnicalDebt.md','w').write('\n'.join(out))
print(f'applied {len(applied)} changed entries')
