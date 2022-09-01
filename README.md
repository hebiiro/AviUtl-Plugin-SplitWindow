# AviUtl プラグイン - SplitWindow

AviUtl のウィンドウを再帰分割可能なペインに貼り付けられるようにします。
[最新バージョンをダウンロード](../../releases/latest/)

※ UniteWindow とは同時に使用できません。

## 導入方法

1. 以下のファイルを AviUtl の Plugins フォルダに配置します。
	* SplitWindow.aul
	* SplitWindow.xml
	* SplitWindow (フォルダ)

## 用語説明

* ハブ - Hub - 拠点。メインウィンドウ。旧名シングルウィンドウ。
* コロニー - Colony - 集落。サブウィンドウ。旧名なし。
* シャトル - Shuttle - 乗り物。ウィンドウを格納して浮いたり着陸したりする。旧名ウィンドウ。

## 使用方法

### ペインを分割する

1. ペインを右クリックしてメニューを表示します。
2. 「垂直線で分割」か「水平線で分割」を選択します。
3. (1)～(2) を繰り返します。

### ペインにウィンドウをドッキングする

1. ペイン (のタイトルまたはタブ) を右クリックしてメニューを表示します。
2. ドッキングしたいウィンドウを選択します。

* メニューの 2 列目は現在表示されているウィンドウ、3 列目は非表示になっているウィンドウです。

### ドッキングを解除する

1. ペインのタイトルまたはタブを右クリックしてメニューを表示します。
2. 「ドッキングを解除」を選択します。

### 分割を解除する

1. 分割を解除したいボーダーを右クリックしてメニューを表示します。
2. 「分割なし」を選択します。

### ドッキング状態でメニューを使用する

1. ペインの左上にある「M」を左クリックしてメニューを表示します。

### タブの並び順を替える

1. 移動したいタブを右クリックしてメニューを表示します。
2. 「左に移動する」または「右に移動する」を選択します。

## 設定方法

ハブのタイトルバーを右クリックしてシステムメニューを表示します。

### コロニーを新規作成

ハブとほぼ同じ機能のウィンドウを作成します。マルチモニタ用。

### レイアウトのインポート

レイアウトファイルを選択してインポートします。

### レイアウトのエクスポート

レイアウトファイルを選択してエクスポートします。

### SplitWindowの設定

SplitWindowの設定ダイアログを開きます。

* ```配色``` 背景色とボーダーの配色を指定します。
	* ```塗り潰し``` 背景色を指定します。
	* ```ボーダー``` ボーダーの色を指定します。
	* ```ホットボーダー``` ホットボーダーの色を指定します。
* ```アクティブキャプションの配色``` アクティブキャプションの配色を指定します。
	* ```背景``` アクティブキャプションの背景色を指定します。
	* ```テキスト``` アクティブキャプションのテキストの色を指定します。
* ```非アクティブキャプションの配色``` 非アクティブキャプションの配色を指定します。
	* ```背景``` 非アクティブキャプションの背景色を指定します。
	* ```テキスト``` 非アクティブキャプションのテキストの色を指定します。
* ```その他```
	* ```ボーダーの幅``` ペインを分割するボーダーの幅を指定します。
	* ```タイトルの高さ``` ペイン上部のタイトルの高さを指定します。
	* ```タブの高さ``` タブの高さを指定します。
	* ```メニューの折り返し``` メニューの最大行数を指定します。
	* ```タブモード``` タブモードを指定します。
* ```配色ではなくテーマを使用する``` チェックを入れると、描画を「黒窓」に任せます。
* ```マウスホイール時スクロールを優先する``` チェックを入れると、コンボボックスなどの上でマウスホイールをしてもスクロールが優先されます。

### 設定ファイル

SplitWindow.xml をテキストエディタで開いて編集します。

