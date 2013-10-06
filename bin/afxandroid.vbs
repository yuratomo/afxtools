'[Android target ids]
' android-3                                            (Android 1.5)
' Google Inc.:Google APIs:3
' android-4                                            (Android 1.6)
' Google Inc.:Google APIs:4
' android-7                                            (Android 2.1)
' Google Inc.:Google APIs:7
' android-8
' Google Inc.:Google APIs:8                            (Android 2.2)
' KYOCERA Corporation:DTS Add-On:8
' LGE:Real3D Add-On:8
' Samsung Electronics Co., Ltd.:GALAXY Tab Addon:8
' Samsung Electronics Co., Ltd.:GALAXY Tab Addon:8
' android-9                                            (Android 2.3.1)
' Google Inc.:Google APIs:9
' Sony Ericsson Mobile Communications AB:EDK:9
' android-10                                           (Android 2.3.3)
' Google Inc.:Google APIs:10
' android-11                                           (Android 3.0)
' Google Inc.:Google APIs:11
' android-12                                           (Android 3.1)
' Google Inc.:Google APIs:12

Dim ao
Set ao = Createobject("afxw.obj")
ao.Exec "&SET 0 android-8"
ao.Exec "&SET 1 HelloProject"
ao.Exec "&SET 2 HelloWorld"
ao.Exec "&SET 3 jp.co.seesaa.yuratomo"
ao.Exec "&SET 4 test_vm"
call ao.MesPrint("ANDROID用 あふw内部変数を設定しました。")
call ao.MesPrint("  ターゲットID:"   + ao.Extract("$0"))  ' プロジェクト作成時、仮想マシン作成時に使用
call ao.MesPrint("  プロジェクト名:" + ao.Extract("$1"))  ' プロジェクト作成時に使用
call ao.MesPrint("  クラス名:"       + ao.Extract("$2"))  ' プロジェクト作成時に使用
call ao.MesPrint("  パッケージ名:"   + ao.Extract("$3"))  ' プロジェクト作成時に使用
call ao.MesPrint("  仮想マシン名:"   + ao.Extract("$4"))  ' 仮想マシン作成時、仮想マシン実行時に使用
Set ao = Nothing
