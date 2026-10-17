#include "pch.h"
#include "winmain.h"

#if defined(_WIN32) && defined(PINBALL_STANDALONE)
#include <windows.h>

// The release is one file. Its notices remain available without game resources.
static bool ExportLicenses(const char* commandLine)
{
    if (strcmp(commandLine, "--licenses") != 0)
        return false;
    auto module = GetModuleHandleW(nullptr);
    auto resource = FindResourceW(module, MAKEINTRESOURCEW(101), MAKEINTRESOURCEW(10));
    auto data = resource ? LoadResource(module, resource) : nullptr;
    auto bytes = data ? LockResource(data) : nullptr;
    auto length = resource ? SizeofResource(module, resource) : 0;
    // CREATE_NEW deliberately preserves any existing local document.
    auto file = CreateFileW(L"SpaceCadetPinball-LICENSES.txt", GENERIC_WRITE, 0,
        nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    DWORD written = 0;
    bool success = file != INVALID_HANDLE_VALUE && bytes &&
        WriteFile(file, bytes, length, &written, nullptr) && written == length;
    if (file != INVALID_HANDLE_VALUE)
        CloseHandle(file);
    if (!success)
        MessageBoxW(nullptr, L"Unable to create SpaceCadetPinball-LICENSES.txt. Choose a writable folder where this file does not already exist.", L"Licenses", MB_OK | MB_ICONERROR);
    return true;
}
#endif

int main(int argc, char* argv[])
{
	std::string cmdLine;
	for (int i = 1; i < argc; i++)
	{
		if (i > 1)
			cmdLine += " ";
		cmdLine += argv[i];
	}

#if defined(_WIN32) && defined(PINBALL_STANDALONE)
    if (ExportLicenses(cmdLine.c_str())) return 0;
#endif
	return winmain::WinMain(cmdLine.c_str());
}

#if _WIN32
#include <windows.h>

// Windows subsystem main
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
#if defined(PINBALL_STANDALONE)
    if (ExportLicenses(lpCmdLine)) return 0;
#endif
	return winmain::WinMain(lpCmdLine);
}

// fopen to _wfopen adapter, for UTF-8 paths
FILE* fopenu(const char* path, const char* opt)
{
	wchar_t* wideArgs[2]{};
	for (auto& arg : wideArgs)
	{
		auto src = wideArgs[0] ? opt : path;
		auto length = MultiByteToWideChar(CP_UTF8, 0, src, -1, nullptr, 0);
		arg = new wchar_t[length];
		MultiByteToWideChar(CP_UTF8, 0, src, -1, arg, length);
	}

	auto fileHandle = _wfopen(wideArgs[0], wideArgs[1]);
	for (auto arg : wideArgs)
		delete[] arg;

	return fileHandle;
}
#endif

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
