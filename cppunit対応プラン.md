# CPPUnit（sample/cppunit）対応プラン（rev.5 / 2026-08-09 — **実装開始 GO**）

> rev.4 で **GO 判定**（レビュー: 「Blocker なし。実装フェーズへ進めて問題ない」）。
> rev.5 は GO 時に指定された実装指示の明確化 2 点 + Low 1 点のみを反映した最終版:
> ① G3-P5 に value-name lookup の scope 復元を明記（型名 lookup と別経路であることを固定）
> ② G3-P2 の「規則 1〜5」を「`lookup_unqualified_type()` の規則 1〜4」へ修正
> ③ G0 で baseline タグの commit hash も記録（タグ移動事故への保険・Low）

**目的**: `sample/cppunit` の CPPUnit ライブラリ 12 TU + ヘッダ 19 本を、
tpp（`dev\tcc.exe`）で**原本無改変のまま**コンパイル・リンクし、テストドライバで
実行できるようにする。
**G0 実測後の確定（2026-08-09・ユーザー決定 = 案 A）**: ドライバ `all_test.cpp` 原本は
生の例外（`throw std::runtime_error` / `catch (std::exception&)`）を使うため
スコープ外とし、**tpp 用ドライバ `all_test_tpp.cpp` を新規作成**する（例外なし・
TEST_ASSERT 系のみ・ライブラリ全機能を叩く。詳細 §7.4）。
原本無改変ゲートの対象は**ライブラリ 12 TU + ヘッダ 19 本**。

**ベース**: `master`（`cc31384`）。`feature/cpp-support` は**マージしない**
（実測で `tccgen.c` に衝突 266 ブロック。並行実装のため機械的に解決不能 —
検証記録は会話ログおよび本プラン §6）。参照実装としてのみ使う。

**実測日**: 2026-08-09。本プランの数値はすべて `dev\tcc.exe`（master ビルド）と
`sample/cppunit` 現物に対する実測である。推測は「未確認」と明示する。

**rev.4 の改訂内容**（rev.3 再レビュー = [cppunit対応プランレビュー.md](cppunit対応プランレビュー.md) への対応）:

| 再レビュー指摘 | 対応箇所 |
|---|---|
| Blocker 1: base merge が「宣言一致」判定で false error / 誤実装を作る | G3 の merge を「typedef は指定する型へ正規化して比較 + subobject 支配規則」に修正。複雑ケースは `tcc_error` に倒す。「先に見つかった方を採用」は明文で禁止 |
| Blocker 2: qualified lookup が unqualified と混線（規則番号も矛盾） | `lookup_unqualified_type()` と `lookup_qualified_class_type(C)` を別関数として定義。後者は C 自身 + bases のみで enclosing/global へ逃げない。`O::I::T` の負例を P3 に追加 |
| High 3: scalar POD `new` の意味論が未定義 | G4 に「G0 分類の結果で分岐: 0 件ならスコープ外、あるなら `new int`（未初期化）/ `new int()`（ゼロ初期化）を実装」と明記 |
| High 4: 純粋仮想 27 件に dtor が含まれるか未確認 | G0 の分類に「27 件を regular method / destructor に分類」を追加。pure virtual dtor があれば G5/G6 順序を再判断 |
| High 5: `findstr` の曖昧一致でゲートが抜ける | G7 を `findstr /x`（完全行一致）または数値抽出比較に固定 |
| High 6: 非 polymorphic の delete まで vptr を読む危険 | G6 に「選択された dtor が virtual の場合のみ G6 経路、non-virtual は G4 直呼び維持」の invariant + 非 virtual delete 回帰を追加 |
| 細部: `sizeof(I::x)` は構文依存 | base beats enclosing テストを `O::I i; sizeof(i.x)` 形へ変更 |

**rev.3 の改訂内容**（rev.2 再レビューへの対応）:

| 再レビュー指摘 | 対応箇所 |
|---|---|
| Blocker 1: `git diff -- path` は worktree vs index 比較で原本無改変を証明しない | G0 で baseline タグを打ち、G7 は **baseline commit と worktree の比較**に変更 |
| Blocker 2-A: base class は enclosing class より先 | G3 lookup 規則を「クラス自身 → その bases → 外側クラス → その bases → …」へ修正 |
| Blocker 2-B: 多重 base の typedef ambiguity 判定がない | G3 に lookup-set merge + ambiguity エラーを追加（virtual diamond の同一宣言は許可） |
| Blocker 2-C: non-type local が typedef を隠すケース | Step 1 を「name lookup で non-type が当たったら型探索失敗（フォールバック禁止）」へ修正 |
| Blocker 3: dtor 順序テストが加算式で順序を検証していない | `state = state * 10 + n` の順序依存形（期待値 123）へ変更 |
| High 1: 「3 consumer 超で別チェーン」の 3 に根拠なし | 件数基準を削除し「member-chain invariant を壊すか」で判定 |
| High 2: scalar new/delete 31/19 件の形別分類がない | G0 のインベントリに全件分類を追加 |
| High 3: pure スタブの linkage 未定義 | G5 で「各 TU の internal/static symbol」に固定 |
| High 4: offset-to-top の ABI 具体化不足 | G6 に vptr[-1] 配置・ポインタ幅 signed・既存 slot 不変の invariant を明記 |
| High 5: G6 後の virtual `delete NULL` 再テスト | G6 の回帰テストに追加（vptr 読みより先に NULL check） |
| 指摘外: テスト件数基準の取得手段が弱い | G0 に 4 段の優先順位（MSVC 実行 → 他コンパイラ → preprocessed 比較 → 生 grep）を明記 |

**rev.2 の改訂内容**（rev.1 レビューへの対応）:

| レビュー指摘 | 対応箇所 |
|---|---|
| Blocker 1: 「全リスト」は未証明 | §1 を「確認済みリスト」へ改題、G0 新設、G7 まで追加要求発見の可能性を明記 |
| Blocker 2: G3 のスコープ解決設計不足 | §G3 に 5 段の lookup 規則を明文化、shadow / enclosing / base のテスト追加 |
| Blocker 3: default 引数の defining scope | G3-P5 として独立フェーズ化。トークン + 定義クラス/スコープを保存し再生時に復元 |
| Blocker 4: 純粋仮想の NULL 判定矛盾 | G5 を明示 metadata（pure flag）方式に変更 |
| Blocker 5: 多重継承 `delete base_ptr` の free 先 | G6 に complete-object 補正設計と secondary-base delete 実行テストを追加 |
| High 1: `::` 読み飛ばしは silent miscompile | G1 をグローバル symbol table の正式 lookup に変更（shadow 時も正しい側を返す） |
| High 2: friend 読み捨ての過剰一般化 | G2 を `friend class Identifier;` のみに限定、他形は `tcc_error` |
| High 3: member chain consumer 未監査 | G3-P0（着手前監査）を新設 |
| High 4: G3 が 1 コミット | G3 は 1 branch / **P1〜P5 の 5 コミット**に変更 |
| High 5: branch 基点が曖昧 | §2 で「各ガイド完了後 master へ merge、次ガイドは最新 master 基点」と明示 |
| High 6: exit 0 だけでは弱い | G7 を「期待テスト件数固定値 + failures/errors = 0」ゲートに変更（基準値は G0 で取得） |
| High 7: 原本無改変が文章のみ | G0 で原本をコミットし、G7 で `git diff --exit-code -- sample/cppunit` を機械ゲート化 |

---

## 0. 現状の実測サマリ

| 条件 | 結果 | 最初の停止 |
|---|---|---|
| 既定 | **0 / 12 TU** | `#include <string>`（cuconfig.h:171）— STL 不在 |
| `-DMINIMUM_SET` | **0 / 12 TU** | 先頭 `::`（cuconfig.h:178、`CPPUNIT::` が空展開） |
| `-DMINIMUM_SET` + 先頭 `::` を手当て（コピーで実験） | 0 / 12 | クラス内 typedef（SimpleString.h:18 `typedef char value_type;`） |

