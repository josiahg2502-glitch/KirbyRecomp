#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int x, y, w, h;
} SDL_Rect;

typedef void* SDL_Renderer;
typedef void* SDL_Texture;

typedef int (*pfn_dynapi_entry)(uint32_t, void*, uint32_t);
typedef int (*pfn_SDL_GetRendererOutputSize)(SDL_Renderer* renderer, int *w, int *h);
typedef SDL_Texture* (*pfn_SDL_CreateTexture)(SDL_Renderer* renderer, uint32_t format, int access, int w, int h);
typedef int (*pfn_SDL_UpdateTexture)(SDL_Texture* texture, const SDL_Rect* rect, const void* pixels, int pitch);
typedef int (*pfn_SDL_RenderCopy)(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_Rect* srcrect, const SDL_Rect* dstrect);
typedef void (*pfn_SDL_RenderPresent)(SDL_Renderer* renderer);
typedef void (*pfn_SDL_DestroyTexture)(SDL_Texture* texture);
typedef int (*pfn_SDL_RenderSetViewport)(SDL_Renderer* renderer, const SDL_Rect* rect);
typedef int (*pfn_SDL_RenderSetClipRect)(SDL_Renderer* renderer, const SDL_Rect* rect);
typedef int (*pfn_SDL_RenderReadPixels)(SDL_Renderer* renderer, const SDL_Rect* rect, uint32_t format, void* pixels, int pitch);
typedef int (*pfn_SDL_SetTextureBlendMode)(SDL_Texture* texture, int blendMode);

typedef struct {
    uint32_t scancode;
    int32_t sym;
    uint16_t mod;
    uint32_t unused;
} SDL_Keysym_t;

typedef struct {
    uint32_t type;
    uint32_t timestamp;
    uint32_t windowID;
    uint8_t state;
    uint8_t repeat;
    uint8_t padding2;
    uint8_t padding3;
    SDL_Keysym_t keysym;
} SDL_KeyboardEvent_t;

typedef union {
    uint32_t type;
    SDL_KeyboardEvent_t key;
    uint8_t padding[56];
} SDL_Event_t;

typedef int (*pfn_SDL_PollEvent)(void* event);
typedef int (*pfn_SDL_WaitEvent)(void* event);
typedef int (*pfn_SDL_WaitEventTimeout)(void* event, int timeout);

static pfn_SDL_RenderPresent g_real_RenderPresent = NULL;
static pfn_SDL_RenderCopy g_real_RenderCopy = NULL;
static pfn_SDL_CreateTexture g_real_CreateTexture = NULL;
static pfn_SDL_UpdateTexture g_real_UpdateTexture = NULL;
static pfn_SDL_GetRendererOutputSize g_real_GetRendererOutputSize = NULL;
static pfn_SDL_DestroyTexture g_real_DestroyTexture = NULL;
static pfn_SDL_RenderSetViewport g_real_RenderSetViewport = NULL;
static pfn_SDL_RenderSetClipRect g_real_RenderSetClipRect = NULL;
static pfn_SDL_RenderReadPixels g_real_RenderReadPixels = NULL;
static pfn_SDL_SetTextureBlendMode g_real_SetTextureBlendMode = NULL;
static pfn_SDL_PollEvent g_real_PollEvent = NULL;
static pfn_SDL_WaitEvent g_real_WaitEvent = NULL;
static pfn_SDL_WaitEventTimeout g_real_WaitEventTimeout = NULL;

static SDL_Texture* g_tex_bezel = NULL;
static SDL_Renderer* g_last_renderer = NULL;
static uint8_t* g_bezel_pixels = NULL;

static int g_bezel_enabled = 1;
static int g_bezel_config_loaded = 0;

static void load_bezel_config(void) {
    if (g_bezel_config_loaded) return;
    g_bezel_config_loaded = 1;
    char exe_dir[MAX_PATH];
    exe_dir[0] = '\0';
    GetModuleFileNameA(NULL, exe_dir, MAX_PATH);
    char* p = strrchr(exe_dir, '\\');
    if (p) *p = '\0';
    
    char ini_path[MAX_PATH];
    snprintf(ini_path, MAX_PATH, "%s\\config.ini", exe_dir);
    int val = GetPrivateProfileIntA("Launcher", "bezel", 1, ini_path);
    g_bezel_enabled = (val != 0);
}

