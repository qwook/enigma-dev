/** Copyright (C) 2013 Robert B. Colton
***
*** This file is a part of the ENIGMA Development Environment.
***
*** ENIGMA is free software: you can redistribute it and/or modify it under the
*** terms of the GNU General Public License as published by the Free Software
*** Foundation, version 3 of the license or any later version.
***
*** This application and its source code is distributed AS-IS, WITHOUT ANY
*** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
*** FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
*** details.
***
*** You should have received a copy of the GNU General Public License along
*** with this code. If not, see <http://www.gnu.org/licenses/>
**/
#include "Bridges/General/DX11Context.h"
#include "Graphics_Systems/General/GSd3d.h"
#include "Direct3D11Headers.h"
#include "Graphics_Systems/General/GStextures.h"

#include "../General/GSmodel.h"
#include "Universal_System/var4.h"
#include "Universal_System/roomsystem.h"

#define __GETR(x) ((x & 0x0000FF))/255.0
#define __GETG(x) ((x & 0x00FF00)>>8)/255.0
#define __GETB(x) ((x & 0xFF0000)>>16)/255.0

namespace enigma {
  bool d3dMode = false;
  bool d3dHidden = false;
  bool d3dZWriteEnable = true;
  int d3dCulling = 0;
}

namespace enigma_user
{

void d3d_depth_clear() {

}

void d3d_depth_clear_value(float value) {

}

void d3d_start()
{
	enigma::d3dMode = true;
	enigma::d3dCulling =  rs_none;
	d3d_set_hidden(false);
}

void d3d_end()
{
	enigma::d3dMode = false;
	enigma::d3dCulling = rs_none;
	d3d_set_hidden(false);
}

void d3d_set_hidden(bool enable)
{

    enigma::d3dHidden = enable;
}   

void d3d_set_zwriteenable(bool enable)
{
	enigma::d3dZWriteEnable = enable;

}

void d3d_set_lighting(bool enable)
{

}

void d3d_set_fog(bool enable, int color, double start, double end)
{
  d3d_set_fog_enabled(enable);
  d3d_set_fog_color(color);
  d3d_set_fog_start(start);
  d3d_set_fog_end(end);
  d3d_set_fog_hint(rs_nicest);
  d3d_set_fog_mode(rs_linear);
}

void d3d_set_fog_enabled(bool enable)
{

}

void d3d_set_fog_mode(int mode)
{

}

void d3d_set_fog_hint(int mode) {

}

void d3d_set_fog_color(int color)
{

}

void d3d_set_fog_start(double start)
{

}

void d3d_set_fog_end(double end)
{

}

void d3d_set_fog_density(double density)
{

}

void d3d_set_culling(int mode)
{
	enigma::d3dCulling = mode;

}

bool d3d_get_mode()
{
    return enigma::d3dMode;
}

bool d3d_get_hidden()
{
    return enigma::d3dHidden;
}

int d3d_get_culling() {
	return enigma::d3dCulling;
}

void d3d_set_fill_mode(int fill)
{

}

void d3d_set_line_width(float value) {

}

void d3d_set_point_size(float value) {

} 

void d3d_set_depth_operator(int mode) {

}

void d3d_set_perspective(bool enable)
{

}

void d3d_set_depth(double dep)
{

}

void d3d_set_shading(bool smooth)
{

}

}

namespace enigma {
  extern unsigned char currentcolor[4];
}

