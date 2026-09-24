from __future__ import annotations
import argparse
from pathlib import Path

OLD='s_d3d.swap_chain->lpVtbl->Present(s_d3d.swap_chain, 1, 0); /* vsync */'
NEW='''{ const char* comet_vsync = getenv("COMET_VSYNC");\n        UINT comet_sync_interval = (!comet_vsync || comet_vsync[0] != '0') ? 1u : 0u;\n        s_d3d.swap_chain->lpVtbl->Present(s_d3d.swap_chain, comet_sync_interval, 0); } /* Comet host VSync */'''

DIAG_PATCHES = (
    (
        'command queue HRESULT',
        '''    if (FAILED(hr)) {
        printf("[D3D12] ERROR: CreateCommandQueue failed\\n");
        factory->lpVtbl->Release(factory);
        return -1;
    }''',
        '''    if (FAILED(hr)) {
        fprintf(stderr, "[D3D12] ERROR: CreateCommandQueue failed (0x%08lX)\\n", (unsigned long)hr);
        factory->lpVtbl->Release(factory);
        return -1;
    }''',
        'CreateCommandQueue failed (0x%08lX)',
    ),
    (
        'SwapChain3 HRESULT',
        '''    if (FAILED(hr)) {
        printf("[D3D12] ERROR: QueryInterface for SwapChain3 failed\\n");
        return -1;
    }''',
        '''    if (FAILED(hr)) {
        fprintf(stderr, "[D3D12] ERROR: QueryInterface for SwapChain3 failed (0x%08lX)\\n", (unsigned long)hr);
        return -1;
    }''',
        'QueryInterface for SwapChain3 failed (0x%08lX)',
    ),
    (
        'RTV descriptor heap',
        '''    hr = s_d3d.device->lpVtbl->CreateDescriptorHeap(
        s_d3d.device, &rtv_heap_desc, &IID_ID3D12DescriptorHeap, (void**)&s_d3d.rtv_heap);
    if (FAILED(hr)) return -1;''',
        '''    hr = s_d3d.device->lpVtbl->CreateDescriptorHeap(
        s_d3d.device, &rtv_heap_desc, &IID_ID3D12DescriptorHeap, (void**)&s_d3d.rtv_heap);
    if (FAILED(hr)) {
        fprintf(stderr, "[D3D12] ERROR: RTV descriptor heap creation failed (0x%08lX)\\n", (unsigned long)hr);
        return -1;
    }''',
        'RTV descriptor heap creation failed',
    ),
    (
        'swap-chain backbuffer',
        '''        hr = s_d3d.swap_chain->lpVtbl->GetBuffer(
            s_d3d.swap_chain, i, &IID_ID3D12Resource, (void**)&s_d3d.render_targets[i]);
        if (FAILED(hr)) return -1;''',
        '''        hr = s_d3d.swap_chain->lpVtbl->GetBuffer(
            s_d3d.swap_chain, i, &IID_ID3D12Resource, (void**)&s_d3d.render_targets[i]);
        if (FAILED(hr)) {
            fprintf(stderr, "[D3D12] ERROR: swap-chain GetBuffer[%u] failed (0x%08lX)\\n",
                    i, (unsigned long)hr);
            return -1;
        }''',
        'swap-chain GetBuffer[%u] failed',
    ),
    (
        'DSV descriptor heap HRESULT',
        '''        if (FAILED(hr)) {
            printf("[D3D12] DSV heap creation failed\\n");
            return -1;
        }''',
        '''        if (FAILED(hr)) {
            fprintf(stderr, "[D3D12] ERROR: DSV heap creation failed (0x%08lX)\\n", (unsigned long)hr);
            return -1;
        }''',
        'DSV heap creation failed (0x%08lX)',
    ),
    (
        'command allocator',
        '''        hr = s_d3d.device->lpVtbl->CreateCommandAllocator(
            s_d3d.device, D3D12_COMMAND_LIST_TYPE_DIRECT,
            &IID_ID3D12CommandAllocator, (void**)&s_d3d.cmd_allocators[i]);
        if (FAILED(hr)) return -1;''',
        '''        hr = s_d3d.device->lpVtbl->CreateCommandAllocator(
            s_d3d.device, D3D12_COMMAND_LIST_TYPE_DIRECT,
            &IID_ID3D12CommandAllocator, (void**)&s_d3d.cmd_allocators[i]);
        if (FAILED(hr)) {
            fprintf(stderr, "[D3D12] ERROR: CreateCommandAllocator[%u] failed (0x%08lX)\\n",
                    i, (unsigned long)hr);
            return -1;
        }''',
        'CreateCommandAllocator[%u] failed',
    ),
    (
        'command list',
        '''    if (FAILED(hr)) return -1;

    /* Close the command list (it starts in recording state) */''',
        '''    if (FAILED(hr)) {
        fprintf(stderr, "[D3D12] ERROR: CreateCommandList failed (0x%08lX)\\n", (unsigned long)hr);
        return -1;
    }

    /* Close the command list (it starts in recording state) */''',
        'CreateCommandList failed (0x%08lX)',
    ),
    (
        'fence',
        '''    hr = s_d3d.device->lpVtbl->CreateFence(
        s_d3d.device, 0, D3D12_FENCE_FLAG_NONE,
        &IID_ID3D12Fence, (void**)&s_d3d.fence);
    if (FAILED(hr)) return -1;

    s_d3d.fence_event = CreateEvent(NULL, FALSE, FALSE, NULL);''',
        '''    hr = s_d3d.device->lpVtbl->CreateFence(
        s_d3d.device, 0, D3D12_FENCE_FLAG_NONE,
        &IID_ID3D12Fence, (void**)&s_d3d.fence);
    if (FAILED(hr)) {
        fprintf(stderr, "[D3D12] ERROR: CreateFence failed (0x%08lX)\\n", (unsigned long)hr);
        return -1;
    }

    s_d3d.fence_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!s_d3d.fence_event) {
        fprintf(stderr, "[D3D12] ERROR: CreateEvent for fence failed (winerr=%lu)\\n",
                (unsigned long)GetLastError());
        return -1;
    }''',
        'CreateEvent for fence failed',
    ),
    (
        'root signature HRESULT',
        '''        if (FAILED(hr)) {
            printf("[D3D12] Root signature creation failed\\n");
            return -1;
        }''',
        '''        if (FAILED(hr)) {
            fprintf(stderr, "[D3D12] ERROR: Root signature creation failed (0x%08lX)\\n",
                    (unsigned long)hr);
            return -1;
        }''',
        'Root signature creation failed (0x%08lX)',
    ),
)

