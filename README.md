![m5stickc](https://user-images.githubusercontent.com/1772636/113172205-2be91a00-9283-11eb-8870-6bb9cd06eae0.gif)

# Ryap
M5StickC のセンサー情報（精度、ジャイロ、姿勢（回転））を Wifi 経由で送信します。

[AxisOrange](https://github.com/naninunenoy/AxisOrange) に多大な影響を受けています。
Bluetooth 経由での接続であればこちらを利用されるのがよいかと思います。

## 利用方法
1. git clone
2. https://github.com/machidyo/Ryap/blob/master/src/main.cpp#L13-L15 を設定
3. M5StickC にインストール
4. Wifi に接続できることの確認
  *  `--ryap--192.198.137.143` と表示されることを確認します
  *  サンプルであり、取得したIPは環境によって異なります。
  *  Wifi に接続できていないときは `--ryap--` だけの表示になります。SSID、パスワード等を見直してください。 

## 動作確認方法
1. [RypaUnity](https://github.com/machidyo/RyapUnity) を git clone
2. Unity で開いて実行します。
3. M5StickC の動きに合わせて、Unity 画面上の M5StickC も動くことを確認します。
4. 動きはあっているのだけどそもそもの向きが異なる場合、一度 M5StickC 水平にし、Aボタンを押して初期姿勢を合わせてから再度確認してみてください。

