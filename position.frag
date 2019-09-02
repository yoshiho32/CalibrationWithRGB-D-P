#version 430 core

#define MILLIMETER 0.001
#define DEPTH_SCALE (-65535.0 * MILLIMETER)
#define DEPTH_MAXIMUM (-10.0)

#define FAR  300
#define NEAR 20

// スケール
const vec2 scale = vec2(
  1.546592,
  1.222434
);

// テクスチャ
layout (location = 0) uniform sampler2D depth;

// テクスチャ座標
in vec2 texcoord;

// フレームバッファに出力するデータ
layout (location = 0) out vec3 position;

// デプス値をスケーリングする
float s(in float z)
{
  return z == 0.0 ? DEPTH_MAXIMUM : z * DEPTH_SCALE;
}

void main(void)
{
  // デプス値を取り出す
  float z = s(texture(depth, texcoord).r);

  // デプス値からカメラ座標値を求める
  position = vec3((texcoord - 0.5) * scale * z * step(-FAR, -z *  DEPTH_SCALE) * step (NEAR, z * DEPTH_SCALE), z);
}
