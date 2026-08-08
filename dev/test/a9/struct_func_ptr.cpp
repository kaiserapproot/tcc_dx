typedef long long (*callback_t)(void *, unsigned, unsigned long long, long long);

struct Holder
{
    callback_t callback;
};

long long invoke(Holder *holder, void *handle, unsigned message,
                 unsigned long long first, long long second)
{
    return holder->callback(handle, message, first, second);
}

long long sample(void *handle, unsigned message,
                 unsigned long long first, long long second)
{
    return handle != 0 || message != 2 || first != 3 || second != 4;
}

int main()
{
    Holder holder;
    holder.callback = sample;
    return (int)invoke(&holder, 0, 2, 3, 4);
}
