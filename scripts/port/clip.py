#!/usr/bin/env python3
"""Record a staged clip of an Expanded feature, and with --compare the same run with Weapon Abilities off beside it.

    clip.py NAME --stage FILE --input FILE --from N --length M [--compare] [--out DIR]
            [--exe PATH] [--data DIR] [--save DIR] [-- darkcloud options...]

Runs darkcloud (Expanded, release build by default) on the stage with the input script and an event log,
through record_clip.py, in a copy of the save so the runs leave it as it was. With --compare a second run goes at
the same time with the Weapon Abilities option off (expanded.weapon_abilities in its copy's config.json). Writes, in
--out (default port/run/clips, which git ignores):

    NAME.mp4                the run; with --compare the run on the left and the one without weapon abilities on
                            the right, each one's sound in its own ear
    NAME.events.jsonl       the run's event log (NAME.off.events.jsonl for the one without)
    NAME.sheet.png          one frame a second of NAME.mp4

An --out inside the repository that git does not ignore is refused, so a recording is never committed.
See expanded/docs/recording.md.
"""
import argparse
import json
import os
import shutil
import subprocess
import sys
import tempfile

from record_clip import default_exe, refuse_tracked

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, '..', '..'))


def weapon_abilities_off(save):
    """Turns the Weapon Abilities option off in the save folder's config.json, keeping its other settings."""
    path = os.path.join(save, 'config.json')
    config = {}
    if os.path.exists(path):
        with open(path) as f:
            config = json.load(f)
    config.setdefault('expanded', {})['weapon_abilities'] = False
    with open(path, 'w') as f:
        json.dump(config, f, indent=4)


def record(a, out, events, save, game):
    cmd = [sys.executable, os.path.join(HERE, 'record_clip.py'), '--exe', a.exe, '--out', out,
           '--from', str(a.start), '--length', str(a.length), '--save', save]
    if a.data:
        cmd += ['--data', a.data]
    cmd += ['--', '--fast-load', '--stage', a.stage, '--input', a.input, '--event-log', events]
    return subprocess.Popen(cmd + game, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('name', help='the clip\'s name (its files are NAME.*)')
    ap.add_argument('--stage', required=True, help='the stage file (--stage)')
    ap.add_argument('--input', required=True, help='the input script (--input)')
    ap.add_argument('--from', dest='start', type=int, required=True, help='first frame recorded')
    ap.add_argument('--length', type=int, required=True, help='frames recorded')
    ap.add_argument('--compare', action='store_true', help='also record the run with Weapon Abilities off, side by side')
    ap.add_argument('--out', default=os.path.join(ROOT, 'port', 'run', 'clips'))
    ap.add_argument('--exe', default=default_exe())
    ap.add_argument('--data', default=os.environ.get('DC_DATA'))
    ap.add_argument('--save', default=os.environ.get('DC_SAVE'), help='the save folder the stage loads from')
    argv, game = sys.argv[1:], []
    if '--' in argv:
        argv, game = argv[:argv.index('--')], argv[argv.index('--') + 1:]
    a = ap.parse_args(argv)
    if not a.save:
        sys.exit('--save (or DC_SAVE) is needed: the stage starts from a save')
    a.stage, a.input = os.path.abspath(a.stage), os.path.abspath(a.input)
    base = os.path.join(a.out, a.name)
    suffixes = ['.mp4', '.sheet.png', '.events.jsonl'] + (['.off.events.jsonl'] if a.compare else [])
    refuse_tracked([base + suffix for suffix in suffixes])
    os.makedirs(a.out, exist_ok=True)
    work = tempfile.mkdtemp(prefix='dc-clip-')
    try:
        runs = [''] + (['.off'] if a.compare else [])
        jobs = []
        for suffix in runs:
            save = os.path.join(work, 'save' + suffix)
            shutil.copytree(a.save, save)
            if suffix:
                weapon_abilities_off(save)
            video = os.path.join(work, f'run{suffix}.mp4')
            jobs.append((video, record(a, video, f'{base}{suffix}.events.jsonl', save, game)))
        for video, job in jobs:
            output, _ = job.communicate()
            if job.returncode != 0:
                sys.exit(f'{video}: recording failed\n{output}')
        videos = [video for video, _ in jobs]
        if a.compare:
            subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-i', videos[0], '-i', videos[1], '-filter_complex',
                            '[0:v][1:v]hstack[v];[0:a][1:a]amerge,pan=stereo|c0<c0+c1|c1<c2+c3[a]', '-map', '[v]',
                            '-map', '[a]', '-c:v', 'libx264', '-pix_fmt', 'yuv420p', '-crf', '22', '-c:a', 'aac',
                            '-b:a', '160k', base + '.mp4'], check=True)
        else:
            shutil.move(videos[0], base + '.mp4')
        rows = max(1, -(-a.length // 240))  # four seconds a row
        subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-i', base + '.mp4', '-vf',
                        f'fps=1,scale=320:-1,tile=4x{rows}', '-frames:v', '1', base + '.sheet.png'], check=True)
    finally:
        shutil.rmtree(work, ignore_errors=True)
    print(f'{base}.mp4, {base}.events.jsonl, {base}.sheet.png')


if __name__ == '__main__':
    main()
