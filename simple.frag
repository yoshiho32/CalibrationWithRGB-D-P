#version 430 core

// テクスチャ
layout (location = 2) uniform sampler2D color;      // カラーのテクスチャ
layout (rgba32f, binding = 3) uniform readonly image2D depth; // オブジェクトの座標

// ラスタライザから受け取る頂点属性の補間値
in vec4 idiff;                                      // 拡散反射光強度
in vec4 ispec;                                      // 鏡面反射光強度
in vec2 texcoord;                                   // テクスチャ座標

// フレームバッファに出力するデータ
layout (location = 0) out vec4 fc;                  // フラグメントの色


void main(void)
{
  // デプスが有効がどうかでテクスチャの色を変える
  ivec2 tex = ivec2(texcoord.x * 512, texcoord.y * 424);

  vec4 p = imageLoad(depth, tex);

  // テクスチャマッピングを行って陰影を求める
  //fc = idiff + ispec;
  fc = texture(color , texcoord)*p.w;
  //fc = vec4(1 - texcoord.x * 256 / 512, 1 - texcoord.y * 256 / 424, 0, 1) * step(ispec.x, 0.9);//texture(color, texcoord);// * idiff + ispec;
}

