// BUG-33: static members declared in a header and defined in ANOTHER TU
// (the TestUtility::trimFileName shape).  The call site must emit an
// extern reference instead of failing with "static member not found".
class Util {
public:
    static const char* trim(const char* s);
    static int scale(int v);
    static const int factor;
};