**方針の根幹**: この CPPUnit は VC++6 / 古い GCC 向けの退避スイッチ（`cu_NO_*`）を備え、
`-DMINIMUM_SET` で **template / namespace / 例外 / RTTI / STL** を全部外し、同梱の
`SimpleString` / `SimpleList` / `SimpleAutoPtr`（マクロ版）に切り替わる。
`-Dcu_NO_EXPLICIT` を足せば `explicit` も空マクロになる（cuconfig.h:117-119）。
したがって tpp に足すのは下記 §1 の機能（**現時点で確認済みの範囲**）でよく、
template 等の大物は不要。

**ビルド条件（確定）**: `-DMINIMUM_SET -Dcu_NO_EXPLICIT -I sample\cppunit`

---

## 1. 現時点で確認済みの必要機能（実測ベース）

> **注意（Blocker 1 対応）**: 下表は「最初の停止地点を順に 3 回実測した結果 + grep による
> 構文の存在確認」であり、**全リストであることは証明されていない**。grep は「構文が存在する」
> ことしか示さず、tpp がその意味論を正しく処理できるかは各ガイドの実行テストで初めて確認できる。
> **CPPUnit 全 TU が通る（G7）以前は、G3 修正後などに別の未対応構文
> （`operator=` / 変換演算子 / enum / qualified static member / 暗黙 dtor / copy ctor /
> ポインタ調整 / 関数ポインタ等）が新たに露出する可能性がある。** 露出した場合は本プランを
> 改訂し、ガイドを追加してから実装する（無断でスコープ拡大しない）。

| # | 機能 | 使用数 | 現状 | 対応ガイド |
|---|---|---|---|---|
| 1 | 先頭 `::`（グローバルスコープ修飾） | 18 | `identifier が必要です` | G1 |
| 2 | `friend` 宣言の受理 | 1（`friend class Logger;`） | `';' が必要です` | G2 |
| 3 | クラス内 typedef | 29 | `'value_type' に対する無効な型` | G3-P1/P2 |
| 4 | 修飾型名 `Class::type` / `Class::Inner` | 15+ | 式として誤解釈 | G3-P3 |
| 5 | ネストクラスの二重修飾クラス外定義 `Outer::Inner::op++(int)` | 4 | 未対応 | G3-P4 |
| 6 | デフォルト引数でのクラススコープ名（`= npos`、10 箇所） | 10 | 未検証（再生時スコープ） | G3-P5 |
| 7 | `new` / `delete` | 31 / 19 | `'new' は宣言されていません` | G4 |
| 8 | `new T[n]` / `delete[]` | 4 / 5 | 同上 | G4 |
| 9 | 純粋仮想 `= 0` + 抽象クラス | 27 | `',' が必要です` | G5 |
| 10 | 仮想デストラクタ（BUG-24） | 8 | `identifier が必要です` | G6 |

**既に動くもの（実測確認済み、実装不要）**:
ネストクラス定義そのもの / `static const` メンバ + クラス外初期化 /
デフォルト引数（static 呼び出し・ctor 含む。ただしクラススコープ名の再生は #6 で別途）/
後置 `operator++(int)` / `const` メンバ関数 / 参照 / 単一継承（`Iterator : public IteratorBase`）。

**スコープ外（MINIMUM_SET が回避するため実装しない）**:
template（`TestCaller<Fixture>` 含む）/ namespace / 例外 / RTTI / STL。

---

## 2. ガイド分割と順序

規約どおり **1 ガイド = 1 feature branch**。
**branch 運用（High 5 対応・明示）**: 各ガイドは完了後に master へ merge し、
**次ガイドの branch はその時点の最新 master から作る**（全 branch を `cc31384` 固定起点に
しない — G5/G6 は G4 の成果物を前提とするため、固定起点では統合テストが成立しない）。
コミット粒度は原則 1 ガイド = 1 コミットだが、**G3 のみ P1〜P5 の 5 コミット**とする
（High 4 対応 — `git bisect` と回帰特定のため。1 branch = 1 guide は維持）。

```
G0 インベントリ + ベースラインゲート（小・実装なし）
→ G1 先頭::正式lookup（小）→ G2 friend class 限定対応（極小）
→ G3 クラス内typedef（大・P0監査 + P1〜P5 = 5コミット）
→ G4 new/delete（中）→ G5 純粋仮想 metadata（中）
→ G6 仮想dtor + complete-object delete（中〜大）→ G7 CPPUnit フルゲート（小）
```

依存関係: G4 の `delete` は dtor 呼び出しを含むため G6 より先に「非仮想 dtor 直呼び」で入れ、
G6 で vtable 経由 + complete-object 補正に昇格する。G5 の抽象クラス検査は `new` の拒否を
含むため G4 の後。G3 は最大だが他と独立なので、G1/G2 の後いつでも並行可。

---

## 3. 各ガイドの詳細

### G0: インベントリ + ベースラインゲート — `chore/cppunit-baseline`（新設）

実装を伴わない準備ガイド。以下を成果物とする。

1. **原本のコミット + baseline 固定（rev.3 Blocker 1 対応）**: `sample/cppunit` の原本
   `*.cpp / *.h` を無改変のまま master へコミットし、**そのコミットにタグ
   `cppunit-original-base` を打って baseline を固定**する。あわせて**そのコミットの
   hash を本プランへ追記して記録**する（rev.4 GO 時の Low 指摘 — タグが誤って
   force-update された場合でも hash 直指定で baseline を検証できる保険）。
   `git diff --exit-code -- sample/cppunit`（コミット指定なし）は「worktree vs index」の
   比較にすぎず、誤修正を `git add` / commit した時点で緑になるため**無改変の証明にならない**。
   以後の機械ゲートは必ず **baseline commit と現在の worktree の比較**
   （`git diff --exit-code cppunit-original-base -- <原本 *.cpp/*.h のパス>`）で行う
   （staged / committed / unstaged を問わず G0 原本との差を検出できる）。
2. **期待テスト件数の基準取得（High 6 の前提）**: **基準出力
   （`tests run = N / failures = 0 / errors = 0` 相当）を記録**する。
   取得手段は以下の優先順位とし、どの段で取得したかを記録に残す:
   1. MSVC（`test/cppuniut/` 既存ハーネス or 直ビルド）+ `-DMINIMUM_SET -Dcu_NO_EXPLICIT` で実行
   2. 別の既知正常な C++ コンパイラ（g++ 等が使えれば）+ 同一 defines で実行
   3. `-DMINIMUM_SET` の preprocessed source からテスト登録集合を抽出し、既定構成の
      実行結果 N と登録集合を突き合わせる
   4. 生ソース grep は最後の手段（登録マクロの条件コンパイル漏れを拾えないため）
3. **構文インベントリの拡充（Blocker 1 対応）**: `-DMINIMUM_SET` でプリプロセスした
   12 TU + all_test.cpp に対し、`operator=` / 変換演算子 / enum / qualified static member /
   copy ctor / 関数ポインタ / メンバポインタ等を grep し、§1 の表を拡充する。
   grep は「存在確認」であり意味論の保証ではないことを表に明記する。
4. **`new` / `delete` 全件の形別分類（rev.3 High 2 対応）**:
   `new[]` 4 箇所の型内訳（`rg "new\s+\w+\s*\[|delete\s*\[\]" sample/cppunit`）に加え、
   **scalar `new` 31 件 / `delete` 19 件も全件を形別に分類**して表にする:
   - `new` 側: `new Class` / `new Class()` / `new Class(args)` / `new POD` / `new POD()` /
     `new 修飾型名（Class::type）` の別
   - `delete` 側: 静的型が `Derived*` / `Base*`（仮想 dtor の要否・ポインタ調整の有無）/
     `POD*` / NULL になり得る経路の別
   G4 着手後に `new int()` のような未想定形が露出するのを防ぐ。非 POD の `new[]` が
   あれば G4 のスコープを再判断する。
