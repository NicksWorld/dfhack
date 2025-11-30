#include "Error.h"
#include "Internal.h"

#include "modules/DFSDL.h"

#include "Debug.h"
#include "PluginManager.h"

#include <SDL_stdinc.h>

#ifdef WIN32
# include <regex>
#endif

namespace DFHack {
    DBG_DECLARE(core, dfsdl, DebugCategory::LINFO);
}

using namespace DFHack;
using std::string;
using std::vector;

static DFLibrary *g_sdl_handle = nullptr;
static DFLibrary *g_sdl_image_handle = nullptr;
static const vector<string> SDL_LIBS {
#ifdef WIN32
    "SDL2.dll"
#elif defined(_DARWIN)
    "SDL.framework/Versions/A/SDL",
    "SDL.framework/SDL"
#else
    "libSDL2-2.0.so.0"
#endif
};
static const vector<string> SDL_IMAGE_LIBS {
#ifdef WIN32
    "SDL2_image.dll"
#elif defined(_DARWIN)
    "SDL_image.framework/Versions/A/SDL_image",
    "SDL_image.framework/SDL_image"
#else
    "libSDL2_image-2.0.so.0"
#endif
};


// Function pointer definitions
#define SDL_DYNAPI_PROC(rc, fn, params, args, ret) \
    static rc (*g_##fn) params = nullptr;
#include "SDL_dynapi_procs.h"
#undef SDL_DYNAPI_PROC

SDL_Surface * (*g_IMG_Load)(const char *) = nullptr;

bool DFSDL::init(color_ostream &out) {
    for (auto &lib_str : SDL_LIBS) {
        if ((g_sdl_handle = OpenPlugin(lib_str.c_str())))
            break;
    }
    if (!g_sdl_handle) {
        out.printerr("DFHack could not find SDL\n");
        return false;
    }

    for (auto &lib_str : SDL_IMAGE_LIBS) {
        if ((g_sdl_image_handle = OpenPlugin(lib_str.c_str())))
            break;
    }
    if (!g_sdl_image_handle) {
        out.printerr("DFHack could not find SDL_image\n");
        return false;
    }

    #define bind(handle, name) \
        g_##name = (decltype(g_##name))LookupPlugin(handle, #name); \
        if (!g_##name) { \
            out.printerr("DFHack could not find: " #name "\n"); \
            return false; \
        }

    bind(g_sdl_image_handle, IMG_Load);
#undef bind

    // Load all core SDL function pointers
    #define SDL_DYNAPI_PROC(rc, fn, params, args, ret) \
    g_##fn = (decltype(g_##fn))LookupPlugin(g_sdl_handle, #fn); \
    if (!g_##fn) { \
        out.printerr("DFHack could not find: " #fn "\n"); \
    }
    #include "SDL_dynapi_procs.h"
    #undef SDL_DYNAPI_PROC

    DEBUG(dfsdl,out).print("sdl successfully loaded\n");
    return true;
}

// It's ok to leave NULLs in the raws list (according to usage in g_src)
void DFSDL::cleanup() {
    if (g_sdl_handle) {
        ClosePlugin(g_sdl_handle);
        g_sdl_handle = nullptr;
    }
    if (g_sdl_image_handle) {
        ClosePlugin(g_sdl_image_handle);
        g_sdl_image_handle = nullptr;
    }
}

// Wrapper functions
#define SDL_DYNAPI_PROC(rc, fn, params, args, ret) \
    rc DFHack::DFSDL::DF##fn params { \
        ret g_##fn args; \
    }
#include "SDL_dynapi_procs.h"
#undef SDL_DYNAPI_PROC

SDL_Surface * DFSDL::DFIMG_Load(const char *file) {
    return g_IMG_Load(file);
}


// convert tabs to spaces so they don't get converted to '?'
static char * tabs_to_spaces(char *str) {
    for (char *c = str; *c; ++c) {
        if (*c == '\t')
            *c = ' ';
    }
    return str;
}

static string normalize_newlines(const string & str, bool to_spaces = false) {
    string normalized_str = str;
#ifdef WIN32
    static const std::regex CRLF("\r\n");
    normalized_str = std::regex_replace(normalized_str, CRLF, "\n");
#endif
    if (to_spaces)
        std::replace(normalized_str.begin(), normalized_str.end(), '\n', ' ');

    return normalized_str;
}

DFHACK_EXPORT string DFHack::getClipboardTextCp437() {
    if (!g_sdl_handle || g_SDL_HasClipboardText() != SDL_TRUE)
        return "";
    char *text = tabs_to_spaces(g_SDL_GetClipboardText());
    string textcp437 = UTF2DF(normalize_newlines(text, true));
    DFHack::DFSDL::DFSDL_free(text);
    return textcp437;
}

DFHACK_EXPORT bool DFHack::getClipboardTextCp437Multiline(vector<string> * lines) {
    CHECK_NULL_POINTER(lines);

    if (!g_sdl_handle || g_SDL_HasClipboardText() != SDL_TRUE)
        return false;
    char *text = tabs_to_spaces(g_SDL_GetClipboardText());
    vector<string> utf8_lines;
    split_string(&utf8_lines, normalize_newlines(text), "\n");
    DFHack::DFSDL::DFSDL_free(text);

    for (auto utf8_line : utf8_lines)
        lines->emplace_back(UTF2DF(utf8_line));

    return true;
}

DFHACK_EXPORT bool DFHack::setClipboardTextCp437(string text) {
    if (!g_sdl_handle)
        return false;
    return 0 == DFHack::DFSDL::DFSDL_SetClipboardText(DF2UTF(text).c_str());
}

DFHACK_EXPORT bool DFHack::setClipboardTextCp437Multiline(string text) {
    if (!g_sdl_handle)
        return false;
    vector<string> lines;
    split_string(&lines, text, "\n");
    std::ostringstream str;
    for (size_t idx = 0; idx < lines.size(); ++idx) {
        str << DF2UTF(lines[idx]);
        if (idx < lines.size() - 1) {
#ifdef WIN32
            str << "\r\n";
#else
            str << "\n";
#endif
        }
    }
    return 0 == DFHack::DFSDL::DFSDL_SetClipboardText(str.str().c_str());
}


