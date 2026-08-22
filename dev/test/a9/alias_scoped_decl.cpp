// A class-alias typedef used as the qualifier of a nested-type local
// declaration - the TestResult.cpp:25 shape: cuconfig.h forward-declares
// SimpleList, typedefs it to cu_List, and the dtor body then declares
// `cu_List::iterator p;`.  cpp_unget_scoped_expr only consulted
// struct_find, so the alias was not seen as a class and the declaration
// was mis-fed to gexpr ("static member not found").
class SimpleList;
typedef ::SimpleList cu_List;
class SimpleList {
  public:
    struct It { int x; };
    typedef It iterator;
};
struct TR {
    ~TR();
    int y;
};
int side = 0;
TR::~TR()
{
    cu_List::iterator p;
    p.x = 41;
    side = p.x + y;
}
int main()
{
    {
        TR t;
        t.y = 1;
    }
    return side == 42 ? 0 : 1;
}