* ```<config>```
	* ```borderWidth``` ボーダーの幅を指定します。
	* ```captionHeight``` キャプションの高さを指定します。
	* ```tabHeight``` タブの高さを指定します。
	* ```tabMode```
		* ```title``` タイトルにタブを表示します。
		* ```top``` タイトルの下にタブを表示します。
		* ```bottom``` ペインの下側にタブを表示します。
	* ```menuBreak``` 指定された項目数でメニューを折り返します。0 を指定した場合は折り返されません。
	* ```fillColor``` 背景の塗りつぶし色を指定します。
	* ```borderColor``` ボーダーの色を指定します。
	* ```hotBorderColor``` ホット状態のボーダーの色を指定します。
	* ```activeCaptionColor``` アクティブキャプションの背景色を指定します。
	* ```activeCaptionTextColor``` アクティブキャプションのテキストの色を指定します。
	* ```inactiveCaptionColor``` 非アクティブキャプションの背景色を指定します。
	* ```inactiveCaptionTextColor``` 非アクティブキャプションのテキストの色を指定します。
	* ```useTheme``` ```YES``` を指定すると UI の描画にテーマを使用するようになります。
	* ```forceScroll``` ```YES``` を指定するとマウスホイール時スクロールを優先します。
	* ```<hub>```
		* ```<placement>``` ハブの位置を指定します。ウィンドウ位置がおかしくなった場合はこのタグを削除してください。
		* ```<pane>``` 入れ子にできます。
			* ```splitMode``` ```none```、```vert```、```horz``` のいずれかを指定します。
			* ```origin``` ```topLeft```、```bottomRight``` のいずれかを指定します。
			* ```isBorderLocked``` ```YES``` を指定するとボーダーをロックします。
			* ```border``` ```origin``` からの相対値でボーダーの座標を指定します。
			* ```<dockShuttle>``` このペインにドッキングさせるウィンドウを指定します。
	* ```<colony>``` ```<hub>``` とほぼ同じ設定ができます。
	* ```<floatShuttle>``` フローティングウィンドウの位置を指定します。ウィンドウ位置がおかしくなった場合はこのタグを削除してください。

## 更新履歴

* 3.2.0 - 2022/09/01 ボーダーをロックできるように変更
* 3.1.0 - 2022/09/01 タイトルの高さとタブの高さを個別に変更
* 3.0.5 - 2022/08/19 拡張編集のダミーウィンドウを作成するように変更 (「VoiceroidUtil」用)
* 3.0.4 - 2022/08/12 フォーカスが 0 のときの挙動を変更
* 3.0.3 - 2022/08/12 VST プラグインのダイアログの挙動を変更
* 3.0.2 - 2022/07/25 マウスホイール時スクロールを優先する機能を切り替えられるように変更
* 3.0.1 - 2022/07/15 「キーフレームプラグイン」と共存できるように修正
* 3.0.0 - 2022/07/05 サブウィンドウを作成できるように修正
* 2.2.0 - 2022/07/05 タブを上に配置できるように修正
* 2.1.0 - 2022/07/01 タブを下に配置できるように修正
* 2.0.0 - 2022/06/30 タブを追加
* 1.1.0 - 2022/06/27 ドッキング状態でもメニュー操作ができるように修正
* 1.0.9 - 2022/06/27 ウィンドウのフローティング化が不完全だった問題を修正
* 1.0.8 - 2022/06/26 「patch.aul」がコンソールウィンドウを制御できなくなる問題を修正
* 1.0.7 - 2022/06/26 「ショートカット再生」でショートカットキーが効かない問題を修正
* 1.0.6 - 2022/06/25 設定ダイアログのサイズが変わったときに描画が乱れる問題を修正
* 1.0.5 - 2022/06/25 コンボボックスとトラックバーのマウスホイール処理を無効化
* 1.0.4 - 2022/06/25 キャプションとボーダーの描画方式を選択できるように変更
* 1.0.3 - 2022/06/25 非表示のウィンドウも直接ドッキングできるように変更
* 1.0.2 - 2022/06/25 シングルウィンドウリサイズ時にボーダーの位置がおかしくなる問題を修正
* 1.0.1 - 2022/06/24 「拡張ツールバー」と共存できるように修正
* 1.0.0 - 2022/06/24 初版

## 動作確認

* (必須) AviUtl 1.10 & 拡張編集 0.92 http://spring-fragrance.mints.ne.jp/aviutl/
* (共存確認) patch.aul r42 https://scrapbox.io/ePi5131/patch.aul

## クレジット

* Microsoft Research Detours Package https://github.com/microsoft/Detours
* aviutl_exedit_sdk https://github.com/ePi5131/aviutl_exedit_sdk
* Common Library https://github.com/hebiiro/Common-Library

## 作成者情報
 
* 作成者 - 蛇色 (へびいろ)
* GitHub - https://github.com/hebiiro
* Twitter - https://twitter.com/io_hebiiro

## 免責事項

この作成物および同梱物を使用したことによって生じたすべての障害・損害・不具合等に関しては、私と私の関係者および私の所属するいかなる団体・組織とも、一切の責任を負いません。各自の責任においてご使用ください。
