#version 420 core
// The per-pixel image containing the head pointers
layout (binding = 0, r32ui) uniform uimage2D head_pointer_image;
// Buffer containing linked lists of fragments
layout (binding = 1, rgba32ui) uniform uimageBuffer list_buffer;
// This is the maximum number of overlapping fragments allowed
#define MAX_FRAGMENTS 60

// Temporary array used for sorting fragments
uvec4 curfragment;

layout (location = 0) out vec4 color;
uniform vec2 resolution;


float near = 0.1; 
float far = 1000.0; 
float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * near * far) / (far + near - z * (far - near));	
}
void main(void)
{ 
    uint current_index;
    uint fragment_count = 0;
    int count=0;
    vec2 screenuv=gl_FragCoord.xy/resolution;
    current_index = imageLoad(head_pointer_image, ivec2(gl_FragCoord).xy).x;
    //curfragment=imageLoad(list_buffer, int(current_index));
    curfragment.z=floatBitsToUint(1.0);
    while (current_index != 0 && fragment_count < MAX_FRAGMENTS)
    {
        uvec4 fragment = imageLoad(list_buffer, int(current_index));
        float depth=uintBitsToFloat(fragment.z);
        current_index = fragment.x;
        if(depth>gl_FragCoord.z){
            float curdepth=uintBitsToFloat(curfragment.z);
            
            if(fragment.y==1)
               count++;
            if(fragment.y==2){
                if(depth<curdepth)
                    curfragment=fragment;
                count--;
            }
                
        
        }
        fragment_count++;
    }
    float depth = LinearizeDepth(gl_FragCoord.z) / far;
    vec4 modulator = unpackUnorm4x8(curfragment.w);
    if(count!=0)
        color = modulator;
    else
        discard;
}
