#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#endif

#include "runtime.h"

#if defined(GBAGAME_RECOMP_UI)
#include "game_launcher_boot.h"
#endif

namespace {

constexpr const char* kKirbyUsaSha1 = "37a476567d133c146fee6b5e2eb0b07a215da6b0";

#if defined(_WIN32)
void auto_enable_bezel_hook() {
    // If SDL_DYNAMIC_API is already explicitly configured in the environment, honor it
    if (getenv("SDL_DYNAMIC_API")) return;

    char exe_path[MAX_PATH];
    if (GetModuleFileNameA(NULL, exe_path, MAX_PATH)) {
        char* last_slash = strrchr(exe_path, '\\');
        if (last_slash) {
            *(last_slash + 1) = '\0';
            std::string dll_path = std::string(exe_path) + "kirby_bezel.dll";
            FILE* f = fopen(dll_path.c_str(), "rb");
            if (f) {
                fclose(f);
                SetEnvironmentVariableA("SDL_DYNAMIC_API", dll_path.c_str());
            }
        }
    }
}
#endif

}  // namespace

int main(int argc, char** argv) {
#if defined(_WIN32)
    auto_enable_bezel_hook();
#endif

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            std::printf(
                "KirbyRecomp [--bios <path>] [--rom <path>] [game.toml]\n\n"
                "Windowed play opens the launcher first; --no-launcher skips it once.\n"
                "Press Escape during play to open the in-game menu.\n"
                "Press F10 at any time to toggle the custom 1080p Bezel on or off.\n");
            return 0;
        }
    }

    gbarecomp::RunOptions opts;
    opts.builtin_game_name = "Kirby: Nightmare in Dream Land (USA)";
    opts.builtin_rom_sha1 = kKirbyUsaSha1;
    opts.launcher_region = "USA";
    opts.launcher_boxart = "assets/img/boxart_kirby.tga";
    opts.launcher_game_config = "game.toml";
    opts.launcher_save_path = "saves/kirby_nid.sav";
    opts.launcher_expose_widescreen = true;
    opts.launcher_expose_adaptive_view = true;
    opts.widescreen_view_width = 284;
    opts.launcher_enable_rom_patches = false;

    // PC conveniences: rewind + fast-forward, save states, sharper scaling.
    opts.expose_assist_tools = true;
    opts.assist_tools_enabled_by_default = true;
    opts.assist_fast_forward_multiplier_default = 4;
    opts.rewind_history_seconds = 30;
    opts.launcher_expose_sharp_filter = true;
    opts.launcher_default_sharp_filter = true;
    opts.freely_resizable_window = true;
    opts.max_view_width = 432;

#if defined(GBAGAME_RECOMP_UI)
    std::vector<std::string> args(argv, argv + argc);
    if (game_launcher_preboot(args, opts)) return 0;
    std::vector<char*> av;
    av.reserve(args.size());
    for (auto& arg : args) av.push_back(arg.data());
    return gbarecomp::run_game(static_cast<int>(av.size()), av.data(), opts);
#else
    return gbarecomp::run_game(argc, argv, opts);
#endif
}
