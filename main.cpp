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

// SSBOのデータのチェックを行う際は 1
#define DATACHECK 1
#define DATACHECK_CALC 0

#define FAR  300
#define NEAR 20

// lsmを用いる際は 1
#define LSM_OPTION 1

// LU分解を行う際は 1
#define LU_DEP 1

// ssboのデータセット
struct DataSet {
	GLfloat data_xyz[4];
	GLfloat data_xy2[4];
	GLfloat data_xyzx[4];
};

// テクスチャを作成する
// CLAMP_MODEでテクスチャの端を指定
// 0 : GL_CLAMP_TO_EDGE  （折り返す
// デフォルト	 1 : GL_CLAMP_TO_BORDER (borderで塗る（真っ黒）
void makeTex(GLuint p, int width, int height, int CLAMP_MODE = 1) {

	// テクスチャの作成
	glBindTexture(GL_TEXTURE_2D, p);

	// LAP_MODEの値によって挙動を変える
	if (CLAMP_MODE == 0) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	if (CLAMP_MODE == 1) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		static const GLfloat border[] = { 0.0, 0.0, 0.0, 0.0 };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);
}


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

  // OpenGL Version 4.3 Core Profile を選択する
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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

#if LSM_OPTION
  // デプスデータから計算に必要な値を計算するシェーダ
  ComputeShader comp(width, height, "calc.comp");

  // lsmを計算するシェーダ
  ComputeShader lsm(width, width, "lsm.comp");
  ComputeShader lsmOutput(2, 2, "lsmOutput.comp");

  // LU分解を計算するシェーダ
  ComputeShader ludep(1, 1, "ludep.comp");

  // データ出力用のSSBO
  const GLint count(1);

  // SSBO を作る
  //   ・メモリ確保は glBufferData() で行うから一度これを実行しておかないとダメ．
  //   ・第２引数の size は確保するメモリのサイズをバイト数で指定する．
  //   ・最後の第４引数 usage は使い方のヒントで読み書き禁止とかではない．
  //   ・最後の第４引数 usage は使い方のヒントで読み書き禁止とかではない．
  //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBufferData.xhtml
  GLuint ssbo;
  glGenBuffers(1, &ssbo);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
  glBufferData(GL_SHADER_STORAGE_BUFFER, count * sizeof(DataSet), NULL, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
  // データの格納先
  std::vector<GLfloat> output(count * sizeof(DataSet));

  // 計算用のテクスチャ
  GLuint tex1;
  glGenTextures(1, &tex1);
  makeTex(tex1, width, height);

  GLuint tex2;
  glGenTextures(1, &tex2);
  makeTex(tex2, width, height);

  GLuint tex3;
  glGenTextures(1, &tex3);
  makeTex(tex3, width, height);

  GLuint tex4;
  glGenTextures(1, &tex4);
  makeTex(tex4, 32, 32);

  GLuint tex5;
  glGenTextures(1, &tex5);
  makeTex(tex5, 32, 32);

  GLuint tex6;
  glGenTextures(1, &tex6);
  makeTex(tex6, 32, 32);

  GLuint tex7;
  glGenTextures(1, &tex7);
  makeTex(tex7, 2, 2);

  GLuint tex8;
  glGenTextures(1, &tex8);
  makeTex(tex8, 2, 2);

  GLuint tex9;
  glGenTextures(1, &tex9);
  makeTex(tex9, 2, 2);

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

	// LSMの計算を行う
#if LSM_OPTION 
	// computeshaderを使用する
	comp.use();

	glUniform1i(0, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, positionTexture[0]);

	glActiveTexture(GL_TEXTURE1);
	glBindImageTexture(1, tex1, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glActiveTexture(GL_TEXTURE2);
	glBindImageTexture(2, tex2, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glActiveTexture(GL_TEXTURE3);
	glBindImageTexture(3, tex3, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

#if DATACHECK_CALC
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, ssbo);

	comp.calculate(width, height);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, 0);
	// SSBO から値を取り出す
	//   ・glMapbuffer() で取り出したポインタは glUnmapBuffer() すると無効になる・
	//   ・glUnmapBuffer() する前に処理を終えるかデータをコピーしておく．
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, count * sizeof(DataSet), output.data());

	std::cout << "calc : ";
	for (int i = 0; i < 12; i++) {
		std::cout << output[i] << " - ";
	}
	std::cout << sizeof(DataSet) << std::endl;

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
#else
	comp.calculate(width, height);
#endif
	
	// lsm-computeshaderを使用する
	// 512-512 を 32-32 にする
	lsm.use();
	glActiveTexture(GL_TEXTURE1);
	glBindImageTexture(1, tex1, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

	glActiveTexture(GL_TEXTURE2);
	glBindImageTexture(2, tex2, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

	glActiveTexture(GL_TEXTURE3);
	glBindImageTexture(3, tex3, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

	glActiveTexture(GL_TEXTURE4);
	glBindImageTexture(4, tex4, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glActiveTexture(GL_TEXTURE5);
	glBindImageTexture(5, tex5, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glActiveTexture(GL_TEXTURE6);
	glBindImageTexture(6, tex6, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

#if DATACHECK
	// 計算結果の出力先
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, ssbo);

	lsm.calculate(32, 32);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, 0);
	//処理の実行:結果がfloatでSSBOに入る
	// データの格納先
	//std::vector<GLfloat> output(count * sizeof(DataSet));
	// SSBO から値を取り出す
	//   ・glMapbuffer() で取り出したポインタは glUnmapBuffer() すると無効になる・
	//   ・glUnmapBuffer() する前に処理を終えるかデータをコピーしておく．
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, count * sizeof(DataSet), output.data());

	std::cout << "first lsm : ";
	for (int i = 0; i < 12; i++) {
		std::cout << output[i] << " ";
	}
	std::cout << std::endl;

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
#else
	lsm.calculate(32, 32);
#endif
	
	// 二回目の計算 32-32 を 2-2 にする
	lsm.use();

	glActiveTexture(GL_TEXTURE1);
	glBindImageTexture(1, tex4, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

	glActiveTexture(GL_TEXTURE2);
	glBindImageTexture(2, tex5, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

	glActiveTexture(GL_TEXTURE3);
	glBindImageTexture(3, tex6, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

	glActiveTexture(GL_TEXTURE4);
	glBindImageTexture(4, tex7, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glActiveTexture(GL_TEXTURE5);
	glBindImageTexture(5, tex8, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glActiveTexture(GL_TEXTURE6);
	glBindImageTexture(6, tex9, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

#if DATACHECK
	// 計算結果の出力先
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, ssbo);

	lsm.calculate(2, 2);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, 0);
	//処理の実行:結果がfloatでSSBOに入る
	// データの格納先
	//std::vector<GLfloat> output(count * sizeof(DataSet));
	// SSBO から値を取り出す
	//   ・glMapbuffer() で取り出したポインタは glUnmapBuffer() すると無効になる・
	//   ・glUnmapBuffer() する前に処理を終えるかデータをコピーしておく．
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, count * sizeof(DataSet), output.data());

	std::cout << "second lsm : ";
	for (int i = 0; i < 12; i++) {
		std::cout << output[i] << " ";
	}
	std::cout << std::endl;

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
#else
	lsm.calculate(2, 2);
#endif
	lsmOutput.use();
	glActiveTexture(GL_TEXTURE4);
	glBindImageTexture(1, tex7, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glActiveTexture(GL_TEXTURE5);
	glBindImageTexture(2, tex8, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glActiveTexture(GL_TEXTURE6);
	glBindImageTexture(3, tex9, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	// 計算結果の出力先
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, ssbo);

	lsmOutput.calculate(1, 1);

#if DATACHECK
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, 0);
	// SSBO から値を取り出す
	//   ・glMapbuffer() で取り出したポインタは glUnmapBuffer() すると無効になる・
	//   ・glUnmapBuffer() する前に処理を終えるかデータをコピーしておく．
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, count * sizeof(DataSet), output.data());

	// 計算結果をコマンドラインに出力する
	std::cout << "lsm : ";
	for (int i = 0; i < 12; i++) {
		std::cout << output[i] << " ";
	}
	std::cout << std::endl;

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
#endif

	// LU分解を行っている部分
#if LU_DEP 
	
	ludep.use();
	// 計算結果の出力先
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, ssbo);

	ludep.calculate(1, 1);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, 0);

	// SSBO から値を取り出す
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, count * sizeof(DataSet), output.data());

	// 計算結果をコマンドラインに出力する
	std::cout << "x = " << output[0] << ", y = " << output[1] << ", z = " << output[2] << std::endl;

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

#endif

#endif

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
