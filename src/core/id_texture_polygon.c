#include "id_texture_polygon.h"
#include "id_layout_rotation.h"
#include "../assets/tim2.h"
#include <math.h>
#include <fenv.h>
#include <string.h>
static uint32_t word(const unsigned char*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static float add(float a,float b){volatile float v=a+b;return v;}
static float mul(float a,float b){volatile float v=a*b;return v;}
gh_id_error gh_id_polygon_texture_open(const gh_id_state*m,uint32_t slot,uint32_t n,gh_id_texture*out){gh_id_error error=gh_id_texture_open(m,slot,n,out);const gh_bundle_entry*t;size_t end;uint32_t count,off,i;gh_tim2_picture p;gh_id_texture view;
 if(error!=GH_ID_UNSUPPORTED)return error;
 if(!m||!out||slot>=32||!n)return GH_ID_ARGUMENT;t=&m->pair[slot].texture_data;if(!t->bytes||t->size<4)return GH_ID_BOUNDS;
 count=word(t->bytes);if(count>(t->size-4)/4||n>count)return GH_ID_BOUNDS;off=word(t->bytes+(size_t)n*4);if(off<4+(size_t)count*4||off>=t->size)return GH_ID_BOUNDS;
 end=t->size;for(i=1;i<=count;i++){uint32_t x=word(t->bytes+(size_t)i*4);if(x&&(x<4+(size_t)count*4||x>t->size))return GH_ID_BOUNDS;if(x>off&&x<end)end=x;}
 if(gh_tim2_first(t->bytes+off,end-off,&p)!=GH_TIM2_OK)return GH_ID_UNSUPPORTED;
 if(p.width!=64||p.height!=64||p.image_type!=4||p.clut_type!=3||p.colors!=16||((p.tex0>>20)&63)!=20||((p.tex0>>26)&15)!=6||((p.tex0>>30)&15)!=6||((p.tex0>>35)&3)||((p.tex0>>51)&63)||((p.tex0>>56)&31))return GH_ID_UNSUPPORTED;
 view.source=(gh_id_view){t->bytes+off,end-off,off};view.width=64;view.height=64;view.tex0=p.tex0;*out=view;return GH_ID_OK;
}
gh_id_error gh_id_polygon_texture_rgba(const gh_id_texture*t,void*out,size_t bytes){gh_tim2_picture p;gh_tim2_error e;if(!t||!out)return GH_ID_ARGUMENT;
 if(t->width!=64||t->height!=64)return gh_id_texture_rgba(t,out,bytes);
 e=gh_tim2_first(t->source.bytes,t->source.size,&p);if(e)return GH_ID_UNSUPPORTED;if(p.width!=64||p.height!=64||p.tex0!=t->tex0)return GH_ID_UNSUPPORTED;
 e=gh_tim2_rgba4(&p,out,bytes);return e==GH_TIM2_OK?GH_ID_OK:e==GH_TIM2_BOUNDS?GH_ID_BOUNDS:GH_ID_UNSUPPORTED;
}
gh_id_error gh_id_texture_polygon_prepare(const gh_id_state*m,const gh_id_base*b,const gh_id_entry*e,gh_id_texture_polygon*out){gh_id_texture_polygon q;gh_id_error error;fenv_t saved;float s,c,w,h,cx,cy,wc,negative_ws,hs,hc;unsigned i;
 if(!m||!b||!e||!out)return GH_ID_ARGUMENT;if(e->kind|| (e->b89!=0&&e->b89!=3)||e->f80!=1||e->texture.bytes)return GH_ID_UNSUPPORTED;
 q.color_mode=b->mode==6?e->b8d:b->mode;q.font_mode=e->h8e;if(q.color_mode!=6||q.font_mode!=5)return GH_ID_UNSUPPORTED;
 error=gh_id_polygon_texture_open(m,e->b8a,(uint32_t)e->b8b+1,&q.texture);if(error)return error;
 if(fegetenv(&saved))return GH_ID_UNSUPPORTED;if(fesetround(FE_TONEAREST)){fesetenv(&saved);return GH_ID_UNSUPPORTED;}
 error=gh_id_sincos_reference(e->draw_rotation,&s,&c);if(error){fesetenv(&saved);return error;}
 w=mul(e->f68,e->draw_scale_x);h=mul(e->f6c,e->draw_scale_y);cx=add(e->draw_x,1);cy=add(e->draw_y,1);wc=mul(w,c);negative_ws=mul(-w,s);hs=mul(h,s);hc=mul(h,c);
 if(e->b89==0){float left=add(cx,-mul(wc,.5f)),right=add(cx,mul(wc,.5f)),up=add(cy,mul(negative_ws,.5f)),down=add(cy,-mul(negative_ws,.5f));hs=mul(hs,.5f);hc=mul(hc,.5f);
  q.x[0]=add(left,hs);q.y[0]=add(up,-hc);q.x[1]=add(right,hs);q.y[1]=add(down,-hc);q.x[2]=add(left,-hs);q.y[2]=add(up,hc);q.x[3]=add(right,-hs);q.y[3]=add(down,hc);
 }else{q.x[0]=add(cx,hs);q.y[0]=add(cy,-hc);q.x[1]=add(add(cx,wc),hs);q.y[1]=add(add(cy,-negative_ws),-hc);q.x[2]=cx;q.y[2]=cy;q.x[3]=add(cx,wc);q.y[3]=add(cy,-negative_ws);}
 q.u[0]=q.u[2]=e->f70[0];q.u[1]=q.u[3]=e->f70[2];q.v[0]=q.v[1]=e->f70[1];q.v[2]=q.v[3]=e->f70[3];q.color=e->color;q.alignment=e->b89;q.clip=0;
 if(!isfinite(w)||!isfinite(h)||w<=0||h<=0)error=GH_ID_UNSUPPORTED;
 for(i=0;i<4;i++)if(!isfinite(q.x[i])||!isfinite(q.y[i])||!isfinite(q.u[i])||!isfinite(q.v[i]))error=GH_ID_UNSUPPORTED;
 if(fesetenv(&saved))return GH_ID_UNSUPPORTED;if(error)return error;*out=q;return GH_ID_OK;
}
