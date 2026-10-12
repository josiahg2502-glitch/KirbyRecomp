# recomp_master_misses.toml.frag — AUTO-GENERATED proposal.
# These guest PCs were reached by runtime_dispatch with no generated
# function and were bridged through the interpreter this session.
# A HUMAN reviews these and merges the genuine ones into the binary's
# config; this file is NEVER auto-merged (PRINCIPLES.md "Never
# auto-write game.toml").
#
# Proposal shapes:
#   [[extra_func]]  — a standalone missed function entry.
#   [[jump_table]]  — flagged in a comment when a tight run of
#       consecutive same-mode misses looks like a computed-jump
#       switch's case targets. Sizing the table covers the whole
#       switch in ONE entry; prefer it over the per-case [[extra_func]].
# BIOS PCs (< 0x4000) belong in bios/gba_bios.toml; cart PCs in the
# game's game.toml.
#
# game:  AGB KIRBY DX
# code:  A7KE
# sha1:  37a476567d133c146fee6b5e2eb0b07a215da6b0

[[extra_func]]
addr = 0x03001030
mode = "arm"
note = "proposed from self-heal miss-log; bridged x57"

[[extra_func]]
addr = 0x030010FC
mode = "arm"
note = "proposed from self-heal miss-log; bridged x57"

[[extra_func]]
addr = 0x03001F40
mode = "thumb"
note = "proposed from self-heal miss-log; bridged x48"

[[extra_func]]
addr = 0x03007150
mode = "thumb"
note = "proposed from self-heal miss-log; bridged x57"

[[extra_func]]
addr = 0x03007DF4
mode = "thumb"
note = "proposed from self-heal miss-log; bridged x6"

