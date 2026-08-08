# amateras 対応 作業履歴

対象期間: 2026-08-08 〜 2026-08-09
ブランチ: `fix/cpp-shadowed-type-name`（基点 `d761d92` = `fix/tcc-compatible-crt-headers` の HEAD）

関連文書: [amateras対応プラン.md](amateras対応プラン.md)（rev.5） / [amateras対応.md](amateras対応.md) / [レビュー.md](レビュー.md)

---

## 0. 到達点（一言）

**amateras が tpp の TCC でビルド・実行でき、C++ ソース（`.cpp`）から MMD モデルを描画できるようになった。**
描画結果は既存の C 版ビューア `model_view.exe` と同一（顔・目・ヘアバンドのテクスチャまで一致）。

---

## 1. コミット一覧（tpp 側）

| コミット | 内容 |
|---|---|
| `ad1a4e7` | fix(cpp): BUG-20 C++ の名前隠蔽規則に従って型名を探索する |
| `0f67088` | docs(amateras): 対応プラン rev.5 と実施結果を記録 |
| `a244e65` | fix(cpp): BUG-21/22 メンバ関数内の名前解決と暗黙 this を修正 |

---

## 2. 経緯

### 2.1 調査と計画（rev.1 → rev.5）

初期調査時の [amateras対応.md](amateras対応.md) は `cross.h` の停止を
「amateras 側の MMD/OpenGL 実装の問題」と結論づけていたが、**これは誤りだった**。
実際は tpp 側の C++ 名前隠蔽未実装で、amateras のコードを一切使わない 4 行で再現する。

計画は 3 回のレビューを受け、指摘を全件実機検証して rev.5 まで改訂した。主な訂正:

| レビュー指摘 | 結果 |
|---|---|
| 生成コマンドが再現できない | **誤指摘**。正本は `base_inc/only/tcc/tcc_only.h`。`-merge` で**バイト完全一致**を確認 |
| 生成物 `inc/**` の直接編集は規約違反 | 正しい。直接編集案を撤回（AGENTS.md:67） |
| amateras が dirty worktree | 正しい。事前ゲートを追加 |
| 真偽値ヘルパーでは不十分 | 正しい。**現状すでに壊れている**ことを実測（ケース P/Q）し、解決済みシンボルを返す設計へ変更 |
| `decl()` の修飾名探索は別扱い | 正しい。rev.3 は内部矛盾していた |
| 負例と C テストが実行されない | 正しい。`run_all.bat` は `a9\*.cpp` しか列挙していなかった |
| 「正本は CP932 で編集」 | **私の重大な誤り**。正本 3 ファイルは UTF-8 BOM なしで、lint も同ゾーンに `utf8-nobom` を要求 |

MSVC（VS2022）を参照実装として使い、「TCC が拒否するコードが本当に合法な C++ か」を各ケースで裏取りした。

### 2.2 フェーズ 1 — BUG-20（`ad1a4e7`）

`cross.h:33459` の `tex->gl_tex_id = 0;` が「identifier が必要です」で停止していた。

`cross.h:1116` に C++ 限定の `struct tex` があり、引数 `tex` がそれを隠すべきところ、
`parse_btype()` が `struct_find()`（タグ名前空間・スコープ隠蔽の対象外）を無条件に優先し、
内側スコープの引数を無視していた。

共通ヘルパー `cpp_lookup_type_name()` を新設して C++ 非修飾名探索の規則を集約し、
`parse_btype()` と `cpp_tok_starts_type_name()` の 2 経路を差し替えた。
`decl()` の修飾名検出（`X::member`）は C++ では type/namespace 限定探索が正しいため意図的に据え置いた。

**この 1 件で `cross.h` 全体が通るようになり**、計画がフェーズ 3 の反復作業と見込んでいた分も解消した。

### 2.3 フェーズ 2 — amateras 正本の修正

正本のみ修正して再生成（`inc/**` は直接編集していない。UTF-8 BOM なしを維持）。

