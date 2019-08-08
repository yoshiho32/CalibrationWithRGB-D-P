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

#if LSM_OPTION
// ssboのデータセット
struct DataSet {
	GLfloat data_xyz[4];
	GLfloat data_xy2[4];
	GLfloat data_xyzx[4];
  };

#endif

// テクスチャを作成する
// CLAMP_MODEでテクスチャの端を指定
// 0 : GL_CLAMP_TO_EDGE  （折り返す
// 1 : GL_CLAMP_TO_BORDER (borderで塗る（真っ黒）
void makeTex(GLuint p, int width, int height, int CLAMP_MODE = 0) {

	// テクスチャの作成
	//glGenTextures(1, &p);
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

  // 深度センサの解像度の最大値
  int MAX_WH = width>=height ? width : height;



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
  ComputeShader lsm_output(width, height, "lsm_output.comp");
  
  //計算用のテクスチャ
  GLuint xyz_tex1;
  glGenTextures(1, &xyz_tex1);
  GLuint xyz_tex2;
  glGenTextures(1, &xyz_tex2);
  GLuint xy2_tex1;
  glGenTextures(1, &xy2_tex1);
  GLuint xy2_tex2;
  glGenTextures(1, &xy2_tex2);
  GLuint xyzx_tex1;
  glGenTextures(1, &xyzx_tex1);
  GLuint xyzx_tex2;
  glGenTextures(1, &xyzx_tex2);


  // データ数
  const GLint count(1);

  // SSBO を作る
  //   ・メモリ確保は glBufferData() で行うから一度これを実行しておかないとダメ．
  //   ・第２引数の size は確保するメモリのサイズをバイト数で指定する．
  //   ・最後の第４引数 usage は使い方のヒントで読み書き禁止とかではない．
  //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBufferData.xhtml
  GLuint ssbo;
  glGenBuffers(1, &ssbo);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
  glBufferData(GL_SHADER_STORAGE_BUFFER, count * sizeof(DataSet), NULL, GL_STATIC_READ);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

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


	// 頂点位置の計算
	position.use();
	glUniform1i(0, 0);
	glActiveTexture(GL_TEXTURE0);
	sensor.getDepth();
	const std::vector<GLuint> &positionTexture(position.calculate());

#if PCA_OPTION 
	  pca.use();
#endif

#if LSM_OPTION

	  /* 処理１：デプスデータからx,y,z,x^2,y^2,xy,yz,xz,w(データの有効性)を取得 */
	  calc_xyz.use();
	  // depthデータの送信
	  glUniform1i(0, 0);
	  glActiveTexture(GL_TEXTURE0);
	 
	  glBindTexture(GL_TEXTURE_2D, positionTexture[0]);

	  // 処理結果の保存
	  // x, y, z, w の保存
	  makeTex(xyz_tex1, MAX_WH, MAX_WH, 1);
	  glActiveTexture(GL_TEXTURE1);
	  glBindImageTexture(1, xyz_tex1, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	  // x^2, y^2 の保存
	  makeTex(xy2_tex1, MAX_WH, MAX_WH, 1);
	  glActiveTexture(GL_TEXTURE2);
	  glBindImageTexture(2, xy2_tex1, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	  // xy, yz, xz  の保存
	  makeTex(xyzx_tex1, MAX_WH, MAX_WH, 1);
	  glActiveTexture(GL_TEXTURE3);
	  glBindImageTexture(3, xyzx_tex1, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ssbo);

	  // 計算実行
	  calc_xyz.calculate(width, height);

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
		  std::cout << "-" << output[i];
	  }
	  std::cout << std::endl;
	  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	  
	  /* 処理2：データをすべて加算する *//*
	  lsm.use();
	  // 保存したデータの受け渡し
	  glActiveTexture(GL_TEXTURE0);
	  glBindImageTexture(0, xyz_tex1, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	  glActiveTexture(GL_TEXTURE1);
	  glBindImageTexture(1, xy2_tex1, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	  glActiveTexture(GL_TEXTURE2);
	  glBindImageTexture(2, xyzx_tex1, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

	  // 計算データの保存先
	  MAX_WH /= 16;
	  makeTex(xyz_tex2, MAX_WH, MAX_WH, 1);
	  makeTex(xy2_tex2, MAX_WH, MAX_WH, 1);
	  makeTex(xyzx_tex2, MAX_WH, MAX_WH, 1);
	  glActiveTexture(GL_TEXTURE3);
	  glBindImageTexture(3, xyz_tex2, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	  glActiveTexture(GL_TEXTURE4);
	  glBindImageTexture(4, xy2_tex2, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	  glActiveTexture(GL_TEXTURE5);
	  glBindImageTexture(5, xyzx_tex2, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	  // 計算の実行

	  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	  glBufferData(GL_SHADER_STORAGE_BUFFER, count * sizeof(DataSet), NULL, GL_STATIC_READ);
	  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ssbo);



	  lsm.calculate(MAX_WH, MAX_WH);//512*512→32*32のテクスチャに

	  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, 0);
	  //処理の実行:結果がfloatでSSBOに入る
	  // データの格納先
	  std::vector<GLfloat> output(count * sizeof(DataSet));
	  // SSBO から値を取り出す
	  //   ・glMapbuffer() で取り出したポインタは glUnmapBuffer() すると無効になる・
	  //   ・glUnmapBuffer() する前に処理を終えるかデータをコピーしておく．
	  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	  glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, count * sizeof(DataSet), output.data());

	  for (int i = 0; i < count * sizeof(DataSet); i++) {
		  std::cout << "-" << output[i];
	  }
	  std::cout << sizeof(DataSet) << std::endl;

	  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	  
	  lsm.use();
	  // 保存したデータの受け渡し
	  glActiveTexture(GL_TEXTURE0);
	  glBindImageTexture(0, xyz_tex2, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	  glActiveTexture(GL_TEXTURE1);
	  glBindImageTexture(1, xy2_tex2, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	  glActiveTexture(GL_TEXTURE2);
	  glBindImageTexture(2, xyzx_tex2, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

	  // 計算データの保存先
	  MAX_WH /= 16;
	  makeTex(xyz_tex1, MAX_WH, MAX_WH, 1);
	  makeTex(xy2_tex1, MAX_WH, MAX_WH, 1);
	  makeTex(xyzx_tex1, MAX_WH, MAX_WH, 1);
	  glActiveTexture(GL_TEXTURE3);
	  glBindImageTexture(3, xyz_tex1, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	  glActiveTexture(GL_TEXTURE4);
	  glBindImageTexture(4, xy2_tex1, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	  glActiveTexture(GL_TEXTURE5);
	  glBindImageTexture(5, xyzx_tex1, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	  // 計算の実行
	  lsm.calculate(MAX_WH, MAX_WH);//32*32→2*2のテクスチャに

	  lsm_output.use();
	  glActiveTexture(GL_TEXTURE0);
	  glBindImageTexture(0, xyz_tex1, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	  glActiveTexture(GL_TEXTURE1);
	  glBindImageTexture(1, xy2_tex1, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	  glActiveTexture(GL_TEXTURE2);
	  glBindImageTexture(2, xyzx_tex1, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	  
	  // 計算結果の出力先

	  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	  glBufferData(GL_SHADER_STORAGE_BUFFER, count * sizeof(DataSet) , NULL, GL_STATIC_READ);
	  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo);

	  lsm_output.calculate(2, 2);
	  
	  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, 0);
	  //処理の実行:結果がfloatでSSBOに入る
	  // データの格納先
	  std::vector<GLfloat> output(count * sizeof(DataSet));
	  // SSBO から値を取り出す
	  //   ・glMapbuffer() で取り出したポインタは glUnmapBuffer() すると無効になる・
	  //   ・glUnmapBuffer() する前に処理を終えるかデータをコピーしておく．
	  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	  glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, count * sizeof(DataSet), output.data());

	  for (int i = 0; i < count * sizeof(DataSet); i++) {
		  std::cout << "-" << output[i];
	  }
	  std::cout << sizeof(DataSet) <<std::endl;
	  
	  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	  */
	  
#endif

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
