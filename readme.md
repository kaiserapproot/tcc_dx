tiny-c

# tcc_dx プロジェクト概要

この tcc は、独自に DirectX や OpenGL を使えるようにビルドした Tiny C Compiler プロジェクトです。

## 特徴
- DirectX（特に DirectX9）や OpenGL を Windows環境で簡単に利用できるように構成
- 標準の tcc では難しいグラフィックスAPIの利用をサポート
- サンプルコードを通じて、GUIやグラフィックスの実装例を学べます

## サンプル内容（sampleフォルダ）
- **DirectX**: DirectX9 を使った描画サンプル
- **microui**: 軽量UIライブラリ microui のサンプル
- **Nuklear**: シンプルなGUIライブラリ Nuklear のサンプル
- **DirectXのドッキングビュー**: DirectX9 を使ったドッキングUIのサンプル
- **OpenGL**: OpenGL を使った描画サンプル

各サンプルフォルダには、ビルド用バッチファイル（.bat）が用意されており、簡単に実行ファイルを生成できます。

## macOS（Intel Mac）でのビルド

Xcode（またはコマンドラインツール）が入っていれば、次のコマンドでビルドからテストまで実行します。

```sh
./build_mac.sh            # ビルド + テスト
./build_mac.sh --no-test  # ビルドのみ
build/macos/tcc hello.cpp -o hello
build/macos/tcc -run hello.c
```

`dev\tcc_set.bat` と同じように tcc を PATH に追加するには、`dev/tcc_set.sh` を `source` します（未ビルドなら先にビルドします。`--rebuild` を付けるとビルドし直します）。

```sh
source dev/tcc_set.sh
tcc hello.cpp -o hello && ./hello
```

### OpenGL（macOS）

macOS では SDK の `OpenGL.framework` と `GLUT.framework` を使います。`sample/opengl/main_gl.c`（Win32 + WGL）を GLUT に移植した `main_glut.c` があります。

```sh
sh sample/opengl/build_opengl_mac.sh run   # ビルドして起動（ESC で終了）
```

自分のプログラムでは、tcc が clang の `-framework` に対応していないので、ヘッダは `-I build/macos/frameworks`（`<OpenGL/gl.h>`・`<GLUT/glut.h>` が見つかる）で指定し、リンクはフレームワークの `.tbd` をそのまま渡します。

```sh
FW=$(xcrun --show-sdk-path)/System/Library/Frameworks
tcc -w -I build/macos/frameworks app.c $FW/OpenGL.framework/OpenGL.tbd $FW/GLUT.framework/GLUT.tbd -o app
```

DirectX / Win32 API を使うサンプルは Windows 専用です。macOS 版の違いは [tpp仕様.md](tpp仕様.md) の §5A を参照してください。

---



