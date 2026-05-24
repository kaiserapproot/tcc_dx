# Windows ターミナル操作ガイドライン
- **実行ファイル**: カレントディレクトリのバイナリ実行には必ず `./` または `.\` を付けること。
- **バイナリ解析**: 自作スクリプトを避け `certutil -encodehex <file> out.hex` を使用すること。
- **クォート**: `powershell -Command` での複雑なエスケープを避けること。
- **コマンド**: `ls` → `dir`, `grep` → `findstr`, `rm -rf` → `Remove-Item -Recurse`, `head` → `gc -Head` を徹底すること。
- **Python を使うな。環境にインストールされていない**