5. **純粋仮想 27 件の分類（rev.4 High 4 対応）**: 27 件を
   **regular method / destructor（`virtual ~X() = 0;`）に分類**する。
   pure virtual destructor が 1 件でもあれば G5 は G6 と切り離せない
   （dtor のスロット化が前提になり、かつ pure virtual dtor は派生を生成する限り
   定義も必要）ため、**G5/G6 の順序と分割を再判断**する。0 件なら現行の
   G5 → G6 順で問題なし。

**受け入れ**: 上記 1〜5 の成果物（コミット + baseline タグ + 基準値記録 + インベントリ表）が揃うこと。

### G1: 先頭 `::` — `fix/cpp-global-scope-op`

**最小再現**（実測で NG 確認済み）:

```cpp
class C { public: int v; };
typedef ::C D;                    // error: identifier が必要です
int main(){ D d; d.v=0; return ::gfn(); }
```

**実装（High 1 対応 — 「読み飛ばし」から「正式 lookup」へ変更）**:
字句上 `::` は `':' ':'` の 2 トークン。`unary()` の式頭と `parse_btype()` の
型名解決入口で、`tok == ':'` かつ次トークンも `':'` の場合に両方読み、
続く識別子を**グローバルスコープの symbol table で明示的に解決**する。
「読み飛ばして通常解決」は、ローカルが同名グローバルを隠している場合に
**コンパイル成功のまま誤コードを生成する**（silent miscompile）ため採らない。

- 解決機構: `sym_find(tok)` の結果からシンボルチェーンを辿り、ファイルスコープ
  （グローバル）側のシンボルを選択する。tpp の Sym にスコープ判別情報
  （`sym_scope` 相当）がどう入っているかは**着手前に実機確認**する（事前 grep:
  `findstr /n "sym_scope local_stack global_stack" tccgen.c tcc.h`）。
- **フォールバック方針**: グローバル側の選択が既存機構で安価に実装できない場合でも、
  「shadow を検出したら `tcc_error`」までは必ず実装する。
  **「未対応なら error」>「間違ったコードを生成」**を原則とする。

**受け入れテスト**: `a9/gscope_*.cpp`
- typedef 位置 / 式位置 / 型宣言位置の 3 形
- **shadow テスト（必須・実行値検証）**:
  ```cpp
  int x = 1;
  int main() { int x = 2; return ::x == 1 ? 0 : 1; }
  ```
  正式 lookup 実装時は exit 0、フォールバック時はコンパイルエラーを期待値とする
  （どちらに落ちたかを tpp仕様.md に記録）。
- C モード非回帰: `.c` で `a ? b : c` の `:` と衝突しないこと
  （三項演算子のテストを負例側に含める。`cond ? a : ::b` の並びも負例に含める）。

### G2: `friend` 宣言の受理 — `feat/cpp-friend-skip`

**実装（High 2 対応 — 対応範囲を限定）**:
`tcctok.h` に `friend` を追加（C++ 専用キーワード、`.c` では識別子降格 — 既存 A-2 機構）。
`struct_decl()` のメンバループで `TOK_FRIEND` を見たら、**`friend class Identifier;` の
形のみ**読み捨てて受理する。それ以外の friend 宣言（`friend int fn(C&);` 等の
friend 関数宣言）は **`tcc_error` で拒否**する。

- 理由: `friend int f(C&);` は access 指定であると同時に**関数宣言**でもあり、
  読み捨てるとその宣言自体が消える（silent miscompile 系）。CPPUnit で必要なのは
  `friend class Logger;` の 1 件のみ（実測）なので、限定対応が安全。

> **注意（feature ブランチの実測教訓）**: `tcctok.h` を変更するとトークン番号がずれるため
> **/t:Rebuild（全リビルド）必須**。インクリメンタルだと「TU ごとにキーワードが
> 認識されない」不可解な状態になる（f14b534 のコミットメッセージに記録あり）。

**受け入れ**:
- 正例: `friend class X;` が通ること + `.c` で `int friend;` が通ること
- **負例**: `friend int fn(C&);` が `tcc_error` になること（rev.1 では正例だったものを負例へ変更）

### G3: クラス内 typedef と修飾型名 — `feat/cpp-class-typedef`（1 branch / P1〜P5 の 5 コミット）

最大のガイド。CPPUnit の骨格（SimpleString.h:18-27, SimpleList.h:18-75）が丸ごと依存する。

#### G3 共通: 型名 lookup 規則（rev.1 Blocker 2 + rev.3 Blocker 2-A/B/C 対応 — 本ガイドの設計中核）

unqualified な型名 `T` は以下の順で解決する。`cpp_cur_func_class` 1 個の検索では
**設計上足りない**（本プラン自身の受け入れ条件 `Node(value_type v=0)` /
`const_Iterator(const Iterator&)` が外側/兄弟クラスの解決を要求している）。

```
lookup_unqualified_type(name):

1. block/local scope を通常の name lookup で検索
   ├─ 型（VT_TYPEDEF）が見つかったら return
   └─ 非型シンボル（変数等）が見つかったら「型探索失敗」で確定
      （クラス側へフォールバックしない — non-type が typedef を隠す）

2. lookup_class(現在のクラス, name)   // ← qualified lookup と共通の部品
   2a. クラス自身のメンバ（typedef / ネストクラス）→ あれば return
   2b. 見つからなければ直接基底クラス群を lookup-set として merge
       lookup-set = { 正規化した型, subobject-set }
       ├─ typedef は「宣言」ではなく「その typedef が指定する型」へ
       │  正規化してから比較する（A::T=int と B::T=int は同一 → OK）
       ├─ 正規化後も異なる型が複数 base から見つかったら ambiguity エラー
       ├─ 一方の subobject-set が他方に支配される（同一 subobject 経由 /
       │  virtual diamond の同一宣言）場合は支配される側を捨てて 1 件扱い
       └─ 上記で判定しきれない複雑ケースは tcc_error に倒す
          （CPPUnit に不要なフル実装はしない。「先に見つかった base を
          採用して return」だけは絶対に禁止 — silent miscompile 源）

3. 見つからなければ外側クラスへ（内→外へ順に）
   → 各外側クラスにも lookup_class（自身 → その基底の merge）を適用

4. 最後にグローバル / enclosing scope（既存の通常解決）


lookup_qualified_class_type(C, name):   // Class::name 用（P3/P4 — 別関数にする）
   lookup_class(C, name) のみ（C 自身 → C の bases の merge）
   ※ enclosing class / global へは絶対にフォールバックしない
```

- **基底クラスは外側クラスより先**（rev.3 Blocker 2-A）。C++ の member lookup は
  「クラス自身 → その base 群」を 1 つのクラススコープとして探索し切ってから
  parent scope へ進む。rev.2 の「enclosing → base」順では
  `struct Inner : B { T x; };` で `B::T` より `Outer::T` を先に拾い silent miscompile になる。
- **多重 base の ambiguity 判定は省略不可**（rev.3 Blocker 2-B）。tpp は仮想 MI まで
  実装済みであり、「最初に見つかった base で return」の実装は
  `struct D : B1, B2 { T x; };`（B1/B2 が別の `T` を持つ）を無音で誤コンパイルする。
- **merge は「宣言一致」ではなく「指定する型」で比較**（rev.4 Blocker 1）。
  `struct A { typedef int T; }; struct B { typedef int T; }; struct D : A, B { T x; };`
  は両 base の `T` が同じ型 int を指定するため**エラーにしてはいけない**（宣言比較だと
  false error になる）。標準の merge（declaration set + subobject set、支配される側を
  捨てる規則）をフル実装する必要はなく、正規化比較で判定しきれないケースは
  `tcc_error` に倒してよい。
