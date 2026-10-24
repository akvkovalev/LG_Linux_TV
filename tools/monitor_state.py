from __future__ import print_function
import re
SB='/sys/realtek_boards/sb2_dbg'
ARM='/sys/realtek_boards/arm_wrapper'
DCMT='/sys/mt/cntl'
def read(path):
    with open(path) as f:return f.read()
def write(path,text):
    with open(path,'w') as f:f.write(text+'\n')
def snapshot():
    sb=[];arm=[]
    for line in read(SB).splitlines():
        match=re.match(r'^\[(\d+)\] \[([0-9a-f]+) - ([0-9a-f]+)\](.*)',line)
        if not match or int(match.group(1))>16:continue
        ident=int(match.group(1));tail=match.group(4)
        enabled='no cpu check enable' not in tail
        command='set%d clear'%ident
        if enabled:
            tags=re.findall(r'\[([^\]]+)\]',tail)
            names={'V':'vcpu','S':'scpu','A':'acpu','all':'all'}
            if not tags or any(t not in names for t in tags):raise ValueError('Unknown CPU flags: '+line)
            mode='rw' if '(R/W)' in tail else 'w' if '(write)' in tail else 'r' if '(read)' in tail else None
            if mode is None:raise ValueError('Unknown monitor mode')
            command='set%d monitor %s %s-%s %s,%s'%(ident,','.join(names[t] for t in tags),match.group(2),match.group(3),mode,'nstr' if 'non-STR' in tail else 'str')
        sb.append({'id':ident,'enabled':enabled,'line':line,'restore':command})
    for line in read(ARM).splitlines():
        match=re.match(r'^set(\d+): 0x([0-9a-f]+)-0x([0-9a-f]+), 0x([0-9a-f]+)\[ctrl\]',line)
        if not match:continue
        ident=int(match.group(1));ctrl=int(match.group(4),16);enabled=bool(ctrl&1)
        command='set%d clear'%ident
        if enabled:
            if 'write monitor enabled' in line:mode='w'
            elif 'read monitor enabled' in line:mode='r'
            else:raise ValueError('Unknown ARM wrapper mode: '+line)
            command='set%d %s-%s %s'%(ident,match.group(2),match.group(3),mode)
        arm.append({'id':ident,'enabled':enabled,'line':line,'restore':command})
    dcmt=[]
    for line in read('/sys/mt/stat').splitlines():
        if not re.match(r'^DCU\d+ mem',line):continue
        match=re.match(r'^DCU1 mem(\d+),(.*)$',line)
        if not match:raise ValueError('Unsupported DCMT controller: '+line)
        ident=int(match.group(1));tail=match.group(2);enabled=bool(tail)
        command='unset mem%d'%ident
        if enabled:
            fields=re.match(r'^(seq&block|seq|block),([0-9a-f]+)-([0-9a-f]+)\(phy ([0-9a-f]+)-([0-9a-f]+)\),mode([123]),(.*),$',tail)
            if not fields:raise ValueError('Unknown DCMT format: '+line)
            kind,start,end,phy_start,phy_end,mode,modules=fields.groups()
            if (start,end)!=(phy_start,phy_end):raise ValueError('Unsupported interleaved DCMT range: '+line)
            names=modules.split(',')
            if not all(re.match(r'^[A-Za-z0-9_]+\((r|w|rw)\)$',n) for n in names):
                raise ValueError('Unknown DCMT module flags: '+line)
            command='set 0x%s,0x%s ip:%s mode:%s type:%s mem:%d'%(start,end,','.join(names),mode,{'seq&block':'sb','seq':'s','block':'b'}[kind],ident)
        dcmt.append({'id':ident,'enabled':enabled,'line':line,'restore':command})
    if len(sb)!=16 or len(arm)!=4 or [x['id'] for x in dcmt]!=list(range(16)):
        raise ValueError('Unexpected monitor inventory')
    return {'sb2':sb,'arm':arm,'dcmt':dcmt}
def disable(state):
    for item in state['sb2']:write(SB,'set%d clear'%item['id'])
    for item in state['arm']:write(ARM,'set%d clear'%item['id'])
    for item in state['dcmt']:write(DCMT,'unset mem%d'%item['id'])
    now=snapshot()
    if any(x['enabled'] for group in now.values() for x in group):raise RuntimeError('A monitor remains enabled')
    return now
def restore(state):
    for item in state['dcmt']:write(DCMT,item['restore'])
    for item in state['arm']:write(ARM,item['restore'])
    for item in state['sb2']:write(SB,item['restore'])
    now=snapshot()
    for group in ('sb2','arm','dcmt'):
        for old,new in zip(state[group],now[group]):
            if old['line']!=new['line']:raise RuntimeError('Monitor readback mismatch: '+old['line']+' / '+new['line'])
    return now
