
#include <windows.h>
#include <iostream>

int WINAPI maliciousMessageBox(
    HWND hWnd,
    LPCSTR lpText,
    LPCSTR lpCaption,
    UINT uType)
{
    return MessageBoxA(
        nullptr,
        "Hooked",
        "Noder stom ahi",
        MB_ICONWARNING | MB_CANCELTRYCONTINUE | MB_DEFBUTTON2
    );
}

int main()
{
    // Loading our DLL
    const auto hGetProcIDDLL = LoadLibrary(
        L"C:\\Users\\Ofer\\projects\\IAT-hooking\\IAT-Hooking\\x64\\Debug\\InnocentDll.dll"
    );

    if (hGetProcIDDLL == NULL) {
        std::cout << "Cannot locate the DLL file" << std::endl;
        return 1;
    }

    std::cout << "DLL loaded successfully" << std::endl;

    // Get PE structures
    auto base = reinterpret_cast<BYTE*>(hGetProcIDDLL);

    auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);

    auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>(
        base + dos->e_lfanew
        );

    auto importDir =
        nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];

    auto desc = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(
        base + importDir.VirtualAddress
        );

    // Select the SECOND imported DLL
    auto secondDesc = desc + 1;

    if (secondDesc->Name == 0) {
        std::cout << "There is no second imported DLL" << std::endl;
        return 1;
    }

    auto dllName = reinterpret_cast<const char*>(
        base + secondDesc->Name
        );

    std::cout << "Second DLL: " << dllName << std::endl;

    // Get INT and IAT of the second DLL
    auto intTable = reinterpret_cast<IMAGE_THUNK_DATA*>(
        base + secondDesc->OriginalFirstThunk
        );

    auto iat = reinterpret_cast<IMAGE_THUNK_DATA*>(
        base + secondDesc->FirstThunk
        );

    // Select the FIRST imported function
    if (intTable->u1.AddressOfData == 0) {
        std::cout << "No imported functions found" << std::endl;
        return 1;
    }

    if (IMAGE_SNAP_BY_ORDINAL(intTable->u1.Ordinal)) {
        std::cout << "First function is imported by ordinal" << std::endl;
        return 1;
    }

    auto import = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(
        base + intTable->u1.AddressOfData
        );

    std::cout << "First function: " << import->Name << std::endl;

    // Change IAT entry
    DWORD oldProtect;

    if (VirtualProtect(
        &iat->u1.Function,
        sizeof(ULONG_PTR),
        PAGE_READWRITE,
        &oldProtect))
    {
        iat->u1.Function =
            reinterpret_cast<ULONG_PTR>(&maliciousMessageBox);

        DWORD temp;
        VirtualProtect(
            &iat->u1.Function,
            sizeof(ULONG_PTR),
            oldProtect,
            &temp
        );
    }
    else {
        std::cout << "Cannot change IAT protection" << std::endl;
        return 1;
    }

    // Call exported function
    using MessageBoxSymbol = void(*)();

    auto messageBoxFunc = reinterpret_cast<MessageBoxSymbol>(
        GetProcAddress(hGetProcIDDLL, "MessageBoxThread")
        );

    if (!messageBoxFunc) {
        std::cout << "Cannot find MessageBoxThread" << std::endl;
        return 1;
    }

    messageBoxFunc();

    FreeLibrary(hGetProcIDDLL);

    return 0;
}
