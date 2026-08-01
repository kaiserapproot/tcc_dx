/* 抽象クラスのインスタンス化はエラーになること（コンパイル失敗を期待） */
class Iface {
public:
    virtual int f() = 0;
};

int main(void)
{
    Iface x;      /* 純粋仮想関数が未実装 */
    return 0;
}
