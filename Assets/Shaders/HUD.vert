#version 410 core

layout (location = 0) in vec2 inVertexPositionModelSpace;
layout (location = 1) in vec2 inTexCoord;

layout (std140) uniform ub_common
{
    vec2 u_screenSize;
    float u_time;
};
uniform vec2 u_positionScreenSpace;
uniform vec2 u_sizeScreenSpace;

varying vec2 f_texcoord;

out vec2 texCoord;

void main()
{
    vec2 positionScreenSpace = u_positionScreenSpace;
    positionScreenSpace = round(positionScreenSpace);

    float ndcX = (((positionScreenSpace.x / u_screenSize.x) * 2.0) - 1.0);
    float ndcY = (((positionScreenSpace.y / u_screenSize.y) * 2.0) - 1.0);
    float width = ((u_sizeScreenSpace.x / u_screenSize.x)) * 2.0;
    float height = ((u_sizeScreenSpace.y / u_screenSize.y)) * 2.0;
    gl_Position = vec4(ndcX + (inVertexPositionModelSpace.x * width), -(ndcY + (inVertexPositionModelSpace.y * height)), 0.0, 1.0);
    texCoord = inTexCoord;
    f_texcoord = inTexCoord;
}
