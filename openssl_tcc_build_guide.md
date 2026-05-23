# Windows + Tiny C Compiler (TCC) で OpenSSL 3.5.6 をビルドする手順

実際のビルド作業履歴をもとにまとめた手順です。  
環境: Windows 10、MSYS2シェル使用

---

## 前提環境

| 項目 | パス |
|---|---|
| TCC | `E:\work\work_uma\dev\dev\` (tcc_set.bat あり) |
| OpenSSL ソース | `E:\work\work_uma\dev\src\openssl-3.5.6\` |
| Strawberry Perl | `E:\work\work_uma\dev\src\strawberry-perl-5.42.2.1-64bit-portable\` |
| インストール先 | `E:\work\work_uma\dev\openssl-out\` |
| MSYS2 | インストール済み (Configure/make は MSYS2 シェルで実行) |

---

## 手順1: MSYS2 シェルで環境変数を設定する

MSYS2シェルを開き、以下を実行してTCCにパスを通す。

```bash
export PATH="/e/work/work_uma/dev/dev:$PATH"
export C_INCLUDE_PATH="/e/work/work_uma/dev/dev/include"
export CPATH="/e/work/work_uma/dev/dev/include"
export LIBRARY_PATH="/e/work/work_uma/dev/dev/lib"
```

確認:

```bash
tcc -v
# → "tcc version 0.9.28rc (x86_64 Windows)" と表示されればOK
```

---

## 手順2: MSYS2 に perl と make をインストールする

MSYS2のMSYSシェル（pacman用）で実行:

```bash
pacman -Syu    # MSYS2 本体を更新（ウィンドウが閉じたら再度開いて実行）
pacman -Su
pacman -S perl make
```

> **注意**: `pacman -Syu` でキーリングエラーが出た場合:
> ```bash
> sed -i 's/SigLevel    = Required/SigLevel = Never/' /etc/pacman.conf
> pacman -Syu
> pacman -S perl make
> sed -i 's/SigLevel = Never/SigLevel    = Required/' /etc/pacman.conf
> ```

---

## 手順3: TCC のヘッダを修正する（ビルド前の一回限り）

TCC 付属のヘッダにある GCC/MSVC 固有記述が TCC でコンパイルエラーになるため、事前に修正する。

### 3-1: `intrin-impl.h` に TCC 用スキップガードを追加

```bash
sed -i '1s/^/#ifndef __TINYC__\n/' /e/work/work_uma/dev/dev/include/psdk_inc/intrin-impl.h
echo "#endif /* __TINYC__ */" >> /e/work/work_uma/dev/dev/include/psdk_inc/intrin-impl.h
```

### 3-2: `wspiapi.h` に TCC 用スキップガードを追加

```bash
sed -i '1s/^/#ifdef __TINYC__\n#define _WSPIAPI_H_\n#endif\n/' /e/work/work_uma/dev/dev/include/wspiapi.h
```

### 3-3: `tcclib.h` に `__builtin_prefetch` 定義を追加

```bash
echo "#define __builtin_prefetch(x, ...) ((void)(x))" >> /e/work/work_uma/dev/dev/include/tcclib.h
```

---

## 手順4: OpenSSL ソースを修正する（ビルド前の一回限り）

TCC が認識できない MSVC 専用の整数リテラルサフィックス `UI64` を `ULL` に一括置換する。

```bash
cd /e/work/work_uma/dev/src/openssl-3.5.6

grep -rl "UI64" crypto/ apps/ ssl/ providers/ include/ 2>/dev/null | xargs sed -i 's/UI64/ULL/g'
```

---

## 手順5: Configure を実行する

MSYS2シェルで、Git 付属の Perl（Unix互換パス対応）を使って Configure を実行する。

```bash
cd /e/work/work_uma/dev/src/openssl-3.5.6

"C:\Program Files\Git\usr\bin\perl" Configure mingw64 \
  no-asm \
  no-shared \
  no-tests \
  no-ui-console \
  no-legacy \
  no-quic \
  --prefix=/e/work/work_uma/dev/openssl-out \
  --openssldir=/e/work/work_uma/dev/openssl-out/ssl \
  CC=tcc \
  -DOPENSSL_NO_ASM \
  -DOPENSSL_NO_INLINE_ASM
