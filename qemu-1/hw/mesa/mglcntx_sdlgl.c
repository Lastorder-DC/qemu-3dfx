/*
 * QEMU MESA GL Pass-Through
 *
 *  Copyright (c) 2020
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library;
 * if not, see <http://www.gnu.org/licenses/>.
 */

#include "qemu/osdep.h"
#include "qemu/timer.h"
#include "qemu-common.h"
#include "ui/console.h"

#include "mesagl_impl.h"

#define DPRINTF(fmt, ...) \
    do { fprintf(stderr, "glcntx: " fmt "\n" , ## __VA_ARGS__); } while(0)
#define DPRINTF_COND(cond,fmt, ...) \
    if (cond) { fprintf(stderr, "glcntx: " fmt "\n" , ## __VA_ARGS__); }

#if (defined(CONFIG_LINUX) && CONFIG_LINUX) || \
    (defined(CONFIG_DARWIN) && CONFIG_DARWIN)
#define MESAGL_SDLGL 1
#if defined(CONFIG_DARWIN) && CONFIG_DARWIN
const char dllname[] = "/System/Library/Frameworks/OpenGL.framework/Libraries/libGL.dylib";
int MGLUpdateGuestBufo(mapbufo_t *bufo, int add) { return 0; }
#endif
#if defined(CONFIG_LINUX) && CONFIG_LINUX
#include "sysemu/kvm.h"

int MGLUpdateGuestBufo(mapbufo_t *bufo, int add)
{
    int ret = GetBufOAccelEN()? kvm_enabled():0;

    if (ret && bufo) {
        bufo->lvl = (add)? MapBufObjGpa(bufo):0;
        kvm_update_guest_pa_range(MBUFO_BASE | (bufo->gpa & ((MBUFO_SIZE - 1) - (qemu_real_host_page_size - 1))),
            bufo->mapsz + (bufo->hva & (qemu_real_host_page_size - 1)),
            (void *)(bufo->hva & qemu_real_host_page_mask),
            (bufo->acc & GL_MAP_WRITE_BIT)? 0:1, add);
    }

    return ret;
}
#endif
#else
#define MESAGL_SDLGL 0
#endif

#if MESAGL_SDLGL
#define GL_GLEXT_LEGACY
#include "SDL2/SDL_opengl.h"
#include "SDL2/SDL.h"

typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef uint8_t BYTE;
typedef struct tagPIXELFORMATDESCRIPTOR {
  WORD  nSize;
  WORD  nVersion;
  DWORD dwFlags;
  BYTE  iPixelType;
  BYTE  cColorBits;
  BYTE  cRedBits;
  BYTE  cRedShift;
  BYTE  cGreenBits;
  BYTE  cGreenShift;
  BYTE  cBlueBits;
  BYTE  cBlueShift;
  BYTE  cAlphaBits;
  BYTE  cAlphaShift;
  BYTE  cAccumBits;
  BYTE  cAccumRedBits;
  BYTE  cAccumGreenBits;
  BYTE  cAccumBlueBits;
  BYTE  cAccumAlphaBits;
  BYTE  cDepthBits;
  BYTE  cStencilBits;
  BYTE  cAuxBuffers;
  BYTE  iLayerType;
  BYTE  bReserved;
  DWORD dwLayerMask;
  DWORD dwVisibleMask;
  DWORD dwDamageMask;
} PIXELFORMATDESCRIPTOR, *PPIXELFORMATDESCRIPTOR, *LPPIXELFORMATDESCRIPTOR;

#define WGL_NUMBER_PIXEL_FORMATS_ARB            0x2000
#define WGL_DRAW_TO_WINDOW_ARB                  0x2001
#define WGL_DRAW_TO_BITMAP_ARB                  0x2002
#define WGL_ACCELERATION_ARB                    0x2003
#define WGL_NEED_PALETTE_ARB                    0x2004
#define WGL_NEED_SYSTEM_PALETTE_ARB             0x2005
#define WGL_SWAP_LAYER_BUFFERS_ARB              0x2006
#define WGL_SWAP_METHOD_ARB                     0x2007
#define WGL_NUMBER_OVERLAYS_ARB                 0x2008
#define WGL_NUMBER_UNDERLAYS_ARB                0x2009
#define WGL_TRANSPARENT_ARB                     0x200A
#define WGL_TRANSPARENT_RED_VALUE_ARB           0x2037
#define WGL_TRANSPARENT_GREEN_VALUE_ARB         0x2038
#define WGL_TRANSPARENT_BLUE_VALUE_ARB          0x2039
#define WGL_TRANSPARENT_ALPHA_VALUE_ARB         0x203A
#define WGL_TRANSPARENT_INDEX_VALUE_ARB         0x203B
#define WGL_SHARE_DEPTH_ARB                     0x200C
#define WGL_SHARE_STENCIL_ARB                   0x200D
#define WGL_SHARE_ACCUM_ARB                     0x200E
#define WGL_SUPPORT_GDI_ARB                     0x200F
#define WGL_SUPPORT_OPENGL_ARB                  0x2010
#define WGL_DOUBLE_BUFFER_ARB                   0x2011
#define WGL_STEREO_ARB                          0x2012
#define WGL_PIXEL_TYPE_ARB                      0x2013
#define WGL_COLOR_BITS_ARB                      0x2014
#define WGL_RED_BITS_ARB                        0x2015
#define WGL_RED_SHIFT_ARB                       0x2016
#define WGL_GREEN_BITS_ARB                      0x2017
#define WGL_GREEN_SHIFT_ARB                     0x2018
#define WGL_BLUE_BITS_ARB                       0x2019
#define WGL_BLUE_SHIFT_ARB                      0x201A
#define WGL_ALPHA_BITS_ARB                      0x201B
#define WGL_ALPHA_SHIFT_ARB                     0x201C
#define WGL_ACCUM_BITS_ARB                      0x201D
#define WGL_ACCUM_RED_BITS_ARB                  0x201E
#define WGL_ACCUM_GREEN_BITS_ARB                0x201F
#define WGL_ACCUM_BLUE_BITS_ARB                 0x2020
#define WGL_ACCUM_ALPHA_BITS_ARB                0x2021
#define WGL_DEPTH_BITS_ARB                      0x2022
#define WGL_STENCIL_BITS_ARB                    0x2023
#define WGL_AUX_BUFFERS_ARB                     0x2024
#define WGL_NO_ACCELERATION_ARB                 0x2025
#define WGL_GENERIC_ACCELERATION_ARB            0x2026
#define WGL_FULL_ACCELERATION_ARB               0x2027
#define WGL_SWAP_EXCHANGE_ARB                   0x2028
#define WGL_SWAP_COPY_ARB                       0x2029
#define WGL_SWAP_UNDEFINED_ARB                  0x202A
#define WGL_TYPE_RGBA_ARB                       0x202B
#define WGL_TYPE_COLORINDEX_ARB                 0x202C
#define WGL_SAMPLE_BUFFERS_ARB                  0x2041
#define WGL_SAMPLES_ARB                         0x2042

typedef struct tagFakePBuffer {
    int width;
    int height;
} HPBUFFERARB;

static const PIXELFORMATDESCRIPTOR pfd = {
    .nSize = sizeof(PIXELFORMATDESCRIPTOR),
    .nVersion = 1,
    .dwFlags = 0x225,
    .cColorBits = 32,
    .cRedBits = 8, .cGreenBits = 8, .cBlueBits = 8, .cAlphaBits = 8,
    .cRedShift = 16, .cGreenShift = 8, .cBlueShift = 0, .cAlphaShift = 24,
    .cDepthBits = 24,
    .cStencilBits = 8,
    .cAuxBuffers = 0,
};
static const int iAttribs[] = {
    WGL_NUMBER_PIXEL_FORMATS_ARB, 1,
    WGL_DRAW_TO_WINDOW_ARB, 1,
    WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
    WGL_SWAP_METHOD_ARB, WGL_SWAP_EXCHANGE_ARB,
    WGL_SUPPORT_OPENGL_ARB, 1,
    WGL_DOUBLE_BUFFER_ARB, 1,
    WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
    WGL_COLOR_BITS_ARB, 32,
    WGL_RED_BITS_ARB, 8,
    WGL_RED_SHIFT_ARB, 16,
    WGL_GREEN_BITS_ARB, 8,
    WGL_GREEN_SHIFT_ARB, 8,
    WGL_BLUE_BITS_ARB, 8,
    WGL_BLUE_SHIFT_ARB, 0,
    WGL_ALPHA_BITS_ARB, 8,
    WGL_ALPHA_SHIFT_ARB, 24,
    WGL_DEPTH_BITS_ARB, 24,
    WGL_STENCIL_BITS_ARB, 8,
    WGL_AUX_BUFFERS_ARB, 0,
    WGL_SAMPLE_BUFFERS_ARB, 0,
    WGL_SAMPLES_ARB, 0,
    0,0
};

static SDL_Window *window;
static SDL_GLContext ctx[MAX_LVLCNTX];

static HPBUFFERARB hPbuffer[MAX_PBUFFER];
static int cDepthBits, cStencilBits, cAuxBuffers;
static int cSampleBuf[2];

static int find_xstr(const char *xstr, const char *str)
{
    int xlen, ret = 0;
    char *xbuf, *stok;
    if (xstr) {
        xlen = strnlen(xstr, 3*PAGE_SIZE);
        xbuf = g_new(char, xlen + 1);
        strncpy(xbuf, xstr, xlen + 1);
        stok = strtok(xbuf, " ");
        while (stok) {
            if (!strncmp(stok, str, strnlen(str, 64))) {
                ret = 1;
                break;
            }
            stok = strtok(NULL, " ");
        }
        g_free(xbuf);
    }
    return ret;
}

int MGLExtIsAvail(const char *xstr, const char *str)
{ return find_xstr(xstr, str); }

struct GammaRamp {
    uint16_t r[256];
    uint16_t g[256];
    uint16_t b[256];
};

static void MesaInitGammaRamp(void)
{
    struct GammaRamp ramp;

    for (int i = 0; i < 256; i++) {
        ramp.r[i] = (uint16_t)(((i << 8) | i) & 0xFFFFU);
        ramp.g[i] = (uint16_t)(((i << 8) | i) & 0xFFFFU);
        ramp.b[i] = (uint16_t)(((i << 8) | i) & 0xFFFFU);
    }
    SDL_SetWindowGammaRamp(window, ramp.r, ramp.g, ramp.b);
}

static void cwnd_mesagl(void *swnd, void *nwnd, void *opaque)
{
    window = (SDL_Window *)swnd;
    ctx[0] = SDL_GL_GetCurrentContext();
    if (ctx[0]) {
        SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &cDepthBits);
        SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &cStencilBits);
        SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &cSampleBuf[0]);
        SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &cSampleBuf[1]);
        DPRINTF("MESAGL window [SDL2 %p] ready", swnd);
    }
}

