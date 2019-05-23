# About this project

YaneuraOu is the most strong Shogi engine(AI player) in the world , WCSC29 1st winner , educational and USI compliant engine.

やねうら王は、将棋の思考エンジンとして世界最強で、WCSC29(世界コンピュータ将棋選手権/2019年)において優勝しました。教育的でUSIプロトコル準拠の思考エンジンです。


# お知らせ

2019/05/05 WCSC29(世界コンピュータ将棋選手権/2019年)に『やねうら王 with お多福ラボ2019』として出場し、見事優勝を果たしました。(WCSC初参加、優勝)　このあと一年間は世界最強を名乗ります。(｀･ω･´)ｂ


# やねうら王エンジンの大会での戦績

- 2017年 世界コンピュータ将棋選手権(WCSC27) 『elmo』優勝
- 2017年 第5回将棋電王トーナメント(SDT5) 『平成将棋合戦ぽんぽこ』優勝
- 2018年 世界コンピュータ将棋選手権(WCSC28) 『Hefeweizen』優勝
- 2019年 世界コンピュータ将棋選手権(WCSC29) 『やねうら王 with お多福ラボ2019』優勝。
  - 決勝の上位8チームすべてがやねうら王の思考エンジンを採用。


# 現在進行中のサブプロジェクト

## やねうら王2018 OTAFUKU (やねうら王2018 with お多福ラボ)

2019年も引き続き、このエンジン部を改良していきます。

## やねうら王詰め将棋solver

《tanuki-さんが開発中》

長手数の詰将棋が解けるsolverです。

# 過去のサブプロジェクト

過去のサブプロジェクトである、やねうら王nano , mini , classic、王手将棋、取る一手将棋、協力詰めsolver、連続自己対戦フレームワークなどはこちらからどうぞ。

- [過去のサブプロジェクト](/docs/README2017.md)

## やねうら王評価関数ファイル

- やねうら王2017Early用 - Apery(WCSC26)、Apery(SDT4)＝「浮かむ瀬」の評価関数バイナリがそのまま使えます。
- やねうら王2017 KPP_KKPT型評価関数 - 以下のKPP_KKPT型ビルド用評価関数のところにあるものが使えます。
- やねうら王2018 Otafuku用 KPPT型　→ やねうら王2017Early(KPPT)と同様
- やねうら王2018 Otafuku用 KPP_KKPT型　→ やねうら王2017Early(KPP_KKPT)と同様

### 「Re : ゼロから始める評価関数生活」プロジェクト(略して「リゼロ」)

ゼロベクトルの評価関数(≒駒得のみの評価関数)から、「elmo絞り」(elmo(WCSC27)の手法)を用いて強化学習しました。従来のソフトにはない、不思議な囲いと終盤力が特徴です。
やねうら王2017Earlyの評価関数ファイルと差し替えて使うことが出来ます。フォルダ名に書いてあるepochの数字が大きいものほど新しい世代(強い)です。

