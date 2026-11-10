"""Offline compiler input capture. No device, key or delivery IO."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import runpy
import re
import stat
import struct
import subprocess
import xml.etree.ElementTree as ET
import zlib


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def plain(path):
    for parent in (path, *path.parents):
        try:
            info = parent.lstat()
        except FileNotFoundError:
            continue
        if stat.S_ISLNK(info.st_mode) or getattr(info, 'st_file_attributes', 0) & 0x400:
            raise ValueError('Linked build input/output refused')


def write_new(path, data):
    plain(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('x', encoding='utf-8') as stream:
        json.dump(data, stream, ensure_ascii=False, indent=2)
        stream.write('\n')


def git(repo, *args):
    return subprocess.check_output(['git', '-C', str(repo), *args], text=True).strip()


def sdk_inputs(root):
    """Keep actual sources/libraries; omit only compiler outputs and UI state."""
    plain(root)
    files = {}
    for directory, dirs, names in os.walk(root, followlinks=False):
        for name in dirs:
            plain(Path(directory) / name)
        dirs[:] = [name for name in dirs if not name.lower().startswith('out_')]
        for name in names:
            if '.uvguix' in name or name.startswith('uv4_') and name.endswith('.log'):
                continue
            path = Path(directory) / name
            plain(path)
            files[path.relative_to(root).as_posix()] = {'bytes': path.stat().st_size, 'sha256': digest(path)}
    return dict(sorted(files.items()))


def source_inputs(repo):
    manifest = json.loads((repo / 'tools/build/source-files.json').read_text(encoding='utf-8'))
    selected = {}
    for name in manifest['files']:
        path = repo / name
        if path.is_absolute() and not path.resolve().is_relative_to(repo.resolve()):
            raise ValueError('Source allowlist escape')
        plain(path)
        selected[name] = {'bytes': path.stat().st_size, 'sha256': digest(path)}
    return selected


def check_hooks(project):
    """Only the maintained raw fromelf converter may run as a Keil hook."""
    root = ET.parse(project).getroot()
    for node in root.iter():
        if node.tag not in ('BeforeMake', 'BeforeCompile', 'AfterMake'):
            continue
        for index in (1, 2):
            if node.findtext('RunUserProg' + str(index)) != '1':
                continue
            command = node.findtext('UserProg' + str(index) + 'Name') or ''
            if node.tag != 'AfterMake' or not command.startswith('fromelf --bincombined ') or any(c in command for c in '&|;\r\n'):
                raise ValueError('Unexpected Keil user hook; no build/signing permitted')


def capture(repo, sdk, scope, compilers=()):
    for relative in (
        'projects/target_apps/ble_examples/ble_app_inktag/Keil_5/inktag_da14585.uvprojx',
        'projects/target_apps/ble_examples/ble_app_inktag_recovery/Keil_5/ble_app_ota.uvprojx',
        'utilities/secondary_bootloader/secondary_bootloader.uvprojx',
    ):
        check_hooks(sdk / relative)
    sources = source_inputs(repo)
    sdk_files = sdk_inputs(sdk)
    compiler_trees = {str(path.parent.parent): sdk_inputs(path.parent.parent)
        for path in compilers if path.name.lower() == 'armclang.exe'}
    return {'sourceCommit': source_ref(repo),
        'sourceTree': source_ref(repo, tree=True), 'scope': scope,
        'sourceTreeDirty': source_dirty(repo),
        'sourceSnapshotSha256': hashlib.sha256(json.dumps({'repo': sources, 'sdk': sdk_files}, sort_keys=True).encode()).hexdigest(),
        'repo': str(repo), 'sdk': str(sdk), 'sourceInputs': sources,
        'compilerPaths': [str(p) for p in compilers],
        'toolchain': {str(p): {'bytes': p.stat().st_size, 'sha256': digest(p)} for p in compilers},
        'toolchainInputs': compiler_trees, 'sdkInputs': sdk_files}


def verify_capture(before):
    after = capture(Path(before['repo']), Path(before['sdk']), before['scope'],
                    [Path(p) for p in before['compilerPaths']])
    if before != after:
        raise ValueError('Build inputs changed; signing/collection refused')


def source_ref(repo, tree=False):
    if (repo/'.git').exists(): return git(repo,'rev-parse','HEAD^{tree}' if tree else 'HEAD')
    return 'allowlist-snapshot:' + digest(repo/'tools/build/source-files.json')
def source_dirty(repo):
    return bool(git(repo,'status','--porcelain')) if (repo/'.git').exists() else False

def main():
    parser=argparse.ArgumentParser(description='Pure offline compiler input capture; no keys or package production.')
    parser.add_argument('action',choices=['capture','check'])
    parser.add_argument('--repo',type=Path,required=True)
    parser.add_argument('--sdk',type=Path)
    parser.add_argument('--scope',choices=['UnsignedComparison'],default='UnsignedComparison')
    parser.add_argument('--job',type=Path,required=True)
    parser.add_argument('--compiler',type=Path,action='append',default=[])
    args=parser.parse_args(); destination=args.job/'inputs-before.json'
    if args.action=='capture': write_new(destination,capture(args.repo,args.sdk,args.scope,args.compiler))
    else: verify_capture(json.loads(destination.read_text(encoding='utf-8')))
if __name__=='__main__': main()