| 正本 | 内容 |
|---|---|
| `base_inc/mmd/render/mmd_image_load.h` | `__TINYC__` で `STBI_NO_SIMD` を定義。`-DSTBI_NO_SIMD` の指定が不要に |
| `base_inc/platform/win/win_imp.h` | `cr_win()` 末尾の重複ブロック 5 行を削除。併せて `init()` の `win_p = new_win;` を `SET_WIN_P(new_win);` へ（`win_p` は TLS 取得マクロで lvalue ではない） |
| `base_inc/only/tcc/tcc_only.h` | `win_define.h` を `cross_global_value.h` より前へ移動（`<windows.h>` の供給元が後ろにあり単体 include できなかった） |
| `base_inc/cross_define.h` | `__TINYC__` 向けの `extern double sqrt(double)` を `extern "C"` で囲む（C++ でマングルされ `__tcc_sqrt_d` 未定義になっていた） |

再生成前に、現行 `inc/cross.h` が現行 `base_inc` からの再生成と**バイト完全一致**であることを確認し、
未コミットの他者作業を壊さないことを検証した。

### 2.4 検証 — ビルドと実行

| 検証 | 結果 |
|---|---|
| `cross.h` C モード（`-DSTBI_NO_SIMD` なし） | OK |
| `cross.h` C++ モード（同上） | OK |
| amateras C ヘッダテスト build / run | OK / OK |
| CUnit `test_cross.c` build / run | OK / **9/9 テスト・68/68 アサーション** |
| `encoding_lint.ps1` | **FAIL=0** |

### 2.5 ウィンドウ表示の実証

`cross.h` だけでウィンドウを生成し、実画面をキャプチャして確認した。

- タイトル `Cross Platform Window` / クラス名 `cross_` / 可視 True / クライアント 1200x800
- `WM_TIMER` → `WM_CLOSE` → `WM_DESTROY` で自走終了、プロセス残存なし

### 2.6 C++ からの MMD 描画 — BUG-21 / BUG-22（`a244e65`）

C++ TU から `cross.h` を使うと `init()` で即クラッシュした。原因は cross.h の設計にあった:

```c
#ifdef __cplusplus
struct window_t          // C++ では struct を開いたままにし
#else
typedef struct window_t
#endif
{ ... };                 // init()/reg_win()/cr_win()/... をメンバ関数として取り込む
```

さらに `init_opengl()`/`begin_frame()`/`end_frame()` は
`struct render_opengl : public window_t` のメンバである。
つまり C++ からは `render_opengl` オブジェクト経由で呼ぶのが正しい使い方で、
この経路が下記 2 件の実装漏れを必ず踏んでいた。

**BUG-21: メンバ関数内でクラススコープがグローバルより優先されていなかった**
tcc はクラス内インライン本体を全てグローバル関数へ引き上げるため、
「別クラスのメソッド名」が自クラスのデータメンバを隠していた。
cross.h では `win_txt::clear()` が `window_t::clear[4]` を隠し、
`clear[0] = 0.0f` が「lvalue が必要です」で失敗していた。

**BUG-22: 無修飾のメンバ関数呼び出しに `this` が渡っていなかった**
`init()` は `return init_common(...)` だが、引き上げ済みグローバルの第 1 引数は `this` であるため、
引数が 1 つ足りないまま呼ばれ実行時に落ちていた。
`this->f()` の形へ書き換えて既存のメンバ呼び出し経路へ流すことで、
仮想ディスパッチと MI の基底オフセットも維持したまま解決した。

ヘッダ側も 1 件修正した。

- `dev/include/GL/glu.h` — `class GLUnurbs;` 等が `extern "C" {` の内側にあり、
  tcc の `extern "C"` は C++ キーワードを識別子化するため解析できない。
  `__TINYC__` では同等の struct typedef を使う C 側分岐へ回した（`db3dee2` と同じ方式）。

