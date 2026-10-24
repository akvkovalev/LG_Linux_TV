/* Temporary RAM-only PID 1. ARM EABI syscalls; no libc or floating point. */
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;
#ifndef CODEX_CHILD_DELAY
#define CODEX_CHILD_DELAY 5
#endif
#ifndef CODEX_HOST_ALIVE
#define CODEX_HOST_ALIVE 0
#endif
#ifndef TV_IP_A
#define TV_IP_A 192
#define TV_IP_B 168
#define TV_IP_C 89
#define TV_IP_D 76
#endif
#ifndef NETCONSOLE_IP_A
#define NETCONSOLE_IP_A 192
#define NETCONSOLE_IP_B 168
#define NETCONSOLE_IP_C 89
#define NETCONSOLE_IP_D 97
#endif
#ifndef TV_MAC_0
#define TV_MAC_0 0xa8
#define TV_MAC_1 0x23
#define TV_MAC_2 0xfe
#define TV_MAC_3 0xfa
#define TV_MAC_4 0x1c
#define TV_MAC_5 0xa8
#endif
#ifndef NETCONSOLE_MAC_0
#define NETCONSOLE_MAC_0 0x30
#define NETCONSOLE_MAC_1 0x56
#define NETCONSOLE_MAC_2 0x0f
#define NETCONSOLE_MAC_3 0x03
#define NETCONSOLE_MAC_4 0x7d
#define NETCONSOLE_MAC_5 0x2b
#endif
#ifndef NETCONSOLE_PORT
#define NETCONSOLE_PORT 6666
#endif
static volatile u32 *crumb;
static volatile u32 *watchdog;
static long mac_status=-1;
static long call(long n,long a,long b,long c,long d,long e,long f)
{
    register long r0 __asm__("r0")=a, r1 __asm__("r1")=b;
    register long r2 __asm__("r2")=c, r3 __asm__("r3")=d;
    register long r4 __asm__("r4")=e, r5 __asm__("r5")=f;
    register long r7 __asm__("r7")=n;
    __asm__ volatile("svc 0" : "+r"(r0) : "r"(r1),"r"(r2),"r"(r3),"r"(r4),"r"(r5),"r"(r7) : "memory","cc");
    return r0;
}
static void zero(void *p,u32 n) { volatile u8 *s=p; while(n--) *s++=0; }
static void mark(u32 value) { if(crumb) { *crumb=value; __asm__ volatile("dsb sy" ::: "memory"); } }
static u32 text(char *out,u32 at,const char *in) { while(*in) out[at++]=*in++;return at; }
static u32 hex(char *out,u32 at,u32 value)
{
    int shift;at=text(out,at,"0x");
    for(shift=28;shift>=0;shift-=4) out[at++]="0123456789abcdef"[(value>>shift)&15];
    return at;
}
static void pause_one(void) { long t[2]={1,0};call(162,(long)t,0,0,0,0,0); }
static void pause_poll(void) { long t[2]={0,40000000};call(162,(long)t,0,0,0,0,0); }
static u32 logfile(char *out,u32 at,const char *path)
{
    long f=call(5,(long)path,0,0,0,0,0),count;
    if(f<0)return at;
    at=text(out,at,path);out[at++]='=';
    if(path[1]!='s')call(19,f,-128,2,0,0,0);
    count=call(3,f,(long)(out+at),128,0,0,0);if(count>0)at+=(u32)count;
    call(6,f,0,0,0,0,0);out[at++]='\n';return at;
}
static long configure(long sock)
{
    u8 req[40];long result;
    zero(req,sizeof(req));req[0]='e';req[1]='t';req[2]='h';req[3]='0';
    result=call(54,sock,0x8913,(long)req,0,0,0);if(result<0)return result; /* SIOCGIFFLAGS */
    req[16]&=(u8)~1U;
    result=call(54,sock,0x8914,(long)req,0,0,0);if(result<0)return result; /* interface down */
    zero(req+16,24);req[16]=1; /* ARPHRD_ETHER */
    req[18]=TV_MAC_0;req[19]=TV_MAC_1;req[20]=TV_MAC_2;req[21]=TV_MAC_3;req[22]=TV_MAC_4;req[23]=TV_MAC_5;
    mac_status=call(54,sock,0x8924,(long)req,0,0,0); /* SIOCSIFHWADDR */
    zero(req+16,24);
    result=call(54,sock,0x8913,(long)req,0,0,0);if(result<0)return result;
    req[16]|=1;
    result=call(54,sock,0x8914,(long)req,0,0,0);if(result<0)return result; /* SIOCSIFFLAGS */
    zero(req+16,24);req[16]=2;req[20]=TV_IP_A;req[21]=TV_IP_B;req[22]=TV_IP_C;req[23]=TV_IP_D;
    result=call(54,sock,0x8916,(long)req,0,0,0);if(result<0)return result; /* SIOCSIFADDR */
    req[20]=255;req[21]=255;req[22]=255;req[23]=0;
    return call(54,sock,0x891c,(long)req,0,0,0); /* SIOCSIFNETMASK */
}
static long pin_neighbor(long sock)
{
    u8 req[68];zero(req,sizeof(req));
    req[0]=2;req[4]=NETCONSOLE_IP_A;req[5]=NETCONSOLE_IP_B;req[6]=NETCONSOLE_IP_C;req[7]=NETCONSOLE_IP_D;
    req[16]=1;req[18]=NETCONSOLE_MAC_0;req[19]=NETCONSOLE_MAC_1;req[20]=NETCONSOLE_MAC_2;req[21]=NETCONSOLE_MAC_3;req[22]=NETCONSOLE_MAC_4;req[23]=NETCONSOLE_MAC_5;
    req[32]=6;req[52]='e';req[53]='t';req[54]='h';req[55]='0';
    return call(54,sock,0x8955,(long)req,0,0,0); /* SIOCSARP, temporary neighbor */
}
__attribute__((noreturn)) void _start(void)
{
    long pid=call(20,0,0,0,0,0,0),fd,map,sock,net,arp,child=-2,status=0,child_status=0,waited=0;
    long micom=-1,alive=-1,poll_result=-1;u8 alive_command=0x27,poll_data[3]={0,0,0};
    struct {u16 command;u16 size;u32 pointer;} alive_arg={0,1,(u32)&alive_command};
    struct {u8 command;u8 padding;u16 size;u32 pointer;} poll_arg={0xc0,0,3,(u32)poll_data};
    char buffer[1536],uts[390],klog[16384];u32 length,ticks=0;
    u32 poll_count=0,poll_errors=0;
    u8 destination[16];char *argv[]={"/bin/sh","/init-alpine",0};
    char *env[]={"PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin","HOME=/root","TERM=vt100",0};
    if(pid!=1) {
        length=text(buffer,0,"CODEX freestanding init selftest: ARM syscalls work; pid=");
        length=hex(buffer,length,(u32)pid);buffer[length++]='\n';
        call(4,1,(long)buffer,length,0,0,0);call(1,0,0,0,0,0,0);for(;;){}
    }
    call(14,(long)"/dev/mem",0x2180,0x101,0,0,0);
    fd=call(5,(long)"/dev/mem",2,0,0,0,0);
    if(fd>=0) {
        map=call(192,0,4096,3,1,fd,0x18060);
        if((u32)map<0xfffff000U) crumb=(volatile u32 *)((u32)map+0x10c);
        map=call(192,0,4096,1,1,fd,0x18062);
        if((u32)map<0xfffff000U) watchdog=(volatile u32 *)(u32)map;
    }
    mark(0xc0de2800);
    mark(0xc0de2804);
    call(21,(long)"devtmpfs",(long)"/dev",(long)"devtmpfs",0,0,0);
    mark(0xc0de2808);
    call(21,(long)"proc",(long)"/proc",(long)"proc",0,0,0);
    call(21,(long)"sysfs",(long)"/sys",(long)"sysfs",0,0,0);
    mark(0xc0de2810);
    zero(uts,sizeof(uts));call(122,(long)uts,0,0,0,0,0);
    sock=call(281,2,2,0,0,0,0);
    net=sock<0?sock:configure(sock);
    mark(net<0?0xc0de7100|(((u32)-net&255)<<2):0xc0de2820);
    arp=sock<0?sock:pin_neighbor(sock);
    zero(destination,sizeof(destination));destination[0]=2;
    destination[2]=(NETCONSOLE_PORT>>8)&255;destination[3]=NETCONSOLE_PORT&255;
    destination[4]=NETCONSOLE_IP_A;destination[5]=NETCONSOLE_IP_B;
    destination[6]=NETCONSOLE_IP_C;destination[7]=NETCONSOLE_IP_D;
    micom=call(5,(long)"/dev/sys-misc",2,0,0,0,0);
    for(;;) {
        /* Exact stock HostAliveSignal command, through the matching kernel API. */
        if(CODEX_HOST_ALIVE && micom>=0 && ticks>=5) {
            mark(0xc0de2840);
            alive=call(54,micom,0x40047312,(long)&alive_arg,0,0,0);
            mark(0xc0de2844);
        }
        if(ticks==CODEX_CHILD_DELAY) {
            mark(0xc0de2830);child=call(2,0,0,0,0,0,0); /* fork after network/timer proof */
            if(child==0) {
                mark(0xc0de2838);
                call(6,sock,0,0,0,0,0);call(6,fd,0,0,0,0,0);
                waited=call(11,(long)argv[0],(long)argv,(long)env,0,0,0);
                mark(0xc0de6400|(((u32)-waited&255)<<2));
                call(1,127,0,0,0,0,0);for(;;){}
            }
            mark(child<0?0xc0de7400|(((u32)-child&255)<<2):0xc0de2834);
        }
        if(child>0)waited=call(114,-1,(long)&status,1,0,0,0);
        if(waited==child && child>0) {
            child_status=status;
            mark((status&0x7f)?0xc0de7800|(((u32)status&0xff)<<2):0xc0de7c00|((((u32)status>>8)&0xff)<<2));
        }
        length=text(buffer,0,"CODEX FREESTANDING PID1 kernel=");length=text(buffer,length,uts+130);
        length=text(buffer,length," pid=");length=hex(buffer,length,(u32)pid);
        length=text(buffer,length," net=");length=hex(buffer,length,(u32)net);
        length=text(buffer,length," mac=");length=hex(buffer,length,(u32)mac_status);
        length=text(buffer,length," arp=");length=hex(buffer,length,(u32)arp);
        length=text(buffer,length," ticks=");length=hex(buffer,length,ticks);
        length=text(buffer,length," child=");length=hex(buffer,length,(u32)child);
        length=text(buffer,length," wait=");length=hex(buffer,length,(u32)waited);
        length=text(buffer,length," status=");length=hex(buffer,length,(u32)child_status);
        length=text(buffer,length," marker=");length=hex(buffer,length,crumb?*crumb:0);buffer[length++]='\n';
        length=text(buffer,length,"watchdog_ctrl=");length=hex(buffer,length,watchdog?watchdog[0x204/4]:0);
        length=text(buffer,length," watchdog_timeout=");length=hex(buffer,length,watchdog?watchdog[0x210/4]:0);buffer[length++]='\n';
        length=text(buffer,length,"host_alive_enabled=");length=hex(buffer,length,CODEX_HOST_ALIVE);
        length=text(buffer,length," micom_fd=");length=hex(buffer,length,(u32)micom);
        length=text(buffer,length," host_alive_result=");length=hex(buffer,length,(u32)alive);buffer[length++]='\n';
        length=text(buffer,length,"micom_poll_count=");length=hex(buffer,length,poll_count);
        length=text(buffer,length," poll_result=");length=hex(buffer,length,(u32)poll_result);
        length=text(buffer,length," poll_errors=");length=hex(buffer,length,poll_errors);
        length=text(buffer,length," poll_data=");length=hex(buffer,length,((u32)poll_data[0]<<16)|((u32)poll_data[1]<<8)|poll_data[2]);buffer[length++]='\n';
        length=text(buffer,length,"emcu_magic=");length=hex(buffer,length,crumb?crumb[1]:0);
        length=text(buffer,length," emcu_tick=");length=hex(buffer,length,crumb?crumb[2]:0);
        length=text(buffer,length," scpu_tick=");length=hex(buffer,length,crumb?crumb[3]:0);buffer[length++]='\n';
        length=logfile(buffer,length,"/sys/class/net/eth0/carrier");
        length=logfile(buffer,length,"/sys/class/net/eth0/address");
        length=logfile(buffer,length,"/proc/net/dev");
        length=logfile(buffer,length,"/proc/interrupts");
        length=logfile(buffer,length,"/run/log/sshd.log");
        length=logfile(buffer,length,"/run/log/network.log");
        length=logfile(buffer,length,"/proc/3/wchan");
        length=logfile(buffer,length,"/proc/3/status");
        if(ticks<CODEX_CHILD_DELAY)mark(0xc0de2824);
        if(sock>=0)call(290,sock,(long)buffer,length,0,(long)destination,16);
        if(ticks<CODEX_CHILD_DELAY)mark(0xc0de2828);
        call(4,1,(long)buffer,length,0,0,0);
        /* Read-only kernel log snapshot: do not clear or consume printk ring. */
        {
            long count=call(103,3,(long)klog,ticks?4096:sizeof(klog),0,0,0);
            long offset=0;
            while(count>offset) {
                long chunk=count-offset;if(chunk>1200)chunk=1200;
                length=text(buffer,0,"CODEX KLOG tick=");length=hex(buffer,length,ticks);
                length=text(buffer,length," offset=");length=hex(buffer,length,(u32)offset);
                buffer[length++]='\n';
                {long i;for(i=0;i<chunk;i++)buffer[length++]=klog[offset+i];}
                if(sock>=0)call(290,sock,(long)buffer,length,0,(long)destination,16);
                offset+=chunk;
            }
        }
        /* Stock micomservice performs ReadCommand(0xc0, 3) about every 42 ms.
           Reproduce that liveness poll while the diagnostic PID 1 is active. */
        {
            u32 p;
            for(p=0;p<25;p++) {
                if(micom>=0) {
                    poll_result=call(54,micom,0x80047313,(long)&poll_arg,0,0,0);
                    poll_count++;
                    if(poll_result<0)poll_errors++;
                }
                pause_poll();
            }
        }
        ticks++;
    }
}
