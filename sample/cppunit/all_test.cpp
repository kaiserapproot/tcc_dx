#include <windows.h>
#include <time.h>
#include "TestCase.h"
#include "TestResult.h"
#include "TestRunner.h"
#include "TestRegistry.h"
#include "TestFailure.h"
#include "TestListener.h"
#ifndef FOREGROUND_CYAN
#define FOREGROUND_CYAN (FOREGROUND_BLUE | FOREGROUND_GREEN)
#endif
#ifndef FOREGROUND_YELLOW
#define FOREGROUND_YELLOW (FOREGROUND_RED | FOREGROUND_GREEN)
#endif
USING_NAMESPACE_CPPUNIT
// ConsoleUtilクラスを追加（MessageOutputTestクラスの前に配置）
class ConsoleUtil {
    public:
        // 色付きメッセージ出力
        static void printColored(const char* message, WORD color) {
            HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
            SetConsoleTextAttribute(hConsole, color | FOREGROUND_INTENSITY);
            printf("%s", message);
            // 元の色に戻す
            SetConsoleTextAttribute(hConsole, 
                FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        }
    
        // テスト実行中メッセージの出力
        static void printTestMethod(const char* methodName, WORD color) {
            char buf[256];
            sprintf(buf, "実行中のテスト: %s\n", methodName);
            printColored(buf, color);
        }
    };
class MessageOutputTest : public TestCase 
{
public:
    // 出力テストのためのテストケース
    // このクラスでは以下の項目をテストします：
    // - 基本的なアサーション出力
    // - 等値比較の出力
    // - メッセージ付きアサーション出力
    // - エラー出力
    // - 失敗出力
    MessageOutputTest() : TestCase("テスト出力の確認") {}
    
    void runTest() {
        try {
            testSuccessCase();      // 成功ケース - 基本的なアサーション
            testAssertEqualsCase(); // 等値比較 - 意図的な失敗を含む
            testMessageCase();      // メッセージ付きケース
            testErrorCase();        // エラーケース - 例外発生
            testFailureCase();      // 失敗ケース - アサーション失敗
        }
        catch (std::exception& e) {
            char buf[256];
            sprintf(buf, "例外が発生: %s", e.what());
            TEST_ASSERT_MESSAGE(false, buf);
        }
    }

private:
    // 基本的な成功テスト
    void testSuccessCase() {
        ConsoleUtil::printTestMethod("testSuccessCase", FOREGROUND_GREEN);
        TEST_ASSERT(true);
    }

    // 等値比較テスト
    void testAssertEqualsCase() {
        ConsoleUtil::printTestMethod("testAssertEqualsCase", FOREGROUND_CYAN);
        int expected = 1;
        int actual = 2;
        char buf[256];
        sprintf(buf, "期待値: %d, 実際の値: %d", expected, actual);
        TEST_ASSERT_MESSAGE(expected == actual, buf);
    }

    // メッセージ付きテスト
    void testMessageCase() {
        ConsoleUtil::printTestMethod("testMessageCase", FOREGROUND_BLUE);
        TEST_ASSERT_MESSAGE(true, "テストケース: カスタムメッセージ出力");
    }

    // エラーケース
    void testErrorCase() {
        ConsoleUtil::printTestMethod("testErrorCase", FOREGROUND_RED);
        throw std::runtime_error("意図的なエラー発生");
    }

    // 失敗ケース
    void testFailureCase() {
        ConsoleUtil::printTestMethod("testFailureCase", FOREGROUND_YELLOW);
        TEST_ASSERT_MESSAGE(false, "意図的な失敗");
    }
};

class MyTestListener : public TestListener {
    public:
        MyTestListener() : m_filePtr(stdout), m_testSuccess(true), 
                          m_totalTests(0), m_successCount(0) {}
                          const char* getCurrentDateTime() {
                            static char buf[128];
                            time_t now = time(NULL);
                            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
                            return buf;
                        }
        // テスト開始時の処理
        void startTest(Test* test) {
            m_totalTests++;
            m_testSuccess = true;
            
            ConsoleUtil::printColored("\n===================================\n", FOREGROUND_GREEN);
            char buf[256];
            sprintf(buf, "? テスト開始 [%d] \n", m_totalTests);
            ConsoleUtil::printColored(buf, FOREGROUND_GREEN);
            sprintf(buf, "テスト名: %s\n", test->getName());
            ConsoleUtil::printColored(buf, FOREGROUND_GREEN);
            ConsoleUtil::printColored("-----------------------------------\n", FOREGROUND_GREEN);
        }
        
        // テスト終了時の処理
        void endTest(Test* test) {
            if (m_testSuccess) {
                m_successCount++;
            }
            
            WORD color = m_testSuccess ? FOREGROUND_GREEN : FOREGROUND_RED;
            
            ConsoleUtil::printColored("\n-----------------------------------\n", color);
            char buf[256];
            sprintf(buf, "テスト終了: %s\n", test->getName());
            ConsoleUtil::printColored(buf, color);
            sprintf(buf, "結果: %s\n", m_testSuccess ? "? 成功" : "? 失敗");
            ConsoleUtil::printColored(buf, color);
            ConsoleUtil::printColored("===================================\n\n", color);
            
            printTestResult(test, m_testSuccess);
        }
        
