#!/usr/bin/env python3
"""Record a headless run of the port as a video with its sound.

    record_clip.py --out clip.mp4 --from N --length M [--exe PATH] [--data DIR] [--save DIR]
                   [--width W] [--keep DIR] -- [darkcloud options...]

Runs darkcloud headless for N + M frames with --record from frame N and DC_AUDIO_WAV, then has ffmpeg
put the frames and the sound from frame N on together. The sound is the mixer's as the game clock
ticks (loading screens included), so a frame's place in it is its clock tick over the tick rate, both of
which --record logs in ticks.txt; the video plays at the frames' own pace in ticks. Options after -- go
to the game (--jump, --input, --roster, --fast-load...). Needs ffmpeg on the PATH. The game is a release
build when one is built: a debug build records about ten times slower. A video or kept folder inside the
repository must be somewhere git ignores, such as port/run/clips, so a recording is never committed.
"""
import argparse
import os
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))


def default_exe():
    """The first darkcloud built of the release builds, then the debug ones."""
    name = 'darkcloud.exe' if os.name == 'nt' else 'darkcloud'
    for build in ('macos-arm64-release', 'pc-release', 'macos-arm64', 'pc'):
        exe = os.path.join(ROOT, 'port', 'build', build, name)
        if os.path.exists(exe):
            return exe
    return os.path.join(ROOT, 'port', 'build', 'macos-arm64-release', name)


def refuse_tracked(paths):
    """Exits when any of paths is inside the repository and git would not ignore it."""
    for path in paths:
        path = os.path.abspath(path)
        if os.path.commonpath([path, ROOT]) != ROOT:
            continue
        rel = os.path.relpath(path, ROOT)
        if subprocess.run(['git', '-C', ROOT, 'check-ignore', '-q', rel]).returncode != 0:
            sys.exit(f'{rel} would be in the repository and not ignored by git: record into port/run/clips '
                     'or outside the repository')


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--out', required=True, help='the video (.mp4)')
    ap.add_argument('--from', dest='start', type=int, required=True, help='first frame recorded')
    ap.add_argument('--length', type=int, required=True, help='frames recorded')
    ap.add_argument('--exe', default=default_exe())
    ap.add_argument('--data', default=os.environ.get('DC_DATA'))
    ap.add_argument('--save')
    ap.add_argument('--width', type=int, default=960, help='the video width (default 960)')
    ap.add_argument('--keep', help='keep the frames and sound in this folder')
    ap.add_argument('game', nargs=argparse.REMAINDER, help='-- then darkcloud options')
    a = ap.parse_args()
    if shutil.which('ffmpeg') is None:
        sys.exit('ffmpeg is not on the PATH')
    game = a.game[1:] if a.game[:1] == ['--'] else a.game
    refuse_tracked([a.out] + ([os.path.join(a.keep, 'ticks.txt')] if a.keep else []))
    work = a.keep or tempfile.mkdtemp(prefix='dc-clip-')
    os.makedirs(work, exist_ok=True)
    wav = os.path.join(work, 'audio.wav')
    cmd = [a.exe, '--headless', '--record', work, '--record-from', str(a.start), '--frames', str(a.start + a.length)]
    if a.data:
        cmd += ['--data', a.data]
    if a.save:
        cmd += ['--save', a.save]
    env = dict(os.environ, DC_AUDIO_WAV=wav)
    run = subprocess.run(cmd + game, env=env)
    if run.returncode != 0:
        sys.exit(f'darkcloud exited with {run.returncode}')
    frames = sorted(f for f in os.listdir(work) if f.endswith('.png'))
    if not frames:
        sys.exit('no frames were recorded')
    first = int(frames[0][:-4])
    with open(os.path.join(work, 'ticks.txt')) as f:
        ticks = [line.split() for line in f if line.strip()]
    rate = float(ticks[0][2])
    span = int(ticks[-1][1]) - int(ticks[0][1])
    fps = rate * (len(ticks) - 1) / span if span > 0 else rate
    video = ['-framerate', f'{fps:.4f}', '-start_number', str(first), '-i', os.path.join(work, '%06d.png')]
    sound = ['-ss', f'{int(ticks[0][1]) / rate:.4f}', '-i', wav] if os.path.exists(wav) else []
    out = ['-vf', f'scale={a.width}:-2', '-c:v', 'libx264', '-pix_fmt', 'yuv420p', '-crf', '20']
    if sound:
        out += ['-c:a', 'aac', '-b:a', '160k', '-shortest']
    subprocess.run(['ffmpeg', '-y', '-loglevel', 'error'] + video + sound + out + [a.out], check=True)
    if not a.keep:
        shutil.rmtree(work)
    print(f'{a.out}: {len(frames)} frames from {first} at {fps:.2f} fps')


if __name__ == '__main__':
    main()