```

> **なぜ Git の Perl を使うか**: Strawberry Perl は `MSWin32` 版のためバックスラッシュパスを返してしまい、Configure がエラーになる。Git 付属の Perl は MSYS2 と同じ Unix 互換パスを扱える。

| オプション | 理由 |
|---|---|
| `no-asm` | アセンブリコードを無効化（TCC非対応） |
| `no-shared` | 静的ライブラリのみビルド |
| `no-tests` | テスト不要 |
| `no-legacy` | .rc ファイル (windres) が GCC を要求するのを回避 |
| `no-quic` | `SIO_UDP_NETRESET` 等 TCC 未定義シンボルを回避 |

---

## 手順6: Makefile を修正する

Configure 完了後、Makefile を TCC 向けに修正する。

### 6-1: `.res.obj`（windres 生成ファイル）のリンク行から参照を削除

```bash
sed -i 's/[^ ]*\.res\.obj//g' Makefile
```

### 6-2: TCC 標準インクルードを優先させる（IPv6マクロ二重定義の回避）

```bash
sed -i 's/-DOPENSSL_NO_INLINE_ASM/-DOPENSSL_NO_INLINE_ASM -nostdinc -I\/e\/work\/work_uma\/dev\/dev\/include -Iinclude/' Makefile
```

---

## 手順7: ビルドを実行する

```bash
cd /e/work/work_uma/dev/src/openssl-3.5.6

make CC=tcc AR=ar RANLIB=ranlib RC='true' > build.log 2>&1
tail -50 build.log
```

`libcrypto.a`・`libssl.a` が生成されればコンパイル成功。

---

## 手順8: TCC 互換ライブラリ (`tcc_compat.c`) を作成してリンクする

コンパイルは成功してもリンク時に TCC が持たない MSVC/GCC 組み込み関数が不足する。これらをラップするファイルを作成する。

### 8-1: `tcc_compat.c` を作成

```bash
cat > /e/work/work_uma/dev/src/openssl-3.5.6/tcc_compat.c << 'EOF'
#include <windows.h>
#include <stdint.h>
#include <stdlib.h>

/* _Interlocked intrinsics */
long _InterlockedExchange(long volatile *t, long v) { return InterlockedExchange(t, v); }
void* _InterlockedExchangePointer(void* volatile *t, void* v) { return InterlockedExchangePointer(t, v); }
long _InterlockedCompareExchange(long volatile *t, long e, long c) { return InterlockedCompareExchange(t, e, c); }
long _InterlockedExchangeAdd(long volatile *t, long v) { return InterlockedExchangeAdd(t, v); }
__int64 _InterlockedAdd64(__int64 volatile *t, __int64 v) { return InterlockedAdd64(t, v); }
__int64 _InterlockedAnd64(__int64 volatile *t, __int64 v) { return InterlockedAnd64(t, v); }
__int64 _InterlockedOr64(__int64 volatile *t, __int64 v) { return InterlockedOr64(t, v); }
__int64 _InterlockedExchange64(__int64 volatile *t, __int64 v) { return InterlockedExchange64(t, v); }
long _InterlockedOr(long volatile *t, long v) { return InterlockedOr(t, v); }

/* MSVC runtime */
int _fstat64i32(int fd, void *buf) { return 0; }
int _stat64i32(const char *p, void *buf) { return 0; }

/* strtoll/strtoull 実装 (TCC に存在しない場合) */
long long strtoll(const char *s, char **e, int b) {
    long long r = 0, sign = 1;
    while (*s == ' ') s++;
    if (*s == '-') { sign = -1; s++; } else if (*s == '+') s++;
    for (; *s; s++) {
        int d = (*s >= '0' && *s <= '9') ? *s-'0' :
                (*s >= 'a' && *s <= 'z') ? *s-'a'+10 :
                (*s >= 'A' && *s <= 'Z') ? *s-'A'+10 : 99;
        if (d >= b) break;
        r = r * b + d;
    }
    if (e) *e = (char*)s;
    return sign * r;
}
unsigned long long strtoull(const char *s, char **e, int b) {
    return (unsigned long long)strtoll(s, e, b);
}

