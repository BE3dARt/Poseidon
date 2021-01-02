#pragma once

#include <filesystem>
#include <imgui/imgui_impl_dx11.h>
#include <imgui/imgui_internal.h>
#include <imgui/imgui_impl_win32.h>
#include <HookLib/HookLib.h>

namespace fs = std::filesystem;


inline bool Cheat::Logger::Init()
{
    fs::path log;
#ifdef STEAM
    wchar_t buf[MAX_PATH];
    if (!GetModuleFileNameW(hinstDLL, buf, MAX_PATH)) return false;
    log = fs::path(buf).remove_filename() / "log.txt";
#else
#ifdef UWPDEBUG
    log = "C:\\Users\\dimae\\AppData\\Local\\Packages\\Microsoft.SeaofThieves_8wekyb3d8bbwe\\TempState\\log.txt";
#else
    return true;
#endif
#endif
    file = CreateFileW(log.wstring().c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    return file != INVALID_HANDLE_VALUE;
}

inline bool Cheat::Logger::Remove()
{
    if (!file) return true;
    return CloseHandle(file);
}

void Cheat::Logger::Log(const char* format, ...)
{
#if defined STEAM || defined UWPDEBUG
    SYSTEMTIME rawtime;
    GetSystemTime(&rawtime);
    char buf[MAX_PATH];
    auto size = GetTimeFormatA(LOCALE_CUSTOM_DEFAULT, 0, &rawtime, "[HH':'mm':'ss] ", buf, MAX_PATH) - 1;
    size += snprintf(buf + size, sizeof(buf) - size, "[TID: 0x%X] ", GetCurrentThreadId());
    va_list argptr;
    va_start(argptr, format);
    size += vsnprintf(buf + size, sizeof(buf) - size, format, argptr);
    WriteFile(file, buf, size, NULL, NULL);
    va_end(argptr);
#endif
}
