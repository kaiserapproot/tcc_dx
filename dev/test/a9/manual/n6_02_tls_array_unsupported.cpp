// N6-02 fail-closed: 受理するのは `thread_local int` と修飾無しの plain class 型
// のみ. 配列 / const / pointer 等は診断で拒否する.
// 期待: "N6-02 supports only thread_local int or a plain class type"
struct P {
    int value;
    P() { value = 1; }
};

thread_local P objects[2];

int main()
{
    return objects[0].value;
}
