/* Recovered actual2CA518 chain, MIT LICENSE.upstream. PrimarypatchedELF
 * evidence: analysis/sound-spatial-step-evidence.json. Nochip/emulatorruntime. */
#include "sound_spatial_step.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
_Static_assert(offsetof(gh_boot_work,system_flags)==0x58c,"747A2C Work58C");
static uint32_t word(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static unsigned half(const uint8_t *p){return (unsigned)p[0]|(unsigned)p[1]<<8;}
static float number(uint32_t n){float f;memcpy(&f,&n,4);return f;}
static uint32_t bits(float f){uint32_t n;memcpy(&n,&f,4);return n;}
static float add(float a,float b){volatile float r=a+b;return r;}
static float sub(float a,float b){volatile float r=a-b;return r;}
static float mul(float a,float b){volatile float r=a*b;return r;}
static float divide(float a,float b){volatile float r=a/b;return r;}
static gh_sound_scene_status fail(uint32_t*f,uint32_t pc,gh_sound_scene_status e){if(f)*f=pc;return e;}
static void cross(float*out,const float*a,const float*b){out[0]=sub(mul(a[1],b[2]),mul(a[2],b[1]));out[1]=sub(mul(a[2],b[0]),mul(a[0],b[2]));out[2]=sub(mul(a[0],b[1]),mul(a[1],b[0]));}
static int normalize(float*v){float n=add(add(mul(v[0],v[0]),mul(v[1],v[1])),mul(v[2],v[2]));float length=sqrtf(n);if(!(length>0)||!isfinite(length))return 0;float q=divide(1,length);for(unsigned i=0;i<3;i++)v[i]=mul(v[i],q);return 1;}
gh_sound_scene_status gh_sound_listener_update_002cc150(gh_sound_control*s,const gh_boot_work*w,const gh_game_init_state*init,const gh_game_pl_camera*camera,const gh_title_player_view*player,gh_camera_numeric_policy policy,uint32_t*f){
 if(f)*f=0;if(!s||!s->initialized||!w||!init)return GH_SOUND_SCENE_ARGUMENT;
 uint32_t flags=word((const uint8_t*)w+0x5e4);s->listener_mode110=(flags&0x20080040u)?0:1;
 int effective=player?s->listener_mode110:0;
 if(!effective){
  if(!camera||!camera->constructed||!camera->identity||init->active_camera!=camera->identity)return fail(f,0x2cc150,GH_SOUND_SCENE_UNSUPPORTED);
  for(unsigned i=0;i<4;i++)s->listener_position_c0[i]=word(camera->fields+0x210+i*4);
  for(unsigned i=0;i<16;i++)s->listener_matrix_d0[i]=word(camera->fields+0x140+i*4);
  return GH_SOUND_SCENE_OK;
 }
 if(!player->bytes||player->size<GH_TITLE_PLAYER_BYTES||player->alias_f0!=player->bytes+0xb0)return GH_SOUND_SCENE_ARGUMENT;
 if(policy!=GH_CAMERA_NATIVE_BINARY32_REFERENCE)return fail(f,0x30a2b0,GH_SOUND_SCENE_UNSUPPORTED);
 fenv_t env;if(feholdexcept(&env))return fail(f,0x2cc150,GH_SOUND_SCENE_UNSUPPORTED);if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return fail(f,0x2cc150,GH_SOUND_SCENE_UNSUPPORTED);}
 const float offset[]={0,1.5f,-.1f,1};float position[4],aim[3],z[3],x[3],y[3],matrix[16]={0};int good=1;
 for(unsigned r=0;r<4;r++){float v=mul(number(word(player->bytes+0x80+r*4)),offset[0]);for(unsigned k=1;k<4;k++)v=add(v,mul(number(word(player->bytes+0x80+(k*4+r)*4)),offset[k]));position[r]=v;if(!isfinite(v))good=0;}
 for(unsigned i=0;i<3;i++){aim[i]=number(word(player->alias_f0+i*4));if(i==1)aim[i]=add(aim[i],1.5f);z[i]=sub(aim[i],position[i]);if(!isfinite(aim[i]))good=0;}
 const float up[]={0,1,0};cross(x,up,z);if(!normalize(x)||!normalize(z))good=0;cross(y,z,x);
 if(good){matrix[15]=1;for(unsigned r=0;r<3;r++){matrix[r*4]=x[r];matrix[r*4+1]=y[r];matrix[r*4+2]=z[r];}
  for(unsigned r=0;r<3;r++)matrix[12+r]=-add(add(mul(matrix[r],position[0]),mul(matrix[4+r],position[1])),mul(matrix[8+r],position[2]));
  for(unsigned i=0;i<16;i++)if(!isfinite(matrix[i]))good=0;
 }
 if(fesetenv(&env))return fail(f,0x2cc150,GH_SOUND_SCENE_UNSUPPORTED);if(!good)return fail(f,0x30a8c8,GH_SOUND_SCENE_UNSUPPORTED);
 for(unsigned i=0;i<4;i++)s->listener_position_c0[i]=bits(position[i]);for(unsigned i=0;i<16;i++)s->listener_matrix_d0[i]=bits(matrix[i]);return GH_SOUND_SCENE_OK;
}
/* Original 2C8CF8 dispatches TS type 2 to the sphere check and type 3
 * to the bounded cylinder. Both require a strict radial intersection. */