/* C99 */
intmax_t strtoimax(const char *s, char **e, int b) { return (intmax_t)strtoll(s, e, b); }
uintmax_t strtoumax(const char *s, char **e, int b) { return (uintmax_t)strtoull(s, e, b); }

/* builtins */
void __builtin_prefetch(const void *p, ...) { (void)p; }
void* __builtin_alloca(size_t s) { return malloc(s); }
void _mm_mfence(void) { }
unsigned __int64 __readgsqword(unsigned long o) { (void)o; return 0; }

/* WinSock */
const char* gai_strerrorA(int e) { (void)e; return "gai error"; }

/* Fiber */
BOOL ConvertFiberToThread(void) { return TRUE; }
EOF
```

### 8-2: コンパイルして `_link.txt` に追加

```bash
tcc -m64 -c tcc_compat.c -o tcc_compat.obj

# 不足ライブラリも追加
echo "-ladvapi32"    >> _link.txt
echo "-luser32"      >> _link.txt
echo "-lkernel32"    >> _link.txt
echo "tcc_compat.obj" >> _link.txt
```

### 8-3: 再リンク

```bash
tcc -m64 -o apps/openssl.exe @_link.txt 2>&1 | tail -30
```

---

## 手順9: 動作確認

MSYS2シェルからは出力が見えないため、cmd.exe 経由で確認する。

```bat
cmd.exe /c "E:\work\work_uma\dev\src\openssl-3.5.6\apps\openssl.exe version & echo ExitCode=%ERRORLEVEL%"
```

`ExitCode=0` が返れば **ビルド成功**。

---

## 手順10: インストール

```bash
make install CC=tcc AR=ar RANLIB=ranlib RC='true' > install.log 2>&1
tail -20 install.log
```

インストール先: `E:\work\work_uma\dev\openssl-out\`

---

## トラブルシューティング早見表

| エラー内容 | 対処 |
|---|---|
| `This perl implementation doesn't produce Unix like paths` | Git の Perl (`C:\Program Files\Git\usr\bin\perl`) を使う |
| `Locale::Maketext::Simple` not found | 手順5のとおり Git の Perl を使えば発生しない |
| `windres: preprocessing failed` (gcc/cc1 not found) | Configure に `no-legacy no-quic` を追加し、Makefile から `.res.obj` を除去、`RC='true'` でビルド |
| `'IN6_IS_ADDR_*' defined twice` | Makefile に `-nostdinc -I/e/.../dev/include -Iinclude` を追加し `make clean` 後に再ビルド |
| `UI64` 構文エラー | `grep -rl "UI64" ... | xargs sed -i 's/UI64/ULL/g'` で一括置換 |
| `undefined symbol '_InterlockedExchange'` 等 | `tcc_compat.c` を作成してリンク（手順8） |
| `undefined symbol 'strtoll'` | `tcc_compat.c` の `strtoll`/`strtoull` 実装を使う（手順8） |
| MSYS2 の pacman がキーエラー | `SigLevel = Never` に変更して `pacman -Syu` 後に元に戻す |

---

## まとめ: ビルド成功の流れ

```
[MSYS2] 環境変数設定 (PATH/CPATH/LIBRARY_PATH)
    ↓
[MSYS2] MSYS2 に perl/make をインストール
    ↓
[MSYS2] TCC ヘッダ修正 (intrin-impl.h / wspiapi.h / tcclib.h)
    ↓
[MSYS2] OpenSSL ソース修正 (UI64 → ULL 一括置換)
    ↓
[MSYS2] Configure (Git Perl + no-asm/no-legacy/no-quic)
    ↓
[MSYS2] Makefile 修正 (.res.obj 削除 / -nostdinc 追加)
    ↓
[MSYS2] make CC=tcc AR=ar RANLIB=ranlib RC='true'
    ↓
[MSYS2] tcc_compat.c 作成 → コンパイル → _link.txt に追加
    ↓
[MSYS2] tcc -m64 -o apps/openssl.exe @_link.txt
    ↓
[cmd.exe] openssl.exe version → ExitCode=0 ✓
    ↓
[MSYS2] make install
```
