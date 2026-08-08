# amateras ビルド対応プラン（rev.5 / 2026-08-08）

レビュー指摘を全件実機検証し、確認できた事実のみで再構成した。
検証コマンドと結果は各項に併記する。**未確認のまま残した事項は §2.8 に明示する。**

参照実装として MSVC（VS2022 Professional / `cl /EHsc /c`）を用い、
「TCC が拒否するコードが本当に合法な C++ か」を各ケースで裏取りした（§2.2 の MSVC 列）。

**rev.1 → rev.2 の変更**

- cross_tpp.h の正本と生成コマンドを特定し、**バイト完全一致で再現できることを実証**（レビュー P0 は入力ヘッダの取り違えによる誤指摘）。
- 生成物 `inc/**` の直接編集案を撤回（AGENTS.md 違反）。
- amateras の dirty worktree 保護ゲートを追加。
- 名前隠蔽の破綻範囲を実機マトリクスで確定し、修正スコープを事実に合わせて縮小・明確化。
- 新ブランチ基点を `d761d92` に明記。
- stb の SIMD 無効化を第三者コード改変から `mmd_image_load.h` へ変更。
- 新規発見（レビュー・初版のいずれにも無い）: 同一スコープ隠蔽での誤 "redefinition"。

**rev.2 → rev.3 の変更**

