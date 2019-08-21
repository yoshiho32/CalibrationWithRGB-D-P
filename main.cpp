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

// ssboのデータセット
struct DataSet {
	GLfloat data_xyz[4];
	GLfloat data_xy2[4];
	GLfloat data_xyzx[4];
};

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
  // データ数

  // OpenGL Version 3.2 Core Profile を選択する
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
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

  ComputeShader comp(width, height, "calc.comp");

  const GLint count(10);

  // SSBO を作る
  //   ・メモリ確保は glBufferData() で行うから一度これを実行しておかないとダメ．
  //   ・第２引数の size は確保するメモリのサイズをバイト数で指定する．
  //   ・最後の第４引数 usage は使い方のヒントで読み書き禁止とかではない．
  //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBufferData.xhtml
  GLuint ssbo;
  glGenBuffers(1, &ssbo);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
  glBufferData(GL_SHADER_STORAGE_BUFFER, count * sizeof(DataSet), NULL, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


  // 背景色を設定する
  glClearColor(background[0], background[1], background[2], background[3]);

  // 隠面消去処理を有効にする
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);

  // ウィンドウが開いている間くり返し描画する
  while (!window.shouldClose())
  {
#if GENERATE_POSITION
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
	
	comp.use();

	glUniform1i(0, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, positionTexture[0]);

	// 計算結果の出力先
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ssbo);

	comp.calculate(width, height);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, 0);
	//処理の実行:結果がfloatでSSBOに入る
	// データの格納先
	std::vector<GLfloat> output(count * sizeof(DataSet));
	// SSBO から値を取り出す
	//   ・glMapbuffer() で取り出したポインタは glUnmapBuffer() すると無効になる・
	//   ・glUnmapBuffer() する前に処理を終えるかデータをコピーしておく．
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, count * sizeof(DataSet), output.data());

	for (int i = 0; i < 12; i++) {
		std::cout << output[i] << " - ";
	}
	std::cout << sizeof(DataSet) << std::endl;

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

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
