#pragma once

#include <filesystem>
#include <imgui/imgui_impl_dx11.h>
#include <imgui/imgui_internal.h>
#include <imgui/imgui_impl_win32.h>
#include <HookLib/HookLib.h>


inline bool Cheat::Tools::CompareByteArray(BYTE* data, BYTE* sig, SIZE_T size)
{
    for (SIZE_T i = 0; i < size; i++) {
        if (data[i] != sig[i]) {
            if (sig[i] == 0x00) continue;
            return false;
        }
    }
    return true;
}

inline BYTE* Cheat::Tools::FindSignature(BYTE* start, BYTE* end, BYTE* sig, SIZE_T size)
{
    for (BYTE* it = start; it < end - size; it++) {
        if (CompareByteArray(it, sig, size)) {
            return it;
        };
    }
    return 0;
}

void* Cheat::Tools::FindPointer(BYTE* sig, SIZE_T size, int addition = 0)
{
    auto base = static_cast<BYTE*>(gBaseMod.lpBaseOfDll);
    auto address = FindSignature(base, base + gBaseMod.SizeOfImage - 1, sig, size);
    if (!address) return nullptr;
    auto k = 0;
    for (; sig[k]; k++);
    auto offset = *reinterpret_cast<UINT32*>(address + k);
    return address + k + 4 + offset + addition;
}

inline BYTE* Cheat::Tools::FindFn(HMODULE mod, BYTE* sig, SIZE_T sigSize)
{
    if (!mod || !sig || !sigSize) return 0;
    MODULEINFO modInfo;
    if (!K32GetModuleInformation(GetCurrentProcess(), mod, &modInfo, sizeof(MODULEINFO))) return 0;
    auto base = static_cast<BYTE*>(modInfo.lpBaseOfDll);
    auto fn = Tools::FindSignature(base, base + modInfo.SizeOfImage - 1, sig, sigSize);
    if (!fn) return 0;
    for (; *fn != 0xCC && *fn != 0xC3; fn--);
    fn++;
    return fn;
}

inline bool Cheat::Tools::PatchMem(void* address, void* bytes, SIZE_T size)
{
    DWORD oldProtection;
    if (VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &oldProtection))
    {
        memcpy(address, bytes, size);
        return VirtualProtect(address, size, oldProtection, &oldProtection);
    };
    return false;
}

/*inline bool Cheat::Tools::HookVT(void** vtable, UINT64 index, void* FuncH, void** FuncO)
{
    if (!vtable || !FuncH || !vtable[index]) return false;
    if (FuncO) { *FuncO = vtable[index]; }
    PatchMem(&vtable[index], &FuncH, 8);
    return FuncH == vtable[index];
}*/

inline BYTE* Cheat::Tools::PacthFn(HMODULE mod, BYTE* sig, SIZE_T sigSize, BYTE* bytes, SIZE_T bytesSize)
{
    if (!mod || !sig || !sigSize || !bytes || !bytesSize) return 0;
    auto fn = FindFn(mod, sig, sigSize);
    if (!fn) return 0;
    return Tools::PatchMem(fn, bytes, bytesSize) ? fn : 0;
}

inline bool Cheat::Tools::FindNameArray()
{
    static BYTE sig[] = { 0x48, 0x8b, 0x3d, 0x00, 0x00, 0x00, 0x00, 0x48, 0x85, 0xff, 0x75, 0x3c };
    auto address = reinterpret_cast<decltype(FName::GNames)*>(FindPointer(sig, sizeof(sig)));
    if (!address) return 0;
    Logger::Log("%p\n", address);
    FName::GNames = *address;
    return FName::GNames;
}

inline bool Cheat::Tools::FindObjectsArray()
{
    static BYTE sig[] = { 0x89, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8B, 0xDF, 0x48, 0x89, 0x5C, 0x24 };
    UObject::GObjects = reinterpret_cast<decltype(UObject::GObjects)>(FindPointer(sig, sizeof(sig), 16));
    return UObject::GObjects;
}

inline bool Cheat::Tools::FindWorld()
{
    static BYTE sig[] = { 0x48, 0x8B, 0x05, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x88, 0x00, 0x00, 0x00, 0x00, 0x48, 0x85, 0xC9, 0x74, 0x06, 0x48, 0x8B, 0x49, 0x70 };
    UWorld::GWorld = reinterpret_cast<decltype(UWorld::GWorld)>(FindPointer(sig, sizeof(sig)));
    return UWorld::GWorld;
}

inline bool Cheat::Tools::InitSDK()
{
    if (!UCrewFunctions::Init()) return false;
    if (!UKismetMathLibrary::Init()) return false;
    return true;
}