- §2.4 の `redefinition` の原因を **[tccgen.c:8345-8354](tccgen.c#L8345-L8354) の暗黙 typedef 注入**と特定（rev.2 では「推定・未確定」だった）。
- 追加実測ケース J / L により、同問題が「**同一グローバルスコープ・struct 定義が先**」の順序でのみ起きる狭い条件と確定。
- **`cpp_tok_starts_type_name()` の実害を確認（ケース M）。rev.2 の「実害未確認・優先度下げ」を撤回し、必須へ格上げ。**
  グローバル ctor `Foo g(tex);` が関数宣言として**黙って誤パース**され、エラー位置が後方へずれる。
  レビュー P1 の結論は正しかった（ただし到達経路はレビューの例とは異なる）。
- マトリクスを 13 ケースへ拡張。J / L / N を「現状 PASS を維持すべき負例」として回帰テストに追加。

**rev.3 → rev.4 の変更（レビュー第 2 回の指摘を反映）**

- **[P1-1 採用] 真偽値ヘルパー設計を撤回**。「型か否か」では
  「内側 typedef が外側 class を隠す」ケース（新規実測 P / Q — **現時点で既に壊れている**）を
  救えないため、**解決済みシンボルを返すヘルパー**へ設計変更（§3.1）。
- **[P1-1 採用] `decl()` 13079 を共通ヘルパーの対象から除外**。ここは `X::member` の
  修飾名検出であり、C++ では type/namespace 限定探索で通常の隠蔽規則とは意味が異なる。
  rev.3 は §3.1 項目 3 で「限定探索が正しい」と書きながら同じヘルパーの適用先にも挙げており、
  **内部矛盾していた**（§3.1）。
- **[P1-2 採用] `run_all.bat` の実態に合わせてテスト計画を修正**。現行は `a9\*.cpp` しか
  列挙せず `.c` は対象外、負例の置き場所も無く、`KNOWNFAIL` は失敗の継続を検証しない。
  ランナー側の改修を明示タスク化（§3.2）。**実行ゲートは exit 0 必須**のため
  テストプログラムは成功時 0 を返す設計にする（rev.3 のケース M 案は 5 を返す誤りだった）。
- **[P1-3 決着] `cross_tpp.h` は (a) 自己完結化を採用**。実測により、正本 `tcc_only.h` で
  `win_define.h` を `cross_global_value.h` より前へ移す**1 行の移動**で解決することを確認（§4.4）。
- [軽微 採用] 「MSVC でも同様のはず」の推定を実測に置換（MSVC が `C1018` で拒否、§2.5）。
  「推定のまま残した項目は無い」という冒頭の断定も、未確認事項を明示する表現へ修正。
- [軽微 採用] amateras 側の完了ゲートをコマンド単位まで具体化（§4.5、10 項目）。
- マトリクスを **15 ケース**へ拡張し、**MSVC 参照列**を追加（A/C/M/P/Q は MSVC で PASS =
  合法な C++ であることを裏取り済み）。

**rev.4 → rev.5 の変更（レビュー第 3 回の指摘を反映）**

- **[P0 修正] エンコーディング指示の誤りを訂正**。rev.4 の「正本は CP932 で編集」は
  **正本を破損させ lint も落とす**誤りだった。実測では今回の正本 3 ファイルはすべて
  `base_inc\**` の **UTF-8 BOM なし**で、lint も同ゾーンに `utf8-nobom` を要求している。
  CP932 化は `generate_cross.exe` に任せる（§4.6 を新設）。
- **[P1 修正] ヘルパーの返却契約を明確化**。返る `Sym*` には typedef シンボルと
  タグシンボルの 2 種類があり `parse_btype()` での扱いが異なる。
  **typedef を一律に `type->ref` へ入れてはならない**ため、種別を返す設計＋擬似コードを明記（§3.1）。
  併せて rev.4 のフォールバック説明の誤り（`struct X` は `TOK_STRUCT` 経路／ケース J は
  フォールバックに入らない）を訂正し、**前方宣言**という正しい実例へ差し替え（実機確認済み）。
- **[P1 修正] SIMD 完了ゲートの対象 TU を分離**。rev.4 はゲート 8 と 10 が実質同一で、
  かつ SIMD 検証を stb を含まない `cross_tpp.h` に当てていた。
  「自己完結性 = `cross_tpp.h`」「SIMD 恒久化 = `cross.h`（`-DSTBI_NO_SIMD` なし）」へ整理（§4.5）。
- [軽微 採用] 負例の期待診断照合を **ASCII の安定要素のみ**（ファイル名・行番号・`error:`）に限定。
  TCC の診断本文は CP932 日本語のため UTF-8 の `.expected` と衝突する（§3.2.1）。
- [軽微 採用] コンパイル生成物（`.o` / `.obj`）は作業ツリーではなく `%TEMP%` へ出力（§3.2.1 / §4.5）。
- [軽微 採用] 完了前の確認を `git diff --stat -- inc/` から**リポジトリ全体の
  `git status --short` + `git diff --stat`** へ拡大（§4.5 ゲート 4）。

---

## 0. レビュー指摘の検証結果

| レビュー指摘 | 判定 | 根拠 |
|---|---|---|
| [P0] 生成手順が成立しない | **誤り（前提の取り違え）** | 正本は `base_inc/only/tcc/tcc_only.h`。レビューは `cross_base.h` を入力にしていた。下記 §1.1 でバイト完全一致を確認 |
| [P0] `build_generate_cross.ps1` は `-merge` しない | **正しい** | [build_generate_cross.ps1:52-55](file:///E:/work/work_cross_platform/kaiser_system/amateras/tools/build_generate_cross.ps1) は exe をビルドするだけ。初版の記述が誤り |
| [P0] `inc/**` 直接編集は規約違反 | **正しい** | AGENTS.md:67 / architecture-principles.md:67 「生成物のため手編集しない」 |
| [P0] amateras が dirty worktree | **正しい** | §1.2 参照 |
| [P1] `cpp_tok_starts_type_name()` に同じ誤探索 | **正しい（実害を確認。ただし経路はレビューの例と異なる）** | §2.3 ケース M |
| [P1] `decl()` 13079 の直接探索 | **正しい（存在する）** | §2.3 |
| [P1] 新ブランチ基点が未指定 | **正しい** | §3.0 |
| [P1] amateras 側の完了ゲート不足 | **正しい** | §4 に反映 |
| [P1] include 順の「任意」扱いは不整合 | **妥当** | §4.4 に格上げ |
| stb は `mmd_image_load.h` に置くべき | **正しい** | §4.3 |
| テスト網羅項目の追加 | **正しい（ただし一部は現状 PASS）** | §2.2 の実測で正例/負例を再構成 |
| フェーズ3の 1 ブランチ混在リスク | **正しい** | §5 に分割条件を明記 |

### 0.1 第 2 回レビュー指摘の検証結果（rev.4）

| レビュー指摘 | 判定 | 根拠 |
|---|---|---|
| [P1-1] 真偽値ヘルパーでは不十分（内側 typedef が外側 class を隠す） | **正しい。実測で現状も既に壊れていることを確認** | §2.3.1 ケース P / Q。MSVC は両方 PASS |
| [P1-1] `decl()` 13079 は修飾名探索なので別扱い | **正しい。rev.3 は内部矛盾していた** | §3.1 修正 2 で適用先から除外 |
| [P1-2] 負例と C テストは現行ランナーで実行されない | **正しい** | [run_all.bat:15](dev/test/run_all.bat#L15) は `a9\*.cpp` のみ、`a9\*.c` 無し |
| [P1-2] `KNOWNFAIL` は失敗の継続を検証しない | **正しい** | [run_all.bat:12,20-22](dev/test/run_all.bat#L12) は除外するだけ |
| [P1-3] (a)/(b) 未決定で確定プランでない | **正しい** | §4.4 で **(a) を採用**。1 行移動で解決すると実測 |
| [軽微] 「推定のまま残した項目は無い」は矛盾 | **正しい** | 冒頭を修正し §2.8 に未確認事項を明示 |
| [軽微] amateras ゲートをコマンド化すべき | **正しい** | §4.5 を 10 項目のコマンド表へ |

### 0.2 第 3 回レビュー指摘の検証結果（rev.5）

| レビュー指摘 | 判定 | 根拠 |
|---|---|---|
| [P0] 「正本を CP932 で編集」は正本を破損させる | **正しい。重大な誤り** | 正本 3 ファイルは実測で **UTF-8 BOM なし**。[encoding_lint.ps1:248](file:///E:/work/work_cross_platform/kaiser_system/amateras/tools/encoding_lint.ps1) が `base_inc` に `utf8-nobom` を要求（§4.6） |
| [P1] `Sym*` の返却契約が曖昧（typedef とタグで扱いが違う） | **正しい** | §3.1 に種別付き契約と擬似コードを追加 |
| [P1] rev.4 のフォールバック説明が不正確 | **正しい（2 点とも）** | `struct X` は [tccgen.c:8502](tccgen.c#L8502) の `TOK_STRUCT` 経路。ケース J は `sym_find()` が空にならない |
| [P1] SIMD ゲートの対象 TU が曖昧 | **正しい** | rev.4 のゲート 8 と 10 は実質同一。`cross_tpp.h` は stb を含まない（§4.5） |
| [軽微] `.expected` の日本語診断は文字コード衝突 | **正しい** | TCC の診断は CP932 日本語。ASCII 要素のみで照合（§3.2.1） |
| [軽微] `.o` / `.obj` を作業ツリーへ出さない | **正しい** | 現に `dev/test/a9/` へ `.o` が多数生成されている（§3.2.1-4） |
| [軽微] 完了前確認はリポジトリ全体で | **正しい** | §4.5 ゲート 4 |

---

## 1. amateras 側の前提事実（実測）

### 1.1 cross_tpp.h の正本と生成コマンド（確定）

正本は `base_inc/only/tcc/tcc_only.h`（7 行の include ハブ）。

```powershell
cd e:\work\work_cross_platform\kaiser_system\amateras
.\tools\generate_cross.exe -merge -o <出力先> base_inc\only\tcc\tcc_only.h cross_tpp.h
```

一時ディレクトリへ生成して現物と比較した結果:

| | 生成 | 現物 | 一致 |
|---|---|---|---|
| `SHIFT_JIS/cross_tpp.h` | 466 行 / 15,205 bytes | 466 行 / 15,205 bytes | **バイト完全一致** |
| `UTF_8/cross_tpp.h` | 466 行 / 16,170 bytes | 466 行 / 16,170 bytes | **バイト完全一致** |

→ 生成系の修正は不要。再生成の再現性は確保済み。

`cross.h` 側の正本は `base_inc/cross_all.h`（`tools/generate_cross_session.json` の
`merge_session.merge_in_path` に記録あり、出力先 `amateras\inc`、出力名 `cross.h`）。

### 1.2 amateras の作業ツリー状態（着手前に要解決）

ブランチ `master`、HEAD `0c30782`。

```
AM  base_inc/3d_format/統合ビューア usase.md
 M  base_inc/mmd/render/mmd_draw_helper.h
 M  inc/SHIFT_JIS/cross.h
 M  inc/UTF_8/cross.h
??  base_inc/3d_format/統合ビューア統合作業履歴.md
??  test/build_run_model_viewer.ps1
??  test/tcc/viewer/
```

`inc/**` の 2 ファイルが未コミット変更を持つ。この状態で再生成すると既存変更を上書きする。
**着手前にユーザーへ帰属を確認すること**（§4.0 のゲート）。

---

## 2. 技術的な原因（実機確定）

### 2.1 問題 1: `cross.h:33459` — C++ 名前隠蔽規則の未実装（tpp 側・本命）

`mmd_gl_free_texture()` 内の `tex->gl_tex_id = 0;` で `error: identifier が必要です`。

`cross.h:1116` に C++ モード限定の `struct tex` があり、引数 `tex` がそれを隠すべきところ、
`parse_btype()`（[tccgen.c:8626-8665](tccgen.c#L8626-L8665)）が cpp モードで文頭の識別子を
`struct_find()`（タグ名前空間・スコープ隠蔽の対象外）で先に型解決し、`sym_find()` が返す
内側スコープの引数シンボルを無視する。結果「型 `tex` で始まる宣言」と誤認し、declarator で
`->` に当たって [tccgen.c:9092](tccgen.c#L9092) の `expect("identifier")` に落ちる。

### 2.2 破綻範囲の実測マトリクス（`dev\tcc.exe` 実行）

MSVC 列は参照実装での結果（`cl /EHsc /c`）。**MSVC が通るのに tpp が落ちるものは tpp のバグ**。

| # | ケース | tpp | MSVC | 備考 |
|---|---|---|---|---|
| A | 引数が型名を隠す + 文頭 `tex->id = 0;` | **FAIL** | PASS | cross.h の実例 |
| B | ローカル変数が隠す + 文頭 `tex->id = 0;` | **FAIL** | – | A と同一根本原因 |
| C | 同一スコープ `struct tex{}; int tex = 5;` | **FAIL** `redefinition` | PASS | **新規発見**・別原因（§2.4） |
| D | 上記 C を **C モード**（`.c`）で | **PASS** | – | C 非回帰の基準線 |
| E | 隠蔽下で `struct tex local;`（明示型指定） | **PASS** | – | 修正不要 |
| F | ブロック終了後に型が再び見える | **PASS** | – | 修正不要 |
| G | 隠蔽下の**ローカル** ctor `Foo lf(tex);` | **PASS** | – | 修正不要 |
| H | 隠蔽下の文頭 単純代入 `tex = 3;` | **FAIL** | – | A と同一原因 |
| I | 隠蔽下の文頭 メンバ代入 `tex.id = 0;` | **FAIL** | – | A と同一原因 |
| J | 逆順 `int tex = 5; struct tex{...};` | **PASS** | – | C の順序依存を示す（§2.4） |
| L | `struct tex{...};` + **関数内**で `int tex = 5;` | **PASS** | – | C はスコープ依存（§2.4） |
| M | J の順序 + グローバル ctor `Foo g(tex);` | **FAIL**（黙って誤パース） | PASS | §2.3。`cpp_tok_starts_type_name` 経路 |
| N | M の対照（隠蔽なし `Foo g(val);`） | **PASS**（`g.a == 5`） | – | M が隠蔽起因であることの対照 |
| **P** | **内側 `typedef int X;` が外側 `struct X` を隠す** | **FAIL** `'{' が必要です` | PASS | **新規発見**（§2.3）。真偽値ヘルパーでは救えない |
| **Q** | **内側 `typedef Y X;` 経由で `X v; v.only`** | **FAIL** `field not found: only` | PASS | 外側 `struct X` を誤採用したことの直接証拠 |

→ 壊れているのは 4 系統:

1. **文頭に隠蔽名が来る式文**（A/B/H/I）— `parse_btype()`
2. **グローバル ctor 引数の誤判定**（M）— `cpp_tok_starts_type_name()`
3. **内側 typedef が外側 class に負ける**（P/Q）— `parse_btype()` の `struct_find()` 優先（§2.3）
4. 独立問題の `redefinition`（C）— 別ブランチ（§2.4）

E/F/G/J/L/N は現状で正常に動くため、**回帰防止テストとして固定**する（修正で壊さないことの保証）。

### 2.3 他経路の監査結果（レビュー P1 への回答）

- `cpp_tok_starts_type_name()`（[tccgen.c:1229-1268](tccgen.c#L1229-L1268)）に
  「`struct_find()` 無条件優先 + 内側の非 typedef を跨いだ `prev_tok` typedef 探索」がある点は
  **レビューの指摘どおり**。
- 呼び出し箇所は [tccgen.c:13152](tccgen.c#L13152) の **1 箇所のみ**（グローバル `decl()`、`l == VT_CONST`）。
  レビューが挙げた「クラス名と同名の**引数**を `Foo f(tex);` に渡す」という経路自体は、
  グローバルスコープに引数が存在しないため到達しない（ローカルの同型ケースは G で **PASS** を実測）。
- **ただし別経路で実害を確認した（ケース M）**。§2.4 の順序依存を使って
  「変数を先に宣言 → 同名 struct を定義」とすると `redefinition` を回避したうえで
  タグが登録されるため、`cpp_tok_starts_type_name()` の `struct_find()` 無条件優先に到達する:

```cpp
int tex = 5;
struct tex { float u, v; };
struct Foo { int a; Foo(int x) { a = x; } };
Foo g(tex);        // 誤: 「tex 型を引数に取る関数 g の宣言」と解釈される
int main() { return g.a; }   // error: lvalue が必要です  ← エラー位置がここまでずれる
```

  隠蔽なしの対照（ケース N）は正常に動く（`g.a == 5`）。
  **その場でエラーにならず関数宣言として黙って通る**ため、規模の大きな TU では
  意味が変わったまま先に進む危険がある。
  → レビュー P1 の懸念は**結論として正しい。優先度を「必須」に格上げ**する。
- `decl()` 内の直接探索（[tccgen.c:13079-13084](tccgen.c#L13079-L13084)）も実在。
  ~~同じく集約対象。~~ **rev.4 で撤回** — ここは `X::member` の**修飾名検出**であり、
  C++ の qualified-id 左辺は type/namespace 限定探索が正しい（変数による隠蔽を**無視すべき**）。
  通常の隠蔽規則を適用すると逆に壊れる。**共通ヘルパーの適用先から外す**（§3.1）。

#### 2.3.1 新規発見: 内側 typedef が外側 class に負ける（ケース P / Q）

レビュー P1-1 の指摘を検証したところ、**現時点で既に壊れている**ことが判明した。
`parse_btype()` の cpp 分岐（[tccgen.c:8629](tccgen.c#L8629)）が `struct_find()` を
無条件に優先するため、内側スコープの typedef が外側の同名 class に負ける。

```cpp
struct X { int member; };
int main() {
    typedef int X;      // 内側の typedef が外側の struct X を隠すのが正しい
    X value = 1;        // tpp: error: '{' が必要です（";" が見つかりました）
    return value;       //      → 外側の struct X と解釈し、定義の '{' を期待している
}
```

型の取り違えを直接示す証拠（ケース Q）:

```cpp
struct X { int a; int b; };
struct Y { int only; };
int main() {
    typedef Y X;        // X は Y のはず
    X v;
    v.only = 7;         // tpp: error: field not found: only  → 外側 struct X を採用している
    return v.only;
}
```

両ケースとも MSVC は正常にコンパイルできる（合法な C++）。

**設計上の含意**: `sym_find(X)` の先頭は typedef なので、
「型か否か」を返す真偽値ヘルパーでは `true` が返るだけで、その後に
`struct_find()` 優先の既存処理が走れば**依然として外側の struct X が選ばれる**。
→ ヘルパーは「**どの型が可視か**」を返さなければならない（§3.1 で設計変更）。

### 2.4 新規発見: 同一スコープ隠蔽での誤 `redefinition`（ケース C）— 原因確定

`struct tex { ... }; int tex = 5;` は C++ では合法（変数がクラス名を隠し、クラス名は
`struct tex` でのみ到達可能になる）が、tpp は `redefinition: 'tex'` を出す。
C モードでは通る（ケース D）。

**原因は [tccgen.c:8345-8354](tccgen.c#L8345-L8354) の暗黙 typedef 注入**（確定）:

```c
if (is_class) {
    tag_v = s->v & ~SYM_STRUCT;
    ctype = *type;
    ctype.t |= VT_TYPEDEF;
    if (is_class == 1 || !sym_find(tag_v))
        sym_push(tag_v, &ctype, VT_TYPEDEF, 0);   // クラス名を通常の識別子空間へ注入
}
```

C++ の struct/class はタグ名と同名の typedef を通常の識別子名前空間へ push する。
その後の同一スコープ `int tex = 5;` がこの typedef と衝突して `redefinition` になる。
正しい C++ 挙動は「後続の変数宣言がクラス名を**隠す**（エラーにしない）」。

実測で判明した**適用条件の狭さ**:

- ケース J（`int tex = 5;` が**先**）は **PASS** — 8352 の `!sym_find(tag_v)` ガードが
  typedef 注入を抑止するため。
- ケース L（`struct tex{}` の後、**関数内**で `int tex = 5;`）は **PASS** — 内側スコープなら
  別 sym として push され衝突しないため。
- 失敗するのは「**同一（グローバル）スコープで、struct 定義が先・変数宣言が後**」の順序のみ。

→ 極めて狭いエッジケースで amateras は踏んでいない。**別ガイド・別ブランチに分離し、優先度は低**。

**依存関係の注意**: 将来この `redefinition` を修正すると `struct tex{}; int tex;` が書けるように
なり、§2.3 のケース M へ到達する経路が増える。**フェーズ 1（§3.1）を先に入れること**を前提とする。

### 2.5 問題 2: `cross_tpp.h:351` — 生成元の重複ブロック（amateras 側）

`cr_win()` 末尾のプラットフォーム分岐終端 5 行が丸ごと二重化している。
1 つ目の `#endif` で `cr_win()` 内の `#if` が閉じるため、2 つ目の `#elif` が
`#if defined(_HEAD_ONLY)` の `#else` 配下に落ちて `#elif は #else の後に使用できません` になる。

**TCC 固有ではないことを実測で確認**（rev.3 の「MSVC/GCC でも不正」という推定を置換）:
同じ構造の最小再現を MSVC でコンパイルすると
`fatal error C1018: 予期しない #elif` で同様に拒否される。

生成元は [base_inc/platform/win/win_imp.h:172-181](file:///E:/work/work_cross_platform/kaiser_system/amateras/base_inc/platform/win/win_imp.h)。
`cr_win():end:0` マーカー出現数を全関連ファイルで数えた結果、重複は
`win_imp.h` / `inc/SHIFT_JIS/cross_tpp.h` / `inc/UTF_8/cross_tpp.h` の 3 件のみ
（`win_new.h` / `test_cross.h` / `wsl.h` / `win_imp_wip.h` / `win_imp_single.h` は 1 回で正常）。

### 2.6 問題 3: `emmintrin.h` 不在（amateras 側・既知）

`-DSTBI_NO_SIMD` を外すと `cross.h:23226`（埋め込み stb_image）が x64 検出で SSE2 経路に入り停止。
これは amateras 側で**既知**であり、[build_generate_cross.ps1:31-33](file:///E:/work/work_cross_platform/kaiser_system/amateras/tools/build_generate_cross.ps1)
が `-DSTBI_NO_SIMD` をコメント「(P-13)」付きで既に渡している。恒久化の置き場所は §4.3。

### 2.7 問題 4: `cross_tpp.h` 単体 include 不可（amateras 側）

`cross_tpp.h` は 3 行目で `CRITICAL_SECTION` を使う `cross_global_value.h` を読むが、
`<windows.h>` を供給する `win_define.h` 相当は後方にある。単体 include では
`cross_global_value.h:4: ';' が必要です ("cross_lock_t")` で停止する。
正本 `tcc_only.h` の include 順に由来し、**1 行の移動で解消することを実測済み**（§4.4）。

### 2.8 残作業の見通しと、未確認のまま残る事項

`cross.h` の 33459 行以降 約 1.17 万行に `template` / `new` / `delete` / `operator` 定義 / `virtual`
の出現は **0 件**。`namespace` は 1 件のみで PS5 ガード（`#if defined(PLATFORM_PS5)`）内のため
Windows ビルドでは無関係。→ 大物構文の実装は不要と判断できる。

**未確認事項（断定しない）**:

- 33459 行以降に**どれだけ**小粒のパーサギャップが残るかは未測定。
  「同種の反復で済む」は上記の構文出現数から導いた**見込みであって確認ではない**。
  実測はフェーズ 1 適用後の反復（§5）でしか得られない。
- `cross_mmd.h` / `fix_mmd.h` は隠蔽衝突の箇所数を数えただけで、コンパイルは未実施。
- ケース B/E〜L の MSVC 参照は未取得（A/C/M/P/Q のみ取得済み）。
  いずれも tpp 側が PASS しているため優先度は低い。

---

## 3. フェーズ 1: tpp 側 — 隠蔽名の型誤判定修正

### 3.0 ブランチ基点（レビュー P1 反映）

再現に必要な Windows ヘッダ互換修正 `db3dee2` は `fix/tcc-compatible-crt-headers` にのみ存在し、
`master` には**入っていない**（`git branch --contains db3dee2` で確認、`merge-base` は `0326b52`）。

- 基点: **`d761d92`**（= `fix/tcc-compatible-crt-headers` の HEAD）
- ブランチ名: `fix/cpp-shadowed-type-name`
- 統合順: `fix/tcc-compatible-crt-headers` → 本ブランチ → master へマージ

### 3.1 修正内容

対象は「**通常（非修飾）の型名探索**」に限定し、**修飾名探索は別物として扱う**（レビュー P1-1）。
救うのは A/B/H/I（文頭式）、M（グローバル ctor 引数）、P/Q（内側 typedef）の 3 系統。

#### 修正 1: 探索ヘルパーは「解決済みシンボル」を返す（真偽値ではない）

rev.3 の `cpp_innermost_is_type(int v)`（真偽値）は**撤回**する。
ケース P/Q では `sym_find(X)` の先頭が typedef なので真偽値は `true` を返すが、
その後に `struct_find()` 優先の既存処理が走れば依然として外側 class を選んでしまい、
**バグが直らない**（§2.3.1）。

**返却契約（レビュー P1-1 反映）**: 返る `Sym*` には**性質の異なる 2 種類**があり、
呼び出し側の扱いも違う。**typedef シンボルを一律に `type->ref` へ入れてはならない**。
そこで種別を出力引数で返す。

```c
/* Kind of binding cpp_lookup_type_name() resolved to. */
#define CPP_TN_NONE     0   /* not a type name here (hidden, or unknown) */
#define CPP_TN_TYPEDEF  1   /* a typedef Sym from sym_find()             */
#define CPP_TN_TAG      2   /* a struct/union/enum tag Sym from struct_find() */

/* C++ unqualified lookup for a type name.  Returns the resolved Sym and
   sets *kind, or returns NULL (kind = CPP_TN_NONE) when the innermost
   ordinary-namespace binding is an object, parameter or function - those
   hide an outer class of the same name.
   Ordinary lookup only - do NOT use for qualified-id (X::y), which must
   consider types and namespaces only. */
static Sym *cpp_lookup_type_name(int v, int *kind);
```

探索順:

1. `sym_find(v)` の**最も内側の束縛だけを見る**（`prev_tok` を辿らない）。
   - 非 typedef の実体（変数・引数・関数）→ `NULL` / `CPP_TN_NONE`（隠蔽されている）。
   - typedef → その **typedef シンボル**を返し `CPP_TN_TYPEDEF`（ケース P/Q が直る鍵）。
2. `sym_find(v)` が**何も返さなかった場合に限り** `struct_find(v)` → `CPP_TN_TAG`。

呼び出し側（`parse_btype()`）の分岐:

```c
s = cpp_lookup_type_name(tok, &kind);
if (kind == CPP_TN_TYPEDEF) {
    /* same shape as the existing typedef path (8685-8691):
       strip VT_TYPEDEF and take the typedef's target type/ref. */
    type->t   = (s->type.t & ~VT_TYPEDEF) | quals;
    type->ref = s->type.ref;          /* NOT s */
    sym_to_attr(ad, s);
} else if (kind == CPP_TN_TAG) {
    /* same shape as the existing struct path (8658-8660):
       the tag Sym itself is the referenced type. */
    type->t   = s->type.t | quals;
    type->ref = s;                    /* the tag Sym */
} else {
    goto the_end;                     /* not a type -> parse as an expression */
}
```

**ケース P が直る理由**: `typedef int X;` は `s->type.ref == NULL`、`s->type.t == VT_INT`。
上の typedef 分岐は `int` を正しく取り出す。タグ扱いにして `type->ref = s` としてしまうと
「struct 型」と誤認するため、この 2 分岐の区別が必須。

**`struct_find()` フォールバックが必要な実例**（rev.4 の説明は誤りだったので差し替え）:

```cpp
struct FwdOnly;        // 本体なし -> 8345 の暗黙 typedef 注入は走らない
FwdOnly* g_p;          // sym_find() は空 -> struct_find() で解決する必要がある（現状 PASS）
```

rev.4 は例として「elaborated-type-specifier `struct X`」と「ケース J」を挙げていたが、
**どちらも誤り**（レビュー指摘）:

- 明示的な `struct X` は [tccgen.c:8502](tccgen.c#L8502) の `case TOK_STRUCT:` で処理され、
  この default 分岐（ヘルパー）には来ない。
- ケース J は通常識別子 `tex` が既に存在するため `sym_find()` は空にならず、
  フォールバックには入らない（`CPP_TN_NONE` になるのが正しい挙動）。

#### 修正 2: 呼び出し側の差し替え（2 経路のみ）

| 箇所 | 扱い | 根拠 |
|---|---|---|
| [tccgen.c:8626-8665](tccgen.c#L8626-L8665) `parse_btype()` cpp 分岐 | **ヘルパーへ置換（必須）** | ケース A/B/H/I/P/Q |
| [tccgen.c:1229-1268](tccgen.c#L1229-L1268) `cpp_tok_starts_type_name()` | **ヘルパーの結果で判定（必須）** | ケース M |
| [tccgen.c:13079-13084](tccgen.c#L13079-L13084) `decl()` | **置換しない** | 下記 |

`decl()` の 13079 は `X::member` の**修飾名検出**であり、C++ [basic.lookup.qual] では
qualified-id 左辺は type/namespace のみを探索する。つまり**変数による隠蔽を無視するのが正しい**。
ここへ通常の隠蔽規則を適用すると、隠蔽された class の out-of-class メンバ定義が壊れる。
→ 本ブランチでは**現状維持**とし、必要になった時点で独立した type-only lookup として設計する。
（rev.3 は「限定探索が正しい」と書きながら適用先にも挙げており矛盾していた。レビュー指摘で修正。）

#### 修正 3: 制約として記録（今回は対象外）

- メンバ変数がクラス名を隠すケース（メンバ関数本体内）は本修正では救えない。
  amateras の 9 型名とメンバ名の衝突は未検出のため、踏んだら追加対応。
- 変数に隠されたクラス名での `Foo::x`（式パーサ側
  [tccgen.c:10140-10152](tccgen.c#L10140-L10152)）は上記の type-only lookup 案件。
  amateras に該当パターンが無いため既知の制限として記録に留める。

規約遵守: 関数単位の最小差分、新規変数はブロック先頭宣言（MSVC C90）、
既存 TCC ソースへのコメントは ASCII 英語（日本語コメント追加は禁止）。

### 3.2 回帰テスト（テストランナーの改修を含む）

**先にランナーの実態を確認した結果、rev.3 の計画はそのままでは実行されないことが判明した**
（レビュー P1-2）。現行 [run_all.bat:15](dev/test/run_all.bat#L15) の Phase 1 は
`a9\*.cpp` のみを列挙し `a9\*.c` を含まない。Phase 2（[run_all.bat:37](dev/test/run_all.bat#L37)）も
`a9\*.cpp` のみで、**リンクして実行し exit 0 を要求する**。
`KNOWNFAIL`（[run_all.bat:12](dev/test/run_all.bat#L12)）はゲートから除外するだけで、
「失敗し続けること」は検証しない。

#### 3.2.1 ランナー改修（本ブランチに含める）

1. Phase 1 の列挙に **`a9\*.c` を追加**（ケース D の C モード非回帰を実際にゲートさせる）。
2. **`a9\negative\*.cpp` 用の専用ループを新設**する。
   - コンパイル**非 0 で PASS、0 で FAIL**（現行 Phase の合否と逆）。
   - 併せて**期待診断を照合**し、無関係な構文エラーによる見かけ上の PASS を防ぐ。
     照合キーは **ASCII の安定要素のみ**（ファイル名・行番号・`error:`）とし、
     **診断メッセージ本文（日本語）では照合しない**。
     本ビルドの TCC はエラー文を CP932 の日本語で出すため、UTF-8 で書いた `.expected` と
     突き合わせると文字コードが衝突して誤判定する（レビュー軽微指摘）。
     例: `<name>.cpp:12: error:` までを照合する。
   - `a9\negative\` は Phase 1 / Phase 2 の `a9\*.cpp` 列挙に**含まれない**位置に置く
     （サブディレクトリなのでワイルドカードには掛からない）。
3. `KNOWNFAIL` は今回は触らない（別課題）。ただし新設の負例ループは
   「失敗し続けること」を検証するので、`KNOWNFAIL` の代替として使える旨をコメントに残す。
4. 生成物の出力先を作業ツリーの外へ寄せる。Phase 1 は現状 `%%~dpnf.o` として
   ソースと同じ場所に `.o` を作っており、`dev/test/a9/` に大量の `.o` が残っている。
   新設ループは **`%TEMP%` 配下へ出力**する（Phase 2 の `EXEOUT` と同じ方針）。
   既存 Phase 1 の出力先変更は差分が広がるため**別コミット**とする。

#### 3.2.2 テストの配置と合否設計

**重要**: Phase 2 は exit 0 を要求するため、**テストプログラムは成功時に 0 を返す**こと。
rev.3 のケース M 案は `return g.a;`（= 5）としており、直っていてもゲートを落とす誤りだった。
値の検証は `return (g.a == 5) ? 0 : 1;` の形にする。

| 置き場所 | 内容 | 合否 |
|---|---|---|
| `a9\*.cpp` | 修正で通るべき: A / B / H / I / **M** | コンパイル + 実行 exit 0 |
| `a9\*.cpp` | 維持すべき正常系: E / F / G / J / L / N | 同上 |
| `a9\*.c` | **D（C モード非回帰）** | コンパイル（要 §3.2.1-1） |
| `a9\negative\*.cpp` | 隠蔽下で `tex t;` を型として使う不正コード | **コンパイル失敗で PASS**（要 §3.2.1-2） |

- **P / Q は必ず含める**（§2.3.1）。真偽値ヘルパー設計に戻ると即座に落ちる番人になる。
- C（`redefinition`）は §2.4 の別ブランチへ回すため本ブランチには含めない。
  ただし J / L は「C の周辺で現に動いているもの」の保護として本ブランチに含める。

### 3.3 完了ゲート

- `build.bat` 緑（`tccgen.c` 変更のため **Release x64 と Debug Win32 の両方**）
- 既存 a9 回帰の全通過（ctor 構文、`Class::member`、struct メンバ関数、MI/仮想 MI）
- **`Class::member` の out-of-class 定義が壊れていないこと**（§3.1 修正 2 で `decl()` を
  意図的に据え置いたため、ここが無傷であることの確認は必須）
- §2.2 マトリクスの期待どおりの結果（P/Q を含む）
- 新設の `a9\negative\` ループが実際に PASS/FAIL を判定していること
  （わざと通る負例を一時的に置いて FAIL することを 1 度確認する）
- `cross.h` の C++ コンパイルが 33459 行を通過し、停止位置が前進すること

---

## 4. フェーズ 2: amateras 側 — 正本修正と再生成

### 4.0 事前ゲート（レビュー P0 反映・必須）

1. `git status --short` を取得し、§1.2 の未コミット変更（特に `inc/SHIFT_JIS/cross.h`,
   `inc/UTF_8/cross.h`, `base_inc/mmd/render/mmd_draw_helper.h`）の**帰属をユーザーに確認**する。
2. 現状 `master` 直接作業になっているため、作業ブランチを切る（基点コミットを記録）。
3. 既存変更の退避（commit / stash）が済むまで再生成を実行しない。
4. 再生成前後で `git diff --stat` を取り、**期待差分に限定されていること**を確認する。

### 4.1 `cr_win()` 重複ブロックの除去

- 修正対象は**正本のみ**: `base_inc/platform/win/win_imp.h` の 172-181 行から、
  2 回目の `// cr_win():end:0` 〜 `#endif`（5 行）を削除。
- `inc/**` は**直接編集しない**（AGENTS.md:67 / architecture-principles.md:67）。
- 編集時のエンコーディングは **UTF-8 BOM なしを維持**する（§4.6。CP932 化しないこと）。
- 再生成（§1.1 の確定コマンド）:

```powershell
cd e:\work\work_cross_platform\kaiser_system\amateras
.\tools\generate_cross.exe -merge -o .\inc base_inc\only\tcc\tcc_only.h cross_tpp.h
```

- 検証: `cr_win():end:0` マーカーが `inc/SHIFT_JIS/cross_tpp.h`・`inc/UTF_8/cross_tpp.h` の
  両方で 1 回になること。差分が当該 5 行のみであること。

### 4.2 `cross.h` の再生成

`cross.h` は正本 `base_inc/cross_all.h` から生成する。§4.0 の帰属確認が済むまで実行しない。

```powershell
.\tools\generate_cross.exe -merge -o .\inc base_inc\cross_all.h cross.h
```

### 4.3 stb の SIMD 無効化（置き場所を変更）

第三者コード `ext/stb/stb_image.h` は改変せず、既に STB 設定を所有している
[base_inc/mmd/render/mmd_image_load.h:20-35](file:///E:/work/work_cross_platform/kaiser_system/amateras/base_inc/mmd/render/mmd_image_load.h)
の `#include "../../../ext/stb/stb_image.h"` 直前へ条件付き定義を追加する。

```c
// TCC は emmintrin.h を同梱しないため, SSE2 経路に入るとビルドできない (P-13).
#if defined(__TINYC__) && !defined(STBI_NO_SIMD)
#define STBI_NO_SIMD
#endif
```

→ 局所的で、stb 更新時にも壊れない。適用後 `-DSTBI_NO_SIMD` 無しでのビルドを回帰テスト化する。

### 4.4 `cross_tpp.h` の自己完結性 — **(a) 自己完結化を採用（決定）**

rev.3 は (a)/(b) を選ばずに残していた（レビュー P1-3）。**(a) に決定**する。
根拠は、`inc/**` が配布物であり利用者側を単純にするという amateras の原則に照らして
内部専用とする特段の理由が無いこと、および下記のとおり**修正が 1 行の移動で済むと実測できた**こと。

**依存関係の実測**: `<windows.h>` を include しているのは `platform/win/win_define.h` で、
正本 `tcc_only.h` ではそれが `cross_global_value.h`（`CRITICAL_SECTION` を使う）より
**後ろ**に置かれていることが原因だった。

| include 順 | 結果 |
|---|---|
| 現行（`cross_global_value.h` → … → `win_define.h`） | **FAIL** `cross_global_value.h:4: ';' が必要です ("cross_lock_t")` |
| `win_define.h` を `cross_global_value.h` より前へ移動 | **PASS**（exit 0） |

→ 正本 `base_inc/only/tcc/tcc_only.h` で
`#include "../../platform/win/win_define.h"` を 2 行目付近へ移す**1 行の移動**で自己完結する。

**着手手順**（レビュー指摘の確認事項を含む）:

1. `cross_define.h` / `cross_msgbox.h` / `cross_global_value.h` / `win_define.h` の相互依存を
   再確認する（現時点の実測では `cross_global_value.h` と `cross_c_ccpp.h` は
   自前の `#include` を持たず、`win_define.h` が Windows 系一式を供給している）。
2. 正本を修正して再生成（§4.1 と同じコマンド）。
3. **TCC と MSVC の両方**で「`cross_tpp.h` のみを include する TU」をコンパイルして通ること。
4. 通ったら §3 の `window_t` テストも単体 include 形式へ切り替える
   （`<windows.h>` 先行で問題を隠さない）。

### 4.5 完了ゲート（コマンド単位まで具体化）

すべて `cd e:\work\work_cross_platform\kaiser_system\amateras` を起点とする。
`$TCC` = `e:\work\work_github\tpp\tcc_dx\dev\tcc.exe`。

**TU（テスト翻訳単位）は 2 種類**。どちらも先行 include を置かない（自己完結性の検証を兼ねる）。

- `TU_cross` : `#include "cross.h"` + `int main(){return 0;}`
- `TU_tpp`  : `#include "cross_tpp.h"` + `int main(){return 0;}`

オブジェクトファイルは**作業ツリーに出さず一時ディレクトリへ出力する**（`-o $env:TEMP\...`）。

| # | ゲート | コマンド / 期待成果 |
|---|---|---|
| 1 | 生成ツールのビルド | `powershell -NoProfile -ExecutionPolicy Bypass -File tools\build_generate_cross.ps1` → `tools\generate_cross.exe` 更新 |
| 2 | `cross_tpp.h` 再生成 | `.\tools\generate_cross.exe -merge -o .\inc base_inc\only\tcc\tcc_only.h cross_tpp.h` |
| 3 | `cross.h` 再生成 | `.\tools\generate_cross.exe -merge -o .\inc base_inc\cross_all.h cross.h` |
| 4 | 差分限定の確認 | **リポジトリ全体**で `git status --short` と `git diff --stat` を確認（`-- inc/` だけに絞らない）。意図した変更のみであること |
| 5 | エンコーディング | `powershell -NoProfile -ExecutionPolicy Bypass -File tools\encoding_lint.ps1` で **FAIL=0**（§4.6 の規約を守っていること） |
| 6 | 重複マーカー解消 | `inc\SHIFT_JIS\cross_tpp.h` と `inc\UTF_8\cross_tpp.h` で `cr_win():end:0` が各 **1 回** |

コンパイル系ゲート（**目的ごとに分離** — レビュー P1-3 反映）:

| # | 目的 | コンパイラ / ヘッダ | コマンド |
|---|---|---|---|
| 7 | **自己完結性**（§4.4） | TCC / SHIFT_JIS / `TU_tpp` | `& $TCC -c TU_tpp.cpp -I inc\SHIFT_JIS -I base_inc -D_MBCS -D_CONSOLE -o $env:TEMP\t.o` |
| 8 | **自己完結性**（§4.4） | MSVC / UTF_8 / `TU_tpp` | `cl /nologo /EHsc /c TU_tpp.cpp /I inc\UTF_8 /I base_inc /D_MBCS /D_CONSOLE /Fo$env:TEMP\t.obj` |
| 9 | **SIMD 恒久化**（§4.3） | TCC / SHIFT_JIS / `TU_cross` | 上と同形で `TU_cross.cpp`。**`-DSTBI_NO_SIMD` を付けない**こと（付けると §4.3 の検証にならない） |
| 10 | 本体の通し確認 | MSVC / UTF_8 / `TU_cross` | `cl ... TU_cross.cpp /I inc\UTF_8 /I base_inc /D_MBCS /D_CONSOLE` |

7〜10 はすべて exit 0 が条件。

**分離の理由**: rev.4 はゲート 8 に最初から `-DSTBI_NO_SIMD` が無く、ゲート 10 が
「8 をマクロなしで実行」となっていて**同一の内容**だった。さらにゲート 9 は `cross_tpp.h` を
対象にしていたが、`cross_tpp.h` は stb を含まないため SIMD 恒久化の証明にならない。
SIMD の検証対象は `cross.h`（stb を埋め込んでいる側）でなければならない。

### 4.6 エンコーディング規約（**rev.4 の記述は誤り。必読**）

rev.4 は「正本の編集は CP932 指定の PowerShell で行う」と書いていたが、**これは誤りで、
そのとおりにすると正本を破損させ lint も落とす**（レビュー P0）。実測結果は次のとおり。

| 対象 | 実測 | lint の要求（[encoding_lint.ps1:248-251](file:///E:/work/work_cross_platform/kaiser_system/amateras/tools/encoding_lint.ps1)） |
|---|---|---|
| `base_inc\platform\win\win_imp.h` | UTF-8 **BOM なし** | `base_inc` → `utf8-nobom` |
| `base_inc\mmd\render\mmd_image_load.h` | UTF-8 **BOM なし** | 同上 |
| `base_inc\only\tcc\tcc_only.h` | UTF-8 **BOM なし** | 同上 |
| `inc\UTF_8\cross_tpp.h` | UTF-8 **BOM 付き** | `inc\UTF_8` → `utf8-bom`（生成物） |
| `inc\SHIFT_JIS\cross_tpp.h` | CP932（UTF-8 として不正） | `inc\SHIFT_JIS` → `sjis`（生成物） |

**正しい指示**:

- 今回編集する正本 3 ファイルはすべて `base_inc\**` にあり、**UTF-8 BOM なしのまま編集する**。
- `inc\SHIFT_JIS\**` / `inc\UTF_8\**` は**生成物なので直接編集しない**。
- **CP932 への変換は `generate_cross.exe` に任せる**（人手で書き戻さない）。

補足: 「CP932 指定で編集」は tpp リポジトリ側の `tccpp.c`（LF + CP932）に対する既知の注意事項で、
amateras の `base_inc\**` には**当てはまらない**。rev.4 はこれを誤って一般化していた。

---

## 5. フェーズ 3: 反復と分割条件

フェーズ 1+2 適用後、`cross.h` の C++ コンパイルを再実行して次の停止位置を特定し、
最小再現化 → tpp 側 / amateras 側の切り分け、を通るまで反復する。

**分割条件（レビュー指摘反映・厳守）**: 新しい真因が 1 つ見つかるごとに
**別ガイド・別 feature branch・別コミット**とする。1 ブランチに複数の独立修正を混ぜない
（プロジェクト規約「1 ガイド = 1 feature branch = 1 コミット」）。
既に判明している分離対象:

- §2.4 の同一スコープ隠蔽 `redefinition` → 別ブランチ（amateras では未踏のため優先度低）
- メンバ変数によるクラス名隠蔽 → 踏んだ時点で別ブランチ

その後:

1. `cross_tpp.h` の `window_t` C++ テストを追加（§4.4 の決定に沿った include 形式で）
2. `cross_mmd.h` / `fix_mmd.h`（隠蔽衝突 各約 9 箇所）へ対象を拡大
3. amateras 実ヘッダに依存するテストは `dev/test/a9/manual/` へ分離し、
   `run_all.bat` を amateras チェックアウトに依存させない

---

## 6. 修正対象の所在まとめ

| 問題 | 修正する側 | 対象（正本） | 内容 |
|---|---|---|---|
| 文頭式の誤判定 ケース A/B/H/I | **tpp** | `tccgen.c` 8626-8665 | `cpp_lookup_type_name()` へ置換。**必須** |
| 内側 typedef が外側 class に負ける ケース P/Q | **tpp** | `tccgen.c` 8626-8665（同上） | 同ヘルパーが**解決済みシンボルを返す**ことで解決。**必須** |
| グローバル ctor 引数の誤判定 ケース M | **tpp** | `tccgen.c` 1229-1268 | 同ヘルパーの結果で判定。黙った誤パースのため**必須** |
| 修飾名 `X::member` の探索 | **tpp** | `tccgen.c` 13079-13084 | **今回は変更しない**（type-only lookup 案件・§3.1 修正 2） |
| 同一スコープ `redefinition` ケース C | **tpp** | `tccgen.c` 8345-8354 | 暗黙 typedef 注入の衝突。別ブランチ・優先度低 |
| テストランナーの穴 | **tpp** | `dev/test/run_all.bat` | `a9\*.c` 追加 + `a9\negative\` ループ新設（§3.2.1） |
| `#elif` after `#else` | **amateras** | `base_inc/platform/win/win_imp.h:172-181` | 重複 5 行削除 → 再生成 |
| `emmintrin.h` 不在 | **amateras** | `base_inc/mmd/render/mmd_image_load.h` | `__TINYC__` で `STBI_NO_SIMD` → 再生成 |
| 単体 include 不可 | **amateras** | `base_inc/only/tcc/tcc_only.h` | **(a) 自己完結化**（`win_define.h` を前へ 1 行移動） |

## 7. 全体の完了条件

- `dev\tcc.exe` で `#include "cross.h"` の C++ TU（`-D_MBCS -D_CONSOLE`、`STBI_NO_SIMD` は自動）が
  エラー 0 でオブジェクト生成できる。
- `cross_tpp.h` を**単体 include** する `window_t` テストが TCC / MSVC の両方で通る（§4.4）。
- tpp: `build.bat` が Release x64 / Debug Win32 の両方で緑。
  §2.2 マトリクス（P/Q 含む）が期待どおり。`a9\negative\` ループが機能していること（§3.3）。
- amateras: §4.5 のゲート **1〜10** をすべて満たす。