void SetMesaFuncPtr(void *p) { }

void *MesaGLGetProc(const char *proc)
{
    return SDL_GL_GetProcAddress(proc);
}

void MGLTmpContext(void)
{
}

void MGLDeleteContext(int level)
{
    int n = (level >= MAX_LVLCNTX)? (MAX_LVLCNTX - 1):level;
    SDL_GL_MakeCurrent(window, NULL);
    if (n == 0) {
        for (int i = MAX_LVLCNTX; i > 1;) {
            if (ctx[--i]) {
                SDL_GL_DeleteContext(ctx[i]);
                ctx[i] = 0;
            }
        }
    }
    if (!n)
        MGLActivateHandler(0);
}

void MGLWndRelease(void)
{
    if (window) {
        MesaInitGammaRamp();
        mesa_release_window();
        window = 0;
    }
}

int MGLCreateContext(uint32_t gDC)
{
    int i, ret;
    i = gDC & (MAX_PBUFFER - 1);
    if (gDC == ((MESAGL_HPBDC & 0xFFFFFFF0U) | i)) {
        ret = 0;
    }
    else {
        SDL_GL_MakeCurrent(window, NULL);
        for (i = MAX_LVLCNTX; i > 1;) {
            if (ctx[--i]) {
                SDL_GL_DeleteContext(ctx[i]);
                ctx[i] = 0;
            }
        }
        ret = (ctx[0])? 0:1;
    }
    return ret;
}

