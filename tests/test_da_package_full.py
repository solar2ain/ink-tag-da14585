"""Portable source Full packaging tests; synthetic native fixtures, no SDK/key/IO."""
import builtins
import hashlib
import json
from pathlib import Path
import runpy
import struct
import subprocess
import sys
import zlib

import pytest

ROOT = Path(__file__).resolve().parents[1]
M = runpy.run_path(str(ROOT / 'tools/build/package_full.py'))


def save(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)
    return dict(path=path.name, bytes=len(data), sha256=hashlib.sha256(data).hexdigest())


def fixture(tmp_path, policy='optional'):
    root = tmp_path / 'build with spaces'
    root.mkdir()
    vectors = struct.pack('<II', 0x07fd7000, 0x07fc0009)
    raw = {
        'application': vectors + M['STATE']['PIN_COMMON_CAPABILITY'] + b'\0' * 12,
        'recovery': vectors + struct.pack('<III', M['NATIVE']['BOARD'], M['NATIVE']['LAYOUT'],
                         M['NATIVE']['VERIFIER']) + M['NATIVE']['PUBLIC_KEY'][1:] +
                    bytes.fromhex('0d494b0101535001') + bytes([int(policy == 'required')]) + b'\0' * 7,
        'bootloader': vectors + struct.pack('<II', 0x35008, 0),
        'gui': struct.pack('<IIII', 0x44464b49, 1, 10500, zlib.crc32(bytes(10500))) +
               bytes(10500) + b'\xff' * (12288 - 10516),
    }
    proof = dict(status='passed', otaSignaturePolicy=policy,
                 evidence={name:dict(raw=save(root / (name + '.bin'), data)) for name, data in raw.items()})
    project = ('<Project><Targets><Target><TargetName>DA14585</TargetName>'
               '<TargetOption><TargetArmAds><Cads><VariousControls><MiscControls>'
               '-DINKTAG_DA_OTA_SIGNATURE_REQUIRED=' + str(int(policy == 'required')) +
               '</MiscControls></VariousControls></Cads></TargetArmAds></TargetOption>'
               '</Target></Targets></Project>').encode()
    proof['evidence']['recovery']['project'] = save(root / 'project.xml', project)
    version = root / 'version.json'
    vref = save(version, json.dumps(dict(version='0.2.1', recoveryVersion='0.2.1',
          major=0, minor=2, patch=1, recoveryEpoch=1, otaVersion=201)).encode())
    (root / 'inputs-before.json').write_text(json.dumps(dict(sourceInputs={'version.json':vref})))
    p = root / 'native-proof.json'
    p.write_text(json.dumps(proof))
    return p, version, proof


def test_default_packages_full_ota_and_manifest_with_no_crypto_or_clients(tmp_path, monkeypatch):
    p, v, _ = fixture(tmp_path)
    original = builtins.__import__
    def guarded(name, *args, **kwargs):
        assert name.split('.')[0] not in ('cryptography', 'bleak', 'inktag_cli')
        return original(name, *args, **kwargs)
    monkeypatch.setattr(builtins, '__import__', guarded)
    report = M['package'](p, v)
    out = Path(report['output'])
    full = (out / (M['STEM'] + '-full.bin')).read_bytes()
    app = (out / (M['STEM'] + '-ota.img')).read_bytes()
    rec = (out / (M['STEM'] + '-recovery.img')).read_bytes()
    assert len(full) == 262144 and full[0x4003] == 1 and rec[3] == 0
    assert full[0x4040:0x4000 + len(app)] == app[64:]
    assert report['otaSignaturePolicy'] == 'optional' and not report['appPublisherVerified']
    assert not report['targetIdentityIncluded'] and not report['physicalWriterQualified']
    assert report['nativeChecks']['recoveryInitialization']['bothPagesInitialized']
    assert report['nativeChecks']['defaultInitialization']['pinDefaultsCapable']
    assert report['nativeChecks']['defaultInitialization']['screenWireHex'] == M['DEFAULT_SCREEN'].hex()
    for ref in report['files']:
        data = (out / ref['path']).read_bytes()
        assert len(data) == ref['bytes'] and hashlib.sha256(data).hexdigest() == ref['sha256']
    before = {f.name:f.read_bytes() for f in out.iterdir()}
    with pytest.raises(ValueError, match='already exists'):
        M['package'](p, v)
    assert before == {f.name:f.read_bytes() for f in out.iterdir()}


