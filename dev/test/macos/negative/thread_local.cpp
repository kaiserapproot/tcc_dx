// thread_local's runtime is built on Win32 TLS (TlsAlloc / CreateThread
// hooks) and is not ported to macOS: it must be rejected, not miscompiled.
thread_local int counter;

int main()
{
    counter = 1;
    return counter - 1;
}
