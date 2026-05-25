#include "pch.h"
#include "CppUnitTest.h"
#include <windows.h>
#include <stdio.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace
{
    static int path_has_dev_tcc(const wchar_t* dir)
    {
        wchar_t probe[MAX_PATH];
        _snwprintf_s(probe, _TRUNCATE, L"%s\\dev\\tcc.exe", dir);
        return GetFileAttributesW(probe) != INVALID_FILE_ATTRIBUTES;
    }

    static void get_repo_root(wchar_t* buf, size_t nbuf)
    {
        wchar_t env[MAX_PATH];
        DWORD n = GetEnvironmentVariableW(L"TCC_REPO_ROOT", env, MAX_PATH);
        if (n > 0 && n < MAX_PATH) {
            wcsncpy_s(buf, nbuf, env, _TRUNCATE);
            if (path_has_dev_tcc(buf))
                return;
        }
        wchar_t path[MAX_PATH];
        HMODULE hm = NULL;
        GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCWSTR)&get_repo_root, &hm);
        GetModuleFileNameW(hm, path, MAX_PATH);
        wchar_t* slash = path;
        for (;;) {
            slash = wcsrchr(slash, L'\\');
            if (!slash)
                break;
            *slash = 0;
            if (path_has_dev_tcc(path)) {
                wcsncpy_s(buf, nbuf, path, _TRUNCATE);
                return;
            }
            slash = path;
        }
        buf[0] = 0;
    }

    static int run_cmdline(wchar_t* cmdline, int* exit_code)
    {
        STARTUPINFOW si = {};
        PROCESS_INFORMATION pi = {};
        si.cb = sizeof(si);
        if (!CreateProcessW(NULL, cmdline, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
            return -1;
        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD ec = 1;
        GetExitCodeProcess(pi.hProcess, &ec);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        if (exit_code)
            *exit_code = (int)ec;
        return 0;
    }

    static int run_tcc_cmd(const wchar_t* args, int* exit_code)
    {
        wchar_t root[MAX_PATH];
        wchar_t cmdline[2048];
        get_repo_root(root, MAX_PATH);
        if (!root[0])
            return -1;
        _snwprintf_s(cmdline, _TRUNCATE,
            L"cmd /c cd /d \"%s\" && dev\\tcc.exe %s",
            root, args);
        return run_cmdline(cmdline, exit_code);
    }

    static int run_batch(const wchar_t* bat_rel, int* exit_code)
    {
        wchar_t root[MAX_PATH];
        wchar_t cmdline[2048];
        get_repo_root(root, MAX_PATH);
        if (!root[0])
            return -1;
        _snwprintf_s(cmdline, _TRUNCATE,
            L"cmd /c cd /d \"%s\" && call %s",
            root, bat_rel);
        return run_cmdline(cmdline, exit_code);
    }

    static void assert_tcc_compile(const wchar_t* src_rel, const wchar_t* obj_rel)
    {
        wchar_t root[MAX_PATH];
        wchar_t args[1024];
        int ec = -1;
        get_repo_root(root, MAX_PATH);
        if (!root[0])
            Assert::Fail(L"TCC repo root not found (dev\\tcc.exe)");
        _snwprintf_s(args, _TRUNCATE, L"-c %s -o %s", src_rel, obj_rel);
        Assert::AreEqual(0, run_tcc_cmd(args, &ec));
        Assert::AreEqual(0, ec);
    }
}

namespace cppuniut
{
    TEST_CLASS(TccFoundation)
    {
    public:
        TEST_METHOD(TccCompile_SmokeHello)
        {
            assert_tcc_compile(L"dev\\test\\smoke\\hello.c", L"dev\\test\\smoke\\hello.o");
        }

        TEST_METHOD(TccCompile_EmptyCpp)
        {
            assert_tcc_compile(L"dev\\test\\smoke\\empty.cpp", L"dev\\test\\smoke\\empty.o");
        }

        TEST_METHOD(TccCompile_A2_KeywordGate)
        {
            assert_tcc_compile(L"dev\\test\\a2\\keyword_gate.c", L"dev\\test\\a2\\keyword_gate.o");
        }

        TEST_METHOD(TccCompile_A2_RepeatedIdent)
        {
            assert_tcc_compile(L"dev\\test\\a2\\repeated_ident.c", L"dev\\test\\a2\\repeated_ident.o");
        }

        TEST_METHOD(TccCompile_A2_KeywordLex)
        {
            assert_tcc_compile(L"dev\\test\\a2\\keyword_lex.cpp", L"dev\\test\\a2\\keyword_lex.o");
        }

        TEST_METHOD(TccCompile_A3_Cplusplus)
        {
            assert_tcc_compile(L"dev\\test\\a3\\cplusplus.cpp", L"dev\\test\\a3\\cplusplus.o");
        }

        TEST_METHOD(TccCompile_A4_ExternCBlock)
        {
            assert_tcc_compile(L"dev\\test\\a4\\extern_c_block.cpp", L"dev\\test\\a4\\extern_c_block.o");
        }

        TEST_METHOD(TccCompile_A5_ClassMin)
        {
            assert_tcc_compile(L"dev\\test\\a5\\class_min.cpp", L"dev\\test\\a5\\class_min.o");
        }

        TEST_METHOD(TccCompile_A5_ClassInlineBody)
        {
            assert_tcc_compile(L"dev\\test\\a5\\class_inline_body.cpp", L"dev\\test\\a5\\class_inline_body.o");
        }

        TEST_METHOD(TccCompile_A5_KeywordGate)
        {
            assert_tcc_compile(L"dev\\test\\a5\\keyword_gate.cpp", L"dev\\test\\a5\\keyword_gate.o");
        }

        TEST_METHOD(TccCompile_A6_Overload)
        {
            assert_tcc_compile(L"dev\\test\\a6\\overload.cpp", L"dev\\test\\a6\\overload.o");
        }

        TEST_METHOD(TccCompile_A6_RefParam)
        {
            assert_tcc_compile(L"dev\\test\\a6\\ref_param.cpp", L"dev\\test\\a6\\ref_param.o");
        }

        TEST_METHOD(TccCompile_A6_Qualified)
        {
            assert_tcc_compile(L"dev\\test\\a6\\qualified.cpp", L"dev\\test\\a6\\qualified.o");
        }

        TEST_METHOD(TccCompile_A6_ExternCDecl)
        {
            assert_tcc_compile(L"dev\\test\\a6\\extern_c_decl.cpp", L"dev\\test\\a6\\extern_c_decl.o");
        }

        TEST_METHOD(TccRunAll_Batch)
        {
            int ec = -1;
            Assert::AreEqual(0, run_batch(L"dev\\test\\run_all.bat", &ec));
            Assert::AreEqual(0, ec);
        }
    };
}