- **qualified lookup は別関数**（rev.4 Blocker 2）。`Class::name` は
  `lookup_qualified_class_type()`（指定クラス + その bases のみ）で解決し、
  unqualified 用の外側クラス/グローバルへのフォールバックを**共有しない**。
  `O::I::T`（I 自身にも I の base にも T がない）が外側の `O::T` を拾ったら誤り。
- Step 1 の非型シンボルによる隠蔽（rev.3 Blocker 2-C）: `int T = 0; T x;` は
  `C::T` に逃げず**エラー**にする。
- Step 3 の enclosing chain を辿るために、ネストクラスのパース時に「外側クラスへの参照」を
  クラス Sym に保持する（既存構造にあるかは**着手前に確認**、無ければ P1 で追加）。

#### P0 — member chain consumer 監査（High 3 対応・P1 の前に必須、コミット対象はドキュメント）

`VT_TYPEDEF` という「サイズもアドレスも持たない偽メンバ」をメンバチェーンへ混ぜる前に、
メンバチェーンを走査する**全 consumer を grep で列挙し、各 consumer が VT_TYPEDEF を
どう扱うべきかを表にする**。最低限の監査対象:

```
struct_layout / find_field / メンバ式 lookup / ctor 生成 / dtor 生成 /
vtable 走査（cpp_assign_virtual_slots / cpp_emit_vtable / cpp_emit_secondary_vtables）/
重複メンバ検査 / オフセット計算 / デバッグシンボル出力（tccdbg.c）
```

事前 grep 例: `findstr /n "->next" tccgen.c` をクラスメンバ文脈で絞り込み +
`findstr /n "find_field struct_layout" tccgen.c tccdbg.c`。
**判定基準（rev.3 High 1 対応 — 件数ではなく invariant）**: 監査表をもとに
「既存の member-chain invariant（全要素がサイズ/オフセットを持つ、find_field が
実メンバのみ返す、vtable 走査が関数メンバのみ見る等）を壊す consumer が 1 つでもあり、
かつ `if (is_typedef) continue;` 相当の局所 skip で自然に処理できない」場合は、
typedef 用の別チェーン（クラス Sym にぶら下げる独立リスト）へ設計変更する。
逆に全 consumer が局所 skip で安全なら件数が多くても member chain のままでよい。
監査表は本プランへ追記してから P1 に着手する。

#### P1 — クラス本体内の typedef 宣言（コミット 1）

`struct_decl()` のメンバ解析で `VT_TYPEDEF` ストレージを受理し、P0 の監査結果に
従った格納先（メンバチェーン or 別チェーン）へ登録する。`struct_layout()` は
VT_FUNC メンバと同様にスキップ（レイアウトに影響させない）。
クラス本体内の**後続宣言**から unqualified で見えること:

```cpp
class S {
  typedef char value_type;
  typedef value_type* pointer;      // typedef が typedef を参照
  static const size_type npos;      // typedef を型に使う static メンバ
  value_type buf[8];                // データメンバの型
};
```

#### P2 — メソッド本体内の unqualified 解決（コミット 2）

インライン本体とクラス外定義本体の中で `iterator it;` のように使えること。
上記 **`lookup_unqualified_type()` の規則 1〜4 を実装**し（qualified 用の
`lookup_qualified_class_type()` は P3 で接続）、`parse_btype()`（BUG-20 で入れた
`cpp_lookup_type_name()` の cpp 分岐）へ接続する。
**BUG-21 ガードとの整合に注意**: typedef は「型」なので隠蔽判定の許可側。

#### P3 — 修飾型名 `Class::type` / `Class::Inner`（コミット 3）

