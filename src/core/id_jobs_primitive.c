/* Readable actual2AAEF0 and centeredzeroangle branch2C5F8C..2C605C.
 * capVu0Cos(0)=1/Sin(0)=0; no general rotation approximated. */
#include "id_jobs.h"
#include <math.h>
gh_id_error gh_id_texture_quad_prepare(const gh_id_state *manager,const gh_id_base *o,
 const gh_id_entry *e,gh_id_texture_quad *out){
 gh_id_texture_quad q;gh_id_error error;float width,height,cx,cy;
 if(!manager||!o||!e||!out)return GH_ID_ARGUMENT;
 if(e->kind!=0||e->b89!=0||e->draw_rotation!=0||e->f80!=1||e->texture.bytes)return GH_ID_UNSUPPORTED;
 error=gh_id_texture_open(manager,e->b8a,(uint32_t)e->b8b+1,&q.texture);if(error)return error;
 width=e->f68*e->draw_scale_x;height=e->f6c*e->draw_scale_y;cx=e->draw_x+1;cy=e->draw_y+1;
 if(!isfinite(width)||!isfinite(height)||!isfinite(cx)||!isfinite(cy)||width<=0||height<=0)return GH_ID_UNSUPPORTED;
 q.x0=cx-width*0.5f;q.x1=cx+width*0.5f;q.y0=cy-height*0.5f;q.y1=cy+height*0.5f;
 q.u0=e->f70[0];q.v0=e->f70[1];q.u1=e->f70[2];q.v1=e->f70[3];
 if(!isfinite(q.u0)||!isfinite(q.v0)||!isfinite(q.u1)||!isfinite(q.v1))return GH_ID_UNSUPPORTED;
 q.color=e->color;q.color_mode=o->mode==6?e->b8d:o->mode;q.font_mode=e->h8e;q.clip=0;
 *out=q;return GH_ID_OK;
}
