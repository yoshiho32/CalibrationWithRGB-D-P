//
// Kinect (v2) �̃f�v�X�}�b�v�擾
//

// �W�����C�u����
#include <Windows.h>

// �E�B���h�E�֘A�̏���
#include "Window.h"

// �Z���T�֘A�̏���
#include "KinectV2.h"

// �`��ɗp���郁�b�V��
#include "Mesh.h"

// �v�Z�ɗp����V�F�[�_
#include "Calculate.h"

// �听�����͂��s�����߂̃V�F�[�_
#include "ComputeShader.h"

// ���_�ʒu�̐������V�F�[�_ (position.frag) �ōs���Ȃ� 1
#define GENERATE_POSITION 1

// �听������(Principal Component Analysis)�ŕ��ʂ��o���Ȃ� 1�@��������
#define PCA_OPTION 0

// �ŏ����@(Least Squares Method)�ŕ��ʂ��o���Ȃ� 1
#define LSM_OPTION 1

// �f�v�X�Z���T�[�̃f�[�^�͈͂̎w��
#define NEAR_RENGE 100
#define FAR_RENGE 4000

#if LSM_OPTION
// ssbo�̃f�[�^�Z�b�g
struct DataSet {
	GLfloat data_xyz[4];
	GLfloat data_xy2[4];
	GLfloat data_xyzx[4];
  };

#endif

