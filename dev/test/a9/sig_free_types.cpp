// Free function overloads resolved by raw argument type.
int pick(int a) {
    return a;
}

int pick(double d) {
    return (int)d + 100;
}

int pick(const char *s) {
    return s[0] - '0' + 200;
}

int main() {
    int r;

    r = pick(3);          /* 3 */
    r += pick(2.5);       /* + 102 */
    r += pick("7");       /* + 207 */
    return r - 312;
}
