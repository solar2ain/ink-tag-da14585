"""Run the explicit host matrix using GCC or WSL GCC; never load vendor hardware tools."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import time
ROOT=Path(__file__).resolve().parents[2]
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--sanitize',action='store_true');args=parser.parse_args()
    manifest=ROOT/'tools/build/host-tests.json';m=json.loads(manifest.read_text(encoding='utf-8'))
    out=ROOT/'.local/host-tests'/str(time.time_ns());out.mkdir(parents=True)
    compiler=shutil.which('gcc');wsl=None
    if not compiler:
        if not shutil.which('wsl'):raise SystemExit('GCC or WSL GCC required')
        wsl=subprocess.check_output(['wsl','--exec','wslpath','-a',str(ROOT)],text=True).strip()
    report={'scope':m['scope'],'sanitize':args.sanitize,'manifestSha256':hashlib.sha256(manifest.read_bytes()).hexdigest(),'steps':[]}
    for e in m['hostC']:
        binary=(out/(e['id']+('.exe' if compiler and os.name=='nt' else ''))).relative_to(ROOT).as_posix()
        flags=['-std=c99','-O2','-Ifirmware/common/gui','-Ifirmware/common/ble']
        if args.sanitize:flags+=['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer']
        command=[compiler or 'gcc',*flags,*e['defines'],*e['sources'],'-o',binary]
        step={'id':e['id'],'compileCommand':command,'sources':{s:hashlib.sha256((ROOT/s).read_bytes()).hexdigest() for s in e['sources']}}
        for phase,cmd in [('compile',command),('run',[str(ROOT/binary)] if compiler else [binary])]:
            actual=['wsl','bash','-lc','cd '+shlex.quote(wsl)+' && '+shlex.join(cmd)] if wsl else cmd
            p=subprocess.run(actual,cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=180)
            log=out/(e['id']+'-'+phase+'.log');log.write_bytes(p.stdout)
            step[phase]={'exitCode':p.returncode,'log':log.name,'sha256':hashlib.sha256(p.stdout).hexdigest()}
            if p.returncode:break
        report['steps'].append(step);(out/'report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
        print(e['id'],step.get('run',step['compile'])['exitCode'],flush=True)
    failed=[e['id'] for e in report['steps'] if e.get('run',e['compile'])['exitCode']]
    print(json.dumps({'report':str(out/'report.json'),'failed':failed}));return bool(failed)
if __name__=='__main__':raise SystemExit(main())
