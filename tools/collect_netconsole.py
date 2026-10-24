#!/usr/bin/env python3
import argparse
import json
import socket
import time
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('--port', type=int, default=6666)
parser.add_argument('--seconds', type=int, default=240)
parser.add_argument('--output', type=Path, default=Path('netconsole.jsonl'))
args = parser.parse_args()

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(('0.0.0.0', args.port))
sock.settimeout(1)
deadline = time.monotonic() + args.seconds
count = 0
print(f'Listening for netconsole on UDP {args.port}', flush=True)
with args.output.open('w', encoding='utf-8') as log:
    while time.monotonic() < deadline:
        try:
            data, address = sock.recvfrom(65535)
        except socket.timeout:
            continue
        entry = {'received_utc': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()),
                 'source': address, 'text': data.decode('utf-8', 'replace')}
        log.write(json.dumps(entry, ensure_ascii=False) + '\n')
        log.flush(); count += 1
        print(entry['text'], end='' if entry['text'].endswith('\n') else '\n', flush=True)
print(f'Saved {count} datagrams to {args.output}')
