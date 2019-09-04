#version 430 core

#define MILLIMETER 0.001
#define DEPTH_SCALE (-65535.0 * MILLIMETER)
#define DEPTH_MAXIMUM (-10.0)

#define FAR  300
#define NEAR 20

// �X�P�[��
const vec2 scale = vec2(
  1.546592,
  1.222434
);

// �e�N�X�`��
layout (location = 0) uniform sampler2D depth;

layout (location = 4) uniform sampler2D depth2;

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
  return z == 0.0 ? DEPTH_MAXIMUM : z * DEPTH_SCALE;
}

void main(void)
{
  // �f�v�X�l�����o��
  //float z = s(texture(depth, texcoord).r);
  float z = s(texture(depth2, texcoord).r);

  vec2 place = (texcoord - 0.5) * scale * z;

  float zy = data.data_xyz[0] + place.x * data.data_xyz[1] + place.y * data.data_xyz[2];

  // �f�v�X�l����J�������W�l�����߂� // �f�v�X�f�[�^�̗L���͈͂����߂ĕ`��
  position = vec3(place, z);// * step(-FAR, -z *  DEPTH_SCALE) * step (NEAR, z * DEPTH_SCALE), z);
}
