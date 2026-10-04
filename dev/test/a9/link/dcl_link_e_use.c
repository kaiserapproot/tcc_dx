/* C side of dcl_link_e_def.cpp: calls the two functions by their C names. */
int dcl_e_block(int v);
int dcl_e_single(int v);
int dcl_e_cpp_side(void);

int main(void)
{
    if (dcl_e_block(1) != 2) return 1;
    if (dcl_e_single(1) != 3) return 2;
    if (dcl_e_cpp_side() != 0) return 3;
    return 0;
}