int MGLMakeCurrent(uint32_t cntxRC, int level)
{
    uint32_t i = cntxRC & (MAX_PBUFFER - 1), n = (level >= MAX_LVLCNTX)? (MAX_LVLCNTX - 1):level;
    if (cntxRC == (MESAGL_MAGIC - n)) {
        SDL_GL_MakeCurrent(window, ctx[n]);
        InitMesaGLExt();
        wrContextSRGB(ContextUseSRGB());
        if (ContextVsyncOff()) {
            const int val = 0;
            SDL_GL_SetSwapInterval(val);
        }
        if (!n)
            MGLActivateHandler(1);
    }
    if (cntxRC == (((MESAGL_MAGIC & 0xFFFFFFFU) << 4) | i))
    { /* Pbuffer unsupported */ }

    return 0;
}

int MGLSwapBuffers(void)
{
    MGLActivateHandler(1);
    SDL_GL_SwapWindow(window);
    return 1;
}

static int MGLPresetPixelFormat(void)
{
    mesa_prepare_window(&cwnd_mesagl);

    ImplMesaGLReset();
    MesaInitGammaRamp();
    return 1;
}

int MGLChoosePixelFormat(void)
{
    DPRINTF("ChoosePixelFormat()");
    if (!window)
        MGLPresetPixelFormat();
    return 1;
}

