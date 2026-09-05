// N6-02 fail-closed: polymorphic class. vptr は宣言地点のコード
// (cpp_init_local_vptr / cpp_init_global_vptr) が書くため, runtime heap 上の
// TLS object では vptr が null のままになる -> 受理しない.
// 期待: "thread_local polymorphic class object is unsupported in N6-02"
struct V {
    int value;
    V() { value = 1; }
    virtual int get() { return value; }
};

thread_local V object;

int main()
{
    return object.get();
}
