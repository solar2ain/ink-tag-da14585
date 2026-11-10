"""Read the actual Recovery compiler policy; no build, key or device access."""
import argparse
import json
from pathlib import Path
import re
import xml.etree.ElementTree as ET

NAME = 'INKTAG_DA_OTA_SIGNATURE_REQUIRED'


def assert_recovery_policy(project: Path, expected: str) -> dict:
    if expected not in ('optional', 'required'):
        raise ValueError('Unknown Recovery signature policy')
    value = '1' if expected == 'required' else '0'
    targets = [target for target in ET.parse(project).getroot().findall('./Targets/Target')
               if target.findtext('TargetName') == 'DA14585']
    if len(targets) != 1:
        raise ValueError('Missing unique DA14585 Recovery target')
    target = targets[0]
    flags = target.find('TargetOption/TargetArmAds/Cads/VariousControls/MiscControls')
    text = (flags.text or '') if flags is not None else ''
    defines = re.findall(r'(?:^|\s)-D' + NAME + r'(?:=([^\s]+))?(?=\s|$)', text)
    if defines != [value] or re.search(r'(?:^|\s)-U\s*' + NAME + r'\b', text):
        raise ValueError('Recovery compiler signature policy differs or is ambiguous')
    for node in target.iter():
        if node is flags or node.tag not in ('MiscControls', 'Define', 'Undefine'):
            continue
        if NAME in (node.text or ''):
            raise ValueError('Recovery signature policy is overridden below target')
    return {'otaSignaturePolicy': expected, 'compilerDefine': NAME + '=' + value,
            'project': str(project)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--project', type=Path, required=True)
    parser.add_argument('--expected', choices=('optional', 'required'), required=True)
    args = parser.parse_args()
    print(json.dumps(assert_recovery_policy(args.project, args.expected)))


if __name__ == '__main__':
    main()
