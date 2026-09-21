#include "comet_host.h"
#include "comet_settings.h"
#include "comet_compat.h"
#include "ppu_recomp.h"
#include "cellPad.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

extern "C" {
void ps3_hle_register_ctx(uint32_t nid, const char* name, void (*fn)(ppu_context*));
uint16_t vm_read16(uint64_t addr);
uint32_t vm_read32(uint64_t addr);
void vm_write16(uint64_t addr, uint16_t value);
void vm_write32(uint64_t addr, uint32_t value);
int32_t comet_spurs_add_urgent_command(uint64_t jc_ea, uint64_t new_cmd);
}

namespace {
constexpr uint32_t NID_CELL_PAD_GET_DATA = 0x8B72CDA1u; // exact NPEB00142 import
constexpr uint32_t NID_CELL_SPURS_ADD_URGENT_COMMAND = 0x17001000u; // exact NPEB00142 import
constexpr int kOverlayW = 430;
constexpr int kOverlayH = 330;
constexpr UINT_PTR kOverlayTimer = 1;

CometSettings g_cfg;
std::string g_cfg_path;
HWND g_game = nullptr;
HWND g_overlay = nullptr;
bool g_overlay_visible = false;
bool g_borderless_applied = false;
LONG_PTR g_saved_style = 0;
RECT g_saved_rect{};
int g_selected = 0;
bool g_restart_required = false;
POINT g_last_mouse{};
bool g_have_last_mouse = false;
SHORT g_prev_keys[256]{};

const int kRes[][2] = {{1280,720},{1600,900},{1920,1080},{2560,1440},{3840,2160}};
constexpr int kResCount = sizeof(kRes)/sizeof(kRes[0]);

std::string module_dir() {
    char p[MAX_PATH]{};
    GetModuleFileNameA(nullptr, p, MAX_PATH);
    char* slash = std::strrchr(p, '\\');
    if (!slash) slash = std::strrchr(p, '/');
    if (slash) *slash = 0;
    return p[0] ? std::string(p) : std::string(".");
}

bool key_edge(int vk) {
    SHORT now = GetAsyncKeyState(vk);
    bool pressed = (now & 0x8000) && !(g_prev_keys[vk & 255] & 0x8000);
    g_prev_keys[vk & 255] = now;
    return pressed;
}

BOOL CALLBACK enum_game_window(HWND w, LPARAM) {
    if (w == g_overlay || !IsWindowVisible(w)) return TRUE;
    DWORD pid = 0; GetWindowThreadProcessId(w, &pid);
    if (pid != GetCurrentProcessId()) return TRUE;
    LONG_PTR ex = GetWindowLongPtrA(w, GWL_EXSTYLE);
    if (ex & WS_EX_TOOLWINDOW) return TRUE;
    RECT r{}; if (!GetClientRect(w, &r) || r.right-r.left < 320 || r.bottom-r.top < 200) return TRUE;
    g_game = w;
    return FALSE;
}

void locate_game_window() {
    if (g_game && IsWindow(g_game)) return;
    g_game = nullptr;
    EnumWindows(enum_game_window, 0);
}

void position_overlay() {
    if (!g_overlay || !g_game) return;
    RECT cr{}; GetClientRect(g_game, &cr);
    POINT pt{cr.left, cr.top}; ClientToScreen(g_game, &pt);
    SetWindowPos(g_overlay, HWND_TOPMOST, pt.x + 24, pt.y + 24, kOverlayW, kOverlayH,
                 SWP_NOACTIVATE | (g_overlay_visible ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
}

void set_borderless(bool on) {
    locate_game_window();
    if (!g_game) return;
    if (on == g_borderless_applied) return;
    if (on) {
        g_saved_style = GetWindowLongPtrA(g_game, GWL_STYLE);
        GetWindowRect(g_game, &g_saved_rect);
        MONITORINFO mi{sizeof(mi)};
        GetMonitorInfoA(MonitorFromWindow(g_game, MONITOR_DEFAULTTONEAREST), &mi);
        SetWindowLongPtrA(g_game, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowPos(g_game, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top,
                     mi.rcMonitor.right-mi.rcMonitor.left, mi.rcMonitor.bottom-mi.rcMonitor.top,
                     SWP_FRAMECHANGED | SWP_NOACTIVATE);
    } else {
        LONG_PTR style = g_saved_style ? g_saved_style : (WS_OVERLAPPEDWINDOW | WS_VISIBLE);
        SetWindowLongPtrA(g_game, GWL_STYLE, style);
        RECT wr{0,0,g_cfg.width,g_cfg.height};
        AdjustWindowRect(&wr, (DWORD)style, FALSE);
        int w=wr.right-wr.left, h=wr.bottom-wr.top;
        int x = g_saved_rect.right>g_saved_rect.left ? g_saved_rect.left : CW_USEDEFAULT;
        int y = g_saved_rect.bottom>g_saved_rect.top ? g_saved_rect.top : CW_USEDEFAULT;
        SetWindowPos(g_game, HWND_NOTOPMOST, x, y, w, h, SWP_FRAMECHANGED | SWP_NOACTIVATE);
    }
    g_borderless_applied = on;
    position_overlay();
}

void save_settings() {
    comet_settings_save(g_cfg_path, g_cfg);
    _putenv_s("COMET_VSYNC", g_cfg.vsync ? "1" : "0");
}

int resolution_index() {
    int best=0; long long score=LLONG_MAX;
    for (int i=0;i<kResCount;i++) {
        long long dx=kRes[i][0]-g_cfg.width, dy=kRes[i][1]-g_cfg.height;
        long long s=dx*dx+dy*dy; if (s<score){score=s;best=i;}
    }
    return best;
}

void alter_selected(int dir) {
    switch (g_selected) {
    case 0: {
        int i=resolution_index(); i=(i+dir+kResCount)%kResCount;
        g_cfg.width=kRes[i][0]; g_cfg.height=kRes[i][1]; g_restart_required=true; break;
    }
    case 1: g_cfg.borderless=!g_cfg.borderless; set_borderless(g_cfg.borderless); break;
    case 2: g_cfg.vsync=!g_cfg.vsync; break;
    case 3: g_cfg.mouse_enabled=!g_cfg.mouse_enabled; break;
    case 4: g_cfg.mouse_sensitivity=std::clamp(g_cfg.mouse_sensitivity+0.1f*(float)dir,0.1f,6.0f); break;
    case 5: g_cfg.mouse_stick=!g_cfg.mouse_stick; break;
    }
    save_settings();
    if (g_overlay) InvalidateRect(g_overlay,nullptr,FALSE);
}

LRESULT CALLBACK overlay_proc(HWND w, UINT msg, WPARAM wp, LPARAM lp) {
    switch(msg) {
    case WM_ERASEBKGND: return 1;
    case WM_TIMER: InvalidateRect(w,nullptr,FALSE); return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps{}; HDC dc=BeginPaint(w,&ps);
        RECT rc{}; GetClientRect(w,&rc);
        HBRUSH bg=CreateSolidBrush(RGB(22,22,26)); FillRect(dc,&rc,bg); DeleteObject(bg);
        SetBkMode(dc,TRANSPARENT); SetTextColor(dc,RGB(238,238,242));
        HFONT font=CreateFontA(20,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,
                               CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_DONTCARE,"Segoe UI");
        HFONT old=(HFONT)SelectObject(dc,font);
        const char* title="Comet Crash PC - Graphics & Input";
        TextOutA(dc,18,14,title,(int)std::strlen(title));
        SetTextColor(dc,RGB(170,170,180));
        const char* help="F1 close   Up/Down select   Left/Right change";
        TextOutA(dc,18,42,help,(int)std::strlen(help));
        char rows[6][96];
        std::snprintf(rows[0],96,"Resolution        %d x %d%s",g_cfg.width,g_cfg.height,g_restart_required?"  (restart)":"");
        std::snprintf(rows[1],96,"Display mode      %s",g_cfg.borderless?"Borderless":"Windowed");
        std::snprintf(rows[2],96,"VSync             %s",g_cfg.vsync?"On":"Off");
        std::snprintf(rows[3],96,"Mouse input       %s",g_cfg.mouse_enabled?"On":"Off");
        std::snprintf(rows[4],96,"Mouse sensitivity %.1f",g_cfg.mouse_sensitivity);
        std::snprintf(rows[5],96,"Mouse stick       %s",g_cfg.mouse_stick?"Right":"Left");
        for(int i=0;i<6;i++){
            int y=82+i*34;
            if(i==g_selected){ RECT rr{10,y-5,kOverlayW-10,y+27}; HBRUSH hi=CreateSolidBrush(RGB(48,58,82)); FillRect(dc,&rr,hi); DeleteObject(hi); SetTextColor(dc,RGB(255,255,255)); }
            else SetTextColor(dc,RGB(215,215,220));
            TextOutA(dc,22,y,rows[i],(int)std::strlen(rows[i]));
        }
        SetTextColor(dc,RGB(140,140,150));
        const char* mousehelp="LMB=Cross/confirm   RMB=Circle/back   MMB=R1";
        TextOutA(dc,18,294,mousehelp,(int)std::strlen(mousehelp));
        SelectObject(dc,old); DeleteObject(font); EndPaint(w,&ps); return 0;
    }
    }
    return DefWindowProcA(w,msg,wp,lp);
}

void ensure_overlay() {
    if (g_overlay) return;
    WNDCLASSA wc{}; wc.lpfnWndProc=overlay_proc; wc.hInstance=GetModuleHandleA(nullptr);
    wc.lpszClassName="CometCrashHostOverlay"; wc.hCursor=LoadCursor(nullptr,IDC_ARROW);
    RegisterClassA(&wc);
    g_overlay=CreateWindowExA(WS_EX_TOOLWINDOW|WS_EX_TOPMOST|WS_EX_NOACTIVATE,
        wc.lpszClassName,"Comet Crash PC Settings",WS_POPUP,0,0,kOverlayW,kOverlayH,
        nullptr,nullptr,wc.hInstance,nullptr);
    SetTimer(g_overlay,kOverlayTimer,250,nullptr);
    position_overlay();
}

bool game_focused() {
    locate_game_window();
    if (!g_game) return false;
    HWND fg=GetForegroundWindow();
    return fg==g_game || (fg && GetWindow(fg,GW_OWNER)==g_game);
}


void inject_mouse(uint32_t ea) {
    if (!g_cfg.mouse_enabled || g_overlay_visible || !game_focused()) { g_have_last_mouse=false; return; }
    POINT p{}; if (!GetCursorPos(&p)) return;
    if (!g_have_last_mouse) { g_last_mouse=p; g_have_last_mouse=true; return; }
    int dx=p.x-g_last_mouse.x, dy=p.y-g_last_mouse.y; g_last_mouse=p;
    // Respect libpad's change-packet drain semantics. Never synthesize a new
    // packet when the underlying HLE returned len=0 or a held mouse button
    // would make a title's `while (len > 0)` drain loop infinite.
    if ((int32_t)vm_read32(ea) <= 0) return;
    bool l=(GetAsyncKeyState(VK_LBUTTON)&0x8000)!=0;
    bool r=(GetAsyncKeyState(VK_RBUTTON)&0x8000)!=0;
    bool m=(GetAsyncKeyState(VK_MBUTTON)&0x8000)!=0;
    bool ku=(GetAsyncKeyState('W')&0x8000)!=0;
    bool kr=(GetAsyncKeyState('D')&0x8000)!=0;
    bool kd=(GetAsyncKeyState('S')&0x8000)!=0;
    bool kl=(GetAsyncKeyState('A')&0x8000)!=0;
    bool w=dx||dy||l||r||m||ku||kr||kd||kl;
    if (!w) return;
    uint16_t d1=vm_read16(ea+8), d2=vm_read16(ea+10);
    if(l) d2|=0x40; // CELL_PAD_CTRL_CROSS >> 8
    if(r) d2|=0x20; // CIRCLE >> 8
    if(m) d2|=0x08; // R1 >> 8
    if(ku) d1|=0x10;
    if(kr) d1|=0x20;
    if(kd) d1|=0x40;
    if(kl) d1|=0x80;
    vm_write16(ea+8,d1); vm_write16(ea+10,d2);
    float scale=6.0f*g_cfg.mouse_sensitivity;
    int ax=std::clamp((int)std::lround(128.0f+dx*scale),0,255);
    int ay=std::clamp((int)std::lround(128.0f+dy*scale),0,255);
    if(g_cfg.mouse_stick){ vm_write16(ea+12,(uint16_t)ax); vm_write16(ea+14,(uint16_t)ay); }
    else { vm_write16(ea+16,(uint16_t)ax); vm_write16(ea+18,(uint16_t)ay); }
}

void hle_cellPadGetData_mouse(ppu_context* ctx) {
    uint32_t port=(uint32_t)ctx->gpr[3];
    uint32_t ea=(uint32_t)ctx->gpr[4];
    int32_t rc=cellPadGetData(port,(CellPadData*)(uintptr_t)ea);
    ctx->gpr[3]=(uint64_t)(int64_t)rc;
    if(rc==0 && port==0 && ea) inject_mouse(ea);
}

void hle_cellSpursAddUrgentCommand_comet(ppu_context* ctx) {
    const uint32_t jc=(uint32_t)ctx->gpr[3];
    const uint64_t cmd=ctx->gpr[4];
    const int32_t rc=comet_spurs_add_urgent_command(jc,cmd);
    ctx->gpr[3]=(uint64_t)(int64_t)rc;
}
}

void comet_host_init(void) {
    g_cfg_path=module_dir()+"\\comet_crash.ini";
    if(!comet_settings_load(g_cfg_path,g_cfg)) comet_settings_save(g_cfg_path,g_cfg);
    _putenv_s("COMET_VSYNC",g_cfg.vsync?"1":"0");
    std::fprintf(stderr,"[comet-host] %dx%d %s vsync=%s mouse=%s sens=%.1f stick=%s\n",
        g_cfg.width,g_cfg.height,g_cfg.borderless?"borderless":"windowed",
        g_cfg.vsync?"on":"off",g_cfg.mouse_enabled?"on":"off",g_cfg.mouse_sensitivity,
        g_cfg.mouse_stick?"right":"left");
}
void comet_host_shutdown(void) { save_settings(); if(g_overlay) DestroyWindow(g_overlay); g_overlay=nullptr; }
void comet_host_install_hle_overrides(void) {
    comet_compat_install_hle_overrides();
    ps3_hle_register_ctx(NID_CELL_PAD_GET_DATA,"cellPadGetData [Comet mouse]",hle_cellPadGetData_mouse);
    ps3_hle_register_ctx(NID_CELL_SPURS_ADD_URGENT_COMMAND,"cellSpursAddUrgentCommand [Comet jobchain]",hle_cellSpursAddUrgentCommand_comet);
    std::fprintf(stderr,"[comet-host] mouse HLE override installed for NID 0x%08X\n",NID_CELL_PAD_GET_DATA);
    std::fprintf(stderr,"[comet-host] SPURS urgent-command override installed for NID 0x%08X\n",NID_CELL_SPURS_ADD_URGENT_COMMAND);
}
void comet_host_pump(void) {
    locate_game_window(); ensure_overlay();
    if(g_game && g_cfg.borderless != g_borderless_applied) set_borderless(g_cfg.borderless);
    if((game_focused() || g_overlay_visible) && key_edge(VK_F1)){ g_overlay_visible=!g_overlay_visible; if(g_overlay_visible) g_have_last_mouse=false; position_overlay(); }
    if(g_overlay_visible){
        if(key_edge(VK_UP)) g_selected=(g_selected+5)%6;
        if(key_edge(VK_DOWN)) g_selected=(g_selected+1)%6;
        if(key_edge(VK_LEFT)) alter_selected(-1);
        if(key_edge(VK_RIGHT) || key_edge(VK_RETURN)) alter_selected(1);
        if(key_edge(VK_ESCAPE)){g_overlay_visible=false;position_overlay();}
        position_overlay();
    }
}
uint32_t comet_host_width(void){return (uint32_t)g_cfg.width;}
uint32_t comet_host_height(void){return (uint32_t)g_cfg.height;}
int comet_host_vsync(void){return g_cfg.vsync?1:0;}
