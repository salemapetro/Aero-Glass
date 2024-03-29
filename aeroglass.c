#include <windows.h>
#include <stdio.h>
#include <psapi.h>

#ifndef PATH_SEPARATOR
#define PATH_SEPARATOR '\\'
#endif

#define DEBUG 0

typedef BOOL (WINAPI *SetLayeredWindowAttributes_t) (HWND, COLORREF, BYTE,
	DWORD);
static SetLayeredWindowAttributes_t SetLayeredWindowAttributes;
static void changeWindowOpacity(HWND, float);
static BOOL CALLBACK onEnumWindow(HWND, LPARAM);
static const char *format = " %-4d  %-15s %s\n";
static const char *image_name;
static char *program_name;
static double opacity;

char * basename(const char *filename)
{
	char *p = (char *) filename;
	char *f;

	do {
		f = ++p;
	 	p = strchr(f, PATH_SEPARATOR);
	} while (p);

	return f;
}

void remove_extension(char *filename)
{
	char *p = (char *) filename;
	char *d;

	while ((p = strchr(++p, '.'))) {
		d = p;
	}

	if (d) {
		//printf("> %c\n", *d);
		(*d) = '\0';
	}
}

void usage()
{
	printf("\
Usage: %s [OPTION] IMAGE OPACITY\n\
Change the opacity of visible Aero windows.\n\
\n\
All visible windows created by a process whose image name matches IMAGE will \
have their opacity changed to OPACITY.\n\
OPACITY has a range between 0.0 (invisible) and 1.0 (opaque).\n\
\n\
Issues: petro@petrosalema.com\n\
", program_name);
}

int main(int argc, char **argv)
{
	program_name = basename(argv[0]);
	remove_extension((char *) program_name);

	if (1 == argc) {
		usage();
		return 0;
	}

	image_name = argv[1];
	opacity = argc > 2 ? atof(argv[2]) : 1.0;

	HMODULE User32DLL = LoadLibrary(TEXT("USER32.DLL"));

	if (!User32DLL) {
		printf("Could not load module `USER32.DLL.'\n");
		return 1;
	}

	SetLayeredWindowAttributes = (SetLayeredWindowAttributes_t) GetProcAddress(
		User32DLL, "SetLayeredWindowAttributes");

	if (DEBUG) {
		printf(" PID  Image Name      Window Title\n");
		printf(" ---- --------------- ------------\n");
	}

	if (!EnumWindows(onEnumWindow, 0)) {
		printf("Could not enumerate windows.\n");
		return 2;
	}

	FreeLibrary(User32DLL);

	return 0;
}

static BOOL CALLBACK onEnumWindow(HWND hwnd, LPARAM lparam)
{
	if (!IsWindowVisible(hwnd)) {
		return TRUE;
	}

	DWORD pid;
	GetWindowThreadProcessId(hwnd, &pid);

	HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
		FALSE, pid);

	if (!process) {
		return TRUE;
	}

	char process_name[MAX_PATH];
	GetModuleBaseName(process, NULL, process_name, sizeof(process_name));

	if (0 != strcmp(process_name, image_name)) {
		return TRUE;
	}

	changeWindowOpacity(hwnd, opacity);

	if (DEBUG) {
		char text[MAX_PATH];
		GetWindowText(hwnd, text, sizeof(text));
		printf(format, pid, process_name, text);
	}

	CloseHandle(process);

	return TRUE;
}

static void changeWindowOpacity(HWND hwnd, float opacity)
{
	SetWindowLong(hwnd, GWL_EXSTYLE,
		GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
	SetLayeredWindowAttributes(hwnd, RGB(128, 128, 128), 255 * opacity, 0x02);
}