        // 失敗時の処理
        void addFailure(const TestFailure* failure) {
            m_testSuccess = false;
            
            ConsoleUtil::printColored("\n!!! 失敗検出 !!!\n", FOREGROUND_RED);
            ConsoleUtil::printColored("━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n", FOREGROUND_RED);
            
            char buf[256];
            sprintf(buf, "テスト   : %s\n", failure->failedTest()->getName());
            ConsoleUtil::printColored(buf, FOREGROUND_RED);
            sprintf(buf, "ファイル : %s\n", failure->file());
            ConsoleUtil::printColored(buf, FOREGROUND_RED);
            sprintf(buf, "行番号   : %d\n", failure->line());
            ConsoleUtil::printColored(buf, FOREGROUND_RED);
            
            if (failure->what()) {
                sprintf(buf, "詳細     : %s\n", failure->what());
                ConsoleUtil::printColored(buf, FOREGROUND_RED);
            }
            ConsoleUtil::printColored("━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n", FOREGROUND_RED);
            
            printFailureDetail(m_filePtr, *failure);
        }
        
        // エラー時の処理
        void addError(const TestFailure* error) {
            m_testSuccess = false;
            
            ConsoleUtil::printColored("\n? エラー検出 ?\n", FOREGROUND_YELLOW);
            ConsoleUtil::printColored("━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n", FOREGROUND_YELLOW);
            
            char buf[256];
            sprintf(buf, "テスト   : %s\n", error->failedTest()->getName());
            ConsoleUtil::printColored(buf, FOREGROUND_YELLOW);
            if (error->what()) {
                sprintf(buf, "エラー内容: %s\n", error->what());
                ConsoleUtil::printColored(buf, FOREGROUND_YELLOW);
            }
            ConsoleUtil::printColored("━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n", FOREGROUND_YELLOW);
            
            printFailureDetail(m_filePtr, *error);
        }
        
        // サマリーの出力
        void printSummary() {
            WORD color = (m_totalTests == m_successCount) ? FOREGROUND_GREEN : FOREGROUND_YELLOW;
            
            ConsoleUtil::printColored("\n=== テスト実行サマリー ===\n", color);
            char buf[256];
            sprintf(buf, "総テスト数: %d\n", m_totalTests);
            ConsoleUtil::printColored(buf, color);
            sprintf(buf, "成功数: %d\n", m_successCount);
            ConsoleUtil::printColored(buf, color);
            sprintf(buf, "失敗数: %d\n", m_totalTests - m_successCount);
            ConsoleUtil::printColored(buf, color);
            sprintf(buf, "成功率: %.1f%%\n", 
                   (m_totalTests > 0) ? (m_successCount * 100.0 / m_totalTests) : 0.0);
            ConsoleUtil::printColored(buf, color);
            ConsoleUtil::printColored("========================\n", color);
        }
    
    private:
        // テスト結果の出力
        void printTestResult(Test* test, bool ok) {
            fprintf(m_filePtr, "[%s] %s\n", 
                    (ok ? "OK" : "NG"), 
                    test->getName());
        }
        
        // 失敗詳細の出力
        void printFailureDetail(FILE* fp, const TestFailure& failure) {
            fprintf(fp, "\n【テスト失敗の詳細ログ】\n");
            fprintf(fp, "発生日時: %s\n", getCurrentDateTime());
            
            // テストの詳細情報
            fprintf(fp, "テストケース情報:\n");
            fprintf(fp, "  - 実行順序: %d番目\n", m_totalTests);
            fprintf(fp, "  - 成功数/総数: %d/%d\n", m_successCount, m_totalTests);
            
            // スタックトレース的な情報
            fprintf(fp, "\nコールスタック:\n");
            fprintf(fp, "  %s\n", failure.failedTest()->getName());
            fprintf(fp, "  └─ %s(%d)\n", failure.file(), failure.line());
            
            // 失敗の状況説明
            if (failure.what()) {
                fprintf(fp, "\n失敗の状況:\n");
                fprintf(fp, "  %s\n", failure.what());
            }
            
            fprintf(fp, "\n------------------------\n");
        }
    
    private:
        FILE* m_filePtr;        // 出力ファイルポインタ
        bool m_testSuccess;     // 現在のテストの成功/失敗フラグ
        int m_totalTests;       // 総テスト数
        int m_successCount;     // 成功したテスト数
    
};
int main() {
    int ret = 0;
    TestRunner runner;
    TestResult result;
    
    MyTestListener listener;
    result.addListener(&listener);
    
    FILE* fp = NULL;
    fp = fopen("test_result.log", "w");
    if (fp == NULL) {
        printf("エラー: ログファイルを開けません\n");
        return 1;
    }
    
    printf("\n=== テスト実行開始 ===\n");

    // try-catchブロックを単純化
    TestRegistry registry;
    MessageOutputTest* test = new MessageOutputTest();
    registry.addTest(test->getName(), test);
    registry.setAllAvailable(true);
    registry.runTests(&result);
    
    printf("\n=== テスト結果サマリー ===\n");
    printf("実行数: %d\n", result.runCount());
    printf("失敗数: %d\n", result.failureCount());
    printf("エラー数: %d\n", result.errorCount());
    
// main関数内のログ出力部分
// if (fp != NULL) {
//     fprintf(fp, "\n=== 詳細なテスト結果 ===\n");
//     fprintf(fp, "テスト数: %d\n", result.runCount());
//     fprintf(fp, "成功数: %d\n", result.runCount() - result.failureCount() - result.errorCount());
//     fprintf(fp, "失敗数: %d\n", result.failureCount());
//     fprintf(fp, "エラー数: %d\n", result.errorCount());
//     fprintf(fp, "\n実行結果: %s\n", 
//             result.wasSuccessful() ? "成功" : "失敗");
//     fclose(fp);
// }

    ret = result.wasSuccessful() ? 0 : 1;
    // VC++6.0の場合のみコンソールを一時停止
    #if _MSC_VER == 1200
    printf("\n何かキーを押すと終了します...");
    getchar();
    #endif
    return ret;
}