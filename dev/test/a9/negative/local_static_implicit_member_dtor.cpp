struct Member {
    ~Member() {}
};

struct Owner {
    Member member;
};

Owner &get_owner()
{
    static Owner value;
    return value;
}

int main()
{
    get_owner();
    return 0;
}
