# Amateras対応記録

## 2026-08-08: 実施完了 — amateras がビルド・実行できるようになった

対応プランは [amateras対応プラン.md](amateras対応プラン.md)（rev.5）。実施結果は次のとおり。

| 検証 | 結果 |
|---|---|
| `cross.h` C モード（`-DSTBI_NO_SIMD` **なし**） | **OK** |
| `cross.h` C++ モード（同上） | **OK** |
| amateras C ヘッダテスト build / run | **OK / OK**（exit 0） |
| CUnit `test_cross.c` build / run | **OK / OK**（9/9 テスト・68/68 アサーション） |
| tpp `build.bat`（Release x64 + Debug Win32 + 全テスト） | **緑**（0 gating failure） |
| amateras `encoding_lint.ps1` | **FAIL=0** |

修正は 2 箇所に分かれた。

**tpp 側**（コミット `52d7be6`, ブランチ `fix/cpp-shadowed-type-name`）
C++ の名前隠蔽規則に従って型名を探索するよう `tccgen.c` を修正（BUG-20）。
これ 1 件で `cross.h` 全体が通るようになり、プランがフェーズ 3 の反復作業と
見込んでいた分も解消した。

**amateras 側**（正本のみ修正 → 再生成。`inc/**` は直接編集していない）

- `base_inc/mmd/render/mmd_image_load.h` — `__TINYC__` で `STBI_NO_SIMD` を定義。
  これにより `-DSTBI_NO_SIMD` の指定が不要になった。
- `base_inc/platform/win/win_imp.h` — `cr_win()` 末尾の重複ブロック 5 行を削除。
  併せて `init()` の `win_p = new_win;` を `SET_WIN_P(new_win);` に修正
  （`win_p` は TLS 取得マクロで lvalue ではない。`問題と原因.md` の
  `message_loop_thread` と同じ TLS 化以前の残り）。
- `base_inc/only/tcc/tcc_only.h` — `win_define.h` を `cross_global_value.h` より前へ移動。

### 未解決（対象外と判断）

`cross_tpp.h` は単体 include できるようになった（依存順の問題は解消）が、その先の
`win_imp.h` で **`window_t` 構造体と関数本体が食い違っている**ため、まだコンパイルできない。

- 構造体の宣言は旧形: `window_class_name[256]` / `char *title` / `hthread`
- 関数本体は新形を使用: `class_name.buf` / `title.buf` / `thread_id`

これは amateras 側の未完リファクタ（`win_imp_wip.h` の新 `window_t` への移行途中）で、
TCC とは無関係。`cross_tpp.h` は現状どこからも参照されていないため実行経路には影響しない。

## 今回の対応

AmaterasのWindows向けC++コードをTCCで解析・ビルドするため、TCC側のCRT/SDKヘッダーとC++パーサーを修正した。

対象ブランチ・コミット:

- ブランチ: `fix/tcc-compatible-crt-headers`
- コミット: `1504c6e fix(cpp): support TCC-compatible Windows headers`

## 実装した箇所

### TCCヘッダー

- `_mingw_secapi.h`, `stdlib.h`, `math.h`, `swprintf.inl`
  - TCC (`__TINYC__`) ではC++テンプレートと`extern "C++"`オーバーロードを無効化。
- `corecrt.h`
  - TCC C++でも`wchar_t`型を定義。
- `combaseapi.h`, `unknwnbase.h`, `objidl.h`, `msxml.h`, `propidl.h`, `oleauto.h`
  - COMのC++インターフェース、テンプレート、クラス前方宣言をTCCではC形式へ切り替え。
  - `CINTERFACE`をTCC向けに設定。
- `guiddef.h`, `_mingw.h`
  - `__uuidof`テンプレート展開をTCCでは無効化。
- `winbase.h`, `winuser.h`, `winnt.h`
  - TCCで解析できないC++継承・インラインintrinsic実装を回避。
- `psdk_inc/intrin-impl.h`
  - TCCではGCC/MSVC向けintrinsic inline実装を無効化。
- Direct3D helperクラスを無効化するため、TCC時に`D3D10_NO_HELPERS` / `D3D11_NO_HELPERS`を定義。

### TCC C++パーサー (`tccgen.c`)

- C++ `struct` にも、`class`と同じメンバー関数・コンストラクタ処理を適用（既定アクセスはpublic）。
- 既存typedefがあるC++ `struct`では重複typedefを作らない。
- ポインタtypedefをクラス型と誤認する問題を修正。
- クラス型を戻り値に持つグローバル関数を、グローバルコンストラクタ構文と誤認しないよう修正。
- 構造体メンバーの関数ポインタ呼び出しで、不要なC++オーバーロード解決を行わないよう修正。

