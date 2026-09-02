struct Owner {
    Owner(int value) {}
    ~Owner() {}
};

int main(void)
{
    switch (1) {
    case 0:
        Owner a(1);
    case 1:
        break;
    }
    return 0;
}