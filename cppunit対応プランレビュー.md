前回の **3 Blocker は、設計内容としてはすべて解消されています**。G-OP/G-CAST は実装経路・スコープ外・負例・Cモード回帰まで具体化され、G7 も17項目のカバレッジ契約まで入ったので、rev.5 よりかなり強くなっています。

ただし、**新規 Blocker 1件 / High 3件**を見つけました。最終判定は **まだ REJECT** です。

## Blocker — manifest 自体が信頼できない + 完了条件が旧 glob のまま

G7 は現在、

> `original_manifest.txt` に31原本ファイルを書き、その manifest を読んで baseline と diff

という方式です。

これは前回の glob 問題を解決しています。

しかし、**その manifest 自体はゲート対象外**です。

例えば途中で誤って、

```
```

```
SimpleString.cpp
SimpleList.cpp
...
```

のうち `SimpleString.cpp` を manifest から1行削除した状態で、その `SimpleString.cpp` を変更しても、

```
```

```
manifest にない
↓
git diff されない
↓
緑
```

になります。

つまり、

> 31原本ファイルが無改変

を証明する信頼根が、自由に変更可能な `original_manifest.txt` になっています。

これは前回の「globでは新規ファイルを拾う」の逆で、今回は **manifest omission による false green** です。

### 修正推奨

一番強いのは manifest を手書きの source-of-truth にしないことです。

baseline commit 自体から機械生成してください。

概念的には、

```
```

```
baseline commit
  ↓ git ls-tree
sample/cppunit/*.cpp + *.h
  ↓
all_test.cpp を除外
  ↓
期待 31 ファイル
```

です。

そして G7 で、

```
```

```
baseline から導出した集合 == manifest の集合
manifest 行数 == 31
重複なし
```

を先に検証してから各ファイルを diff する。

または、より単純に **manifest を使わず baseline tree から直接31ファイルを列挙して diff** してしまえばいいです。

さらにもう1つ、文書内で明確な矛盾があります。

G7 は glob を禁止して manifest 方式へ修正したのに、**全体完了条件はまだ**

```
```

```
git diff --exit-code cppunit-original-base -- sample/cppunit/*.cpp sample/cppunit/*.h
```

のままです。

これは `all_test.cpp` と `all_test_tpp.cpp` を拾うので、G7 の新設計と矛盾します。

したがって Blocker 1 は、

**「manifest の完全性を機械保証する」+「§4完了条件もmanifest/baseline-tree方式へ変更する」**

の2点セットで修正してください。

***

## High 1 — カバレッジ契約は17件だが、ゲートは「17という数」しか見ていない

今回の17項目表はかなり良いです。

TestCase / TestResult / TestFailure / TestSuite / Registry / Runner / Repeated / Decorator / Setup / Listener / SimpleString / SimpleList / SimpleAutoPtr まで具体的なテスト名に固定されています。

ただし機械ゲートが見るのは、

```
```

```
runCount == 17
failures == 0
errors == 0
```

だけです。

例えば実装ミスで、

```
```

```
test_case_lifecycle
test_case_lifecycle   ← 二重登録
...
test_listener         ← 登録漏れ
```

でも17件なら通ります。

MSVCとの相互比較も両方同じ誤ったドライバを使うため、これを検出できません。

### 推奨

17個の**テスト名集合まで固定**してください。

例えば実行時に各テストが、

```
```

```
PASS:test_case_lifecycle
PASS:test_case_failure_record
...
PASS:test_auto_ptr
```

のような一意 marker を出し、G7 が17個すべてを完全一致で確認する。

あるいはドライバの登録部分を固定テーブル化して、

```
```

```
register_test("test_case_lifecycle", ...);
...
```

その17名を機械チェックする方式でもよいです。

Blockerにはしません。契約表そのものがあるので、人間レビューでは漏れを発見できます。ただし「機械ゲート」としてはあと一段あります。

***

## High 2 — G-OP の `operator->` に「戻り値が NULL」の実行テストが欲しい

G-OP はかなりよくなりました。

*  pointerならbuilt-in経路を維持 
*  classなら `operator->()` 
*  返却型がpointerでなければerror 
* `operator*()` の `T&` lvalue維持 
*  Cモード回帰 

まであります。

ただし、

```
```

```
P *operator->() { return 0; }
```

に対して、

```
```

```
it->v
```

を行った場合は普通にNULL dereferenceになります。

これはコンパイラがNULLチェックすべきという意味ではありません。

重要なのは **`operator->()`** **の返却値をそのまま built-in** **`->`** **へ渡す**ことを固定するテストです。

例えば、

```
```

```
P *operator->() { return &p; }
```

だけだと、実装が内部 `this` や元オブジェクトのアドレスを誤って使っていても偶然通る可能性があります。

戻り先を別オブジェクトにしてください。

```
```

```
struct Proxy {
    P *target;
    P *operator->() { return target; }
};

P a;
P b;
Proxy x;
x.target = &b;

return x->v == b.v;
```

これなら**返却pointerそのものが使われた**ことを実行値で確認できます。

***

## High 3 — G0完了済みなのに、G0手順内には古い 31/19・27件が残っている

§1 と §5 はきれいになっています。

現在の確定値は、

*  scalar new 11 
*  scalar delete 14 
*  pure virtual 9 
*  pure virtual dtor 0 

です。

しかし G0 の手順本文にはまだ、

> scalar new 31 / delete 19

> 純粋仮想27件

が残っています。

G0 は完了済みなので動作上は問題ありませんが、このプランを別LLMに渡すなら、

```
```

```
§1: 11 / 14 / 9
G0: 31 / 19 / 27
```

という二重情報になります。

「G0実施前の計画値」なら、

> **旧計画値（G0実測前。結果は§7参照）**

と明示するか、実測後の確定値へ書き換えた方が安全です。

***

## G-CAST は今回 GO

G-CASTについては新しい Blockerは見つかりませんでした。

特に、

```
```

```
型名と判定できたときだけ T(expr)
→ 既存 (T)expr のcast machineryへ
→ class functional castは明示error
→ 0引数/複数引数もerror
```

という設計は、今回必要な `size_type(-1)` だけを安全に増やす方針になっています。

通常関数呼び出しとの混在テストとCモード非回帰もあるので、この範囲なら十分です。

## 判定

**G1〜G6: GO 維持**\
 **G-OP: GO（High追加テスト推奨）**\
 **G-CAST: GO**\
 **G7: Blockerあり**\
 **rev.6全体: REJECT**

今回、本当に必須なのは1点だけです。

> **原本31ファイルの集合そのものを baseline commit から機械的に固定し、§4の古いglob完了条件も削除する。**

これを直せば、これまでの rev.1〜rev.6 を通じて残っていた「緑なのに実は証明できていない」タイプの穴は、かなり潰し切った状態になります。次版では、私は **最終GO判定を出せる可能性が高い**です。
