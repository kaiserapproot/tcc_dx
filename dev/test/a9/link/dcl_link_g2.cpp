// INITGUID makes DEFINE_GUID expand to `extern "C" const GUID name = {...}`,
// the one definition.
#include <initguid.h>
#include <windows.h>
#include <unknwn.h>
int dcl_g2(void) { return IID_IUnknown.Data4[7] == 0x46 ? 0 : 1; }
