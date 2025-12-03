# PPHavokNav Plugin

## 概要
PPHavokNavは、Havok Navigationの拡張機能を提供するPocketpair製プラグインです。
このプラグインは、MassPalプラグインから以下の機能を分離・移行したものです：

- ランタイムナビメッシュ生成機能
- NavMeshパッチ機能（複数NavMeshインスタンスの管理）

## モジュール構成

### PPHavokNavRuntimeGen (Runtime)
HavokNavigationのランタイム動的NavMesh生成機能を提供します。

主な機能：
- 動的NavMesh生成システム
- NavVolume生成システム
- Chaos Geometry Provider統合
- Bounds Provider (Actor, Box, BoxVolume)
- Input Entity Gatherer

主なクラス：
- `UPPHkNav_RuntimeGenerationSubsystem` - ランタイム生成サブシステム
- `UPPHkNav_DynamicNavMeshSubsystem` - 動的NavMesh管理サブシステム
- `UPPHkNavHavokNavMeshGeneratorFactory` - NavMeshジェネレータファクトリ
- `UPPHkNavHavokNavVolumeGeneratorFactory` - NavVolumeジェネレータファクトリ

### PPHavokNavRuntimeGenEditor (Editor)
PPHavokNavRuntimeGenのエディタ拡張機能を提供します。

主な機能：
- NavMeshベイクユーティリティ
- エディタコマンド
- コンポーネントビジュアライザ

### PPHavokNavPatch (Runtime)
NavMeshパッチ機能を提供します。複数のNavMeshインスタンスを効率的に管理します。

主な機能：
- InstancedNavMeshComponent - 大量のNavMeshインスタンスを管理
- NavMeshMultiPatchComponent - 複数のNavMeshパッチを動的にロード/アンロード
- NavMeshUserEdgeSocket - ユーザー定義エッジ接続

主なクラス：
- `UPPHkNavInstancedNavMeshComponent` - インスタンス化NavMeshコンポーネント
- `UPPHkNavNavMeshMultiPatchComponent` - マルチパッチNavMeshコンポーネント
- `UPPHkNav_NavMeshDataAsset` - NavMeshデータアセット

### PPHavokNavPatchEditor (Editor)
PPHavokNavPatchのエディタ拡張機能を提供します。

主な機能：
- InstancedNavMeshComponentビジュアライザ

## 命名規則
- クラス名プレフィックス: `PPHkNav` (例: `UPPHkNavRuntimeGen`)
- 構造体プレフィックス: `FPPHkNav` (例: `FPPHkNavMeshGenerationParams`)
- インターフェースプレフィックス: `IPPHkNav` (例: `IPPHkNavDynamicNavMeshGenerator`)
- 名前空間: `PPHkNav`
- DLLエクスポートマクロ:
  - `PPHKNAVRUNTIMEGEN_API`
  - `PPHKNAVRUNTIMEGENEDITOR_API`
  - `PPHKNAVPATCH_API`
  - `PPHKNAVPATCHEDITOR_API`

## 依存関係
- HavokCore
- HavokNavigation
- HavokNavigationGeneration (RuntimeGen modules only)
- HavokNavigationGenerationCore (RuntimeGen modules only)

## 移行元
このプラグインは以下のMassPalモジュールから機能を移行しています：
- MassPalNavGen → PPHavokNavRuntimeGen
- MassPalNavGenEditor → PPHavokNavRuntimeGenEditor
- MassPalNavigation (NavMeshパッチ機能のみ) → PPHavokNavPatch
- MassPalNavigationEditor (パッチ関連のみ) → PPHavokNavPatchEditor

注意: Mass Framework連携部分は移行していません。それらの機能は引き続きMassPalプラグインに残ります。

## 使用方法
1. プロジェクトのPluginsフォルダにこのプラグインを配置
2. .uprojectファイルのPlugins配列にPPHavokNavを追加
3. 必要なモジュールを.Build.csファイルの依存関係に追加

例:
```csharp
PublicDependencyModuleNames.AddRange(new string[]
{
    "PPHavokNavRuntimeGen",
    "PPHavokNavPatch",
});
```

## ライセンス
Copyright Pocketpair, Inc. All Rights Reserved.
