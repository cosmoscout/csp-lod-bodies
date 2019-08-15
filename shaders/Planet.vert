#version 400 compatibility

// ==========================================================================
// include helper functions/declarations from VistaPlanet
$VP_TERRAIN_SHADER_UNIFORMS
$VP_TERRAIN_SHADER_FUNCTIONS

// uniforms
uniform vec3 uSunDir;

// outputs
// ==========================================================================

out VS_OUT
{
    vec2  texcoords;
    vec3  normal;
    vec3  position;
    vec3  planetCenter;
    vec2  lngLat;
    float height;
    vec2  vertexPosition;
} vsOut;

// main
// ==========================================================================

void main(void)
{
    // all in view space
    vsOut.position       = VP_getVertexPosition(VP_iPosition, $TERRAIN_PROJECTION_TYPE);
    gl_Position = VP_matProjection * vec4(vsOut.position, 1);

    if (!VP_shadowMapMode)
    {
        #if $LIGHTING_QUALITY > 2
            vsOut.normal         = VP_getVertexNormal(VP_iPosition, $TERRAIN_PROJECTION_TYPE);
        #elif $LIGHTING_QUALITY > 1
            vsOut.normal         = VP_getVertexNormalLow(vsOut.position, VP_iPosition, $TERRAIN_PROJECTION_TYPE);
        #endif
        vsOut.planetCenter   = (VP_matModelView * vec4(0,0,0,1)).xyz;
        vsOut.texcoords      = VP_getTexCoordIMG(VP_iPosition);
        vsOut.height         = VP_getVertexHeight(VP_iPosition);
        vsOut.lngLat         = VP_convertXY2lnglat(VP_getXY(VP_iPosition));
        vsOut.vertexPosition = VP_iPosition;
    }
}