int MGLSetPixelFormat(int fmt, const void *p)
{
    DPRINTF("SetPixelFormat()");
    if (!window)
        MGLPresetPixelFormat();
    else {
        int cColors[4];
        SDL_GL_MakeCurrent(window, ctx[0]);
        SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &cColors[0]);
        SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &cColors[1]);
        SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &cColors[2]);
        SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &cColors[3]);
        glGetIntegerv(GL_AUX_BUFFERS, &cAuxBuffers);
        DPRINTF("%s OpenGL %s", glGetString(GL_RENDERER), glGetString(GL_VERSION));
        DPRINTF("Pixel Format ABGR%d%d%d%d D%2dS%d nAux %d nSamples %d %d",
                cColors[0], cColors[1], cColors[2], cColors[3], cDepthBits, cStencilBits,
                cAuxBuffers, cSampleBuf[0], cSampleBuf[1]);
    }
    return 1;
}

int MGLDescribePixelFormat(int fmt, unsigned int sz, void *p)
{
    //DPRINTF("DescribePixelFormat()");
    if (!window)
        MGLPresetPixelFormat();
    memcpy(p, &pfd, sizeof(PIXELFORMATDESCRIPTOR));
    ((PIXELFORMATDESCRIPTOR *)p)->cDepthBits = cDepthBits;
    ((PIXELFORMATDESCRIPTOR *)p)->cStencilBits = cStencilBits;
    ((PIXELFORMATDESCRIPTOR *)p)->cAuxBuffers = cAuxBuffers;
    return 1;
}

void MGLActivateHandler(int i)
{
    static int last = 0;

#define WA_ACTIVE 1
#define WA_INACTIVE 0
    if (i != last) {
        last = i;
        DPRINTF_COND(GLFuncTrace(), "wm_activate %-32d", i);
        mesa_renderer_stat(i);
    }
}

int NumPbuffer(void)
{
    int i, c;
    for (i = 0, c = 0; i < MAX_PBUFFER;)
        if (hPbuffer[i++].width) c++;
    return c;
}

static int LookupAttribArray(const int *attrib, const int attr)
{
    int ret = 0;
    for (int i = 0; attrib[i]; i+=2) {
        if (attrib[i] == attr) {
            switch (attr) {
                case WGL_DEPTH_BITS_ARB:
                    ret = cDepthBits;
                    break;
                case WGL_STENCIL_BITS_ARB:
                    ret = cStencilBits;
                    break;
                case WGL_AUX_BUFFERS_ARB:
                    ret = cAuxBuffers;
                    break;
                case WGL_SAMPLE_BUFFERS_ARB:
                    ret = cSampleBuf[0];
                    break;
                case WGL_SAMPLES_ARB:
                    ret = cSampleBuf[1];
                    break;
                default:
                    ret = attrib[i+1];
                    break;
            }
            break;
        }
    }
    return ret;
}