現在 [tccgen.c:8646-8655](tccgen.c#L8646-L8655) は `cls ::` を見ると unget して式パーサへ回す。
ここを拡張し、`::` の次の識別子がそのクラスの typedef メンバまたはネストクラスなら
**型として解決**する。これでクラス外定義の戻り値型が通る:

```cpp
SimpleList::iterator SimpleList::insert(iterator pos, value_type value)  // SimpleList.cpp:114
```

`Class::member`（式）との判別は「次の識別子がそのクラスで型か否か」で行う。
型でなければ従来どおり unget して式へ（`Class::npos` 等の静的メンバ式を壊さない）。
修飾名の探索は **`lookup_qualified_class_type()`**（G3 共通規則で定義した別関数 —
指定クラス自身 + その bases のみ）で行う（`Derived::base_typedef` はこれで通る）。
**`lookup_unqualified_type()` を流用しない** — 外側クラス/グローバルへの
フォールバックが混入すると `O::I::T` が外側の `O::T` を拾う誤実装になる（rev.4 Blocker 2）。

#### P4 — 二重修飾のクラス外定義（コミット 4）

`parse_cpp_scope_qualifier` を多段化し `SimpleList::Iterator::operator++(int)` を受理する。
対象は Iterator / const_Iterator の後置 `++` `--` の 4 定義（SimpleList.cpp:215-236）。

#### P5 — デフォルト引数の defining scope 保存（コミット 5・Blocker 3 対応で独立フェーズ化）

デフォルト引数はトークン保存・呼び出し点で再生される方式。呼び出し地点は
宣言クラスのスコープではないため、トークンだけ再生すると `= npos` が
**global lookup になり解決失敗（または誤解決）する**。「テストして失敗したら直す」
ではなく、以下を設計として実装する:

- デフォルト引数の保存データを `tokens + 定義クラス（owning class）+ 宣言スコープ` に拡張する
  （既存の保存構造体にフィールド追加。構造体名は**着手前に確認**:
  `findstr /n "default_arg" tccgen.c tcc.h`）。
- 再生時に **owning class / declaration scope を復元**してからトークンを再生し、
  再生後に必ず復元解除する（エラーパスでも復元解除 — 早期 return に注意）。
  復元は**型名と値名の両経路**に効かせる（rev.4 GO 時の明確化指摘 — P5 の実対象
  `= npos` は**型ではなく値メンバ**であり、`lookup_unqualified_type()` だけでは通らない）:
  - **型名**が現れた場合: `lookup_unqualified_type()` が復元した class scope を使用
  - **値名・関数名**（`npos` 等）が現れた場合: 既存の式 identifier / member lookup が
    復元した `cpp_cur_func_class` / declaration scope を使用
- **着手前確認に追加**: デフォルト引数トークン再生時の unqualified value-name 解決が
  `cpp_cur_func_class` を参照しているか（していなければ復元しても効かない）を
  実機で確認する。

#### G3 受け入れテスト（`a9/`）

- 正例: P1〜P4 の各形。
- 正例（P5・実行値まで検証）: SimpleString.h:31 の形
  ```cpp
  class S {
  public:
      static const int npos;
      void f(int n = npos);   // 呼び出しは非クラススコープから
  };
  ```
  `main` から `s.f();` を呼び、`npos` の実値が渡ることを exit code で検証。
- 正例（**shadow — Blocker 2 の必須テスト**）:
  ```cpp
  class C { public: typedef int T; int f(); };
  int C::f() { typedef char T; return sizeof(T) == sizeof(char) ? 0 : 1; }
  ```
- 正例（**enclosing class typedef**）:
  ```cpp
  class Outer { public: typedef int T; class Inner { public: T value; }; };
  ```
- 正例（**基底クラス typedef**）: `class B { public: typedef int T; }; class D : public B { T x; };`
- 正例（**base beats enclosing — rev.3 Blocker 2-A、実行値まで検証**）:
  ```cpp
  struct B { typedef char T; };
  struct O {
      typedef int T;
      struct I : B { T x; };   // ここは B::T（char）でなければならない
  };
  // 検証は構文依存を避けてインスタンス経由で行う（rev.4 レビュー細部指摘）:
  // O::I i;  return sizeof(i.x) == sizeof(char) ? 0 : 1;
  ```
- 負例（**ambiguous bases — rev.3 Blocker 2-B**）:
  ```cpp
  struct A { typedef int T; };
  struct B { typedef char T; };
  struct D : A, B { T x; };   // 指定する型が異なる → ambiguity エラーになること
  ```
- 正例（**同一型を指定する多重 base — rev.4 Blocker 1、false error 防止**）:
  ```cpp
  struct A { typedef int T; };
  struct B { typedef int T; };
  struct D : A, B { T x; };   // 両方 int を指定 → エラーにせず通ること
  ```
- 負例（**qualified lookup は enclosing へ逃げない — rev.4 Blocker 2**）:
  ```cpp
  struct O {
      typedef int T;
      struct I {};
  };
  O::I::T x;   // I にも I の base にも T はない → エラー（O::T を拾ったら誤り）
  ```
- 負例（**non-type local が typedef を隠す — rev.3 Blocker 2-C**）:
  ```cpp
  class C {
  public:
      typedef int T;
      void f() {
          int T = 0;
          T x;               // C::T へ逃げず、エラーになること
      }
  };
  ```
- 正例: ネストクラスの ctor + mem-init（`Node(value_type v=0) : next(0),prev(0),data(v) {}`）、
  兄弟ネストクラスを引数に取る変換 ctor（`const_Iterator(const Iterator&)`）
- 負例: 他クラスの typedef が unqualified で漏れて見えないこと（`a9/negative/`）
- 最終ゲート: **`SimpleString.h` / `SimpleList.h` を include するだけの TU が原本のまま `-c` で通る**

### G4: `new` / `delete`（+配列形）— `feat/cpp-new-delete`

**参照実装**: feature/cpp-support の `f14b534`。アプローチ（コミットメッセージより）:
- `new C(args)`: malloc → 結果を一時ローカルへ退避 → ctor 呼び出し。一時変数は `loc` から
  確保するため入れ子の new も安全
- `delete p`: `gv_dup` でポインタ複製 → dtor → free の順（**この段階では非仮想 dtor 直呼び。
  仮想 dtor / complete-object 補正は G6 で昇格** — G6 の設計を参照）
- C モードでは `new` / `delete` は通常の識別子（A-2 降格）

ただし**移植ではなく再実装**。master の ctor/dtor 機構（`__cpp_ctor_<Class>` マングル、
`cpp_find_ctor_field` / `cpp_find_dtor_field`、BUG-15/16 の「this はポインタで渡す」規約）に
合わせる。malloc/free は CRT 直呼び（`dev/lib` の既存リンクで解決）。

**scalar POD 形（rev.4 High 3 対応）**: G0 の形別分類の結果で分岐する:
- `new POD` / `new POD()` が **0 件**なら今回スコープ外（`tcc_error` で拒否し負例に登録）。
- **存在する**なら G4 に scalar POD allocation を追加する。このとき
  **`new int`（未初期化のまま）と `new int()`（ゼロ初期化 = value-init）は別処理**で
  あることを実装に明記し、両形を実行値（`*p` の値）まで検証する。
  「分類したのに設計判断に接続しない」を避けるため、分岐のどちらを採ったかを
  G4 コミットメッセージに記録する。

**配列形**: 型内訳は G0 で確定済みの前提（未実施なら G4 着手前に必ず grep）。
SimpleString.cpp は `char`（POD）の `new value_type[n]`。第一段は **POD のみ対応**
（malloc 相当 + 要素 ctor なし）、非 POD 配列は `tcc_error` で拒否して負例に登録
（無音の未初期化を作らない）。

**受け入れ**: feature の `dev/class_abstract_new_test.cpp` から **new/delete 部分のテストを
流用**（テストは移植可能、実装は不可）+ 自前の入れ子 new / delete NULL / new[] POD。
`delete 0;`（NULL delete は no-op）を正例に含める。

> `tcctok.h` 変更（new/delete/friend 等）が入るガイドはすべて **/t:Rebuild**。

### G5: 純粋仮想 + 抽象クラス — `feat/cpp-pure-virtual`

**設計（Blocker 4 対応 — NULL 判定を廃止し明示 metadata 化）**:
rev.1 の「スタブを挿す」と「NULL スロットで抽象判定」は矛盾していた
（スタブを挿した時点で NULL ではない）。rev.2 では**関数ポインタと pure フラグを分離**する:

```
// スロット metadata（cpp_assign_virtual_slots が管理するスロット記録に追加）
#define CPP_VSLOT_PURE 0x01

純粋仮想宣言時:   function pointer = __cpp_pure_virtual スタブ, pure flag = 1
派生で override:  function pointer = derived_func,             pure flag = 0
抽象判定:         クラスの最終スロット集合に pure flag == 1 が 1 つでも残る => abstract
```

- `virtual R f() = 0;` は本体なしで this 挿入済みシンボルだけ登録（呼び出し側の型解決用。
  参照は vtable 経由のみなのでリンクは通る）
- スタブ本体は abort 相当（呼ばれたら即死）を **TU ごとに 1 個、internal/static symbol
  （非 export・TU ローカル）として発行**する（rev.3 High 3 対応）。CPPUnit は 13 TU を
  リンクするため、global symbol で発行すると duplicate symbol になる。
  「1 TU あたり 1 個（static）」であり「最終リンク全体で 1 個」ではないことを明記する
  （vtable スロットは各 TU 内の自 TU スタブを指せばよく、アドレス同一性は不要）。
- 抽象クラスは**オブジェクト宣言と new を拒否**、ポインタ/参照は可。
- master 側は vtable を `greloca` で直接構築するため、スタブ挿入と pure flag は
  `cpp_assign_virtual_slots` のスロット記録 + `cpp_emit_vtable` /
  `cpp_emit_secondary_vtables` に組み込む。

**受け入れ**:
- 派生が全スロットを override すれば生成可 / 一部未 override は依然抽象
- **継承経由の抽象維持（レビュー指定テスト）**:
  ```cpp
  struct A { virtual void f() = 0; };
  struct B : A { };            // override なし → B も抽象
  struct C : B { void f() {} }; // ここで初めて具象
  ```
  `B b;` / `new B` が負例、`C c;` が正例。
- 抽象クラスの値宣言・new が**コンパイルエラー**（負例 2 本）
- `Base* p = new Derived;` で純粋仮想経由のディスパッチが正しい値を返す（実行検証）
- 多重継承との併用（非 primary 基底の純粋仮想）は G6 完了後に追加検証。

### G6: 仮想デストラクタ + complete-object delete — `feat/cpp-virtual-dtor`（BUG-24）

**最小再現**（実測）: `struct B { int t; virtual ~B() {} };` → `identifier が必要です`。

**実装**:
1. パース: `virtual` フラグ付きの `~Class()` を struct_decl の dtor 経路
   （FEAT-4E `__cpp_dtor_fld_*`）に接続する。現在 `virtual` の後に `~` が来ると
   declarator が識別子を要求して落ちる。
2. スロット化: dtor に vtable スロットを割り当てる。**参照アプローチ**（f14b534）:
   dtor は「固定スロット名」で記録し、派生側が `virtual` を書かなくても名前一致で
   override させる（C++ の暗黙 virtual 継承と同じ意味論）。master では
   `cpp_find_dtor_field` の結果を `cpp_assign_virtual_slots` に含める形。
3. ディスパッチ: `delete base_ptr;`（G4）と明示 `p->~B()` を vtable 経由にする。
   ローカル変数の自動 dtor（静的型=動的型）は従来どおり直呼びで良い。

**complete-object 問題（Blocker 5 対応 — 本ガイドの設計中核）**:
多重継承では `D : A, B` に対し `B *b = d;` で `b != d`（B subobject へのポインタ調整）に
なり得る。このとき `b->virtual_dtor(); free(b);` は **malloc が返した先頭アドレスではなく
B subobject を free する** — 即クラッシュまたは heap corruption。
「単一継承テストは全部緑、多重継承だけ heap 破壊」という最悪の形で潜伏するため、
**G6 実装前に設計を確定**する。

**採用案（第一候補）— vtable に offset-to-top を持たせる（Itanium ABI 同等）**:
- 各 vtable（primary / secondary）の関数ポインタ列の直前に
  「この vtable が張られた subobject から complete object 先頭までのオフセット」を格納する
  （primary は 0、secondary は負値）。master の `cpp_emit_secondary_vtables` は
  subobject オフセットを既に知っているため、emit 時に定数で書ける。
- **ABI の固定（rev.3 High 4 対応）**:
  - **vptr は従来どおり slot[0]（最初の関数ポインタ）を指す**。offset-to-top は
    **`vptr[-1]`** に置く（vtable シンボルの先頭に 1 フィールド前置し、vptr 初期化時は
    先頭 + 1 スロット分のアドレスを書く）。
  - **格納幅はターゲットのポインタ幅（Win32 = 4 / x64 = 8 バイト）の signed 整数**。
    `int` 固定にすると x64 で slot 位置がずれ vtable レイアウトが変質するため禁止。
  - **invariant**: 既存の仮想呼び出し `vptr[0]`, `vptr[1]`, … のインデックスは
    一切ずらさない。既存の vmi_* 回帰 8 本が無変更で通ることをこの invariant の
    検証に使う。
- `delete p;` のコード生成: **最初に NULL check（NULL なら全体 skip — vptr 読みより
  必ず先。rev.3 High 5）** → `p` を複製 → vtable から offset-to-top を読み
  `complete = p + offset` を先に計算・退避 → 仮想 dtor を vtable 経由で呼ぶ
  （dtor thunk が this を補正するのは既存の仮想 MI thunk 機構と同じ）→ `free(complete)`。
- **経路限定の invariant（rev.4 High 6 対応）**: 上記の vptr[-1] 経路は
  **静的型で選択された dtor が virtual の場合のみ**適用する。non-virtual dtor /
  非 polymorphic クラス / POD の `delete p` は **G4 の「直呼び dtor + free(p)」を維持**する
  （delete 共通経路に入れると vptr を持たないオブジェクトの vptr を読んで即クラッシュ）。
- 代替案（deleting destructor を vtable に別スロットで持つ / delete 側で static に
  補正する）は、静的型で動的型の subobject オフセットを知り得ないため後者は不可、
  前者は vtable レイアウト変更が大きいため第一候補が困難な場合の予備とする。
- **着手前確認**: master の vtable レイアウト（先頭に何があるか、仮想 MI Phase 2 の
  secondary vtable 構造）を `cpp_emit_vtable` / `cpp_emit_secondary_vtables` で実機確認し、
  offset-to-top フィールド追加が既存の vptr 初期化・thunk と整合するか検証する。
  整合しない場合はプラン改訂に戻す（無断で別方式に切り替えない）。

**受け入れ**:
- `Base* p = new Derived; delete p;` で **Derived → Base の順に dtor が走る**
  ことを副作用（グローバル変数への加算順）で実行検証。
- **secondary-base virtual delete（レビュー指定・最重要テスト、rev.3 Blocker 3 対応で
  順序依存形に変更）**: rev.2 の加算式（`+1 / +10 / +100` で合計 111）は
  どの実行順でも 111 になり**順序を一切検証できない**ため、桁送り式にする:
  ```cpp
  int state;
  struct A { virtual ~A() { state = state * 10 + 3; } };
  struct B { virtual ~B() { state = state * 10 + 2; } };
  struct D : A, B { ~D() { state = state * 10 + 1; } };
  int main() {
      D *d = new D;
      B *b = d;          // b != d（ポインタ調整）
      delete b;          // complete object を free できること
      return state == 123 ? 0 : 1;
  }
  ```
  期待値 123 は **D 本体 → ~B → ~A（基底は宣言の逆順）** 以外では成立しない。
  ASan 相当なしでも heap corruption を起こさず exit 0 になること。
  （可能なら Debug CRT ビルドの実行でヒープ検査も併用する — 未確認: tpp 出力 exe と
  Debug CRT の組み合わせ可否。）
- **virtual `delete NULL` の再テスト（rev.3 High 5 対応）**: G4 の `delete 0;` no-op は
  非仮想経路での検証にすぎない。G6 で delete 経路が「vptr 読み → offset-to-top →
  仮想 dtor」に変わるため、**NULL check を vptr 読みより先に行う**ことを
  ```cpp
  struct B { virtual ~B() {} };
  int main() { B *p = 0; delete p; return 0; }
  ```
  で G6 の回帰テストとして再検証する（NULL check が後だと G4 緑のまま G6 でクラッシュ）。
- **非 virtual `delete` の回帰（rev.4 High 6 対応）**: G6 実装後も
  ```cpp
  struct P { int v; ~P() {} };      // non-virtual dtor（vptr を持たない）
  int main() { P *p = new P; delete p; return 0; }
  ```
  が G4 の直呼び経路のまま動くこと（G6 経路が delete 共通に混入していれば
  vptr 読みでクラッシュして検出できる）。POD ポインタの `delete` も 1 本含める。
- 仮想 MI との併用（vmi_* 回帰 8 本 + セカンダリ vtable の dtor スロット）。
- 既存の `a9/negative` に「仮想 dtor が今まで通り拒否されること」を期待する負例は**無い**ことを
  確認済み（bug22 テストのコメントには「未対応」と書いたので、G6 で本文コメントを更新する）。

### G7: CPPUnit ビルドゲート — `feat/cppunit-build-gate`

1. **`all_test_tpp.cpp` を新規作成**（案 A・§7.4 の決定どおり）: 例外なし・
   TEST_ASSERT/TEST_FAIL 系のみでライブラリ全機能を叩く。意図的失敗を含めないため
   **failures = 0 / errors = 0 / exit 0 が正**。テスト登録数 N をここで固定する。
2. `sample/cppunit/build_cppunit.bat` を新規作成:
   - ライブラリ 12 TU + `all_test_tpp.cpp` を `-DMINIMUM_SET -Dcu_NO_EXPLICIT -I . -w` で `-c`
   - リンク（`-luser32 -lkernel32`。TestRunner はコンソール API のみ使用）
   - `all_test_tpp.exe` を実行。
3. **実行結果ゲート（High 6 対応）**: exit 0 だけでは「テスト登録数 = 0 でも緑」を
   検出できない。`all_test_tpp.exe` の出力をパースし、
   **`実行テスト数 == N（手順 1 で固定した値）` かつ `failures == 0` かつ
   `errors == 0`** を errorlevel で検証する。照合は **`findstr /x`（完全行一致）**
   または件数・failures・errors を個別に数値抽出しての比較とする（rev.4 High 5 対応 —
   部分一致だと「N=1」の検索文字列が「N=10」の行にもヒットしてゲートが抜ける）。
   件数が基準値と異なればゲート失敗。
   **相互検証**: 同じ `all_test_tpp.cpp` を MSVC でもビルド・実行し、同一の N/0/0 に
   なることを G7 完了時に 1 回確認する（ドライバ自体のバグと tpp の誤動作を切り分け）。
4. **原本無改変の機械ゲート（rev.3 Blocker 1 対応 — baseline commit 比較）**: bat 内で
   ```
   git diff --exit-code cppunit-original-base -- sample/cppunit/*.cpp sample/cppunit/*.h
   ```
   を実行し、**G0 で固定した baseline commit（タグ `cppunit-original-base`）と
   現在の worktree の原本 `*.cpp / *.h`** を比較する。
   コミット指定なしの `git diff -- path` は「worktree vs index」比較のため、
   途中ガイドで誤修正を commit してしまうと G7 で緑になり無改変の証明にならない —
   baseline 指定なら staged / committed / unstaged を問わず検出できる。
   pathspec を原本 `*.cpp / *.h` に限定するので、`build_cppunit.bat` 等の
   新規追加ファイルはゲートに引っかからない。
   （`all_test_tpp.cpp` は baseline 後の新規追加ファイルなので pathspec に含まれず、
   ゲートに影響しない）
5. `dev/test/run_all.bat` 末尾から `call` する（`pushd "%~dp0"` 規約を厳守）。

---

## 4. 全体の完了条件

- `sample/cppunit` のライブラリ 12 TU + ヘッダ 19 本が原本無改変・
  `-DMINIMUM_SET -Dcu_NO_EXPLICIT` でコンパイルでき、新規ドライバ `all_test_tpp.cpp` と
  リンクした `all_test_tpp.exe` が **期待テスト件数 N / failures 0 / errors 0** で
  exit 0（§G7 の機械ゲート。all_test.cpp 原本は案 A によりスコープ外 — §7.4）。
- `git diff --exit-code cppunit-original-base -- sample/cppunit/*.cpp sample/cppunit/*.h`
  が緑（G0 baseline commit との比較による原本無改変の機械証明）。
- 各ガイドで `build.bat` 緑（`tccgen.c` 変更のため **Release x64 / Debug Win32 の両方**）、
  `run_all.bat` 0 gating failure。
- **amateras 回帰**: `cross.h` の C / C++ 両モードコンパイル（`STBI_NO_SIMD` なし）と
  CUnit `test_cross.c` の実行が引き続き通ること（各ガイドの完了ゲートに含める）。
- `tpp仕様.md` の §3（動作しない機能）から実装済み項目を削除し、§7 の一覧を更新
  （G1 の shadow 挙動がフォールバック実装になった場合はその旨も記載）。
- `実装済み.md` へ機能追加を反映。

---

## 5. リスクと未確認事項

| 項目 | 内容 | 手当て |
|---|---|---|
| 未対応構文の追加露出（Blocker 1） | G3 以降で operator= / 変換演算子 / enum 等が新たに停止点になる可能性 | G0 のインベントリで先回り検出。露出したらプラン改訂 → ガイド追加（無断拡大しない） |
| G1 のグローバル lookup 実装可否 | tpp の Sym にスコープ判別が安価に取れるか未確認 | 着手前 grep で確認。不可なら「shadow 検出時 error」フォールバック（silent miscompile は作らない） |
| VT_TYPEDEF の consumer 影響（High 3） | member chain 走査箇所が typedef 偽メンバで壊れる可能性 | G3-P0 の全 consumer 監査を P1 前に必須化。member-chain invariant を壊す consumer があれば別チェーンへ設計変更（件数基準は廃止） |
| G3 lookup の探索順・ambiguity | base より enclosing を先に引く、または多重 base の先勝ち実装は silent miscompile | G3 共通の lookup 規則（クラス自身 → bases merge → enclosing）と ambiguity 負例で固定。merge は指定型へ正規化して比較（宣言比較は false error）。qualified は別関数で enclosing へ逃がさない |
| 純粋仮想 27 件に dtor が含まれるか | **未確認**。`virtual ~X() = 0;` があると G5 が G6 と切り離せない | G0 の分類（rev.4 High 4）で確定。含まれれば G5/G6 の順序・分割を再判断 |
| default 引数の再生スコープ（Blocker 3） | 保存構造体へのフィールド追加が既存再生経路と整合するか | G3-P5 で defining scope 保存を設計として実装（テスト任せにしない）。保存構造体は着手前確認 |
| vtable への offset-to-top 追加（Blocker 5） | 既存 vptr 初期化・仮想 MI thunk とのレイアウト整合が未確認 | G6 着手前に `cpp_emit_vtable` / `cpp_emit_secondary_vtables` を実機確認。整合しなければプラン改訂 |
| `SimpleAutoPtr` マクロ版 | `cu_AUTO_PTR(T)` は `SIMPLE_AUTO_PTR(T)`（SimpleAutoPtr.h:64、token-paste でクラス名生成）に展開。TestRunner.cpp:255-258 が使用。クラス本体を生成する DECLARE マクロの呼び出し箇所が TestRunner.cpp 内にあるかは**未確認** | G7 で自然に露見する。無ければ TestRunner.cpp 側の構造を再調査（原本改変はしない前提で cuconfig の別スイッチを探す） |
| MSVC で MINIMUM_SET ベースラインが取れるか | G0 の期待テスト件数取得手段が未確認 | G0 記載の 4 段優先順位（MSVC 実行 → 他コンパイラ → preprocessed 登録集合比較 → 生 grep）で取得し、どの段で取ったか記録 |
| BUG-21/22/23 との相互作用 | G3 の unqualified 解決は BUG-21 の「クラススコープ優先」ガードに重なる | 既存 a9 の bug20〜23 テスト全通過を各ガイドのゲートに含める（run_all で自動） |
| `tcctok.h` 変更 | トークン番号ずれで既存 `.o` と不整合 | 該当ガイド（G2/G4/G5/G6）は必ず /t:Rebuild。build.bat は常に全ビルドなので運用上は既定で満たされる |
| feature/cpp-support がクラス内 typedef を持つか | **未確認**（feature の CPPUnit 検証は先頭 `::` で停止したため到達せず） | 参照するのは new/delete・純粋仮想・仮想 dtor の 3 概念のみ。G3 は master 機構で新規設計 |
| 三項演算子との `:` 衝突（G1） | `cond ? a : ::b` のような並びは字句上 `: :` に見える | G1 の負例に含める。判定は「式頭 or 型名位置」に限定し、二項の後には適用しない |

---

## 6. feature/cpp-support の扱い（決定事項の再掲）

- **マージしない**。実測: 双方向差分 378 ファイル、`git merge-tree` で衝突 8 ファイル、
  うち `tccgen.c` に衝突ブロック 266。同一機能の並行実装のため意味的解決が必要で、
  中間状態は常にビルド不能になる。
- 参照するのは **概念とテスト**のみ:
  - `f14b534` — new/delete・純粋仮想・仮想 dtor の設計判断（本プラン G4/G5/G6 に反映済み）
  - `dev/class_abstract_new_test.cpp` / `dev/abstract_negative_test.cpp` — テストを移植
- feature 固有の vtable 方式（ソーストークン合成）は採らない。master の greloca 方式
  （仮想 MI Phase 2 まで実装済み）が上位のため。

---

## 7. G0 実測結果（2026-08-09 実施）

**baseline**: コミット `ad882a3c5673a238354d6ad72bac88342a18335e`（タグ `cppunit-original-base`、
branch `chore/cppunit-baseline`）。原本 32 ファイル（.cpp 13 + .h 19）を無改変でコミット済み。

### 7.1 期待テスト件数の基準（取得手段: 優先順位 1 = MSVC 実行）

MSVC（VS2026 x64 cl 14.50）+ `-DMINIMUM_SET -Dcu_NO_EXPLICIT` でビルド・実行した実測:

- 12 ライブラリ TU は**そのままコンパイル成功**（警告のみ）。
- **all_test.cpp は MSVC でもコンパイル不能**（後述 7.4）。`/FI stdio.h /FI stdexcept` の
  強制 include（原本無改変のまま）を足すと 13 TU 全部通る。
- 実行結果（基準値）: **実行数 = 1 / 失敗数 = 2 / エラー数 = 0 / exit code = 1**。
  意図的失敗テストを含むため「failures = 0 / exit 0」は正解ではない。
  ただし失敗 2 件のうち 1 件は `throw std::runtime_error` → `catch` 経由で記録されるため、
  **この基準値自体が例外機構に依存**する（例外なしでは同じ数字にならない）。

### 7.2 構文インベントリ（12 TU + ヘッダ、MINIMUM_SET 到達コードのみ）

| 構文 | 実測 | 判定 |
|---|---|---|
| enum / 変換演算子 / 多重継承 / 生の関数ポインタ | **各 0 件** | 対応不要。CPPUnit 内に MI delete は無い（G6 の secondary-base delete はコンパイラ安全要件として維持） |
| 純粋仮想 | **宣言 9 件**（Test.h 4 / TestListener.h 4 / TestCase.h 1）、**すべて regular method、pure virtual dtor 0 件** | G5 → G6 の順序で問題なし（rev.4 High 4 解決） |
| 仮想 dtor | 8 件（Test/TestCase/TestDecorator/TestListener/Mutex/SimpleListener/Logger/TestSuite） | G6 対象。継承はすべて単一 |
| friend | `friend class Logger;`（TestRunner.cpp:34）の **1 件のみ** | G2 の限定対応で十分 |
| `= npos` デフォルト引数 | 9 宣言（SimpleString.h） | G3-P5 対象 |
| 演算子オーバーロード | SimpleString.h に free 演算子 24 本（`+ == != < <= > >=`）+ メンバ `= += []`、Iterator 系にメンバ `* -> 前置/後置 ++ -- == !=`、コピー禁止 idiom の private 宣言 12 クラス | 大半は FEAT-6A ext1〜6 で実装済み。**単項 `operator*()`（deref）と `operator->()` は ext3 一覧に無く対応未確認**（`*p` の実呼び出しが TestRegistry.cpp:16 等にあり）→ G1 前に smoke で確認、未対応ならガイド追加 |
| 関数形式キャスト | `size_type(-1)`（SimpleString.cpp:33、typedef 名のキャスト） | **対応未確認**。G3 の受け入れに smoke を追加して確認 |

### 7.3 new / delete の形別分類（コメント除外、実コードのみ）

- scalar `new`: **11 件、全て `new Class(...)` 形**（`new MessageOutputTest()` /
  `new Node()` / `new Node(value)` / `new Entry(name, test)` / `new Mutex()` /
  `new TestFailure(*失敗)` ×2 / `new Logger(fp)` / `new Logger(stdout)` /
  `new SimpleListener()`）。**`new POD` / `new POD()` は 0 件 → scalar POD new は
  スコープ外に確定**（rev.4 High 3 の分岐は「0 件」側。`tcc_error` で拒否 + 負例）。
- `new[]`: **4 件、全て `new value_type[size]`**（SimpleString.cpp、value_type =
  クラス内 typedef の char）。POD のみで G4 第一段の想定どおり。ただし**型名が
  クラス内 typedef なので G4 の new[] は G3 完了に依存**する（順序は現行どおりで整合）。
- scalar `delete`: 14 件。うち **`Test*` / `TestListener*` 経由の仮想 delete**
  （TestRegistry.h:81、TestDecorator.cpp:18、SimpleAutoPtr の `delete m_ptr`）が G6 実対象。
  `delete m_ptr` は NULL になり得る（release 後）→ NULL no-op 必須。
  `delete (Entry*)*p` / `delete (TestFailure*)*p` は **cast 式 + iterator の `operator*()`
  呼び出し**を含む。
- `delete[]`: 5 件（SimpleString.cpp、char 配列）。
- rev.1 の「31 / 19 件」はコメント内の使用例を含む grep 数だった（実コードは上記）。

### 7.4 【要方針判断】all_test.cpp は MINIMUM_SET で原本のままコンパイル不能

G0 の狙いどおり「追加要求の露出」が発生。**ライブラリ 12 TU ではなくドライバ
all_test.cpp 側**に、MINIMUM_SET と非互換の問題が 3 点ある:

1. **`<stdio.h>` を自前で include していない**（printf/FILE/stdout が未宣言）。
   既定構成では `<string>` 等の間接 include に依存していた。MSVC でもエラー。
   `/FI stdio.h` 相当（tpp は `-include`）で原本のまま回避は可能。
2. **生の `try` / `catch (std::exception&)` / `throw std::runtime_error` を直接使用**
   （cu_TRY マクロを使っていない）。例外はスコープ外のため **tpp では回避不能**。
   `<stdexcept>` も include していない。
3. **意図的失敗テストを含む**（基準は failures=2 / exit 1）。かつ失敗 1 件は
   throw/catch 経由のため、例外なしのランタイムでは同じ基準値を再現できない。

**ライブラリ側（12 TU + ヘッダ）は TEST_FAIL = `addFailure + return` 方式で例外なしに
完結する**ため、問題はドライバのみ。方針の選択肢:

- **案 A（推奨）**: 原本無改変ゲートの対象を「ライブラリ 12 TU + ヘッダ 19 本」とし、
  ドライバは tpp 用に新規作成（`all_test_tpp.cpp` — 例外を使わず TEST_ASSERT 系のみで
  ライブラリ全機能を叩く。テスト数を増やして件数ゲートを強化できる）。
  all_test.cpp 原本は「例外要求のためスコープ外」として tpp仕様.md に記録。
- 案 B: 例外サブセットを実装 — スコープ外・巨大。非推奨。
- 案 C: all_test.cpp を修正 — 原本無改変の完了条件に違反。非推奨。

**決定（2026-08-09・ユーザー承認）: 案 A を採用。**
- 原本無改変ゲート対象 = ライブラリ 12 TU + ヘッダ 19 本（all_test.cpp は対象から除外。
  原本はリポジトリに残すが、tpp でのビルド対象・ゲート対象にしない）。
- `all_test_tpp.cpp` を G7 で新規作成: 例外なし・TEST_ASSERT/TEST_FAIL 系のみで
  TestCase / TestSuite / TestRegistry / TestRunner / RepeatedTest / TestDecorator /
  SimpleString / SimpleList / SimpleAutoPtr を網羅的に叩き、意図的失敗を含めない
  （**failures = 0 / errors = 0 / exit 0 を正**にできる）。
- 期待テスト件数 N は新ドライバのテスト登録数として G7 で固定し、
  **同じ all_test_tpp.cpp を MSVC でもビルド・実行して同一出力（N/0/0）を確認**する
  （tpp 固有の誤動作とドライバ自体のバグを切り分ける相互検証）。
- all_test.cpp 原本がスコープ外である理由（例外・stdio 間接 include・意図的失敗）を
  tpp仕様.md へ記録する（G7）。

## 8. 規模感（目安）

| ガイド | 規模 | 主な変更先 |
|---|---|---|
| G0 ベースライン | 小（実装なし） | `sample/cppunit` 原本コミット + インベントリ表 + 基準値記録 |
| G1 先頭 `::` | 小 | `tccgen.c` unary() / parse_btype() 入口（正式 lookup） |
| G2 friend | 極小 | `tcctok.h` + struct_decl()（`friend class` 限定） |
| G3 クラス内 typedef | **大**（5 コミット） | struct_decl() / struct_layout() / parse_btype() / parse_cpp_scope_qualifier / デフォルト引数保存構造 |
| G4 new/delete | 中 | `tcctok.h` + unary() + ctor/dtor 呼び出し機構 |
| G5 純粋仮想 | 中 | struct_decl() / cpp_assign_virtual_slots（pure flag）/ cpp_emit_vtable / 宣言検査 |
| G6 仮想 dtor | 中〜大 | struct_decl() dtor 経路 / cpp_assign_virtual_slots / delete 経路 / vtable offset-to-top |
| G7 ゲート | 小 | `sample/cppunit/build_cppunit.bat`（件数照合 + git diff ゲート）+ `run_all.bat` |
