# Ryap
M5StickC のセンサー情報（精度、ジャイロ、姿勢（回転））を Wifi 経由で送信します。  

<img src="https://user-images.githubusercontent.com/1772636/113172205-2be91a00-9283-11eb-8870-6bb9cd06eae0.gif" width=240 />

[AxisOrange](https://github.com/naninunenoy/AxisOrange) のコードを一部流用させてもらっています。
Bluetooth 経由での接続やコードの参考はこちらを利用されるのがよいかと思います。

## 利用方法
1. git clone
2. wifi, 送信先の IP を設定（https://github.com/machidyo/Ryap/blob/master/src/main.cpp#L13-L15 を設定）
3. M5StickC にインストール
4. Wifi に接続できることの確認（※1）

※1 Wifi の接続確認の補足
*  `--ryap--192.198.137.143` と IP が表示されることを確認します
*  Wifi に接続できていないときは `--ryap--` だけの表示になります。SSID、パスワード等を見直してください。 
*  IP はサンプルのため環境によって異なります。

## 動作確認方法
1. [RypaUnity](https://github.com/machidyo/RyapUnity) を git clone
2. Unity で開いて実行します。
3. M5StickC の動きに合わせて、Unity 画面上の M5StickC も動くことを確認します。
4. 動きはあっているのだけどそもそもの向きが異なる場合、一度 M5StickC 水平にし、Aボタンを押して初期姿勢を合わせてから再度確認してみてください。動画の途中で実演しています。
