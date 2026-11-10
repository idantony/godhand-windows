#ifndef GH_GAME_TERRAIN_CLOSEST_H
#define GH_GAME_TERRAIN_CLOSEST_H
#ifdef __cplusplus
extern "C" {
#endif
/* Original135348 region classifier and staged binary32 closest point.
 * Caller owns nearest-rounding/FEnv scope; finite nondegenerate triangles. */
int gh_terrain_closest_00135348(const float point[3],const float a[3],const float b[3],const float c[3],float out[3]);
#ifdef __cplusplus
}
#endif
#endif