static gh_sound_scene_status ts_proximity(const uint8_t *record,const float player[3],
    gh_sound_ts_row *row,uint32_t *frontier)
{
    const unsigned kind=record[0x0d];
    if((kind!=2&&kind!=3)||!record[0x0e])
        return fail(frontier,0x2c8cf8,GH_SOUND_SCENE_UNSUPPORTED);

    float delta[3];
    for(unsigned axis=0;axis<3;++axis){
        const float center=number(word(record+0x30+axis*4));
        if(!isfinite(center)||!isfinite(player[axis]))
            return fail(frontier,0x2c8cf8,GH_SOUND_SCENE_ASSET);
        delta[axis]=sub(center,player[axis]);
    }
    float square=mul(delta[0],delta[0]);
    if(kind==2)square=add(square,mul(delta[1],delta[1]));
    square=add(square,mul(delta[2],delta[2]));
    const float distance=sqrtf(square);
    const float limiting_radius=add((float)half(record+0x24),1.5f);
    const float limiting_height=add((float)half(record+0x26),1.5f);
    const int inside=distance<limiting_radius&&(kind==2||fabsf(delta[1])<=limiting_height);
    row->flags0c&=(uint8_t)~2u;
    if(inside){
        row->flags0c|=2u;
        row->volume10=(uint16_t)(uint32_t)distance;
    }
    return GH_SOUND_SCENE_OK;
}

static gh_sound_scene_status ts_voice_error(const gh_se_voice_engine *voices,
    uint32_t entry,uint32_t *frontier,gh_voice_status status)
{
    const uint32_t child=gh_se_voice_frontier(voices);
    return fail(frontier,child?child:entry,
        status==GH_VOICE_UNSUPPORTED?GH_SOUND_SCENE_UNSUPPORTED:GH_SOUND_SCENE_ASSET);
}

static gh_sound_scene_status update_ts_row(gh_sound_ts_row *row,const uint8_t *record,
    gh_se_voice_engine *voices,const gh_se_attached_environment *environment,int suspend,
    const float position[3],uint32_t *frontier)
{
    const int sound_effect=half(record+0x2a)==0;
    if(suspend||!(row->flags0c&1u)){
        if(row->voice14){
            if(sound_effect){
                if(!voices)return fail(frontier,0x2cbc20,GH_SOUND_SCENE_UNSUPPORTED);
                int accepted;
                const gh_voice_status status=gh_se_voice_request_stop_00375050(voices,row->voice14,10,&accepted);
                if(status)return ts_voice_error(voices,0x2cbc20,frontier,status);
            }
            row->state13=4;
            row->voice14=0;
        }
        return GH_SOUND_SCENE_OK;
    }
    if(row->disabled12)return GH_SOUND_SCENE_OK;
    const gh_sound_scene_status proximity=ts_proximity(record,position,row,frontier);
    if(proximity)return proximity;
    float source[4];
    memcpy(source,record+0x30,sizeof(source));
    switch(row->state13){
    case 0:
        if(row->flags0c&2u)row->state13=1;
        break;
    case 1:
        if(sound_effect){
            if(!voices)return fail(frontier,0x2cb968,GH_SOUND_SCENE_UNSUPPORTED);
            const gh_voice_status status=gh_se_voice_request_ts_002cb968(voices,
                (int16_t)half(record+0x2c),(int16_t)half(record+0x2e),source,environment,&row->voice14);
            if(status)return ts_voice_error(voices,0x2cb968,frontier,status);
            if(!row->voice14){row->flags0c=0;break;}
        }
        row->state13=2;
        break;
    case 2:
        if(!(row->flags0c&2u))row->state13=3;
        else if(sound_effect){
            if(!voices)return fail(frontier,0x2cbf48,GH_SOUND_SCENE_UNSUPPORTED);
            int found;
            source[3]=1.0f;
            const gh_voice_status status=gh_se_voice_update_ts_002cbf48(voices,row->voice14,source,row->volume10,&found);
            if(status)return ts_voice_error(voices,0x2cbf48,frontier,status);
            if(!found)row->state13=4;
        }
        break;
    case 3:
        if(sound_effect&&row->voice14){
            if(!voices)return fail(frontier,0x2cbc20,GH_SOUND_SCENE_UNSUPPORTED);
            int accepted;
            const gh_voice_status status=gh_se_voice_request_stop_00375050(voices,row->voice14,30,&accepted);
            if(status)return ts_voice_error(voices,0x2cbc20,frontier,status);
        }
        /* Original state 3 falls through to state 4's reset. */
        row->state13=0;
        row->voice14=0;
        break;
    case 4:
        row->state13=0;
        row->voice14=0;
        break;
    }
    return GH_SOUND_SCENE_OK;
}

