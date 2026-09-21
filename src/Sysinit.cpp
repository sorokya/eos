#include <vcl.h>
#pragma hdrstop

// The RTL/VCL `Sysinit` unit. The reference image carries a build of it that is
// not the one in `vcl50.lib` (same source, different build: its function sizes
// match `Lib/Obj/sysinit.obj` rather than the library member), and it is placed
// immediately after `c0w32` - i.e. the original project supplied it, so its
// definitions took precedence over the library's copy.
//
// This unit therefore deliberately has **no** `#pragma package(smart_init)`: it
// must not contribute a `@@Sysinit@Initialize` export (the export table has
// exactly 65 units and this is not one of them).
//
// Reconstructed from the reference disassembly; verified against
// `GameServer.exe` with `scripts/compare_asm.py` / `scripts/maskdiff.py`.

namespace Sysinit
{
// Delphi register/stack conventions matter byte-for-byte here: the callers in
// the VCL push these arguments, and the reference's body reads [ebp+8]/[ebp+0xc]
// /[ebp+0x10], so this is the stack convention.
void VclInit(bool is_library, bool is_console, int module_handle, bool is_gui);
void VclExit();
} // namespace Sysinit

namespace Sysinit
{
void VclInit(bool is_library, bool is_console, int module_handle, bool is_gui)
{
    (void)is_library;
    (void)is_console;
    (void)module_handle;
    (void)is_gui;
}

void VclExit()
{
}
} // namespace Sysinit
