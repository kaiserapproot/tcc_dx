rev.4 を再度、敵対的に確認しました。

## 結論

**今回は Blocker は見つかりません。実装開始 GO でよいです。**

前回までの rev.1～rev.3 では「緑なのに誤コンパイル」「多重継承で heap corruption」につながる設計穴がありましたが、rev.4 ではそのレベルの問題は潰れています。特に前回の2 Blockerだった G3 の lookup-set と qualified lookup は、かなり明確になりました。

C++ の member lookup は実際に「declaration set + subobject set」で扱い、型宣言はそれが表す型へ置き換えてから merge します。また base ごとの lookup set を再帰的に merge し、支配関係→宣言集合比較→union の順で処理します。rev.4 の「型へ正規化」「先勝ち禁止」「判定不能なら `tcc_error`」という縮小実装方針は、このプロジェクトの目的に対して安全側です。

### 残る指摘は High 1件だけ

G3-P5 の記述だけ、実装前に文言を直すことを勧めます。

現在は、

> `cpp_cur_func_class` 相当を定義クラスへ差し替え、lookup 規則 2〜4 が定義クラス基準で働く状態にしてトークン再生

となっています。

ところが G3 共通で定義した `lookup_unqualified_type()` は名前どおり**型名 lookup**です。一方 P5 の実対象、

```
```

```
static const int npos;
void f(int n = npos);
```

の `npos` は**型ではなく値メンバ**です。実際、プラン自身も P3 では `Class::npos` を式側へ戻すものとして扱っています。

したがって P5 は、

```
```

```
再生時に owning class / declaration scope を復元する。

型名が現れた場合:
    lookup_unqualified_type() が復元した class scope を使用

値名・関数名が現れた場合:
    既存の式 identifier/member lookup が復元した
    cpp_cur_func_class / declaration scope を使用
```

という書き方にした方がいいです。

そして着手前確認に、

```
```

```
デフォルト引数トークン再生時の
unqualified value-name 解決が
cpp_cur_func_class を参照しているか確認する
```

を1項目追加する。

**これは Blocker にはしません。** `npos` の実値まで確認する受け入れテストが既にあるので、間違って実装しても silent pass はしません。

### Low — 文書上の古い番号が1か所残っています

P2 に、

> 上記 **lookup 規則 1〜5 を実装**

とあります。

現在は、

1.  local 
2.  current class 
3.  enclosing class 
4.  global 

＋別関数 `lookup_qualified_class_type()`

なので、単純に

> `lookup_unqualified_type()` の規則 1〜4 を実装

へ直した方が、実装担当 LLM が「5番目はどこだ？」と迷いません。

### G6 は GO

ここはかなり強くなっています。

*  secondary-base からの `delete` 
*  complete-object address の復元 
* `vptr[-1]` 
*  pointer-width signed offset 
*  従来の `vptr[0...]` 不変 
*  NULL check が vptr read より前 
*  virtual dtor のときだけ G6 経路 
*  non-virtual delete の回帰 
*  destructor 順序の実値検証 

まで揃っています。特に non-virtual 経路を G4 のまま維持すると明示したことで、「G6 を入れたら普通の `delete` が壊れた」という事故も検出できます。 

ここについて新しい Blocker はありません。

### G7 も十分強い

現在は単なる exit 0 ではなく、

**期待件数 N + failures=0 + errors=0 + 完全一致/数値比較**

になっています。さらに CPPUnit 原本も baseline commit と比較します。

これはかなり良いです。「0件実行で成功」「1と10の部分一致」「途中でCPPUnit原本を直してテストを通す」の3種類をそれぞれ潰せています。

一つだけさらに堅くするなら、`cppunit-original-base` タグを動かしてしまえば baseline 自体が変わるので、G0 で取得した**commit hash も記録**しておくと完璧です。ただし通常運用では Low です。タグを勝手に force-update しない限り問題にはなりません。

## 最終判定

**rev.1: REJECT**\
 **rev.2: REJECT**\
 **rev.3: REJECT**\
 **rev.4: ✅ GO**

実装前に直すなら実質この2点だけです。

* &#x20;P5 を「type lookup」だけでなく**value-name lookup の scope 復元**まで明記 
* &#x20;P2 の「規則1〜5」を「`lookup_unqualified_type()` 規則1〜4」に修正 

これは設計変更ではなく**実装指示の明確化**です。

それ以外は、G0で実測して新しい未対応構文が発見されたらプランを止めて改訂する、という安全弁も既に明記されています。

したがって、**この rev.4 は実装フェーズへ進めて問題ない**と判定します。