- [リゼロ評価関数 epoch 0](https://drive.google.com/open?id=0Bzbi5rbfN85Nb3o1Zkd6cjVNYkE) : 全パラメーターがゼロの初期状態の評価関数です。
- [リゼロ評価関数 epoch 0.1](https://drive.google.com/open?id=0Bzbi5rbfN85NNTBERmhiMGZlSWs) : [解説記事](http://yaneuraou.yaneu.com/2017/06/20/%E5%BE%93%E6%9D%A5%E6%89%8B%E6%B3%95%E3%81%AB%E5%9F%BA%E3%81%A5%E3%81%8F%E3%83%97%E3%83%AD%E3%81%AE%E6%A3%8B%E8%AD%9C%E3%82%92%E7%94%A8%E3%81%84%E3%81%AA%E3%81%84%E8%A9%95%E4%BE%A1%E9%96%A2%E6%95%B0/)
- [リゼロ評価関数 epoch 1から4まで](https://drive.google.com/open?id=0Bzbi5rbfN85NNWY0RTJlc2x5czg) : [解説記事](http://yaneuraou.yaneu.com/2017/06/12/%E4%BA%BA%E9%96%93%E3%81%AE%E6%A3%8B%E8%AD%9C%E3%82%92%E7%94%A8%E3%81%84%E3%81%9A%E3%81%AB%E8%A9%95%E4%BE%A1%E9%96%A2%E6%95%B0%E3%81%AE%E5%AD%A6%E7%BF%92%E3%81%AB%E6%88%90%E5%8A%9F/)
- [リゼロ評価関数 epoch 5から6まで](https://drive.google.com/open?id=0Bzbi5rbfN85NSS0wWkEwSERZVzQ) : [解説記事](http://yaneuraou.yaneu.com/2017/06/13/%E7%B6%9A-%E4%BA%BA%E9%96%93%E3%81%AE%E6%A3%8B%E8%AD%9C%E3%82%92%E7%94%A8%E3%81%84%E3%81%9A%E3%81%AB%E8%A9%95%E4%BE%A1%E9%96%A2%E6%95%B0%E3%81%AE%E5%AD%A6%E7%BF%92/)
- [リゼロ評価関数 epoch 7](https://drive.google.com/open?id=0Bzbi5rbfN85NWWloTFdMRjI5LWs) : [解説記事](http://yaneuraou.yaneu.com/2017/06/15/%E7%B6%9A2-%E4%BA%BA%E9%96%93%E3%81%AE%E6%A3%8B%E8%AD%9C%E3%82%92%E7%94%A8%E3%81%84%E3%81%9A%E3%81%AB%E8%A9%95%E4%BE%A1%E9%96%A2%E6%95%B0%E3%81%AE%E5%AD%A6%E7%BF%92/)
- [リゼロ評価関数 epoch 8](https://drive.google.com/open?id=0Bzbi5rbfN85NMHd0OEUxcUVJQW8) : [解説記事](http://yaneuraou.yaneu.com/2017/06/21/%E3%83%AA%E3%82%BC%E3%83%AD%E8%A9%95%E4%BE%A1%E9%96%A2%E6%95%B0epoch8%E5%85%AC%E9%96%8B%E3%81%97%E3%81%BE%E3%81%97%E3%81%9F%E3%80%82/)

### やねうら王 KPP_KKPT型ビルド用評価関数

やねうら王2017 KPP_KKPT型ビルドで使える評価関数です。

- [リゼロ評価関数 KPP_KKPT型 epoch4](https://drive.google.com/open?id=0Bzbi5rbfN85NSk1qQ042U0RnUEU) : [解説記事](http://yaneuraou.yaneu.com/2017/09/02/%E3%82%84%E3%81%AD%E3%81%86%E3%82%89%E7%8E%8B%E3%80%81kpp_kkpt%E5%9E%8B%E8%A9%95%E4%BE%A1%E9%96%A2%E6%95%B0%E3%81%AB%E5%AF%BE%E5%BF%9C%E3%81%97%E3%81%BE%E3%81%97%E3%81%9F/)

### Shivoray(シボレー) 全自動雑巾絞り機

自分で自分好みの評価関数を作って遊んでみたいという人のために『Shivoray』(シボレー)という全自動雑巾絞り機を公開しました。

- [ShivorayV4.71](https://drive.google.com/open?id=0Bzbi5rbfN85Nb292azZxRmU0R1U) : [解説記事](http://yaneuraou.yaneu.com/2017/06/26/%E3%80%8Eshivoray%E3%80%8F%E5%85%A8%E8%87%AA%E5%8B%95%E9%9B%91%E5%B7%BE%E7%B5%9E%E3%82%8A%E6%A9%9F%E5%85%AC%E9%96%8B%E3%81%97%E3%81%BE%E3%81%97%E3%81%9F/)

## 定跡集

やねうら王2017Earlyで使える、各種定跡集。
ダウンロードしたあと、zipファイルになっているのでそれを解凍して、やねうら王の実行ファイルを配置しているフォルダ配下のbookフォルダに放り込んでください。

- コンセプトおよび定跡フォーマットについて : [やねうら大定跡はじめました](http://yaneuraou.yaneu.com/2016/07/10/%E3%82%84%E3%81%AD%E3%81%86%E3%82%89%E5%A4%A7%E5%AE%9A%E8%B7%A1%E3%81%AF%E3%81%98%E3%82%81%E3%81%BE%E3%81%97%E3%81%9F/)
- 定跡ファイルのダウンロードは[こちら](https://github.com/yaneurao/YaneuraOu/releases/tag/v4.73_book)

## 世界コンピュータ将棋選手権および2017年に開催される第5回将棋電王トーナメントに参加される開発者の方へ

やねうら王をライブラリとして用いて参加される場合、このやねうら王のGitHub上にあるすべてのファイルおよび、このトップページから直リンしているファイルすべてが使えます。

## ライセンス

やねうら王プロジェクトのソースコードはStockfishをそのまま用いている部分が多々あり、Apery/SilentMajorityを参考にしている部分もありますので、やねうら王プロジェクトは、それらのプロジェクトのライセンス(GPLv3)に従うものとします。

「リゼロ評価関数ファイル」については、やねうら王プロジェクトのオリジナルですが、一切の権利は主張しませんのでご自由にお使いください。
