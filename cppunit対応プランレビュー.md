rev.5 を再度、敵対的に確認しました。

前回の GO 時に残した3点、**P5 の value-name lookup、P2 の規則番号、baseline commit hash 記録は正しく修正されています**。 特に P5 は `npos` が値名であることを明示し、型名経路と値名/関数名経路を分離したので、前回の指摘は解消です。

ただし、G0 実測によってプラン自体が拡張されたため、**rev.5 全体としては再び REJECT** に戻します。既存の G1～G6 設計が悪化したわけではありません。**新しく追加された部分に Blocker 3件**あります。

1. **Blocker — G7 の原本無改変ゲートが** **`all_test_tpp.cpp`** **自身を検出します。** 現在のゲートは `git diff --exit-code cppunit-original-base -- sample/cppunit/*.cpp sample/cppunit/*.h` です。一方、rev.5 では baseline 後に `sample/cppunit/all_test_tpp.cpp` を新規作成します。 「baseline 後の新規ファイルなので pathspec に含まれない」という説明は誤りです。`*.cpp` はファイル名でマッチするので、`all_test_tpp.cpp` を Git に追加・コミットした時点で baseline との差分として **added file** になります。つまり最終 G7 自身が赤になります。さらに `all_test.cpp` も `*.cpp` に入るため、「all\_test.cpp は無改変ゲート対象外」という新しい方針とも一致していません。

   ここは **31原本ファイル（12 TU + 19 h）だけの明示的 manifest** を作るのが一番安全です。あるいは Git pathspec の exclude で `all_test.cpp` と `all_test_tpp.cpp` を明示除外します。私は manifest 方式を推奨します。baseline hash `ad882a3c...` まで固定できているので、31ファイルだけをその hash と比較すれば完全です。
2. **Blocker — G-OP / G-CAST は「必要機能と判明した」のに、まだ実装プランになっていません。** G0 実測によって単項 `operator*()`、`operator->()`、`T(expr)` が実際の停止点だと判明したのは非常に良いです。 しかし現在の記述は「既存機構に乗る見込み」「詳細設計は着手時に追記」となっています。 これは、これまで G1～G6 に要求してきた設計密度と明らかに違います。

   G-OP なら少なくとも、built-in `*` と class `operator*` の判別、戻り値の lvalue/reference 性維持、`operator->` の宣言パース、呼び出し変換、戻り値がポインタでない場合の扱い、負例、Cモード非回帰が必要です。G-CAST なら、`T(expr)` と通常の関数呼び出しの判別、typedef/basic type のみ許可、既存 `(T)expr` の cast machinery への接続、class functional cast の明示拒否、複数引数/空引数の扱い、負例が必要です。特に `delete (Entry*)*p` が実コードに存在するので、G-OP の戻り値カテゴリを雑に実装すると G4 に波及します。

   **G1/G2 の実装開始までは GO** でも構いません。しかし G-OP に入る前に詳細設計を追記する、という条件付きです。「rev.5 全体が最終実装プラン」という意味ではまだ GO にできません。
3. **Blocker —** **`all_test_tpp.cpp`** **の N ゲートが循環しています。** 新ドライバを作り、そのドライバ自身の登録数を N として固定し、MSVC と tpp が同じ N/0/0 なら成功、としています。 これは**選択したテストの同等動作**は証明できますが、**「ライブラリ全機能を叩く」ことは証明しません**。極端には `all_test_tpp.cpp` に正常終了するテストを1個だけ書けば、MSVC=1/0/0、tpp=1/0/0 で完全に緑です。

   案Aそのものは妥当です。元の `all_test.cpp` が生の例外へ依存し、ライブラリ12 TU自体は MINIMUM\_SET で MSVC コンパイル成功しているため、ドライバだけ差し替える判断には合理性があります。 ただし G7 に **テストカバレッジ契約**を追加してください。例えば TestCase / TestSuite / TestRegistry / TestResult / TestFailure / TestRunner / RepeatedTest / TestDecorator / SimpleString / SimpleList / SimpleAutoPtr について「最低1つ以上」ではなく、正常経路・failure記録・登録/検索・所有権/delete・iterator・文字列操作など、必要な振る舞いを表にして、それぞれ対応テスト名を固定します。その表のテスト数から N を導出する形なら、N が意味を持ちます。

そのほかは Blocker ではありませんが、rev.5 では文書の古い情報を掃除した方がいいです。§7 では pure virtual は **9件・pure dtor 0** と確定したのに、§1 はまだ27件、リスク表も「27件に dtor が含まれるか未確認」のままです。 同様に new/delete も G0 実測では scalar new=11、scalar delete=14 と確定していますが、上部の旧カウントが残っています。さらに §8 の規模表には新設した **G-OP / G-CAST が載っていません**。 これは実装担当LLMに渡すならかなり邪魔になるので、rev.6 で一本化した方がよいです。

### 最終判定

**既存 G1～G6 の設計: GO 維持。**\
**rev.5 全体: REJECT。**

修正必須は実質この3点です。

* 原本無改変ゲートを **31原本ファイル限定**にする
* G-OP / G-CAST を G1～G6 と同等の詳細設計＋受け入れテストまで書く
* `all_test_tpp.cpp` に **機能→テストのカバレッジ表**を設け、そこから N を固定する

この3つを直せば、今度は追加実測で発見された問題まで含めた意味での **本当の最終 GO** にできます。