static gh_sound_scene_status update_ts_rows(gh_sound_scene *scene,gh_se_voice_engine *voices,
    const gh_boot_work *work,const gh_title_player_view *player,uint32_t *frontier)
{
    if(!scene->ts_count||scene->ts_count>GH_SOUND_TS_ROWS||!player->alias_f0||
       scene->control->ts_head2c!=&scene->ts_rows[scene->ts_count-1])
        return fail(frontier,0x2c8ad8,GH_SOUND_SCENE_UNSUPPORTED);
    if(voices&&!gh_se_voice_uses_registry(voices,scene->registry))return GH_SOUND_SCENE_ARGUMENT;

    const uint8_t *fields=(const uint8_t *)work;
    const int suspend=(word(fields+0x5e4)&0x20000880u)||
        (word(fields+0x584)&8u)||(work->system_flags&0x01000000u);
    float position[3];
    memcpy(position,player->alias_f0,sizeof(position));
    if(!scene->ts_update_remaining)scene->ts_update_remaining=scene->ts_count;
    while(scene->ts_update_remaining){
        gh_sound_ts_row *row=&scene->ts_rows[scene->ts_update_remaining-1];
        if(row->offset>scene->ts.size||scene->ts.size-row->offset<0x40)
            return fail(frontier,0x2c8ad8,GH_SOUND_SCENE_ASSET);
        gh_sound_ts_row next=*row;
        const gh_sound_scene_status status=update_ts_row(&next,scene->ts.bytes+row->offset,
            voices,&scene->ts_environment,suspend,position,frontier);
        if(status)return status;
        *row=next;
        --scene->ts_update_remaining;
    }
    return GH_SOUND_SCENE_OK;
}

gh_sound_scene_status gh_sound_spatial_step_002ca518(gh_sound_scene*s,gh_se_voice_engine *voices,const gh_boot_work*w,const gh_game_init_state*init,const gh_game_pl_camera*camera,const gh_title_player_view*player,gh_camera_numeric_policy policy,uint32_t*f){
 if(f)*f=0;if(!s||!s->startup_bound||!s->control||!s->control->initialized||!w)return GH_SOUND_SCENE_ARGUMENT;
 gh_sound_scene_status e=gh_sound_listener_update_002cc150(s->control,w,init,camera,player,policy,f);if(e)return e;
 gh_sound_control*c=s->control;
 c->previous_mask54=c->event_mask50;c->flags_ac=c->flags_b0;c->event_mask50=c->event_mask58;
 s->ratios74[0]=s->fade_ratio_bits;s->ratios88[0]=s->default_ratio_bits;
 if(s->bmh.bytes)return fail(f,0x2d22f0,GH_SOUND_SCENE_UNSUPPORTED);
 if(s->ts_count){
  gh_se_attached_environment *environment=&s->ts_environment;
  environment->system_flags_5e4=word((const uint8_t *)w+0x5e4);
  environment->game_flags_5d8=word((const uint8_t *)w+0x5d8);
  environment->debug58c=(int32_t)w->system_flags;
  environment->gain64_bits=s->ratios60[1];
  environment->se_level_q8=0x5a00;
  environment->output_mode=c->output_mode;
  environment->listener_mode110=c->listener_mode110;
  memcpy(environment->listener_position_c0,c->listener_position_c0,sizeof(environment->listener_position_c0));
  memcpy(environment->listener_matrix_d0,c->listener_matrix_d0,sizeof(environment->listener_matrix_d0));
 }
 if(!(w->system_flags&0x80000000u)&&s->ts.bytes&&c->ts_head2c&&player){
  fenv_t env;
  if(feholdexcept(&env))return fail(f,0x2c8cf8,GH_SOUND_SCENE_UNSUPPORTED);
  if(fesetround(FE_TOWARDZERO)){
   (void)fesetenv(&env);
   return fail(f,0x2c8cf8,GH_SOUND_SCENE_UNSUPPORTED);
  }
  e=update_ts_rows(s,voices,w,player,f);
  if(fesetenv(&env))return fail(f,0x2c8cf8,GH_SOUND_SCENE_UNSUPPORTED);
  if(e)return e;
 }
 /* Actual2CB200 isJRra/NOP. Originvoices areNOTiteratedorflushed by2CA518;
  * theirONErealengine isadvanced at the separateoriginalsoundframe boundary. */
 return GH_SOUND_SCENE_OK;
}
