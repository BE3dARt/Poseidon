#include <Windows.h>
#include <Psapi.h>
#include <TlHelp32.h>
#include <stdio.h>
#include <filesystem>
#include <iostream>
#include <aclapi.h>
#include <sddl.h>

namespace fs = std::filesystem;

//https://www.unknowncheats.me/forum/sea-of-thieves/

////////////////////////////////////////////////////////////////////////////////////////////////
// Return:
// "process identifier" (PID) of "SoTGame.exe"
// 
// Function:
// Takes a Snapshot of the specified processes, as well as the heaps, modules, and threads, 
// analyise until "SoTGame.exe" is found.
////////////////////////////////////////////////////////////////////////////////////////////////
inline DWORD GetProcessIdByName(const char* name) {
    PVOID snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 process;
    process.dwSize = sizeof(process);

    DWORD pid = 0;
    while (Process32Next(snapshot, &process)) {
        if (strcmp(process.szExeFile, name) == 0) {
            pid = process.th32ProcessID;
            break;
        }
    }

    CloseHandle(snapshot);
    return pid;
}

inline bool HasModule(const DWORD pid, const char* modName)
{
    bool status = false;
    PVOID snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
    MODULEENTRY32 modEntry = { sizeof(MODULEENTRY32) };
    while (Module32Next(snapshot, &modEntry)) {
        if (strcmp(modEntry.szModule, modName) == 0) {
            status = true;
            break;
        }
    }
    CloseHandle(snapshot);
    return status;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Return:
// If there is a library
// filename of library
// 
////////////////////////////////////////////////////////////////////////////////////////////////
inline bool GetFileExtFromDir(const fs::path& dir, const char* ext, fs::path& file)
{
    for (const auto& entry : fs::directory_iterator(dir))
    {
        if (!entry.is_regular_file()) continue;
        const fs::path currentFile = entry.path();
        if (currentFile.extension() == ext)
        {
            file = currentFile;
            return true;
        }
    }
    return false;
}

// https://www.unknowncheats.me/forum/general-programming-and-reversing/177183-basic-intermediate-techniques-uwp-app-modding.html
inline bool SetAccessControl(const wchar_t* file, const wchar_t* access)
{
    PSECURITY_DESCRIPTOR sec = nullptr;
    PACL currentAcl = nullptr;
    PSID sid = nullptr;
    PACL newAcl = nullptr;
    bool status = false;
    goto init;
end:
    if (newAcl) LocalFree(newAcl);
    if (sid) LocalFree(sid);
    if (sec) LocalFree(sec);
    return status;
init:
    if (GetNamedSecurityInfoW(file, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, &currentAcl, nullptr, &sec) != ERROR_SUCCESS) goto end;
    if (!ConvertStringSidToSidW(access, &sid)) goto end;
    EXPLICIT_ACCESSW desc = { 0 };
    desc.grfAccessPermissions = GENERIC_READ | GENERIC_EXECUTE | GENERIC_WRITE;
    desc.grfAccessMode = SET_ACCESS;
    desc.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    desc.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    desc.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    desc.Trustee.ptstrName = reinterpret_cast<wchar_t*>(sid);
    if (SetEntriesInAclW(1, &desc, currentAcl, &newAcl) != ERROR_SUCCESS) goto end;
    if (SetNamedSecurityInfoW(const_cast<wchar_t*>(file), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, newAcl, nullptr) != ERROR_SUCCESS) goto end;
    status = true;
    goto end;
}

inline bool RemoteInject(const HANDLE& process, const std::wstring& mod)
{
    if (!SetAccessControl(mod.c_str(), L"S-1-15-2-1")) return false;
    auto len = mod.capacity() * sizeof(wchar_t);
    LPVOID alloc = VirtualAllocEx(process, 0, len, MEM_COMMIT, PAGE_READWRITE);
    if (!alloc) return false;
    if (!WriteProcessMemory(process, alloc, mod.data(), len, 0)) return false;
    auto thread = CreateRemoteThread(process, 0, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(LoadLibraryW), alloc, 0, 0);
    if (!thread) return false;
    WaitForSingleObject(thread, INFINITE);
    VirtualFreeEx(process, alloc, len, MEM_RESERVE);
    return true;
}

int wmain(int argc, wchar_t* argv[]) {
    DWORD pid = 0;

    DWORD counter = 0;
    printf("You have 60 seconds to start Sea of Thieves!\n");
    while (true)
    {
        pid = GetProcessIdByName("SoTGame.exe");

        if (!pid)
        {
            Sleep(1000);
            counter += 1;

            printf("Game process not found for %d seconds \n", counter);

            if (counter > 60)
            {
                return 1;
            }
        }
        else {
            break;
        }
    }
       
    const HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
    if (process == INVALID_HANDLE_VALUE)
    {
        printf("Can't get process handle\n");
        system("pause");
        return 1;
    }

    const fs::path workingDir = fs::path(argv[0]).remove_filename();
    fs::path library;
    if (!GetFileExtFromDir(workingDir, ".dll", library))
    {
        printf("Can't find dynamic library in the current folder\n");
        system("pause");
        return 1;
    };

    const auto modName = library.filename().string();
    if (HasModule(pid, modName.c_str()))
    {
        printf("%s has been loaded to process already\n", modName.c_str());
        system("pause");
        return 1;
    }
    if (!RemoteInject(process, library.wstring()) || !HasModule(pid, modName.c_str()))
    {
        printf("Injection has failed or module has unloaded already\n");
        system("pause");
        return 1;
    };
    printf("Successfully injected!\n");
    return 0;
  
}