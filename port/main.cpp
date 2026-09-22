#include "ppu_recomp.h"
#include "win32_compat.h"
#include "comet_host.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <timeapi.h>
#endif

extern "C" {
uint32_t ppu_load_elf(const char* path);
void ppu_recomp_register(void);
void ppu_hle_init(void);
void ppu_sysprx_register(void);
void ppu_fs_register(void);
void lv2_init_syscalls(void);
void cellGame_init_from_paramsfo(const char* sfo_path);
int ppu_run(uint32_t entry_opd, uint32_t stack_top);
void ps3_load_prx_modules(void);
extern const char* ppu_vfs_root;
extern int g_sys_process_exit_called;
extern int32_t g_sys_process_exit_code;
extern uint32_t ppu_vm_size;
uint8_t* vm_base = nullptr;
typedef void (*ps3_guest_caller_fn)(uint32_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t);
extern ps3_guest_caller_fn g_ps3_guest_caller;
uint64_t ppu_guest_call(uint32_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t);
void cellGcmTickVBlank(void);
void cellGcmTickFlip(void);
int cellGcm_take_flip_pending(void);
void cellGcm_rsx_process_fifo(void);
unsigned cellGcm_flip_request_count(void);
extern uint32_t g_last_hle_nid;
extern const char* g_last_hle_name;
int rsx_d3d12_backend_init(uint32_t,uint32_t,const char*);
void rsx_d3d12_backend_present(void);
int rsx_d3d12_backend_pump_messages(void);
}

#define VM_SIZE 0x100010000ull
#define STACK_TOP 0x0FF00000u
static volatile LONG g_frames_presented=0;
static const char* volatile g_last_boot_stage="process start";
extern "C" unsigned ppu_boot_frames_presented(void){ return (unsigned)g_frames_presented; }

static void present_guest_frame(){
    rsx_d3d12_backend_present();
    LONG frame=InterlockedIncrement(&g_frames_presented);
    if(frame==1){
        fprintf(stderr,"[boot-stage] first guest frame presented\n");
        fflush(stderr);
    }
}

static DWORD WINAPI frame_clock(LPVOID){
    const char* title=getenv("PS3_TITLE");
    if(!title||!*title) title="Comet Crash";
    int ok=rsx_d3d12_backend_init(comet_host_width(),comet_host_height(),title)==0;
    unsigned last=0;
    ULONGLONG next=GetTickCount64();
    fprintf(stderr,"[rsx] D3D12 init %s (%ux%u)\n",ok?"OK":"FAILED",comet_host_width(),comet_host_height());
    for(;;){
        Sleep(4);
        comet_host_pump();
        ULONGLONG now=GetTickCount64();
        int fired=0;
        while((long long)(now-next)>=0 && fired<240){
            cellGcmTickVBlank();
            cellGcmTickFlip();
            if(ok&&cellGcm_take_flip_pending()){present_guest_frame();last=cellGcm_flip_request_count();}
            if(ok)cellGcm_rsx_process_fifo();
            next+=16; // guest timing stays ~60 Hz; graphics options must not speed up simulation
            fired++;
        }
        if(fired>=240) next=now;
        if(ok){
            if(cellGcm_take_flip_pending()){present_guest_frame();last=cellGcm_flip_request_count();}
            cellGcm_rsx_process_fifo();
            if(rsx_d3d12_backend_pump_messages()!=0){ok=0;continue;}
            unsigned fc=cellGcm_flip_request_count();
            if(fc!=last){present_guest_frame();last=fc;}
            else if(fc==0)rsx_d3d12_backend_present();
        }
    }
    return 0;
}


static void boot_stage(const char* stage){
    g_last_boot_stage=stage;
    fprintf(stderr,"[boot-stage] %s\n",stage);
    fflush(stderr);
}

static DWORD WINAPI boot_watchdog(LPVOID){
    static const DWORD waits_ms[]={10000u,20000u};
    unsigned elapsed=0;
    for(DWORD wait_ms:waits_ms){
        Sleep(wait_ms); elapsed+=(unsigned)(wait_ms/1000u);
        if(g_frames_presented>0) return 0;
        const char* hle=g_last_hle_name&&*g_last_hle_name?g_last_hle_name:"<none>";
        fprintf(stderr,"[watchdog] no guest frame after %us; last_stage=%s flips=%u last_hle=0x%08X (%s)\n",
                elapsed,g_last_boot_stage?g_last_boot_stage:"<none>",cellGcm_flip_request_count(),g_last_hle_nid,hle);
        fflush(stderr);
    }
    return 0;
}

static char s_root[1024];
static void derive_root(const char* eboot){
    const char* env=getenv("PS3_VFS_ROOT");
    if(env&&*env){ppu_vfs_root=env;return;}
    strncpy(s_root,eboot,sizeof(s_root)-1); s_root[sizeof(s_root)-1]=0;
    for(char*p=s_root;*p;p++)if(*p=='\\')*p='/';
    for(int i=0;i<3;i++){char*s=strrchr(s_root,'/');if(s)*s=0;}
    if(!s_root[0])strcpy(s_root,".");
    ppu_vfs_root=s_root;
}

