/* 引数個数が合わない呼び出しはエラーになること（コンパイル失敗を期待） */
int f(int a) { return a; }
int f(int a, int b) { return a + b; }

int main(void)
{
    return f(1, 2, 3);   /* 一致するオーバーロードなし */
}