### 2.7 MMD 描画の結果

```
MMD loaded from a C++ TU: vtx=10441 idx=48744 batches=15 textures=10
bbox = (-6.93 0.09 -1.60) - (6.93 19.12 1.68)
rendered 45 frames
```

モデル: `assets/alice_pmd/Alice.Pmx`。C 版ビューア `model_view.exe` と同一の見た目
（テクスチャ・トゥーン陰影・輪郭線）になることを画面キャプチャで確認した。

**注意点として判明したこと**: `begin_frame`/`end_frame` は基底 `window_t` 版と
派生 `render_opengl` 版の 2 つが存在し、基底版は `active_kind` が `RENDER_BACKEND_NONE` のとき
何もしない。C++ から使う場合はどちらが選ばれるか曖昧になるため、デモでは
`glClearColor`/`glClear` と `SwapBuffers` を直接呼んで回避した。

---

## 3. 追加した回帰テスト

| ファイル | 内容 |
|---|---|
| `dev/test/a9/bug20_shadow_param.cpp` | 引数・ローカル変数による型名隠蔽（`->` / `.` / 単純代入） |
| `dev/test/a9/bug20_inner_typedef.cpp` | 内側 typedef が外側 class を隠す |
| `dev/test/a9/bug20_global_ctor_arg.cpp` | グローバル ctor 引数。**値まで実行検証**（黙った誤パースを見逃さないため） |
| `dev/test/a9/bug20_scope_restore.cpp` | 修正で壊してはいけない正常系の固定 |
| `dev/test/a9/bug20_c_mode.c` | C モード非回帰 |
| `dev/test/a9/negative/bug20_hidden_as_type.cpp` | 隠蔽名を型として使う不正コードの拒否 |
| `dev/test/a9/bug21_member_vs_global.cpp` | 別クラスのメソッド名によるメンバ隠蔽 |
| `dev/test/a9/bug22_member_calls_member.cpp` | メンバ→メンバ呼び出し、仮想、基底継承 |

`run_all.bat` も改修した。

- Phase 1 の列挙に **`a9\*.c` を追加**（C モード検証が実際にはゲートされていなかった）
- **負例ゲート（Phase 3）を新設**。コンパイル成功なら FAIL、期待診断とも照合する。
  照合は ASCII のみ（`ファイル名:行:error:`）。tcc の診断は CP932 日本語で、
  UTF-8 の `.expected` と本文比較すると文字コードが食い違うため。
- 負例ゲートが「通ってしまう」「違う診断で落ちる」の**両方の失敗モードを検出する**ことを、
  わざと壊したプローブで確認済み。

---

## 4. 現在の状態

### tpp 側 — コミット済み

ブランチ `fix/cpp-shadowed-type-name` に 3 コミット。作業ツリーは本ファイル以外きれい。
`build.bat` は Release x64 / Debug Win32 の両方で緑、**0 gating failure**。

### amateras 側 — **未コミット**（作業ツリーに残置）

```
 M base_inc/cross_define.h              <- 今回の変更
 M base_inc/mmd/render/mmd_image_load.h <- 今回の変更
 M base_inc/only/tcc/tcc_only.h         <- 今回の変更
 M base_inc/platform/win/win_imp.h      <- 今回の変更
 M inc/SHIFT_JIS/cross.h                <- 再生成（他者の進行中作業を含む）
 M inc/SHIFT_JIS/cross_tpp.h            <- 再生成
 M inc/UTF_8/cross.h                    <- 再生成（他者の進行中作業を含む）
 M inc/UTF_8/cross_tpp.h                <- 再生成
 --- 以下は今回の作業とは無関係（着手前から存在） ---
AM base_inc/3d_format/統合ビューアusase.md
 M base_inc/mmd/render/mmd_draw_helper.h
 M base_inc/mmd/render/opengl/mmd_gl_shader_draw.h
?? base_inc/3d_format/統合ビューア統合作業履歴.md
?? test/build_run_model_viewer.ps1
?? test/tcc/viewer/
```

