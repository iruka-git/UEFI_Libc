# UEFI Libc

これは、UEFI BOOTする、64bit整数版 TinyBASICインタープリターに付随する形で作成中のLibcです

# 64bit整数版 TinyBASICインタープリター とは？

・Tiny BASIC for the Motorola MC68000 をUEFIブートするように改造したものです。

・予約語を普通の文字列型に変更

・整数のビット数を16bitから64bitに変更

・多少の Refactoring を実施

・追記： オリジナルのGitHubは2024年時点で消滅しておりました。

https：//github.com/LHerrmeyer/UEFI_Basic


## 環境構築メモ

(1)・Lubuntu24.04 を用意（普通のUbuntu24.04でも良い） QEMUを動かすので、GUIが必要。

(2)・gnu-efiを入れる。

 ```
 # apt install gnu-efi
 ```

(3)・QEMUを入れる。・・・UEFIブートを試すエミュレータ。

 ```
 # apt install qemu-system-x86
 ```

(4)・OVMFを入れる。・・・UEFIのBIOS（変な言い方だが、そう）というかファームウェアね。 Open Virtual Machine Firmware とかそんな感じの。

 ```
 # apt install ovmf
 ```

## QEMU上で動かす

 $ make run

・エラー発生時の行番号は、BASICの行ではなく、 basic.c のエラー発生時の行番号であることに注意

・変数は A-Zしか使用できない。

・文字列変数はない。

・整数は64bit

・配列は実装されない。

・Arduino等にあるような　SD Card File I/O や Port Pin Control など、は実装されない。

## QEMUの終了のさせ方

・[CTRL] ＋ [ALT] + [２] を押してQEMUコンソールに戻す
・>quit で終了


## 注意事項

・Libc (NewLib-4.4.0) 移植途上です。

・とりあえず、リンクエラーする関数をダミー関数として undef.c に置いています。

・以前はLinuxコマンドライン上でもbasicが実行可能なMakefileを用意していましたが、
現在はUEFI環境へのLibc移植に主眼を置いているため、サポートしていません。

## 注意事項２（ malloc() 、 sbrk() ）

・BASICの USER文を実行すると、ub_test.c 内の malloc() / free() テストが走行します。

・こういったストレステストを実施しない場合は、Makefile 内にある qemu の実行パラメータ
　オプションの -m 8G や -m 2G 等のメモリー指定は不要です。（外してから実行します）

・-m 8G を指定する場合は、qemuを動かすホストLinux (VMでももちろん可) の実装メモリー
　は12GB 以上用意してください。





## 実機で動くのか？

ちなみに、てきとーなUSBメモリーをFAT32フォーマットして、

USBドライブ名: /EFI/BOOT/BOOTX64.EFI

のようにさっきのファイルを配置して、

C:ドライブだったSSDを抜いておいて起動すると、ちゃんとBASICが起動する。


だけど、メーカーロゴが残ったままだし、全画面じゃないし（中央部分にしかコンソールがprintしない）不満が・・・


