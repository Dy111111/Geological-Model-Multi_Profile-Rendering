#version 430 core

struct NodeType {
  uint color;
  float depth;
  uint next;
};
// The per-pixel image containing the head pointers
layout (binding = 0, r32ui) uniform uimage2D head_pointer_image;
// Buffer containing linked lists of fragments
layout( binding = 0, std430 ) buffer linkedLists {
  NodeType nodes[];
};
// This is the maximum number of overlapping fragments allowed
#define MAX_FRAGMENTS 75

// Temporary array used for sorting fragments


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
    NodeType curfragment;
    uint current_index;
    uint fragment_count = 0;
    int count=0;
    current_index = imageLoad(head_pointer_image, ivec2(gl_FragCoord).xy).x;
    curfragment.depth=1.0;
    //curfragment=imageLoad(list_buffer, int(current_index));
    
    while (current_index != 0 && fragment_count < MAX_FRAGMENTS)
    {
        NodeType fragment = nodes[current_index];
        float depth=fragment.depth;
        current_index = fragment.next;
        if(abs(depth)>gl_FragCoord.z){
            float curdepth=abs(curfragment.depth);
            
            if(depth<0)
               count++;
            if(depth>0){
                if(abs(depth)<curdepth)
                    curfragment=fragment;
                count--;
            }
                
        
        }
        fragment_count++;
    }
    float depth = LinearizeDepth(gl_FragCoord.z) / far;
    vec4 modulator = unpackUnorm4x8(curfragment.color);
    if(count!=0)
        color = modulator;
    else
        discard;
}
