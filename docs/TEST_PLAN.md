# Blockfall MVP Test Plan

この文書は AI 実装後の受け入れ条件を定義します。

## 1. Build

- [ ] `make` が成功する。
- [ ] `make OS32_SDK=/path/to/os32/build/sdk` が成功する。
- [ ] `build/blockfall.bin` が生成される。
- [ ] OS32 本体のソース変更を要求しない。
- [ ] 外部ライブラリ/外部アセットを要求しない。
- [ ] GCC バージョン固定パスに依存しない。

## 2. Launch / Exit

- [ ] OS32 上で起動できる。
- [ ] 起動直後は READY 状態。
- [ ] プレイ領域 240×320 px が左側に表示される。
- [ ] 右側に SCORE / LINES / LEVEL / NEXT が表示される。
- [ ] `Esc` で終了できる。
- [ ] 終了時に `libos32gfx_shutdown()` を通る。

## 3. Board / Geometry

- [ ] 盤面は 10 列 × 20 行。
- [ ] 1 cell は 16×16 px。
- [ ] 盤面描画実サイズは 160×320 px。
- [ ] プレイ領域内の左右余白は合計 80 px。
- [ ] 盤面外へ固定ブロックが描画されない。

## 4. Pieces

- [ ] I/O/T/S/Z/J/L の 7 種が存在する。
- [ ] 各 piece は 4 cell で構成される。
- [ ] NEXT に次の piece が表示される。
- [ ] 7-bag または仕様で許可された生成方法が機能する。
- [ ] spawn 位置が中央付近である。

## 5. Input

- [ ] Left/A で左へ 1 cell 動く。
- [ ] Right/D で右へ 1 cell 動く。
- [ ] Down/S で soft drop。
- [ ] Up/X で clockwise rotation。
- [ ] Z で counter-clockwise rotation。
- [ ] Space で READY 開始 / PLAYING 中 hard drop。
- [ ] GAME OVER 中 R で restart。
- [ ] 左右を押しっぱなしにするとゲーム側 repeat で移動する。
- [ ] 左右 repeat が OS の文字キーリピート速度に依存しない。
- [ ] 回転キー押しっぱなしで毎 tick 回転しない。
- [ ] Space 押しっぱなしで連続 hard drop しない。

## 6. Collision / Movement

- [ ] 左壁を越えない。
- [ ] 右壁を越えない。
- [ ] 床を越えない。
- [ ] settled block を貫通しない。
- [ ] invalid move は現在位置を変えない。
- [ ] 回転不能時は元の rotation/position を維持する。
- [ ] 壁際回転で仕様の簡易 wall kick が機能する。

## 7. Gravity

- [ ] 自然落下が CPU 実行速度に依存しない。
- [ ] LEVEL 1 の自然落下が概ね仕様値で動く。
- [ ] soft drop は通常 gravity より速い。
- [ ] level が上がると gravity が速くなる。
- [ ] 待機は busy loop ではなく `sys_halt()` を利用する。

## 8. Hard drop / Lock

- [ ] hard drop で衝突直前まで落ちる。
- [ ] hard drop した piece は同 tick で固定される。
- [ ] 下へ進めない piece が正しく固定される。
- [ ] 固定後に次の piece が spawn する。
- [ ] 固定済み盤面の色/IDが保持される。

## 9. Line clear

- [ ] 1 line を消せる。
- [ ] 2 lines を同時に消せる。
- [ ] 3 lines を同時に消せる。
- [ ] 4 lines を同時に消せる。
- [ ] 消した行より上の盤面が正しく下へ詰まる。
- [ ] 最上段側に空行が補充される。
- [ ] 行削除で盤面配列外アクセスが発生しない。

## 10. Score / Lines / Level

- [ ] 1 line = 100 × level。
- [ ] 2 lines = 300 × level。
- [ ] 3 lines = 500 × level。
- [ ] 4 lines = 800 × level。
- [ ] LINES が累積消去数を示す。
- [ ] `level = lines / 10 + 1`。
- [ ] 10 lines 到達で LEVEL 2 になる。
- [ ] restart で score/lines/level が初期化される。

## 11. GAME OVER

- [ ] 新規 piece が spawn できないと GAME OVER。
- [ ] GAME OVER 表示が出る。
- [ ] 最終 score が確認できる。
- [ ] R で READY に戻る。
- [ ] restart 後の board は空。
- [ ] restart 後に新しい game を正常に進行できる。

## 12. Stability

- [ ] 100 piece 以上連続して操作してもフリーズしない。
- [ ] restart を複数回行っても状態が壊れない。
- [ ] 左右同時押しで盤面外へ飛ばない。
- [ ] 回転と hard drop の同時入力で状態が破損しない。
- [ ] line clear 直後の spawn が正常。
- [ ] NEXT の更新で active piece と取り違えない。

## 13. Scope check

MVP に以下が紛れ込んでいないこと。

- [ ] HOLD なし
- [ ] ghost piece なし
- [ ] T-spin scoring なし
- [ ] combo/back-to-back なし
- [ ] BGM/SE なし
- [ ] save/high-score persistence なし
- [ ] network なし
- [ ] mouse なし
- [ ] external assets なし

## 14. AI self-review

完了報告時に以下を記載すること。

1. 変更ファイル
2. 実装機能
3. 実行した build/test
4. OS32 実機/エミュレータで未確認の項目
5. 既知の制約/残課題
