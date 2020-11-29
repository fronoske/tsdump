## tsdump for BonDriver_Linux

本forkは、[tsdump](https://github.com/hayamdk/tsdump)をLinux版BonDriverで動作させるようにしたものです。
使用方法などはreadme.txt（オリジナル版のreadme）を参照してください。

## 背景
オリジナル版のtsdumpは、Windows環境だとBonDriver、Linux環境だとDVBドライバに対応しています。
そのためLinux + PX-Q3U4 という環境では動作させることができませんでした。
しかし[Linux + chardev版ドライバで動作するBonDriver](https://github.com/u-n-k-n-o-w-n/BonDriverProxy_Linux)があることを知り、
tsdumpでもWindowsでビルドすればBonDriver対応になるのだから、なんとかtsdumpをLinux環境でBonDriver対応できるのではないかと考えて
試してみたところ、なんとか動作してそうなものができました。

## 注意事項
本forkは、BonDriver用モジュールのコードのうちWindows独自の箇所をLinuxでビルドが通るように無理やり変更しています。
とりあえずビルドは通っていますが、よく理解せず似たような関数に置き換えたりキャストさせまくったりしているだけので動作はまったく保証できません。
メモリリークもしていないとも限りません。（Makfileの書き方も怪しいです。）
利用する際はその点を留意してください。Pull Request歓迎です。

## 動作環境

以下の環境で動作確認しています。

- Raspberry Pi4 Model B (8GB)
- Raspberry Pi OS
- PX-Q3U4
- px4_drv
- BonDriver_LinuxPT.so

## ライセンス

オリジナル版と同じくGPLです。

## 謝辞
hayamdkさんのオリジナル版がチューナーAPIにアクセスする部分をモジュール化するという素晴らしい設計で最初から実装されていたおかげで、
何の専門知識がなくてもほぼ単純なコード置換でLinux版BonDriverに対応させることができました。感謝いたします。
