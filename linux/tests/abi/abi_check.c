#include <stdio.h>
#include "abi.h"

static int fails;
#define CHECK(n, cond) do { if (!(cond)) { printf("ABI FAIL %d: %s\n", n, #cond); fails++; } } while (0)

static double cb(int i1, double d1, int i2, double d2, int i3, double d3,
                 int i4, double d4, int i5, double d5, S16 s, SDD dd)
{ return i1 + i2 + i3 + i4 + i5 + d1 + d2 + d3 + d4 + d5
         + s.a * 100 + s.b * 1000 + dd.a + dd.b; }

int main(void)
{
    S8 s8; S16 s16; SFI sfi; SDD sdd; S24 s24; S3 s3; SF2 sf2; SF4 sf4; SD2 sd2;
    SPK spk; SPK2 spk2; SPKA spka;
    s8.a = 1; s8.b = 2; s16.a = 3; s16.b = 4; sfi.f = 1.5f; sfi.i = 2;
    sdd.a = 2.5; sdd.b = 3.5;
    sf2.v[0] = 1.5f; sf2.v[1] = 2.5f; sd2.v[0] = 3.5; sd2.v[1] = 4.5;
    sf4.v[0] = 1; sf4.v[1] = 2; sf4.v[2] = 3; sf4.v[3] = 4;
    s24.a = 1; s24.b = 2; s24.c = 3; s3.c[0] = 1; s3.c[1] = 2; s3.c[2] = 3;
    spk.c = 3; spk.d = 4.5; spk2.c = 1; spk2.i = 2; spk2.s = 3; spka.a = 4; spka.b = 5;

    CHECK(1, f_int8(1, 2, 3, 4, 5, 6, 7, 8) == 204);
    CHECK(2, f_dbl10(1, 1, 1, 1, 1, 1, 1, 1, 1, 1) == 55.0);
    CHECK(3, f_mixed(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1)
             == 36 + 45 / 1000.0);
    CHECK(4, v_sum_i(9, 1, 1, 1, 1, 1, 1, 1, 1, 1) == 45);
    CHECK(5, v_sum_d(10, 1., 1., 1., 1., 1., 1., 1., 1., 1., 1.) == 55.0);
    CHECK(6, v_mixed(5, 1, 0.5, 2, 0.25, 3, 0.125, 4, 1.0, 5, 2.0)
             == 15000 + 3.875);
    CHECK(7, v_copy(8, 1, 2, 3, 4, 5, 6, 7, 8) == 36 + 3600);

    CHECK(10, p_s8(s8) == 12);
    CHECK(11, p_s16(s16) == 34);
    CHECK(12, p_sfi(sfi) == 17.0);
    CHECK(13, p_sdd(sdd) == 28.5);
    CHECK(14, p_sf2(sf2) == 17.5);
    CHECK(18, p_sf4(sf4) == 1234.0);
    CHECK(19, p_sd2(sd2) == 39.5);
    CHECK(15, p_s24(s24) == 123);
    CHECK(16, p_s3(s3) == 123);
    CHECK(40, p_spk(spk) == 34.5);
    CHECK(41, p_spk2(spk2) == 10023);
    CHECK(42, p_spka(spka) == 45);
    CHECK(43, p_spk_spill(1, 1, 1, 1, 1, 1, spk, 2) == 6 + 300 + 4000 + 20000);
    CHECK(17, p_spill(1, 1, 1, 1, 1, 1, s16, s24, sdd)
             == 6 + 30 + 400 + 1000 + 20000 + 300000 + 2500000 + 35000000);

    s8 = r_s8(7, 8);        CHECK(20, s8.a == 7 && s8.b == 8);
    s16 = r_s16(9, 10);     CHECK(21, s16.a == 9 && s16.b == 10);
    sfi = r_sfi(0.5f, 11);  CHECK(22, sfi.f == 0.5f && sfi.i == 11);
    sdd = r_sdd(1.25, 2.5); CHECK(23, sdd.a == 1.25 && sdd.b == 2.5);
    sf2 = r_sf2(0.25f, 0.75f); CHECK(24, sf2.v[0] == 0.25f && sf2.v[1] == 0.75f);
    sf4 = r_sf4(5, 6, 7, 8);  CHECK(27, sf4.v[0] == 5 && sf4.v[1] == 6 && sf4.v[2] == 7 && sf4.v[3] == 8);
    sd2 = r_sd2(9.5, 10.5);   CHECK(28, sd2.v[0] == 9.5 && sd2.v[1] == 10.5);
    s24 = r_s24(13, 14, 15); CHECK(25, s24.a == 13 && s24.b == 14 && s24.c == 15);
    s3 = r_s3(4, 5, 6);     CHECK(26, s3.c[0] == 4 && s3.c[1] == 5 && s3.c[2] == 6);
    spk = r_spk(7, 8.5);      CHECK(44, spk.c == 7 && spk.d == 8.5);
    spk2 = r_spk2(9, 10, 11); CHECK(45, spk2.c == 9 && spk2.i == 10 && spk2.s == 11);
    spka = r_spka(12, 13);    CHECK(46, spka.a == 12 && spka.b == 13);

    CHECK(30, call_cb(cb) == 15 + 12.5 + 300 + 4000 + 0.75);
    return fails ? 1 : 0;
}
