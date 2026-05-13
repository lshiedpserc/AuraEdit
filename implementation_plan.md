# AuraEdit — Windows向けリアルタイム顔加工アプリケーション

## プロジェクト概要

既存の画像ファイル（PNG / JPEG）に対して高度な顔認識・ワープ・美肌等のエフェクトを適用するデスクトップアプリケーション。C++/OpenGLコアエンジンの上に透明なQt WebEngine UIを重ね合わせるハイブリッド方式を採用。
※ ウェブカメラ機能はあくまでサブ機能（おまけ）として実装する。

---

## 確認済み環境

| ツール | 状態 | 詳細 |
|--------|------|------|
| Visual Studio 2022 Professional | ✅ インストール済 | C++ Desktop workload確認済 |
| CMake | ✅ VS同梱 (3.31.6) | PATHに未設定 → 設定必要 |
| Qt 6.8.3 (msvc2022_64) | ⚠️ 部分的 | Core/OpenGL あり、**WebEngine/WebChannel/Multimedia 未インストール** |
| Python 3.11.9 | ✅ | |
| MSYS2 | ✅ `C:\msys64` | |
| Git | ✅ 2.50.1 | |
| Node.js | ✅ v24.14.0 | |
| Bazel/Bazelisk | ❌ 未インストール | **インストール必要** |
| OpenCV | ❓ 未確認 | MediaPipeビルドに必要、**インストール必要** |

---

## Phase 0: 環境セットアップ（必須）

### Step 0-1: Bazelisk (Bazel) インストール
```powershell
# Chocolateyまたは手動でbazeliskをインストール
choco install bazelisk
# または手動: https://github.com/bazelbuild/bazelisk/releases からダウンロード
```

### Step 0-2: Qt 追加モジュールインストール
aqtinstall を使用して不足モジュールを追加:
```powershell
pip install aqtinstall
aqt install-qt windows desktop 6.8.3 win64_msvc2022_64 -m qtwebengine qtwebchannel qtmultimedia --outputdir C:\Qt
```

### Step 0-3: CMake PATH設定
```powershell
# VS同梱CMakeをPATHに追加
$env:Path += ";C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"
```

### Step 0-4: OpenCV インストール
MediaPipe C++ が依存する OpenCV を `C:\opencv` に配置:
```powershell
# vcpkg経由 or 公式インストーラーでインストール
```

### Step 0-5: MediaPipe ソースクローン
```powershell
cd C:\dev
git clone https://github.com/google/mediapipe.git
```

### Step 0-6: 環境変数設定
```powershell
$env:BAZEL_VS = "C:\Program Files\Microsoft Visual Studio\2022\Professional"
$env:BAZEL_VC = "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC"
```

---

## Phase 1: コアエンジンのモックアップ（C++）

> [!IMPORTANT]
> **方針変更**: MediaPipe C++ Bazel Buildは、Windows上ではMediaPipeのソースツリー内にプロジェクトを組み込む方式が最も安定します。外部CMakeプロジェクトからのリンクは依存関係（Abseil, Protobuf, TFLite等）の管理が極めて困難なため、**MediaPipeリポジトリ内に `aura_edit` ディレクトリを作成し、Bazel BUILD ファイルで統合する方式** を採用します。

### プロジェクト構造（ワークスペース直接編集方式）

MediaPipeの依存関係を解決しつつAuraEdit内で開発を行うため、ソースはAuraEdit内に配置し、ビルド時にジャンクション（シンボリックリンク）を利用して統合します。