// �e�N�X�`�����쐬����
// CLAMP_MODE�Ńe�N�X�`���̒[���w��
// 0 : GL_CLAMP_TO_EDGE  �i�܂�Ԃ�
// 1 : GL_CLAMP_TO_BORDER (border�œh��i�^�����j
void makeTex(GLuint p, int width, int height, int CLAMP_MODE = 0) {

	// �e�N�X�`���̍쐬
	//glGenTextures(1, &p);
	glBindTexture(GL_TEXTURE_2D, p);

	// LAP_MODE�̒l�ɂ���ċ�����ς���
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
// ���C���v���O����
//
int main()
{
  // GLFW ������������
  if (glfwInit() == GL_FALSE)
  {
    // GLFW �̏������Ɏ��s����
    MessageBox(NULL, TEXT("GLFW �̏������Ɏ��s���܂����B"), TEXT("���܂�̂�"), MB_OK);
    return EXIT_FAILURE;
  }

  // �v���O�����I�����ɂ� GLFW ���I������
  atexit(glfwTerminate);

  // OpenGL Version 3.2 Core Profile ��I������
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // �E�B���h�E���J��
  Window window(640, 480, "Depth Map Viewer");
  if (!window.get())
  {
    // �E�B���h�E���쐬�ł��Ȃ�����
    MessageBox(NULL, TEXT("GLFW �̃E�B���h�E���J���܂���ł����B"), TEXT("���܂�̂�"), MB_OK);
    return EXIT_FAILURE;
  }

  // �[�x�Z���T��L���ɂ���
  KinectV2 sensor;
  if (sensor.getActivated() == 0)
  {
    // �Z���T���g���Ȃ�����
    MessageBox(NULL, TEXT("�[�x�Z���T��L���ɂł��܂���ł����B"), TEXT("���܂�̂�"), MB_OK);
    return EXIT_FAILURE;
  }

  // �[�x�Z���T�̉𑜓x
  int width, height;
  sensor.getDepthResolution(&width, &height);

  // �[�x�Z���T�̉𑜓x�̍ő�l
  int MAX_WH = width>=height ? width : height;



  // �`��Ɏg�����b�V��
  const Mesh mesh(width, height, sensor.getCoordBuffer());

  // �`��p�̃V�F�[�_
  GgSimpleShader simple("simple.vert", "simple.frag");

  // �f�v�X�f�[�^���璸�_�ʒu���v�Z����V�F�[�_
  const Calculate position(width, height, "position.frag");

  // ���_�ʒu����@���x�N�g�����v�Z����V�F�[�_
  const Calculate normal(width, height, "normal.frag");


#if PCA_OPTION
  // �f�v�X�J�����őΏە��ʂ��听������
  ComputeShader pca(width, height, "pca.comp");
#endif

#if LSM_OPTION

  // �f�v�X�J�����őΏە��ʂ��ŏ����@�Ōv�Z
  ComputeShader calc_xyz(width, height, "calc_xyz.comp");
  ComputeShader lsm(width, height, "lsm.comp");
  ComputeShader lsm_output(width, height, "lsm_output.comp");
  
  //�v�Z�p�̃e�N�X�`��
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


  // �f�[�^��
  const GLint count(1);

  // SSBO �����
  //   �E�������m�ۂ� glBufferData() �ōs�������x��������s���Ă����Ȃ��ƃ_���D
  //   �E��Q������ size �͊m�ۂ��郁�����̃T�C�Y���o�C�g���Ŏw�肷��D
  //   �E�Ō�̑�S���� usage �͎g�����̃q���g�œǂݏ����֎~�Ƃ��ł͂Ȃ��D
  //     https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBufferData.xhtml
  GLuint ssbo;
  glGenBuffers(1, &ssbo);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
  glBufferData(GL_SHADER_STORAGE_BUFFER, count * sizeof(DataSet), NULL, GL_STATIC_READ);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

#endif

  // �w�i�F��ݒ肷��
  glClearColor(background[0], background[1], background[2], background[3]);

  // �B�ʏ���������L���ɂ���
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);

  // �E�B���h�E���J���Ă���Ԃ���Ԃ��`�悷��
  while (!window.shouldClose())
  {
#if GENERATE_POSITION
	  //pca�ŕ��ʐ���@�����쐬


	// ���_�ʒu�̌v�Z
	position.use();
	glUniform1i(0, 0);
	glActiveTexture(GL_TEXTURE0);
	sensor.getDepth();
	const std::vector<GLuint> &positionTexture(position.calculate());

#if PCA_OPTION 
	  pca.use();
#endif

#if LSM_OPTION

	  /* �����P�F�f�v�X�f�[�^����x,y,z,x^2,y^2,xy,yz,xz,w(�f�[�^�̗L����)���擾 */
	  calc_xyz.use();
	  // depth�f�[�^�̑��M
	  glUniform1i(0, 0);
	  glActiveTexture(GL_TEXTURE0);
	 
	  glBindTexture(GL_TEXTURE_2D, positionTexture[0]);

	  // �������ʂ̕ۑ�
	  // x, y, z, w �̕ۑ�
	  makeTex(xyz_tex1, MAX_WH, MAX_WH, 1);
	  glActiveTexture(GL_TEXTURE1);
	  glBindImageTexture(1, xyz_tex1, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	  // x^2, y^2 �̕ۑ�
	  makeTex(xy2_tex1, MAX_WH, MAX_WH, 1);
	  glActiveTexture(GL_TEXTURE2);
	  glBindImageTexture(2, xy2_tex1, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	  // xy, yz, xz  �̕ۑ�
	  makeTex(xyzx_tex1, MAX_WH, MAX_WH, 1);
	  glActiveTexture(GL_TEXTURE3);
	  glBindImageTexture(3, xyzx_tex1, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ssbo);

	  // �v�Z���s
	  calc_xyz.calculate(width, height);

	  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, 0);
	  //�����̎��s:���ʂ�float��SSBO�ɓ���
	  // �f�[�^�̊i�[��
	  std::vector<GLfloat> output(count * sizeof(DataSet));
	  // SSBO ����l�����o��
	  //   �EglMapbuffer() �Ŏ��o�����|�C���^�� glUnmapBuffer() ����Ɩ����ɂȂ�E
	  //   �EglUnmapBuffer() ����O�ɏ������I���邩�f�[�^���R�s�[���Ă����D
	  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	  glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, count * sizeof(DataSet), output.data());

	  for (int i = 0; i < 12; i++) {
		  std::cout << "-" << output[i];
	  }
	  std::cout << std::endl;
	  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	  
	  /* ����2�F�f�[�^�����ׂĉ��Z���� *//*
	  lsm.use();
	  // �ۑ������f�[�^�̎󂯓n��
	  glActiveTexture(GL_TEXTURE0);
	  glBindImageTexture(0, xyz_tex1, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	  glActiveTexture(GL_TEXTURE1);
	  glBindImageTexture(1, xy2_tex1, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	  glActiveTexture(GL_TEXTURE2);
	  glBindImageTexture(2, xyzx_tex1, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

	  // �v�Z�f�[�^�̕ۑ���
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
	  // �v�Z�̎��s

	  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	  glBufferData(GL_SHADER_STORAGE_BUFFER, count * sizeof(DataSet), NULL, GL_STATIC_READ);
	  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ssbo);



	  lsm.calculate(MAX_WH, MAX_WH);//512*512��32*32�̃e�N�X�`����

	  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, 0);
	  //�����̎��s:���ʂ�float��SSBO�ɓ���
	  // �f�[�^�̊i�[��
	  std::vector<GLfloat> output(count * sizeof(DataSet));
	  // SSBO ����l�����o��
	  //   �EglMapbuffer() �Ŏ��o�����|�C���^�� glUnmapBuffer() ����Ɩ����ɂȂ�E
	  //   �EglUnmapBuffer() ����O�ɏ������I���邩�f�[�^���R�s�[���Ă����D
	  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	  glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, count * sizeof(DataSet), output.data());

	  for (int i = 0; i < count * sizeof(DataSet); i++) {
		  std::cout << "-" << output[i];
	  }
	  std::cout << sizeof(DataSet) << std::endl;

	  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	  
	  lsm.use();
	  // �ۑ������f�[�^�̎󂯓n��
	  glActiveTexture(GL_TEXTURE0);
	  glBindImageTexture(0, xyz_tex2, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	  glActiveTexture(GL_TEXTURE1);
	  glBindImageTexture(1, xy2_tex2, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	  glActiveTexture(GL_TEXTURE2);
	  glBindImageTexture(2, xyzx_tex2, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

	  // �v�Z�f�[�^�̕ۑ���
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
	  // �v�Z�̎��s
	  lsm.calculate(MAX_WH, MAX_WH);//32*32��2*2�̃e�N�X�`����

	  lsm_output.use();
	  glActiveTexture(GL_TEXTURE0);
	  glBindImageTexture(0, xyz_tex1, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	  glActiveTexture(GL_TEXTURE1);
	  glBindImageTexture(1, xy2_tex1, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	  glActiveTexture(GL_TEXTURE2);
	  glBindImageTexture(2, xyzx_tex1, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	  
	  // �v�Z���ʂ̏o�͐�

	  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	  glBufferData(GL_SHADER_STORAGE_BUFFER, count * sizeof(DataSet) , NULL, GL_STATIC_READ);
	  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo);

	  lsm_output.calculate(2, 2);
	  
	  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, 0);
	  //�����̎��s:���ʂ�float��SSBO�ɓ���
	  // �f�[�^�̊i�[��
	  std::vector<GLfloat> output(count * sizeof(DataSet));
	  // SSBO ����l�����o��
	  //   �EglMapbuffer() �Ŏ��o�����|�C���^�� glUnmapBuffer() ����Ɩ����ɂȂ�E
	  //   �EglUnmapBuffer() ����O�ɏ������I���邩�f�[�^���R�s�[���Ă����D
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

    // �@���x�N�g���̌v�Z
    normal.use();
    glUniform1i(0, 0);
    glActiveTexture(GL_TEXTURE0);
#if GENERATE_POSITION
    glBindTexture(GL_TEXTURE_2D, positionTexture[0]);
#else
    sensor.getPoint();
#endif
    const std::vector<GLuint> &normalTexture(normal.calculate());

    // ��ʏ���
    window.clear();

    // �`��p�̃V�F�[�_�v���O�����̎g�p�J�n
    simple.use();
    simple.loadMatrix(window.getMp(), window.getMw());
    simple.setLight(light);
    simple.setMaterial(material);

#if GENERATE_POSITION
    // ���_�ʒu�e�N�X�`��
    glUniform1i(0, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, positionTexture[0]);
#endif

    // ���_�@���e�N�X�`��
    glUniform1i(1, 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalTexture[0]);

    // �J���[�e�N�X�`��
    glUniform1i(2, 2);
    glActiveTexture(GL_TEXTURE2);
    sensor.getColor();

    // �}�`�`��
    mesh.draw();

    // �o�b�t�@�����ւ���
    window.swapBuffers();
  }
}
