# Amateras対応記録

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

### `cross_tpp.h`の前処理構造エラー

`cr_win()`内で、Windows分岐を閉じる`#endif`の後に`#elif`が続いている。

対象: `amateras/inc/SHIFT_JIS/cross_tpp.h` 351行付近

```cpp
#endif
// cr_win():end:0
#elif TARGET_OS_MAC && !(TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR)
```

これはTCC以前に不正なプリプロセッサ構造であり、Windows向け`window_t`テストはここで停止する。

### `cross.h`の残問題

`cross.h`では`-DSTBI_NO_SIMD`を指定するとMMD/OpenGLコードの解析が進むが、`mmd_gl_free_texture()`内の次の代入で停止する。

```cpp
tex->gl_tex_id = 0;
```

この問題はWindows SDKヘッダーではなく、Amateras側の大規模MMD/OpenGL実装とTCCの組み合わせに属する。

## 次の対応候補

1. `cross_tpp.h`の`cr_win()`周辺の重複`#elif`を修正する。
2. 修正後、`cross_tpp.h`専用の`window_t` C++テストを追加する。
3. `STBI_NO_SIMD`をTCC利用時に自動設定するか、MMD/OpenGL実装をTCC対象外にする。
4. `mmd_gl_free_texture()`のメンバー代入停止原因を最小再現テストで切り分ける。
