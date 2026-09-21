#include <cstdio>

extern "C" void ps3_load_prx_modules(void)
{
    std::fprintf(stderr, "[comet] no game-specific PRX modules registered\n");
    // SPU workloads are registered by generated/spu_workloads.c's constructor.
}
