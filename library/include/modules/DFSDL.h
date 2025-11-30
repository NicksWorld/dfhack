#pragma once

#include "Export.h"
#include "ColorText.h"

#define SDL_PROTOTYPES_ONLY
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>
#undef SDL_PROTOTYPES_ONLY

#include <vector>

struct SDL_Surface;
struct SDL_Rect;
struct SDL_PixelFormat;
struct SDL_Window;
union SDL_Event;

namespace DFHack
{
    struct DFTileSurface
    {
        bool paintOver; // draw over original tile?
        SDL_Surface* surface; // from where it should be drawn
        SDL_Rect* rect; // from which coords (NULL to draw whole surface)
        SDL_Rect* dstResize; // if not NULL dst rect will be resized (x/y/w/h will be added to original dst)
    };
}

/**
 * The DFSDL module - provides access to SDL functions without actually
 * requiring build-time linkage to SDL
 * \ingroup grp_modules
 * \ingroup grp_dfsdl
 */
namespace DFHack::DFSDL
{
    /**
        * Call this on DFHack init so we can load the SDL functions. Returns false on
        * failure.
        */
    bool init(DFHack::color_ostream& out);

    /**
        * Call this when DFHack is being unloaded.
        */
    void cleanup();

    DFHACK_EXPORT SDL_Surface* DFIMG_Load(const char* file);
}

namespace DFHack::DFSDL {
    // Wrapper function definitions
    #define SDL_DYNAPI_PROC(rc, fn, params, args, ret) \
        DFHACK_EXPORT rc DF##fn params;
    #include "SDL_dynapi_procs.h"
    #undef SDL_DYNAPI_PROC
}

namespace DFHack
{

    // System clipboard -- submitted and returned text must be in CP437
    DFHACK_EXPORT std::string getClipboardTextCp437();
    DFHACK_EXPORT bool setClipboardTextCp437(std::string text);

    // interprets 0xa as newline instead of usual CP437 char
    DFHACK_EXPORT bool getClipboardTextCp437Multiline(std::vector<std::string> * lines);
    DFHACK_EXPORT bool setClipboardTextCp437Multiline(std::string text);

}
