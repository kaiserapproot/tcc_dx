extern "C" {

typedef struct ec_s {
    int x;
} ec_s_t;

int ec_s(void);

}

int main(void)
{
    ec_s_t v;
    v.x = 0;
    return 0;
}
