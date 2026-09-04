struct Owner {
    Owner(int value) {}
    ~Owner() {}
};

int main(void)
{
    switch (1) {
    case 0:
        Owner a(2);
    default:
        break;
    }
    return 0;
}