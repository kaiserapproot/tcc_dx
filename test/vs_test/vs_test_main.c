#include <windows.h>
#include <stdio.h>

/*
 * Win32 コンソール/Windowsアプリケーション両対応サンプル
 */
void sample_main(void);

void sample_main(void)
{
    DWORD dwVersion;
    DWORD dwWindowsMajorVersion;
    DWORD dwWindowsMinorVersion;
    DWORD dwBuild;
    char buf[256];
    HANDLE hConsole;
    SYSTEMTIME st;
    DWORD drives;
    int i;
    char driveLetter;
#ifdef _WINDOWS
    char dt[64];
#endif

    dwVersion = GetVersion();
    dwWindowsMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
    dwWindowsMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));
    dwBuild = 0;

    if (dwVersion < 0x80000000)
    {
        dwBuild = (DWORD)(HIWORD(dwVersion));
    }

    sprintf(buf, "Windows バージョン: %d.%d (Build %d)\n",
            dwWindowsMajorVersion, dwWindowsMinorVersion, dwBuild);

#ifdef _CONSOLE
    printf("%s", buf);
    SetConsoleTitle("console");
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    printf("\n現在の日時:\n");
#else
    MessageBox(NULL, buf, "Win32 サンプル（_WINDOWS）", MB_OK | MB_ICONINFORMATION);
#endif

    GetLocalTime(&st);

#ifdef _CONSOLE
    printf("%04d/%02d/%02d %02d:%02d:%02d\n",
            st.wYear, st.wMonth, st.wDay,
            st.wHour, st.wMinute, st.wSecond);

    drives = GetLogicalDrives();
    printf("\n利用可能なドライブ:\n");

    for (i = 0; i < 26; i++)
    {
        int mask = drives & (1 << i);
        if (mask != 0)
        {
            driveLetter = 'A' + i;
            printf("%c:\\n", driveLetter);
        }
    }

    printf("\nキーを押して終了してください...");
    getchar();
#else
    sprintf(dt, "%04d/%02d/%02d %02d:%02d:%02d",
            st.wYear, st.wMonth, st.wDay,
            st.wHour, st.wMinute, st.wSecond);
    MessageBox(NULL, dt, "現在の日時", MB_OK);
#endif
}

#ifdef _CONSOLE
int main(int argc, char **argv)
{
    sample_main();
    return 0;
}
#elif defined(_WINDOWS)
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow)
{
    sample_main();
    return 0;
}
#endif
