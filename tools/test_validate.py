"""Mutate copies of authored package output to prove catalogue checks fail closed."""
import json
from pathlib import Path
import shutil
import sys
import tempfile
import hashlib
import zipfile
from validate import validate


def rejects(action):
    try: action()
    except ValueError: return
    raise AssertionError('Invalid catalogue was accepted')


with tempfile.TemporaryDirectory(prefix='mod-catalogue-check-') as temporary:
    root = Path(temporary)
    for file in Path(sys.argv[1]).iterdir():
        if file.suffix == '.zip' or file.name == 'index.json': shutil.copyfile(file, root / file.name)
    original = (root / 'index.json').read_text()
    validate(root)
    value = json.loads(original)
    value['mods'][0]['downloads']['all']['size'] += 1
    (root / 'index.json').write_text(json.dumps(value))
    rejects(lambda: validate(root))
    value = json.loads(original)
    value['mods'][0]['version'] = '999.0.0'
    (root / 'index.json').write_text(json.dumps(value))
    rejects(lambda: validate(root))
    value = json.loads(original)
    value['mods'][0]['experimental'] = not value['mods'][0].get('experimental', False)
    (root / 'index.json').write_text(json.dumps(value))
    rejects(lambda: validate(root))
    value = json.loads(original)
    value['mods'][0]['downloads']['all']['url'] = 'http://example.invalid/package.zip'
    (root / 'index.json').write_text(json.dumps(value))
    rejects(lambda: validate(root))
    value = json.loads(original)
    value['mods'][0]['downloads']['all']['url']='build/packages/heart-ticker.zip'
    (root/'index.json').write_text(json.dumps(value))
    rejects(lambda: validate(root))
    (root / 'index.json').write_text(original)
    authored=root/'catalogue.json'
    authored.write_text(original)
    rejects(lambda: validate(root,authored))
    value=json.loads(original)
    for entry in value['mods']: entry.pop('downloads')
    authored.write_text(json.dumps(value))
    validate(root,authored)
    # game_id: only "wwhd" with minimum_manager_version 1.5.0 (older ports then say "Requires mod manager 1.5.0")
    first = json.loads(original)['mods'][0]
    package = root / (first['id'] + '.zip')
    packed = package.read_bytes()

    def repack(change):
        with zipfile.ZipFile(root / 'source.zip', 'w') as out, zipfile.ZipFile(package) as source:
            for info in source.infolist():
                data = source.read(info.filename)
                if info.filename == 'manifest.json':
                    manifest = json.loads(data)
                    change(manifest)
                    data = json.dumps(manifest).encode()
                out.writestr(info, data)
        (root / 'source.zip').replace(package)
        value = json.loads(original)
        value['mods'][0]['downloads']['all'].update(size=package.stat().st_size, sha256=hashlib.sha256(package.read_bytes()).hexdigest())
        (root / 'index.json').write_text(json.dumps(value))

    repack(lambda m: None)
    validate(root)  # the repacked unchanged manifest passes
    for change in (lambda m: m.update(game_id='wwhd-usa'), lambda m: m.update(game_id='wwhd-eur'),
                   lambda m: m.pop('minimum_manager_version'), lambda m: m.update(minimum_manager_version='1.4.0')):
        package.write_bytes(packed)
        repack(change)
        rejects(lambda: validate(root))
    package.write_bytes(packed)
    (root / 'index.json').write_text(original)
    validate(root)
    (root / 'unexpected.zip').write_bytes(b'authored invalid extra package')
    rejects(lambda: validate(root))
print('Catalogue checksum, manifest version, experimental flag, game ID, insecure URL and extra-artifact checks passed')
