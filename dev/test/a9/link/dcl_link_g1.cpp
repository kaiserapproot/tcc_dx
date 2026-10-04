// IID_IUnknown = {00000000-0000-0000-C000-000000000046}.  This TU only
// declares it; reading the real bytes proves it refers to the definition in
// dcl_link_g2.cpp and did not emit its own zero-filled copy.
#include <windows.h>
#include <unknwn.h>
int dcl_g2(void);
int main()
{
    return (IID_IUnknown.Data4[0] == 0xC0 && IID_IUnknown.Data4[7] == 0x46
            && dcl_g2() == 0) ? 0 : 1;
}
