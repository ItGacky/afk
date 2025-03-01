#include "framework.h"
#include "afk.h"


static const WCHAR* getAppPath() {
	static WCHAR path[MAX_PATH] = L"";
	if (!path[0])
		GetModuleFileNameW(nullptr, path, std::size(path));
	return path;
}

static const WCHAR* getAppName() {
	static const WCHAR* name = std::wcsrchr(getAppPath(), L'\\') + 1;
	return name;
}

static int alert(HINSTANCE hInstance, UINT idMessage, UINT type) {
	WCHAR strMessage[MAX_PATH];
	LoadStringW(hInstance, idMessage, strMessage, std::size(strMessage));
	return MessageBoxW(nullptr, strMessage, getAppName(), type);
}

static bool isDownloading() {
	bool exists = true;
	PWCHAR pathDownload;
	if SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Downloads, 0, nullptr, &pathDownload)) {
		WCHAR pathFind[MAX_PATH];
		std::swprintf(pathFind, std::size(pathFind), L"%s\\*.crdownload", pathDownload);
		WIN32_FIND_DATAW find;
		auto handle = FindFirstFileW(pathFind, &find);
		exists = (handle != INVALID_HANDLE_VALUE);
		FindClose(handle);
		CoTaskMemFree(pathDownload);
	}
	return exists;
}

enum ModeID {
	SLEEP,
	SUSPEND,
	AUTO
};

const struct Mode {
	ModeID			id;
	const WCHAR*	name;
} MODES[] = {
	{ AUTO, L"auto" },
	{ SLEEP, L"sleep" },
	{ SUSPEND, L"suspend" }
};

static const Mode* parseArgs(const WCHAR* args) {
	for (auto& mode : MODES) {
		if (0 == wcscmp(args, mode.name))
			return &mode;
	}
	return nullptr;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, WCHAR* args, int show) {
	if (auto mode = parseArgs(args)) {
		// suspend PC
		switch (mode->id) {
		case AUTO:
		case SUSPEND:
			if (isDownloading())
				break;
			if (auto powrprof = LoadLibraryW(L"powrprof.dll")) {
				if (auto SetSuspendState = reinterpret_cast<BOOL(WINAPI*)(BOOL, BOOL, BOOL)>(GetProcAddress(powrprof, "SetSuspendState"))) {
					SetSuspendState(FALSE, FALSE, FALSE);
				}
				FreeLibrary(powrprof);
			}
			return 0;
		}
		// suspend monitor
		switch (mode->id) {
		case AUTO:
		case SLEEP:
			PostMessageW(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, 2);
			return 0;
		}
		// canceled
		alert(hInstance, IDS_CANCEL_SUSPEND, MB_OK | MB_ICONWARNING);
		return 2;
	} else {
		// bad usage
		alert(hInstance, IDS_USAGE, MB_OK | MB_ICONERROR);
		return 1;
	}
}
