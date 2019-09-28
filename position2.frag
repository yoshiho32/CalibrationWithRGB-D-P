#version 430 core

#define MILLIMETER 0.001
#define DEPTH_SCALE (-65535.0 * MILLIMETER)
#define DEPTH_MAXIMUM (-10.0)

// �X�P�[��
const vec2 scale = vec2(
  1.546592,
  1.222434
);

// �e�N�X�`��
layout (rgba32f, binding = 0) uniform readonly image2D depth;

// �e�N�X�`�����W
in vec2 texcoord;

// �t���[���o�b�t�@�ɏo�͂���f�[�^
layout (location = 0) out vec3 position;

struct DataSet{
  vec4 data_xyz;
  vec4 data_xy2;
  vec4 data_xyzx;
 };

// �v�Z���ʂ̏o��
layout (std430, binding = 7) buffer Input{
  DataSet data;
};

// �f�v�X�l���X�P�[�����O����
float s(in float z)
{
  return z == 0.0 ? DEPTH_MAXIMUM : z;
}

void main(void)
{
  // �J���[�̃e�N�X�`�����W(0,0)-(1,1)��image2D�̃e�N�X�`�����W(0,0)-(512,424)�ɒu��������(globalInvocationID���Ȃ��̂�)
  ivec2 itexcoord = ivec2(texcoord * ivec2(512, 424));
  // �f�v�X�f�[�^�̎擾
  vec4 pos = imageLoad(depth, itexcoord);

  // �J�������W�ł̈ʒu
  vec2 tex = texcoord - 0.5;

  // �ŏ�2�敽�ʂɂ�����z�̎�
  // z = a + bx + cy 
  float zy =  data.data_xyz[0] + tex.x * data.data_xyz[1] + tex.y * data.data_xyz[2];////(data.data_xyz[0] * tex.x + data.data_xyz[1] * tex.y + data.data_xyz[3]);
  
  // O(0,0,0)��ʂ�Ȃ炱��ł����A���ʂ̎��B�����O��ʂ邩�킩��Ȃ��̂Ń_��
  // z = -(a'x + b'y) / c'
  //float zy = -( data.data_xyz[0] * tex.x + data.data_xyz[1] * tex.y ) / data.data_xyz[2];

  // �f�v�X�l����J�������W�l�����߂� 
  position = pos.z==0 ? vec3(0,0,0) : vec3(-tex*scale, zy);

}

