//
// Kinect (v2) のデプスマップ取得
//

// 標準ライブラリ
#include <Windows.h>

// ウィンドウ関連の処理
#include "Window.h"

// センサ関連の処理
#include "KinectV2.h"

// 描画に用いるメッシュ
#include "Mesh.h"

// 計算に用いるシェーダ
#include "Calculate.h"

// 主成分分析を行うためのシェーダ
#include "ComputeShader.h"

// 頂点位置の生成をシェーダ (position.frag) で行うなら 1
#define GENERATE_POSITION 1

// 主成分分析(Principal Component Analysis)で平面を出すなら 1　＊未実装
#define PCA_OPTION 0

// 最小二乗法(Least Squares Method)で平面を出すなら 1
#define LSM_OPTION 1

// デプスセンサーのデータ範囲の指定
#define NEAR_RENGE 100
#define FAR_RENGE 4000

//
// メインプログラム
//
int main()
{
  // GLFW を初期化する
  if (glfwInit() == GL_FALSE)
  {
    // GLFW の初期化に失敗した
    MessageBox(NULL, TEXT("GLFW の初期化に失敗しました。"), TEXT("すまんのう"), MB_OK);
    return EXIT_FAILURE;
  }

  // プログラム終了時には GLFW を終了する
  atexit(glfwTerminate);

  // OpenGL Version 3.2 Core Profile を選択する
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // ウィンドウを開く
  Window window(640, 480, "Depth Map Viewer");
  if (!window.get())
  {
    // ウィンドウが作成できなかった
    MessageBox(NULL, TEXT("GLFW のウィンドウが開けませんでした。"), TEXT("すまんのう"), MB_OK);
    return EXIT_FAILURE;
  }

  // 深度センサを有効にする
  KinectV2 sensor;
  if (sensor.getActivated() == 0)
  {
    // センサが使えなかった
    MessageBox(NULL, TEXT("深度センサを有効にできませんでした。"), TEXT("すまんのう"), MB_OK);
    return EXIT_FAILURE;
  }

  // 深度センサの解像度
  int width, height;
  sensor.getDepthResolution(&width, &height);

  // 描画に使うメッシュ
  const Mesh mesh(width, height, sensor.getCoordBuffer());

  // 描画用のシェーダ
  GgSimpleShader simple("simple.vert", "simple.frag");

  // デプスデータから頂点位置を計算するシェーダ
  const Calculate position(width, height, "position.frag");

  // 頂点位置から法線ベクトルを計算するシェーダ
  const Calculate normal(width, height, "normal.frag");


#if PCA_OPTION
  // デプスカメラで対象平面を主成分分析
  ComputeShader pca(width, height, "pca.comp");
#endif

#if LSM_OPTION
  // デプスカメラで対象平面を最小二乗法で計算
  ComputeShader calc_xyz(width, height, "calc_xyz.comp");
  ComputeShader lsm(width, height, "lsm.comp");

#endif

  // 背景色を設定する
  glClearColor(background[0], background[1], background[2], background[3]);

  // 隠面消去処理を有効にする
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);

  // ウィンドウが開いている間くり返し描画する
  while (!window.shouldClose())
  {
#if GENERATE_POSITION
	  //pcaで平面推定　＊未作成
#if PCA_OPTION 
	  pca.use();
#endif

#if LSM_OPTION
	  calc_xyz.use();
	  // depthデータの送信
	  glUniform1i(0, 0);
	  glActiveTexture(GL_TEXTURE0);
	  sensor.getDepth();

	  // 処理結果の保存
	  // x^2 と　y^2 の保存
	  glActiveTexture(GL_TEXTURE1);
	  glBindImageTexture(1, calc_xyz.tex_A, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

	  // xy yz xz の保存
	  glActiveTexture(GL_TEXTURE2);
	  glBindImageTexture(2, calc_xyz.tex_B, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

	  calc_xyz.calculate();

	  lsm.use();
	  
	  glActiveTexture(GL_TEXTURE0);
	  glBindImageTexture(0, calc_xyz.tex_C, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

	  glActiveTexture(GL_TEXTURE1);
	  glBindImageTexture(1, calc_xyz.tex_A, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

	  glActiveTexture(GL_TEXTURE2);
	  glBindImageTexture(2, calc_xyz.tex_B, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

	  lsm.use();

#endif

    // 頂点位置の計算
    position.use();
    glUniform1i(0, 0);
    glActiveTexture(GL_TEXTURE0);
    sensor.getDepth();
    const std::vector<GLuint> &positionTexture(position.calculate());
#endif

    // 法線ベクトルの計算
    normal.use();
    glUniform1i(0, 0);
    glActiveTexture(GL_TEXTURE0);
#if GENERATE_POSITION
    glBindTexture(GL_TEXTURE_2D, positionTexture[0]);
#else
    sensor.getPoint();
#endif
    const std::vector<GLuint> &normalTexture(normal.calculate());

    // 画面消去
    window.clear();

    // 描画用のシェーダプログラムの使用開始
    simple.use();
    simple.loadMatrix(window.getMp(), window.getMw());
    simple.setLight(light);
    simple.setMaterial(material);

#if GENERATE_POSITION
    // 頂点位置テクスチャ
    glUniform1i(0, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, positionTexture[0]);
#endif

    // 頂点法線テクスチャ
    glUniform1i(1, 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalTexture[0]);

    // カラーテクスチャ
    glUniform1i(2, 2);
    glActiveTexture(GL_TEXTURE2);
    sensor.getColor();

    // 図形描画
    mesh.draw();

    // バッファを入れ替える
    window.swapBuffers();
  }
}
