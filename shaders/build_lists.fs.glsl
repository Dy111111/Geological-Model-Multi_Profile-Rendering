#version 420 core

layout (early_fragment_tests) in;

layout (binding = 0, r32ui) uniform uimage2D head_pointer_image;
layout (binding = 1, rgba32ui) uniform writeonly uimageBuffer list_buffer;

layout (binding = 0, offset = 0) uniform atomic_uint list_counter;

layout (location = 0) out vec4 color;

struct Light {
    vec3 position;  
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
	
    float constant;
    float linear;
    float quadratic;
};
 //从Mtl中读取的数据
//Material
in vec4 Ambient;
in vec4 Diffuse;
in vec4 Specular;
 
uniform vec3 viewPos;
uniform Light light;
 
uniform float shininess;

uniform uint side;

in vec3 frag_position;
in vec3 frag_normal;

void main(void)
{
    uint index;
    uint old_head;
    uvec4 item;
    vec4 frag_color;

    index = atomicCounterIncrement(list_counter);

    old_head = imageAtomicExchange(head_pointer_image, ivec2(gl_FragCoord.xy), uint(index));


    vec4 modulator = Diffuse;
    //vec4 additive_component = mix(modulator, vec4(1.0), 0.6) * vec4(pow(clamp(NdotH, 0.0, 1.0), 26.0)) * 0.7;

    item.x = old_head;
    item.y = side;
    item.z = floatBitsToUint(gl_FragCoord.z);
    item.w = packUnorm4x8(modulator);

    imageStore(list_buffer, int(index), item);

    frag_color = modulator;

    color = frag_color;
}