### 回帰テスト

- `dev/test/a9/crt_header_compat.cpp`
- `dev/test/a9/struct_member_compat.cpp`
- `dev/test/a9/struct_func_ptr.cpp`

## 検証結果

- `build.bat`:
  - Release x64ビルド成功
  - Debug Win32ビルド成功
  - 既存回帰テスト成功（gating failure 0件）
- `windows.h`単体のC++ヘッダー解析・コンパイル成功。
- `cross.h`はWindows/COM/Direct3Dヘッダー解析後、MMD/OpenGL実装部で停止。
- `cross_tpp.h`は、`windows.h`を先にインクルードしても前処理構造エラーで停止。

## 現在の問題

> **2026-08-08 追記**: 以下 2 件は実機で原因を確定した。詳細・対応手順は
> [amateras対応プラン.md](amateras対応プラン.md)（rev.3）を参照。本節は当時の記録として残す。

### `cross_tpp.h`の前処理構造エラー

`cr_win()`内で、Windows分岐を閉じる`#endif`の後に`#elif`が続いている。

対象: `amateras/inc/SHIFT_JIS/cross_tpp.h` 351行付近

```cpp
#endif
// cr_win():end:0
#elif TARGET_OS_MAC && !(TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR)
```

これはTCC以前に不正なプリプロセッサ構造であり、Windows向け`window_t`テストはここで停止する。

**確定した原因（2026-08-08）**: プラットフォーム分岐の終端 5 行が丸ごと二重化している。
生成物ではなく生成元 `base_inc/platform/win/win_imp.h` の 172-181 行が発生源で、
`-merge` 経由で SHIFT_JIS 版・UTF_8 版の両方へ伝播していた。
修正は**正本のみ**に行い再生成する（`inc/**` は生成物のため直接編集禁止）。

### `cross.h`の残問題

`cross.h`では`-DSTBI_NO_SIMD`を指定するとMMD/OpenGLコードの解析が進むが、`mmd_gl_free_texture()`内の次の代入で停止する。

```cpp
tex->gl_tex_id = 0;
```

~~この問題はWindows SDKヘッダーではなく、Amateras側の大規模MMD/OpenGL実装とTCCの組み合わせに属する。~~

**確定した原因（2026-08-08）— 上記の見立ては誤りだった。これは TCC 側のパーサバグである。**

C++ の名前隠蔽規則（変数・引数が同名のクラス名を隠す）が未実装であることが原因。
`cross.h:1116` に C++ モード限定の `struct tex` があり、引数 `tex` がそれを隠すべきところ、
`parse_btype()`（`tccgen.c:8626-8665`）が文頭の識別子を `struct_find()`（タグ名前空間・
スコープ隠蔽の対象外）で先に型解決し、内側スコープの引数を無視する。
MMD/OpenGL 実装とは無関係で、amateras のコードを一切使わない次の 4 行で再現する。

```cpp
struct tex { float u, v; };
struct T { unsigned int id; };
void f(T* tex) { tex->id = 0; }   // error: identifier が必要です
int main() { return 0; }
```

## 次の対応候補

> **2026-08-08**: 下記は当時の候補。確定版の手順は
> [amateras対応プラン.md](amateras対応プラン.md) のフェーズ 1〜3 を参照。

1. ~~`cross_tpp.h`の`cr_win()`周辺の重複`#elif`を修正する。~~
   → 発生源を `base_inc/platform/win/win_imp.h:172-181` と特定。正本を修正して再生成する。
   生成コマンドは `generate_cross.exe -merge -o .\inc base_inc\only\tcc\tcc_only.h cross_tpp.h`
   （現物とバイト完全一致を確認済み）。
2. 修正後、`cross_tpp.h`専用の`window_t` C++テストを追加する（継続）。
3. ~~`STBI_NO_SIMD`をTCC利用時に自動設定するか、MMD/OpenGL実装をTCC対象外にする。~~
   → MMD/OpenGL の除外は不要。`base_inc/mmd/render/mmd_image_load.h` の stb include 直前で
   `__TINYC__` 条件付きに `STBI_NO_SIMD` を定義する（第三者コード `ext/stb/stb_image.h` は触らない）。
4. ~~`mmd_gl_free_texture()`のメンバー代入停止原因を最小再現テストで切り分ける。~~
   → **完了**。上記のとおり TCC 側の C++ 名前隠蔽未実装と確定。最小再現も作成済み。