static void save_bezel_config(void) {
    char exe_dir[MAX_PATH];
    exe_dir[0] = '\0';
    GetModuleFileNameA(NULL, exe_dir, MAX_PATH);
    char* p = strrchr(exe_dir, '\\');
    if (p) *p = '\0';
    
    char ini_path[MAX_PATH];
    snprintf(ini_path, MAX_PATH, "%s\\config.ini", exe_dir);
    WritePrivateProfileStringA("Launcher", "bezel", g_bezel_enabled ? "1" : "0", ini_path);
}

// 8x8 bitmap font (ASCII 32 to 126) for on-screen HUD
static const uint8_t g_font8x8[95][8] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* ' ' */
    {0x10, 0x10, 0x10, 0x00, 0x10, 0x00, 0x00, 0x00}, /* '!' */
    {0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* '"' */
    {0x10, 0x78, 0x10, 0x78, 0x20, 0x00, 0x00, 0x00}, /* '#' */
    {0x38, 0x50, 0x30, 0x28, 0x78, 0x20, 0x00, 0x00}, /* '$' */
    {0x78, 0x10, 0x20, 0x3c, 0x5c, 0x00, 0x00, 0x00}, /* '%' */
    {0x50, 0x60, 0x68, 0x50, 0x78, 0x00, 0x00, 0x00}, /* '&' */
    {0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* ''' */
    {0x10, 0x20, 0x20, 0x20, 0x20, 0x10, 0x08, 0x00}, /* '(' */
    {0x10, 0x08, 0x08, 0x08, 0x08, 0x10, 0x20, 0x00}, /* ')' */
    {0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* '*' */
    {0x10, 0x10, 0x7c, 0x10, 0x10, 0x00, 0x00, 0x00}, /* '+' */
    {0x00, 0x00, 0x00, 0x00, 0x10, 0x30, 0x00, 0x00}, /* ',' */
    {0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00}, /* '-' */
    {0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00}, /* '.' */
    {0x10, 0x10, 0x20, 0x20, 0x40, 0x40, 0x00, 0x00}, /* '/' */
    {0x30, 0x58, 0x78, 0x68, 0x30, 0x00, 0x00, 0x00}, /* '0' */
    {0x30, 0x10, 0x10, 0x10, 0x38, 0x00, 0x00, 0x00}, /* '1' */
    {0x38, 0x28, 0x08, 0x10, 0x38, 0x00, 0x00, 0x00}, /* '2' */
    {0x38, 0x08, 0x18, 0x08, 0x38, 0x00, 0x00, 0x00}, /* '3' */
    {0x30, 0x30, 0x50, 0x78, 0x10, 0x00, 0x00, 0x00}, /* '4' */
    {0x38, 0x20, 0x38, 0x08, 0x30, 0x00, 0x00, 0x00}, /* '5' */
    {0x38, 0x40, 0x78, 0x48, 0x30, 0x00, 0x00, 0x00}, /* '6' */
    {0x70, 0x10, 0x20, 0x20, 0x40, 0x00, 0x00, 0x00}, /* '7' */
    {0x78, 0x48, 0x30, 0x48, 0x78, 0x00, 0x00, 0x00}, /* '8' */
    {0x30, 0x48, 0x78, 0x08, 0x70, 0x00, 0x00, 0x00}, /* '9' */
    {0x00, 0x10, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00}, /* ':' */
    {0x00, 0x10, 0x00, 0x00, 0x10, 0x30, 0x00, 0x00}, /* ';' */
    {0x00, 0x08, 0x10, 0x30, 0x08, 0x00, 0x00, 0x00}, /* '<' */
    {0x00, 0x38, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00}, /* '=' */
    {0x00, 0x20, 0x10, 0x18, 0x20, 0x00, 0x00, 0x00}, /* '>' */
    {0x08, 0x08, 0x38, 0x00, 0x20, 0x00, 0x00, 0x00}, /* '?' */
    {0x48, 0xb8, 0xb8, 0xb8, 0xa8, 0x80, 0x70, 0x00}, /* '@' */
    {0x10, 0x28, 0x28, 0x38, 0x48, 0x00, 0x00, 0x00}, /* 'A' */
    {0x3c, 0x24, 0x38, 0x24, 0x3c, 0x00, 0x00, 0x00}, /* 'B' */
    {0x30, 0x40, 0x40, 0x40, 0x30, 0x00, 0x00, 0x00}, /* 'C' */
    {0x70, 0x48, 0x48, 0x48, 0x70, 0x00, 0x00, 0x00}, /* 'D' */
    {0x38, 0x20, 0x38, 0x20, 0x38, 0x00, 0x00, 0x00}, /* 'E' */
    {0x38, 0x20, 0x38, 0x20, 0x20, 0x00, 0x00, 0x00}, /* 'F' */
    {0x38, 0x40, 0x58, 0x48, 0x38, 0x00, 0x00, 0x00}, /* 'G' */
    {0x48, 0x48, 0x78, 0x48, 0x48, 0x00, 0x00, 0x00}, /* 'H' */
    {0x38, 0x10, 0x10, 0x10, 0x38, 0x00, 0x00, 0x00}, /* 'I' */
    {0x38, 0x08, 0x08, 0x08, 0x38, 0x00, 0x00, 0x00}, /* 'J' */
    {0x28, 0x30, 0x30, 0x30, 0x28, 0x00, 0x00, 0x00}, /* 'K' */
    {0x20, 0x20, 0x20, 0x20, 0x38, 0x00, 0x00, 0x00}, /* 'L' */
    {0x30, 0x30, 0x68, 0x48, 0x48, 0x00, 0x00, 0x00}, /* 'M' */
    {0x48, 0x68, 0x68, 0x58, 0x48, 0x00, 0x00, 0x00}, /* 'N' */
    {0x30, 0x48, 0x48, 0x48, 0x30, 0x00, 0x00, 0x00}, /* 'O' */
    {0x3c, 0x24, 0x38, 0x20, 0x20, 0x00, 0x00, 0x00}, /* 'P' */
    {0x30, 0x48, 0x48, 0x48, 0x30, 0x18, 0x00, 0x00}, /* 'Q' */
    {0x38, 0x28, 0x30, 0x28, 0x28, 0x00, 0x00, 0x00}, /* 'R' */
    {0x3c, 0x20, 0x18, 0x04, 0x3c, 0x00, 0x00, 0x00}, /* 'S' */
    {0x7c, 0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00}, /* 'T' */
    {0x48, 0x48, 0x48, 0x48, 0x70, 0x00, 0x00, 0x00}, /* 'U' */
    {0x28, 0x28, 0x28, 0x28, 0x10, 0x00, 0x00, 0x00}, /* 'V' */
    {0x48, 0x48, 0x68, 0x78, 0x48, 0x00, 0x00, 0x00}, /* 'W' */
    {0x48, 0x30, 0x20, 0x30, 0x48, 0x00, 0x00, 0x00}, /* 'X' */
    {0x44, 0x28, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00}, /* 'Y' */
    {0x70, 0x10, 0x20, 0x40, 0x78, 0x00, 0x00, 0x00}, /* 'Z' */
    {0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x30, 0x00}, /* '[' */
    {0x20, 0x10, 0x10, 0x08, 0x08, 0x04, 0x00, 0x00}, /* '\' */
    {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x30, 0x00}, /* ']' */
    {0x20, 0x50, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00}, /* '^' */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x00}, /* '_' */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* '`' */
    {0x00, 0x3c, 0x3c, 0x24, 0x3c, 0x00, 0x00, 0x00}, /* 'a' */
    {0x20, 0x3c, 0x24, 0x24, 0x38, 0x00, 0x00, 0x00}, /* 'b' */
    {0x00, 0x18, 0x20, 0x20, 0x38, 0x00, 0x00, 0x00}, /* 'c' */
    {0x08, 0x38, 0x48, 0x48, 0x78, 0x00, 0x00, 0x00}, /* 'd' */
    {0x00, 0x38, 0x78, 0x40, 0x38, 0x00, 0x00, 0x00}, /* 'e' */
    {0x20, 0x70, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00}, /* 'f' */
    {0x00, 0x78, 0x48, 0x78, 0x78, 0x48, 0x70, 0x00}, /* 'g' */
    {0x20, 0x38, 0x28, 0x28, 0x28, 0x00, 0x00, 0x00}, /* 'h' */
    {0x00, 0x30, 0x10, 0x10, 0x38, 0x00, 0x00, 0x00}, /* 'i' */
    {0x00, 0x38, 0x08, 0x08, 0x08, 0x08, 0x38, 0x00}, /* 'j' */
    {0x20, 0x28, 0x30, 0x28, 0x24, 0x00, 0x00, 0x00}, /* 'k' */
    {0x10, 0x10, 0x10, 0x10, 0x38, 0x00, 0x00, 0x00}, /* 'l' */
    {0x00, 0x7c, 0x54, 0x54, 0x54, 0x00, 0x00, 0x00}, /* 'm' */
    {0x00, 0x38, 0x28, 0x28, 0x28, 0x00, 0x00, 0x00}, /* 'n' */
    {0x00, 0x30, 0x48, 0x48, 0x30, 0x00, 0x00, 0x00}, /* 'o' */
    {0x00, 0x38, 0x28, 0x28, 0x38, 0x20, 0x20, 0x00}, /* 'p' */
    {0x00, 0x38, 0x48, 0x48, 0x78, 0x08, 0x08, 0x00}, /* 'q' */
    {0x00, 0x3c, 0x24, 0x20, 0x20, 0x00, 0x00, 0x00}, /* 'r' */
    {0x00, 0x38, 0x20, 0x18, 0x38, 0x00, 0x00, 0x00}, /* 's' */
    {0x20, 0x78, 0x20, 0x20, 0x38, 0x00, 0x00, 0x00}, /* 't' */
    {0x00, 0x28, 0x28, 0x28, 0x38, 0x00, 0x00, 0x00}, /* 'u' */
    {0x00, 0x28, 0x28, 0x28, 0x10, 0x00, 0x00, 0x00}, /* 'v' */
    {0x00, 0x88, 0xb0, 0x70, 0x50, 0x00, 0x00, 0x00}, /* 'w' */
    {0x00, 0x48, 0x30, 0x30, 0x48, 0x00, 0x00, 0x00}, /* 'x' */
    {0x00, 0x28, 0x28, 0x28, 0x10, 0x10, 0x60, 0x00}, /* 'y' */
    {0x00, 0x38, 0x10, 0x10, 0x38, 0x00, 0x00, 0x00}, /* 'z' */
    {0x10, 0x10, 0x20, 0x10, 0x10, 0x10, 0x18, 0x00}, /* '{' */
    {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00}, /* '|' */
    {0x10, 0x10, 0x08, 0x10, 0x10, 0x10, 0x30, 0x00}, /* '}' */
    {0x00, 0x00, 0xd0, 0xb0, 0x00, 0x00, 0x00, 0x00}, /* '~' */
};

#define TOAST_W 720
#define TOAST_H 44
static uint32_t g_toast_pixels[TOAST_W * TOAST_H];
static SDL_Texture* g_tex_toast = NULL;
static DWORD g_toast_expiry = 0;
static char g_toast_msg[128] = {0};

static void draw_toast_text(const char* text) {
    for (int y = 0; y < TOAST_H; y++) {
        for (int x = 0; x < TOAST_W; x++) {
            if (y == 0 || y == TOAST_H - 1 || x == 0 || x == TOAST_W - 1 ||
                y == 1 || y == TOAST_H - 2 || x == 1 || x == TOAST_W - 2) {
                g_toast_pixels[y * TOAST_W + x] = 0xFFF0A000; // Amber Gold Border
            } else {
                g_toast_pixels[y * TOAST_W + x] = 0xEE1E1E1E; // Dark charcoal pill
            }
        }
    }

    int len = (int)strlen(text);
    int start_x = (TOAST_W - (len * 12)) / 2;
    if (start_x < 10) start_x = 10;
    int start_y = (TOAST_H - 16) / 2;

    for (int i = 0; i < len; i++) {
        char c = text[i];
        if (c < 32 || c > 126) c = ' ';
        int glyph_idx = c - 32;
        int cx = start_x + (i * 12);
        for (int gy = 0; gy < 8; gy++) {
            uint8_t row = g_font8x8[glyph_idx][gy];
            for (int gx = 0; gx < 8; gx++) {
                if (row & (1 << (7 - gx))) {
                    for (int dy = 0; dy < 2; dy++) {
                        for (int dx = 0; dx < 2; dx++) {
                            int px = cx + (gx + dx);
                            int py = start_y + (gy * 2 + dy);
                            if (px >= 0 && px < TOAST_W && py >= 0 && py < TOAST_H) {
                                g_toast_pixels[py * TOAST_W + px] = 0xFFFFFFFF; // Clean White Text
                            }
                        }
                    }
                }
            }
        }
    }
}

static int g_gamepad_config_loaded = 0;
static uint8_t g_btn_map[15] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
static int g_trigger_l_target = 9;   // Default: LB (button 9) -> Power Drop / L
static int g_trigger_r_target = 10;  // Default: RB (button 10) -> Power Drop / R
static int g_lt_active = 0;
static int g_rt_active = 0;
static int g_current_preset = 0;

static void apply_preset(int preset) {
    g_current_preset = (preset % 4 + 4) % 4;
    for (int i = 0; i < 15; i++) g_btn_map[i] = (uint8_t)i;

    switch (g_current_preset) {
        case 0: // Xbox / PC Standard
            g_btn_map[2] = 1; // X -> Inhale / Attack (B)
            g_btn_map[3] = 0; // Y -> Jump / Fly (A)
            g_trigger_l_target = 9;  // LT -> L
            g_trigger_r_target = 10; // RT -> R
            snprintf(g_toast_msg, sizeof(g_toast_msg), "PRESET: Xbox Standard (A=Jump, B/X=Inhale, LT=L, RT=R)");
            break;
        case 1: // Nintendo Switch
            g_btn_map[0] = 1; // A -> Inhale (B)
            g_btn_map[1] = 0; // B -> Jump (A)
            g_btn_map[2] = 0; // X -> Jump (A)
            g_btn_map[3] = 1; // Y -> Inhale (B)
            g_trigger_l_target = 9;
            g_trigger_r_target = 10;
            snprintf(g_toast_msg, sizeof(g_toast_msg), "PRESET: Nintendo Switch (B=Jump, A/Y=Inhale)");
            break;
        case 2: // Modern Trigger Action
            g_btn_map[2] = 1; // X -> Inhale
            g_btn_map[3] = 0; // Y -> Jump
            g_trigger_l_target = 9;
            g_trigger_r_target = 1;  // RT -> Inhale / Attack (B)!
            snprintf(g_toast_msg, sizeof(g_toast_msg), "PRESET: Trigger Action (A=Jump, B=Inhale, RT=Attack!)");
            break;
        case 3: // Classic 1:1 Hardware
            g_trigger_l_target = 9;
            g_trigger_r_target = 10;
            snprintf(g_toast_msg, sizeof(g_toast_msg), "PRESET: Classic 1:1 Hardware (A=A, B=B, X=X, Y=Y)");
            break;
    }
    g_toast_expiry = GetTickCount() + 3500;
}

static void load_gamepad_config(void) {
    if (g_gamepad_config_loaded) return;
    g_gamepad_config_loaded = 1;
    apply_preset(0);
}

static void load_bezel_pixels(void) {
    if (g_bezel_pixels) return;
    
    char exe_dir[MAX_PATH];
    exe_dir[0] = '\0';
    GetModuleFileNameA(NULL, exe_dir, MAX_PATH);
    char* p = strrchr(exe_dir, '\\');
    if (p) *p = '\0';
    
    const char* candidate_dirs[] = {
        exe_dir,
        "C:\\Users\\josia\\Desktop\\Recomps\\KirbyRecomp",
        "assets\\img",
        "."
    };
    
    char path[MAX_PATH];
    for (size_t i = 0; i < sizeof(candidate_dirs)/sizeof(candidate_dirs[0]); ++i) {
        snprintf(path, MAX_PATH, "%s\\assets\\img\\bezel_master_1080p.raw", candidate_dirs[i]);
        FILE* f = fopen(path, "rb");
        if (!f) {
            snprintf(path, MAX_PATH, "%s\\bezel_master_1080p.raw", candidate_dirs[i]);
            f = fopen(path, "rb");
        }
        if (f) {
            g_bezel_pixels = (uint8_t*)malloc(8294400);
            if (g_bezel_pixels) {
                fread(g_bezel_pixels, 1, 8294400, f);
            }
            fclose(f);
            break;
        }
    }
}

static int Hooked_SDL_PollEvent(void* ev_ptr);
static int Hooked_SDL_WaitEvent(void* ev_ptr);
static int Hooked_SDL_WaitEventTimeout(void* ev_ptr, int timeout);

static int fetch_next_event(void* ev_ptr, int call_type, int timeout) {
    if (call_type == 1) return Hooked_SDL_WaitEvent(ev_ptr);
    if (call_type == 2) return Hooked_SDL_WaitEventTimeout(ev_ptr, timeout);
    return Hooked_SDL_PollEvent(ev_ptr);
}

static int handle_sdl_event_interception(void* ev_ptr, int call_type, int timeout) {
    if (!ev_ptr) return 0;
    load_gamepad_config();

    SDL_Event_t* event = (SDL_Event_t*)ev_ptr;

    // Gamepad button remapping: 0x651 = SDL_CONTROLLERBUTTONDOWN, 0x652 = SDL_CONTROLLERBUTTONUP
    if (event->type == 0x651 || event->type == 0x652) {
        uint8_t btn = *((uint8_t*)ev_ptr + 12);
        if (btn < 15) {
            uint8_t target = g_btn_map[btn];
            if (target == 255) {
                return fetch_next_event(ev_ptr, call_type, timeout);
            }
            *((uint8_t*)ev_ptr + 12) = target;
        }
    }
    // Gamepad analog trigger conversion: 0x650 = SDL_CONTROLLERAXISMOTION
    else if (event->type == 0x650) {
        uint8_t axis = *((uint8_t*)ev_ptr + 12);
        int16_t value = *(int16_t*)((uint8_t*)ev_ptr + 14);

        if (axis == 4 && g_trigger_l_target >= 0) { // Left Trigger (LT)
            if (value > 16000 && !g_lt_active) {
                g_lt_active = 1;
                *(uint32_t*)ev_ptr = 0x651; // SDL_CONTROLLERBUTTONDOWN
                *((uint8_t*)ev_ptr + 12) = (uint8_t)g_trigger_l_target;
                *((uint8_t*)ev_ptr + 13) = 1; // SDL_PRESSED
            } else if (value < 8000 && g_lt_active) {
                g_lt_active = 0;
                *(uint32_t*)ev_ptr = 0x652; // SDL_CONTROLLERBUTTONUP
                *((uint8_t*)ev_ptr + 12) = (uint8_t)g_trigger_l_target;
                *((uint8_t*)ev_ptr + 13) = 0; // SDL_RELEASED
            }
        }
        else if (axis == 5 && g_trigger_r_target >= 0) { // Right Trigger (RT)
            if (value > 16000 && !g_rt_active) {
                g_rt_active = 1;
                *(uint32_t*)ev_ptr = 0x651; // SDL_CONTROLLERBUTTONDOWN
                *((uint8_t*)ev_ptr + 12) = (uint8_t)g_trigger_r_target;
                *((uint8_t*)ev_ptr + 13) = 1; // SDL_PRESSED
            } else if (value < 8000 && g_rt_active) {
                g_rt_active = 0;
                *(uint32_t*)ev_ptr = 0x652; // SDL_CONTROLLERBUTTONUP
                *((uint8_t*)ev_ptr + 12) = (uint8_t)g_trigger_r_target;
                *((uint8_t*)ev_ptr + 13) = 0; // SDL_RELEASED
            }
        }
    }
    // Keyboard hotkeys: 0x300 = SDL_KEYDOWN, 0x301 = SDL_KEYUP
    else if (event->type == 0x300 || event->type == 0x301) {
        uint32_t sym = (uint32_t)event->key.keysym.sym;
        uint32_t scancode = event->key.keysym.scancode;

        // F9: Quick Controller Preset Switcher
        if (sym == 0x40000042 || scancode == 66) {
            if (event->type == 0x300 && !event->key.repeat) {
                apply_preset(g_current_preset + 1);
            }
            return fetch_next_event(ev_ptr, call_type, timeout);
        }

        // F10: Toggle decorative bezel border on/off (Clean Fullscreen mode)
        if (sym == 0x40000043 || scancode == 67) {
            if (event->type == 0x300 && event->key.repeat == 0) {
                load_bezel_config();
                g_bezel_enabled = !g_bezel_enabled;
                save_bezel_config();
                snprintf(g_toast_msg, sizeof(g_toast_msg), g_bezel_enabled ? "BEZEL: Custom Border Active (F10 to toggle)" : "BEZEL: Clean Fullscreen (No Borders)");
                g_toast_expiry = GetTickCount() + 3000;
            }
            return fetch_next_event(ev_ptr, call_type, timeout);
        }

        // F11: Open/toggle overlay menu (replaces Escape)
        if (sym == 0x40000044 || scancode == 68) {
            if (event->type == 0x300) {
                if (event->key.repeat != 0) {
                    return fetch_next_event(ev_ptr, call_type, timeout);
                }
            }
            event->key.keysym.sym = 27;      // SDLK_ESCAPE
            event->key.keysym.scancode = 41;  // SDL_SCANCODE_ESCAPE
            event->key.keysym.mod = 0;
        }

        // Esc: Instant clean exit
        else if (sym == 27 || scancode == 41) {
            if (event->type == 0x300) {
                event->type = 0x100; // SDL_QUIT
            } else {
                return fetch_next_event(ev_ptr, call_type, timeout);
            }
        }

        // F1: Save state (Slot 1)
        else if (sym == 0x4000003a || scancode == 58) {
            if (event->type == 0x300) {
                if (event->key.repeat != 0) {
                    return fetch_next_event(ev_ptr, call_type, timeout);
                }
                snprintf(g_toast_msg, sizeof(g_toast_msg), "STATE: Slot 1 Saved (Shift+F1)");
                g_toast_expiry = GetTickCount() + 2500;
            }
            event->key.keysym.sym = 0x4000003a; // SDLK_F1
            event->key.keysym.scancode = 58;    // SDL_SCANCODE_F1
            event->key.keysym.mod |= 1;         // KMOD_LSHIFT -> triggers Save State
        }

        // F2: Load state (Slot 1)
        else if (sym == 0x4000003b || scancode == 59) {
            if (event->type == 0x300) {
                if (event->key.repeat != 0) {
                    return fetch_next_event(ev_ptr, call_type, timeout);
                }
                snprintf(g_toast_msg, sizeof(g_toast_msg), "STATE: Slot 1 Loaded (F1)");
                g_toast_expiry = GetTickCount() + 2500;
            }
            event->key.keysym.sym = 0x4000003a; // SDLK_F1
            event->key.keysym.scancode = 58;    // SDL_SCANCODE_F1
            event->key.keysym.mod = 0;          // Clear Shift -> triggers Load State
        }
    }

    return 1;
}

static int Hooked_SDL_PollEvent(void* ev_ptr) {
    if (!g_real_PollEvent) return 0;
    int res = g_real_PollEvent(ev_ptr);
    if (!res || !ev_ptr) return res;
    return handle_sdl_event_interception(ev_ptr, 0, 0);
}

static int Hooked_SDL_WaitEvent(void* ev_ptr) {
    if (!g_real_WaitEvent) return 0;
    int res = g_real_WaitEvent(ev_ptr);
    if (!res || !ev_ptr) return res;
    return handle_sdl_event_interception(ev_ptr, 1, 0);
}

static int Hooked_SDL_WaitEventTimeout(void* ev_ptr, int timeout) {
    if (!g_real_WaitEventTimeout) return 0;
    int res = g_real_WaitEventTimeout(ev_ptr, timeout);
    if (!res || !ev_ptr) return res;
    return handle_sdl_event_interception(ev_ptr, 2, timeout);
}

static int Hooked_SDL_RenderCopy(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_Rect* srcrect, const SDL_Rect* dstrect) {
    load_bezel_config();
    if (texture != g_tex_bezel && texture != g_tex_toast) {
        int sw = 0, sh = 0;
        if (g_real_GetRendererOutputSize) g_real_GetRendererOutputSize(renderer, &sw, &sh);
        if (sw > 0 && sh > 0) {
            SDL_Rect game_rect;
            if (g_bezel_enabled) {
                // Cutout in 1920x1080 master: [311, 154, 1299, 772]
                game_rect.x = (sw * 311) / 1920;
                game_rect.y = (sh * 154) / 1080;
                game_rect.w = (sw * 1299) / 1920;
                game_rect.h = (sh * 772) / 1080;
            } else {
                // Clean Fullscreen: Proper 3:2 GBA aspect ratio with clean black sidebars (pillarbox)
                int target_w = (sh * 3) / 2;
                if (target_w <= sw) {
                    game_rect.w = target_w;
                    game_rect.h = sh;
                    game_rect.x = (sw - target_w) / 2;
                    game_rect.y = 0;
                } else {
                    game_rect.w = sw;
                    game_rect.h = (sw * 2) / 3;
                    game_rect.x = 0;
                    game_rect.y = (sh - game_rect.h) / 2;
                }
            }
            return g_real_RenderCopy(renderer, texture, srcrect, &game_rect);
        }
    }
    return g_real_RenderCopy(renderer, texture, srcrect, dstrect);
}

void Hooked_SDL_RenderPresent(SDL_Renderer* renderer) {
    load_bezel_config();
    if (renderer != g_last_renderer) {
        if (g_tex_bezel && g_real_DestroyTexture) g_real_DestroyTexture(g_tex_bezel);
        g_tex_bezel = NULL;
        if (g_tex_toast && g_real_DestroyTexture) g_real_DestroyTexture(g_tex_toast);
        g_tex_toast = NULL;
        g_last_renderer = renderer;
    }
    
    if (g_bezel_enabled) {
        if (!g_tex_bezel && g_real_CreateTexture && g_real_UpdateTexture) {
            load_bezel_pixels();
            if (g_bezel_pixels) {
                // SDL_PIXELFORMAT_ABGR8888 (0x16762004)
                g_tex_bezel = g_real_CreateTexture(renderer, 0x16762004, 0, 1920, 1080);
                if (g_tex_bezel) {
                    if (g_real_SetTextureBlendMode) {
                        g_real_SetTextureBlendMode(g_tex_bezel, 1);
                    }
                    g_real_UpdateTexture(g_tex_bezel, NULL, g_bezel_pixels, 1920 * 4);
                }
            }
        }
        
        if (g_tex_bezel && g_real_GetRendererOutputSize && g_real_RenderCopy) {
            int screen_w = 0, screen_h = 0;
            g_real_GetRendererOutputSize(renderer, &screen_w, &screen_h);
            
            if (screen_w > 0 && screen_h > 0) {
                if (g_real_RenderSetViewport) g_real_RenderSetViewport(renderer, NULL);
                if (g_real_RenderSetClipRect) g_real_RenderSetClipRect(renderer, NULL);
                
                SDL_Rect dst_full = {0, 0, screen_w, screen_h};
                g_real_RenderCopy(renderer, g_tex_bezel, NULL, &dst_full);
            }
        }
    }

    // On-screen Toast notification
    if (GetTickCount() < g_toast_expiry && g_real_GetRendererOutputSize && g_real_RenderCopy) {
        if (!g_tex_toast && g_real_CreateTexture) {
            g_tex_toast = g_real_CreateTexture(renderer, 0x16762004, 0, TOAST_W, TOAST_H);
            if (g_tex_toast && g_real_SetTextureBlendMode) {
                g_real_SetTextureBlendMode(g_tex_toast, 1);
            }
        }
        if (g_tex_toast && g_real_UpdateTexture) {
            int screen_w = 0, screen_h = 0;
            g_real_GetRendererOutputSize(renderer, &screen_w, &screen_h);
            if (screen_w > 0 && screen_h > 0) {
                draw_toast_text(g_toast_msg);
                g_real_UpdateTexture(g_tex_toast, NULL, g_toast_pixels, TOAST_W * 4);
                SDL_Rect dst_toast;
                dst_toast.x = (screen_w - TOAST_W) / 2;
                dst_toast.y = 25;
                dst_toast.w = TOAST_W;
                dst_toast.h = TOAST_H;
                if (g_real_RenderSetViewport) g_real_RenderSetViewport(renderer, NULL);
                if (g_real_RenderSetClipRect) g_real_RenderSetClipRect(renderer, NULL);
                g_real_RenderCopy(renderer, g_tex_toast, NULL, &dst_toast);
            }
        }
    }
    
    if (g_real_RenderPresent) {
        g_real_RenderPresent(renderer);
    }
}

__declspec(dllexport) int SDL_DYNAPI_entry(uint32_t apiver, void* table, uint32_t tablesize) {
    HMODULE hReal = GetModuleHandleA("SDL2.dll");
    if (!hReal) hReal = LoadLibraryA("SDL2.dll");
    if (!hReal) return -1;
    
    pfn_dynapi_entry real_entry = (pfn_dynapi_entry)GetProcAddress(hReal, "SDL_DYNAPI_entry");
    if (!real_entry) return -1;
    
    int result = real_entry(apiver, table, tablesize);
    if (result != 0) return result;
    
    void** ptable = (void**)table;
    g_real_RenderPresent = (pfn_SDL_RenderPresent)ptable[309];
    g_real_RenderCopy = (pfn_SDL_RenderCopy)ptable[306];
    g_real_CreateTexture = (pfn_SDL_CreateTexture)ptable[269];
    g_real_UpdateTexture = (pfn_SDL_UpdateTexture)ptable[278];
    g_real_GetRendererOutputSize = (pfn_SDL_GetRendererOutputSize)ptable[268];
    g_real_DestroyTexture = (pfn_SDL_DestroyTexture)ptable[310];
    g_real_RenderSetViewport = (pfn_SDL_RenderSetViewport)ptable[285];
    g_real_RenderSetClipRect = (pfn_SDL_RenderSetClipRect)ptable[287];
    g_real_RenderReadPixels = (pfn_SDL_RenderReadPixels)ptable[308];
    g_real_SetTextureBlendMode = (pfn_SDL_SetTextureBlendMode)ptable[276];
    g_real_PollEvent = (pfn_SDL_PollEvent)ptable[85];
    g_real_WaitEvent = (pfn_SDL_WaitEvent)ptable[86];
    g_real_WaitEventTimeout = (pfn_SDL_WaitEventTimeout)ptable[87];
    
    ptable[309] = (void*)Hooked_SDL_RenderPresent;
    ptable[306] = (void*)Hooked_SDL_RenderCopy;
    ptable[85] = (void*)Hooked_SDL_PollEvent;
    ptable[86] = (void*)Hooked_SDL_WaitEvent;
    ptable[87] = (void*)Hooked_SDL_WaitEventTimeout;
    
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);
    }
    return TRUE;
}
