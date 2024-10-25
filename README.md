
# HelloClapHost

HelloClapHost is a simplest CLAP host samples for beginners like a "Hello world". It can only play the notes Do Re Mi using the CLAP plug-in sound source. Now supports Windows only.

Because it omits all error handling, detailed settings, and extended functions, so you will need to refer to the follow official samples to put it into practical use.

CLAPホスト開発入門のためのHello worldで、CLAPプラグイン音源で「ドレミ……」と音を鳴らす最小限のサンプルです。エラー処理や詳細設定、拡張機能は一切省いており、実用化には以下の公式サンプルを参考にする必要があります。

「自分のPC環境で動いて音が鳴ったコード」であるのは間違いないですが、他の人の環境で動くかは分かりません。自分自身が現状ホスト開発入門者なので間違いがあると思います。またもっと簡単にできるかもしれません。

* clap https://github.com/free-audio/clap
* clap-info https://github.com/free-audio/clap-info
* clap-host https://github.com/free-audio/clap-host



## サンプルの実行

"HelloClapHost.exe" を実行するだけです。（カレントフォルダにclap-saw-demo-imgui.clapを置いている状態で）

## コンパイル

### 必要なもの
* Visual Studio 2022
* CMake

### 作業
zipをダウンロードして展開し、コマンドプロンプトを開いて以下を実行します。

```bash
cd HelloClapHost
build.bat
```

HelloClapHost\build\Debugに実行ファイルが生成されます。


## その他

* other_examples/simple_miniaudio_example ... miniaudioによるオーディオ初期化部分の最小サンプル
* other_examples/miniaudio_example ... 実用的なminiaudioによるオーディオ初期化のサンプル


## License
使用しているライブラリ等についてはそれぞれに定められたライセンスになります。（licenses/を参照のこと）
* clap
* clap-info
* jsoncpp
* miniaudio
* clap-saw-demo-imgui
* RtMidi