```text
C:\Users\otame\Documents\work\AuraEdit/  # メインワークスペース
├── src/
│   ├── core/                            # コアエンジン (C++)
│   │   ├── BUILD                        # Bazel BUILD ファイル
│   │   ├── aura_edit_main.cc            # エントリーポイント（画像読込・描画）
│   │   ├── face_mesh_aura.pbtxt         # 推論グラフ設定
│   │   └── ...                          # 以降Phase 2でシェーダー処理を追加
│   ├── shaders/                         # GLSL シェーダー
│   │   ├── passthrough.vert
│   │   ├── passthrough.frag
│   │   ├── warp.vert
│   │   ├── beauty.frag
│   │   └── color_grade.frag
│   └── ui/                              # Web UI (HTML/CSS/JS)
│       ├── web/
│       │   ├── index.html
│       │   ├── styles.css
│       │   └── app.js
├── build_core.bat                       # ビルドスクリプト
└── stitch_face_pop_studio/              # 既存デザインモック

C:\dev\mediapipe/                        # MediaPipeビルド環境
├── mediapipe/
│   ├── examples/
│   │   └── desktop/
│   │       └── aura_edit/               # [JUNCTION] -> AuraEdit/src/core
```

> [!WARNING]
> **GPU推論の制限**: MediaPipe C++ の Windows GPU 推論は限定的です。`--define MEDIAPIPE_DISABLE_GPU=1` フラグでCPU推論で開始し、後のフェーズでGPU推論への移行を検討します。顔メッシュ推論自体はCPUでも60FPS対応可能です（推論はリアルタイムだがワープ/描画はGPUシェーダーが担当）。

### [NEW] BUILD ファイル

```python
cc_binary(
    name = "aura_edit",
    srcs = [
        "main.cpp",
        "engine.cpp",
        "engine.h",
        "camera.cpp",
        "camera.h",
        "renderer.cpp",
        "renderer.h",
        "face_mesh_runner.cpp",
        "face_mesh_runner.h",
    ],
    deps = [
        "//mediapipe/graphs/face_mesh:face_mesh_desktop_live_calculators",
        "//mediapipe/framework:calculator_framework",
        "//mediapipe/framework/formats:landmark_cc_proto",
        "//mediapipe/framework/port:commandlineflags",
        "//mediapipe/framework/port:file_helpers",
        "//mediapipe/framework/port:parse_text_proto",
        "//mediapipe/framework/port:status",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
    ],
    data = [
        "//mediapipe/modules/face_landmark:face_landmark_front_cpu",
    ],
)
```

### [NEW] aura_edit_main.cc — エントリーポイント

- **プライマリワークフロー**: 画像ファイル（PNG/JPEG）読み込み → 推論・ワープ処理 → 保存・表示
- **セカンダリワークフロー**: Webカメラからのストリームキャプチャ（おまけ機能）
- コマンドライン引数によるモード切替（`--input_image_path` vs `--use_camera`）

### [NEW] camera.h / camera.cpp — Webカメラキャプチャ

**Phase 1 実装**: OpenCV `cv::VideoCapture` を使用（MediaPipeの依存ツリーに既にOpenCVが含まれる）

```cpp
class Camera {
public:
    bool open(int deviceIndex = 0, int width = 1280, int height = 720);
    bool readFrame(cv::Mat& frame);
    void close();
    std::vector<std::string> listDevices();
private:
    cv::VideoCapture cap_;
};
```

### [NEW] renderer.h / renderer.cpp — OpenGLレンダラー

- QOpenGLWidget サブクラス
- カメラフレーム → OpenGLテクスチャ変換（PBO経由で高速化）
- 全画面クワッドレンダリング
- ランドマーク点のオーバーレイ描画（デバッグモード）
- FPSカウンター表示

### [NEW] face_mesh_runner.h / face_mesh_runner.cpp — FaceMesh推論

- MediaPipe CalculatorGraph のラッパー
- `face_mesh_desktop_live.pbtxt` グラフを使用
- 468点の NormalizedLandmark を取得し、`std::vector<glm::vec3>` に変換
- 非同期推論スレッドで駆動し、描画スレッドとは独立

---

## Phase 2: シェーダーによるワープ処理の実装（GLSL）

### ワープ方式: メッシュベース三角形テクスチャマッピング

468ランドマーク点から **Delaunay三角形分割** を行い、頂点をシフトすることで顔テクスチャを変形。