namespace enigma_user
{

void d3d_set_projection(gs_scalar xfrom, gs_scalar yfrom, gs_scalar zfrom,gs_scalar xto, gs_scalar yto, gs_scalar zto,gs_scalar xup, gs_scalar yup, gs_scalar zup)
{

}

void d3d_set_projection_ext(gs_scalar xfrom, gs_scalar yfrom, gs_scalar zfrom,gs_scalar xto, gs_scalar yto, gs_scalar zto,gs_scalar xup, gs_scalar yup, gs_scalar zup,double angle,double aspect,double znear,double zfar)
{

}

void d3d_set_projection_ortho(gs_scalar x, gs_scalar y, gs_scalar width, gs_scalar height, double angle)
{
	
}

void d3d_set_projection_perspective(gs_scalar x, gs_scalar y, gs_scalar width, gs_scalar height, double angle)
{

}


// ***** TRANSFORMATIONS BEGIN *****
void d3d_transform_set_identity()
{

}

void d3d_transform_add_translation(gs_scalar xt, gs_scalar yt, gs_scalar zt)
{
	
}

void d3d_transform_add_scaling(gs_scalar xs, gs_scalar ys, gs_scalar zs)
{

}

void d3d_transform_add_rotation_x(double angle)
{

}

void d3d_transform_add_rotation_y(double angle)
{

}

void d3d_transform_add_rotation_z(double angle)
{

}

void d3d_transform_add_rotation_axis(gs_scalar x, gs_scalar y, gs_scalar z, double angle)
{

}

void d3d_transform_set_translation(gs_scalar xt, gs_scalar yt, gs_scalar zt)
{

}

void d3d_transform_set_scaling(gs_scalar xs, gs_scalar ys, gs_scalar zs)
{

}

void d3d_transform_set_rotation_x(double angle)
{

}

void d3d_transform_set_rotation_y(double angle)
{

}

void d3d_transform_set_rotation_z(double angle)
{

}

void d3d_transform_set_rotation_axis(gs_scalar x, gs_scalar y, gs_scalar z, double angle)
{

}

}

#include <stack>
stack<bool> trans_stack;
int trans_stack_size = 0;

namespace enigma_user
{

bool d3d_transform_stack_push()
{

}

bool d3d_transform_stack_pop()
{

}

void d3d_transform_stack_clear()
{

}

bool d3d_transform_stack_empty()
{
    return (trans_stack_size == 0);
}

bool d3d_transform_stack_top()
{

}

bool d3d_transform_stack_disgard()
{
    if (trans_stack_size == 0) return false;
    trans_stack.push(0);
    trans_stack_size--;
    return true;
}

}

// ***** TRANSFORMATIONS END *****

// ***** LIGHTS BEGIN *****
#include <map>
#include <list>
#include "Universal_System/fileio.h"

struct posi { // Homogenous point.
    gs_scalar x;
    gs_scalar y;
    gs_scalar z;
    gs_scalar w;
    posi(gs_scalar x1, gs_scalar y1, gs_scalar z1, gs_scalar w1) : x(x1), y(y1), z(z1), w(w1){}
};

class d3d_lights
{
    map<int,int> light_ind;
    map<int,posi> ind_pos; // Internal index to position.

    public:
    d3d_lights() {}
    ~d3d_lights() {}

    void light_update_positions()
    {

    }

    bool light_define_direction(int id, gs_scalar dx, gs_scalar dy, gs_scalar dz, int col)
    {

    }

    bool light_define_point(int id, gs_scalar x, gs_scalar y, gs_scalar z, double range, int col)
    {

    }

    bool light_define_specularity(int id, int r, int g, int b, double a) 
    {

    }

    bool light_enable(int id)
    {

    }

    bool light_disable(int id)
    {
		
    }
} d3d_lighting;

namespace enigma_user
{

bool d3d_light_define_direction(int id, gs_scalar dx, gs_scalar dy, gs_scalar dz, int col)
{
    return d3d_lighting.light_define_direction(id, dx, dy, dz, col);
}

bool d3d_light_define_point(int id, gs_scalar x, gs_scalar y, gs_scalar z, double range, int col)
{
    return d3d_lighting.light_define_point(id, x, y, z, range, col);
}

bool d3d_light_define_specularity(int id, int r, int g, int b, double a) 
{
    return d3d_lighting.light_define_specularity(id, r, g, b, a);
}

void d3d_light_specularity(int facemode, int r, int g, int b, double a)
{
  float specular[4] = {r, g, b, a};
  
}

void d3d_light_shininess(int facemode, int shine)
{

}

void d3d_light_define_ambient(int col)
{

}

bool d3d_light_enable(int id, bool enable)
{
    return enable?d3d_lighting.light_enable(id):d3d_lighting.light_disable(id);
}

}

namespace enigma {
    void d3d_light_update_positions()
    {
        d3d_lighting.light_update_positions();
    }
}

// ***** LIGHTS END *****