@pytest.mark.parametrize('bad', ['hash', 'policy-raw', 'policy-project', 'app-vectors',
    'loader-vectors', 'loader-selector', 'recovery-key', 'recovery-bound', 'assets-crc',
    'version', 'proof-status', 'proof-path'])
def test_invalid_inputs_publish_no_partial_packages(tmp_path, bad):
    p, v, proof = fixture(tmp_path)
    role = {'app-vectors':'application', 'loader-vectors':'bootloader',
            'loader-selector':'bootloader','assets-crc':'gui'}.get(bad, 'recovery')
    ref = proof['evidence'][role]['raw']; path = p.parent / ref['path']
    data = bytearray(path.read_bytes())
    if bad == 'policy-project':
        ref = proof['evidence']['recovery']['project'];path = p.parent / ref['path']
        data = bytearray(path.read_bytes().replace(b'REQUIRED=0', b'REQUIRED=1'))
    elif bad == 'policy-raw':
        data[data.index(bytes.fromhex('0d494b010153500100')) + 8] = 1
    elif bad in ('app-vectors', 'loader-vectors'):
        data[:8] = bytes(8)
    elif bad == 'loader-selector':
        data[8:12] = bytes(4)
    elif bad == 'recovery-key':
        offset = data.index(M['NATIVE']['PUBLIC_KEY'][1:]);data[offset] ^= 1
    elif bad == 'recovery-bound':
        data += bytes(32705)
    elif bad == 'assets-crc':
        data[16] ^= 1
    elif bad == 'version':
        v.write_bytes(v.read_bytes() + b' ')
    elif bad == 'proof-status':
        proof['status'] = 'failed'
    elif bad == 'proof-path':
        ref['path'] = '../outside.bin'
    else:
        data[0] ^= 1
    if bad not in ('version', 'proof-status', 'proof-path'):
        changed = save(path, bytes(data))
        if bad != 'hash':ref.update(changed)
    p.write_text(json.dumps(proof))
    with pytest.raises(ValueError):
        M['package'](p, v)
    assert not (p.parent / 'packages').exists()


def test_required_never_packages_unsigned_app(tmp_path):
    p, v, _ = fixture(tmp_path, 'required')
    with pytest.raises(ValueError, match='needs --signed-app'):
        M['package'](p, v)
    raw = (p.parent / 'application.bin').read_bytes()
    app = p.parent / 'unsigned.img';app.write_bytes(M['OTA']['package'](raw, '0.2.1.0'))
    with pytest.raises(ValueError, match='publisher verification failed'):
        M['package'](p, v, app)
    assert not (p.parent / 'packages').exists()


def test_real_command_in_unicode_space_path(tmp_path):
    p, v, _ = fixture(tmp_path)
    moved = p.parent.with_name('build ' + chr(0x4e2d) + chr(0x6587))
    p.parent.rename(moved)
    result = subprocess.run([sys.executable, str(ROOT/'tools/build/package_full.py'),
         '--proof', str(moved/p.name), '--version-json', str(moved/v.name)],
         capture_output=True, text=True, timeout=30)
    assert result.returncode == 0, result.stderr
    assert Path(json.loads(result.stdout)['output']).is_dir()


def test_required_full_make_missing_signature_stops_before_compiler(tmp_path):
    result = subprocess.run(['pwsh', '-NoProfile', '-File', str(ROOT/'build.ps1'),
        '-SdkRoot', 'missing-sdk', '-OtaSignaturePolicy', 'required', '-RequireFull'],
        cwd=tmp_path, capture_output=True, text=True, encoding='utf-8', timeout=30)
    assert result.returncode != 0 and 'Required Full needs' in result.stderr
    assert 'Keil UV4.exe not found' not in result.stderr