#ifdef _WIN32
static LONG WINAPI crash_filter(EXCEPTION_POINTERS* ep){
    const DWORD code=ep&&ep->ExceptionRecord?ep->ExceptionRecord->ExceptionCode:0;
    void* address=ep&&ep->ExceptionRecord?ep->ExceptionRecord->ExceptionAddress:nullptr;
    HINSTANCE image=GetModuleHandleA(nullptr);
    uintptr_t rva=(image&&address)?(uintptr_t)address-(uintptr_t)image:0;
    fprintf(stderr,"\n[crash] code=0x%08lX address=%p image_rva=0x%llX thread=%lu\n",
            (unsigned long)code,address,(unsigned long long)rva,(unsigned long)GetCurrentThreadId());
    if(code==EXCEPTION_ACCESS_VIOLATION&&ep&&ep->ExceptionRecord){
        ULONG_PTR op=ep->ExceptionRecord->ExceptionInformation[0];
        ULONG_PTR target=ep->ExceptionRecord->ExceptionInformation[1];
        const char* kind=op==0?"read":op==1?"write":op==8?"execute":"unknown";
        fprintf(stderr,"[crash] access_violation=%s target=0x%llX\n",kind,(unsigned long long)target);
    }
    fflush(stderr);
    return EXCEPTION_CONTINUE_SEARCH;
}

static LONG WINAPI vm_commit(EXCEPTION_POINTERS* ep){
    if(ep->ExceptionRecord->ExceptionCode==EXCEPTION_ACCESS_VIOLATION){
        ULONG_PTR fault=ep->ExceptionRecord->ExceptionInformation[1],base=(uintptr_t)vm_base;
        if(vm_base&&fault>=base&&fault<base+VM_SIZE){
            void*page=(void*)(fault&~(uintptr_t)0xFFFF);
            if(VirtualAlloc(page,0x10000,MEM_COMMIT,PAGE_READWRITE)) return EXCEPTION_CONTINUE_EXECUTION;
        }
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif

static bool alloc_vm(){
    AddVectoredExceptionHandler(1,vm_commit);
    vm_base=(uint8_t*)VirtualAlloc(NULL,VM_SIZE,MEM_RESERVE,PAGE_READWRITE);
    ppu_vm_size=0;
    return vm_base!=nullptr;
}
static void guest_call(uint32_t opd,uint64_t a0,uint64_t a1,uint64_t a2,uint64_t a3,uint64_t a4,uint64_t a5,uint64_t a6,uint64_t a7){
    ppu_guest_call(opd,a0,a1,a2,a3,a4,a5,a6,a7);
}

int main(int argc,char**argv){
    if(argc<2){printf("usage: %s <PPU ELF>\n",argv[0]);return 2;}
    timeBeginPeriod(1);
    setvbuf(stdout,NULL,_IONBF,0);
    setvbuf(stderr,NULL,_IONBF,0);
    SetUnhandledExceptionFilter(crash_filter);
    CreateThread(NULL,0,boot_watchdog,NULL,0,NULL);
    boot_stage("process entered");
    fprintf(stderr,"[boot] ELF: %s\n",argv[1]);
    comet_host_init();
    boot_stage("host settings initialized");
    if(!alloc_vm()){fprintf(stderr,"VM allocation failed\n");return 1;}
    fprintf(stderr,"[boot] guest VM base=%p size=0x%llX\n",(void*)vm_base,(unsigned long long)VM_SIZE);
    boot_stage("guest VM reserved");
    uint32_t entry=ppu_load_elf(argv[1]);
    if(!entry){fprintf(stderr,"[boot] ppu_load_elf failed\n");return 1;}
    boot_stage("PPU ELF loaded");
    derive_root(argv[1]);
    fprintf(stderr,"[boot] VFS root: %s\n",ppu_vfs_root?ppu_vfs_root:"<null>");
    {
        const char* sfo=getenv("PS3_PARAM_SFO");
        char fallback[1200];
        if(!sfo||!*sfo){
            snprintf(fallback,sizeof(fallback),"%s/PARAM.SFO",ppu_vfs_root?ppu_vfs_root:".");
            sfo=fallback;
        }
        fprintf(stderr,"[boot] PARAM.SFO: %s\n",sfo);
        cellGame_init_from_paramsfo(sfo);
    }
    boot_stage("PARAM.SFO initialized");
    ppu_recomp_register();
    boot_stage("recompiled PPU table registered");
    ps3_load_prx_modules();
    boot_stage("PRX registration complete");
    ppu_hle_init();
    boot_stage("HLE NID table initialized");
    ppu_sysprx_register();
    boot_stage("sysPrxForUser registered");
    ppu_fs_register();
    boot_stage("cellFs/VFS registered");
    lv2_init_syscalls();
    boot_stage("LV2 syscall table initialized");
    /* Install title overrides LAST so generic module/fs registration cannot
     * replace Comet's mouse, HG-content or urgent-jobchain handlers. */
    comet_host_install_hle_overrides();
    boot_stage("Comet HLE overrides installed");
    g_ps3_guest_caller=guest_call;
    CreateThread(NULL,4u*1024*1024,frame_clock,NULL,0,NULL);
    boot_stage("frame clock started");
    fprintf(stderr,"[boot] entry OPD 0x%08X, stack top 0x%08X\n",entry,STACK_TOP);
    boot_stage("entering recompiled title");
    int rc=ppu_run(entry,STACK_TOP);
    fprintf(stderr,"[boot] ppu_run returned %d; process_exit_called=%d process_exit_code=%d frames=%u\n",
            rc,g_sys_process_exit_called,(int)g_sys_process_exit_code,ppu_boot_frames_presented());
    boot_stage("recompiled title returned");
    comet_host_shutdown();
    return g_sys_process_exit_called?(int)g_sys_process_exit_code:rc;
}