void MGLFuncHandler(const char *name)
{
    char fname[64];
    uint32_t *argsp = (uint32_t *)(name + ALIGNED(strnlen(name, sizeof(fname))));
    strncpy(fname, name, sizeof(fname)-1);

#define FUNCP_HANDLER(a) \
    if (!memcmp(fname, a, sizeof(a)))

    FUNCP_HANDLER("wglShareLists") {
        uint32_t i, ret = 0;
        i = argsp[1] & (MAX_PBUFFER - 1);
        if ((argsp[0] == MESAGL_MAGIC) && (argsp[1] == ((MESAGL_MAGIC & 0xFFFFFFFU) << 4 | i)))
            ret = 1;
        else {
            DPRINTF("  *WARN* ShareLists called with unknown contexts, %x %x", argsp[0], argsp[1]);
        }
        argsp[0] = ret;
        return;
    }
    FUNCP_HANDLER("wglUseFontBitmapsA") {
        DPRINTF("wglUseFontBitmapA() %x %x %d", argsp[1], argsp[2], argsp[3]);
        argsp[0] = 0;
        return;
    }
    FUNCP_HANDLER("wglSwapIntervalEXT") {
        int val = SDL_GL_SetSwapInterval(argsp[0]);
        if (val != -1) {
            DPRINTF("wglSwapIntervalEXT(%u) %s %-24u", argsp[0], ((val)? "err":"ret"), ((val)? val:1));
            argsp[0] = (val)? 0:1;
            return;
        }
    }
    FUNCP_HANDLER("wglGetSwapIntervalEXT") {
        int val = SDL_GL_GetSwapInterval();
        if (val != -1) {
            argsp[0] = val;
            DPRINTF("wglGetSwapIntervalEXT() ret %-24u", argsp[0]);
            return;
        }
    }
    FUNCP_HANDLER("wglGetExtensionsStringARB") {
        if (1 /* wglFuncs.GetExtensionsStringARB */) {
            //const char *str = wglFuncs.GetExtensionsStringARB(hDC);
            const char wglext[] = "WGL_3DFX_gamma_control "
                "WGL_ARB_create_context "
                "WGL_ARB_extensions_string "
                "WGL_ARB_pixel_format "
                "WGL_EXT_extensions_string "
                "WGL_EXT_swap_control "
                ;
            memcpy((char *)name, wglext, sizeof(wglext));
            *((char *)name + sizeof(wglext) - 2) = '\0';
            //DPRINTF("WGL extensions\nHost: %s [ %d ]\nGuest: %s [ %d ]", str, (uint32_t)strlen(str), name, (uint32_t)strlen(name));
            return;
        }
    }
    FUNCP_HANDLER("wglCreateContextAttribsARB") {
        do {
            uint32_t i, ret;
            for (i = 0; ((i < MAX_LVLCNTX) && ctx[i]); i++);
            argsp[1] = (argsp[0])? i:0;
            if (argsp[1] == 0) {
                ret = (ctx[0])? 1:0;
            }
            else {
                if (i == MAX_LVLCNTX) {
                    for (i = 1; i < (MAX_LVLCNTX - 1); i++)
                        ctx[i] = ctx[i + 1];
                    argsp[1] = i;
                }
                ret = (ctx[0])? 1:0;
            }
            argsp[0] = ret;
            return;
        } while(0);
    }
    FUNCP_HANDLER("wglGetPixelFormatAttribfvARB") {
        const int *ia = (const int *)&argsp[4], n = argsp[2];
        float pf[64];
        for (int i = 0; i < n; i++)
            pf[i] = (float)LookupAttribArray(iAttribs, ia[i]);
        memcpy(&argsp[2], pf, n*sizeof(float));
        argsp[0] = 1;
        return;
    }
    FUNCP_HANDLER("wglGetPixelFormatAttribivARB") {
        const int *ia = (const int *)&argsp[4], n = argsp[2];
        int pi[64];
        for (int i = 0; i < n; i++)
            pi[i] = LookupAttribArray(iAttribs, ia[i]);
        memcpy(&argsp[2], pi, n*sizeof(int));
        argsp[0] = 1;
        return;
    }
    FUNCP_HANDLER("wglChoosePixelFormatARB") {
#define WGL_DRAW_TO_PBUFFER_ARB 0x202D
        const int *ia = (const int *)argsp;
        if (LookupAttribArray(ia, WGL_DRAW_TO_PBUFFER_ARB)) {
            argsp[1] = 0x02;
        }
        else {
            DPRINTF("wglChoosePixelFormatARB()");
            argsp[1] = MGLChoosePixelFormat();
        }
        argsp[0] = 1;
        return;
    }
    FUNCP_HANDLER("wglBindTexImageARB") {
        argsp[0] = 1;
        return;
    }
    FUNCP_HANDLER("wglReleaseTexImageARB") {
        argsp[0] = 1;
        return;
    }
    FUNCP_HANDLER("wglCreatePbufferARB") {
        DPRINTF("Unsupported wglCreatePbufferARB");
        argsp[0] = 0;
        return;
    }
    FUNCP_HANDLER("wglDestroyPbufferARB") {
        DPRINTF("Unsupported wglDestroyPbufferARB");
        argsp[0] = 0;
        return;
    }
    FUNCP_HANDLER("wglQueryPbufferARB") {
        DPRINTF("Unsupported wglQueryPbufferARB");
        argsp[0] = 0;
        return;
    }
    FUNCP_HANDLER("wglGetDeviceGammaRamp3DFX") {
        uint32_t ret;
        struct GammaRamp *pRamp = (struct GammaRamp *)&argsp[2];
        ret = (SDL_GetWindowGammaRamp(window, pRamp->r, pRamp->g, pRamp->b))? 0:1;
        argsp[0] = ret;
        return;
    }
    FUNCP_HANDLER("wglSetDeviceGammaRamp3DFX") {
        uint32_t ret;
        struct GammaRamp *pRamp = (struct GammaRamp *)&argsp[0];
        ret = (SDL_SetWindowGammaRamp(window, pRamp->r, pRamp->g, pRamp->b))? 0:1;
        argsp[0] = ret;
        return;
    }

    DPRINTF("  *WARN* Unhandled GLFunc %s", name);
    argsp[0] = 0;
}

#endif //MESAGL_SDLGL