| 機能 | 対象ランドマーク | ワープ方式 |
|------|-----------------|-----------|
| デカ目 | #33,#133,#362,#263 周辺 | 中心放射拡張 |
| 小顔・輪郭 | #10,#152,#234,#454 周辺 | 法線方向圧縮 |
| 鼻形加工 | #1,#2,#98,#327 周辺 | 中心線収束 |
| 眉加工 | #70,#63,#300,#293 周辺 | 上方リフト |
| 目の形加工 | 目輪郭ランドマーク全体 | カスタム変形 |

### フラグメントシェーダー群

- **beauty.frag**: バイラテラルフィルタ美肌、高機能美肌（周波数分離方式）
- **skin_color.frag**: HSLベース肌色変更
- **hair_color.frag**: セグメンテーションマスクベース髪色変更
- **color_grade.frag**: LUTテクスチャによるトーンカーブ調整

---

## Phase 3: Web UI層の構築と統合

### 透明レイヤー重ね合わせ方式

```
┌──────────────────────────────────┐
│  Qt WebEngine (前面・透明背景)    │ ← HTML/CSS/JS UI
│  スライダー、パネル、ボタン       │
├──────────────────────────────────┤
│  OpenGL (背面)                   │ ← カメラ映像 + シェーダー加工
│  ワープ済みフレーム描画           │
└──────────────────────────────────┘
```

### QWebChannel IPC Bridge

```cpp
class Bridge : public QObject {
    Q_OBJECT
    // 全パラメータをQ_PROPERTYで公開
    Q_PROPERTY(double skinSmooth READ skinSmooth WRITE setSkinSmooth NOTIFY skinSmoothChanged)
    Q_PROPERTY(double eyeSize READ eyeSize WRITE setEyeSize NOTIFY eyeSizeChanged)
    Q_PROPERTY(double faceSlim READ faceSlim WRITE setFaceSlim NOTIFY faceSlimChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    // ...
};
```

### UI画面（Stitchモック準拠）

5つの主要画面:
1. **Camera** — リアルタイムプレビュー + 撮影
2. **Editor** — レタッチツール（スライダー群 + カテゴリチップ）
3. **Gallery** — 保存画像管理
4. **Filter Library** — プリセットフィルター選択
5. **Settings** — アプリ設定（自動保存/出力品質/言語切替 🇯🇵/🇬🇧）

### 言語切替（Phase 1から実装）

```javascript
// i18n system
const i18n = {
  ja: {
    skinSmooth: '肌のなめらかさ',
    eyeSize: '目の大きさ',
    faceSlim: '輪郭の調整',
    settings: '設定',
    // ...
  },
  en: {
    skinSmooth: 'Skin Smoothness',
    eyeSize: 'Eye Size',
    faceSlim: 'Face Contour',
    settings: 'Settings',
    // ...
  }
};
```

---

### 実行順序（更新）

1. **Phase 0**: 環境セットアップ（Bazelisk, Qt, OpenCV, ワークスペースリンク設定）
2. **Phase 1**: 画像ファイルの読み込み + FaceMesh 468点ランドマーク重畳描画（カメラ対応も含む）
3. **Phase 2**: OpenGLウィンドウへの描画統合、ワープシェーダー + 美肌シェーダー実装
4. **Phase 3**: 透明 Qt WebEngine UI + QWebChannel IPC 統合

---

## Verification Plan

### Phase 0 完了条件
- [ ] `bazelisk version` が動作する
- [ ] MediaPipe の face_mesh サンプル (`face_mesh_cpu`) がビルド成功
- [ ] Qt WebEngine / WebChannel / Multimedia が利用可能

### Phase 1 完了条件
- [ ] Webカメラ映像が OpenGL ウィンドウに60FPSで表示される
- [ ] FaceMesh 468点のランドマークが映像上に重畳描画される
- [ ] ウィンドウのリサイズに追従する

### Phase 2 完了条件
- [ ] デカ目・小顔・美肌がリアルタイムで反映される
- [ ] 各パラメータがスライダー値で制御可能

### Phase 3 完了条件
- [ ] 透明UIがOpenGL描画の上に正しく重なる
- [ ] スライダー操作が即座にシェーダーパラメータに反映
- [ ] 日本語/英語切替が動作する
- [ ] モックアップ準拠のデザイン品質