**コミットしていない理由**: amateras は `master` 上に進行中の未コミット変更を持っており、
`inc/cross.h` は他者の作業の再生成結果と今回の変更が同一ファイル内で混ざる。
他者の作業を巻き込むコミットは避けた。

---

## 5. 残作業

### 5.1 amateras 側のコミット（要判断）

- 作業ブランチを切り、`base_inc` の今回分 4 ファイル + 再生成物を分けてコミットする。
- `inc/cross.h` は他者の進行中作業の再生成結果を含むため、**帰属の確認が必要**。

### 5.2 tpp 側の既知ギャップ（別ブランチ・1 件 1 ブランチ）

いずれも `this->` 明示形でも再現するため今回の修正の回帰ではない。

| # | 内容 | 最小再現 | 優先度 |
|---|---|---|---|
| BUG-23 | 引数式が `this` を参照する呼び出しが落ちる。`this` を vstack 外の `cpp_member_this` へ退避するため引数評価で壊れる（BUG-18 と同じ根） | `struct C{int v;int b(int x){v=x;return 1;}int t(){return b(v+1);}};` | **高**（実用コードで頻出） |
| BUG-24 | `virtual ~Base()`（仮想デストラクタ）が解析できない | `struct B{int t;virtual ~B(){}};` | 高（多態的な破棄に必須） |
| BUG-25 | 同一グローバルスコープで `struct tex{}; int tex;` が誤 `redefinition`。暗黙 typedef 注入（`tccgen.c:8345-8354`）が原因。逆順・関数内なら通る | `struct tex{float u,v;}; int tex=5;` | 低（amateras 未踏） |
| BUG-26 | グローバル変数を「どのクラスのメソッド名」とも同名にできない。引き上げ済み本体がその名前を占有し redefinition になる | `struct O{void w(){}}; int w=7;` | 中 |
| — | `extern "C"` が C++ キーワードの字句解析を止める。`extern "C"` 内の `class` 前方宣言が書けず、SDK ヘッダを個別に guard する必要がある（今回 `glu.h` を対応） | `extern "C" { class X; }` | 中（SDK 互換の根本原因） |

### 5.3 amateras 側の未解決

`cross_tpp.h` は単体 include できるようになった（依存順は解消）が、その先の `win_imp.h` で
**`window_t` 構造体と関数本体が食い違っている**ためコンパイルできない。

- 構造体の宣言は旧形: `window_class_name[256]` / `char *title` / `hthread`
- 関数本体は新形を使用: `class_name.buf` / `title.buf` / `thread_id`

`win_imp_wip.h` の新 `window_t` への移行途中と見られ、TCC とは無関係。
`cross_tpp.h` は現状どこからも参照されていないため実行経路には影響しない。

---

## 6. 次の対応（推奨順）

1. **BUG-23 を直す**。`this` を vstack 上に保持する形へ変え、引数評価で壊れないようにする。
   実用的な C++ コードで最も踏みやすく、これが無いと `cross.h` の C++ 経路も
   書き方を選ばないと動かない。BUG-18 の対応（仮想呼び出しで address を vstack に残した）が参考になる。
2. **BUG-24（仮想デストラクタ）を直す**。多態的なクラス階層を扱うなら必須。
3. **amateras 側の変更をコミットする**（§5.1 の帰属確認後）。
4. `begin_frame`/`end_frame` の基底・派生の二重定義を amateras 側で整理するか、
   C++ からの推奨手順を正本にコメントで明文化する（今回はデモ側で回避した）。
5. `cross_tpp.h` の `window_t` 不整合（§5.3）は amateras 側の別作業として棚卸しする。
6. 余力があれば `extern "C"` の実装を「リンケージのみ変更」へ改め、
   SDK ヘッダを個別 guard しなくても済むようにする（`glu.h` のような対応が不要になる）。
