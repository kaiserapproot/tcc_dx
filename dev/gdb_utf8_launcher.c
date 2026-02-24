#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    char modulePath[MAX_PATH];
    if (GetModuleFileNameA(NULL, modulePath, sizeof(modulePath)) == 0) {
        return 1;
    }
    char *p = strrchr(modulePath, '\\');
    char gdbPath[MAX_PATH];
    if (p) {
        *p = '\0';
        snprintf(gdbPath, sizeof(gdbPath), "%s\\gdb.exe", modulePath);
    } else {
        snprintf(gdbPath, sizeof(gdbPath), "gdb.exe");
    }

    // Build command line: "<gdbPath>" [args...]
    size_t cmdLen = strlen(gdbPath) + 3;
    for (int i = 1; i < argc; ++i) cmdLen += strlen(argv[i]) + 3;
    char *cmd = (char*)malloc(cmdLen + 1);
    if (!cmd) return 1;
    cmd[0] = '\0';
    strcat(cmd, "\"");
    strcat(cmd, gdbPath);
    strcat(cmd, "\"");
    for (int i = 1; i < argc; ++i) {
        strcat(cmd, " ");
        int needq = (strchr(argv[i], ' ') != NULL);
        if (needq) strcat(cmd, "\"");
        strcat(cmd, argv[i]);
        if (needq) strcat(cmd, "\"");
    }

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    BOOL ok = CreateProcessA(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
    free(cmd);
    if (!ok) return 1;

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return (int)exitCode;
}