def patch_text(s: str) -> tuple[str,bool]:
    if 'comet_sync_interval' in s:
        return s, False
    if OLD not in s:
        raise ValueError('expected ps3recomp D3D12 Present(1,0) site not found; upstream changed')
    if s.count(OLD) != 1:
        raise ValueError(f'expected one D3D12 Present site, found {s.count(OLD)}')
    return s.replace(OLD,NEW), True

def patch_diagnostics_text(s: str) -> tuple[str,int]:
    changed=0
    for label,old,new,marker in DIAG_PATCHES:
        if marker in s:
            continue
        count=s.count(old)
        if count != 1:
            raise ValueError(f'expected one ps3recomp D3D12 {label} site, found {count}; upstream changed')
        s=s.replace(old,new)
        changed+=1
    return s,changed

def patch_file(path: Path) -> bool:
    src=path.read_text(encoding='utf-8')
    out,vsync_changed=patch_text(src)
    out,diagnostic_changes=patch_diagnostics_text(out)
    changed=vsync_changed or diagnostic_changes>0
    if changed:
        path.write_text(out,encoding='utf-8',newline='\n')
    return changed

def main()->int:
    ap=argparse.ArgumentParser(description='Apply Comet Crash host hooks to ps3recomp D3D12 source')
    ap.add_argument('ps3recomp',type=Path)
    a=ap.parse_args()
    p=a.ps3recomp/'libs'/'video'/'rsx_d3d12_backend.c'
    changed=patch_file(p)
    print(f'{p}: {"patched" if changed else "already patched"}')
    return 0
if __name__=='__main__': raise SystemExit(main())
