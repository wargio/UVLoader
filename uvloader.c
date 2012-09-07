#include "cleanup.h"
#include "config.h"
#include "load.h"
#include "resolve.h"
#include "scefuncs.h"
#include "utils.h"
#include "uvloader.h"

// make sure code is PIE
#ifndef __PIE__
#error "Must compile with -fPIE"
#endif

/********************************************//**
 *  \brief Starting point from exploit
 *  
 *  Call this from your exploit to run UVLoader.
 *  It will first cache all loaded modules and 
 *  attempt to resolve its own NIDs which 
 *  should only depend on sceLibKernel.a
 *  \returns Zero on success, otherwise error
 ***********************************************/
int START_SECTION
uvl_start (void *f_writeline)
{
    vitasetlog (f_writeline);
    IF_DEBUG LOG ("UVLoader %u.%u.%u started.", UVL_VER_MAJOR, UVL_VER_MINOR, UVL_VER_REV);
    u32_t i = 0;
    for (;;)
    {
        LOG ("Ran %u times", ++i);
    }
    SceUID uvl_thread;
// old resolve table code
#if 0
    // TODO: find a place in memory to store table.
    IF_DEBUG LOG ("Caching all modules to resolve table.");
    if (uvl_resolve_all_loaded_modules (RESOLVE_MOD_IMPS | RESOLVE_MOD_EXPS | RESOLVE_IMPS_SVC_ONLY) < 0)
    {
        LOG ("Cannot cache all loaded modules.");
        return -1;
    }
#endif

    // WARNING: No error checks here
    IF_DEBUG LOG ("Resolving UVLoader.");
    uvl_scefuncs_resolve_all ();

    IF_DEBUG LOG ("Creating thread to run loader.");
    uvl_thread = sceKernelCreateThread ("uvloader", uvl_entry, 0x18, 0x10000, 0, NULL);
    if (uvl_thread < 0)
    {
        LOG ("Cannot create UVLoader thread.");
        return -1;
    }
    IF_DEBUG LOG ("Starting loader thread.");
    if (sceKernelStartThread (uvl_thread, 0, NULL) < 0)
    {
        LOG ("Cannot start UVLoader thread.");
        return -1;
    }
    IF_DEBUG LOG ("Removing old game thread.");
    if (sceKernelExitDeleteThread (0) < 0)
    {
        LOG ("Cannot delete current thread.");
        return -1;
    }
    // should not reach here
    IF_DEBUG LOG ("Error removing old thread.");
    return 0;
}

/********************************************//**
 *  \brief Entry point of UVLoader
 *  
 *  \returns Zero on success, otherwise error
 ***********************************************/
int 
uvl_entry ()
{
    int (*start)();

    IF_DEBUG LOG ("Cleaning up memory.");
    if (uvl_cleanup_memory () < 0)
    {
        LOG ("Cannot cleanup memory.");
        return -1;
    }
    IF_DEBUG LOG ("Loading homebrew.");
    if (uvl_load_exe (HOMEBREW_PATH, (void**)&start) < 0)
    {
        LOG ("Cannot load homebrew.");
        return -1;
    }
    // sceKernelRegisterCallbackToEvent on exit
    IF_DEBUG LOG ("Passing control to homebrew.");
    return start();
}
