/** Copyright (C) 2008-2013 Josh Ventura, Harijs Grinbergs, Robert B. Colton
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

#include <string>
#include <cstdio>
#include "../General/OpenGLHeaders.h"
#include "../General/GSbackground.h"
#include "../General/GSscreen.h"
#include "../General/GSd3d.h"
#include "../General/GStextures.h"
#include "../General/GScolors.h"
#include "Bridges/General/GL3Context.h"

using namespace std;

#include "Universal_System/image_formats.h"
#include "Universal_System/var4.h"
#include "Universal_System/estring.h"

#define __GETR(x) (((unsigned int)x & 0x0000FF))
#define __GETG(x) (((unsigned int)x & 0x00FF00) >> 8)
#define __GETB(x) (((unsigned int)x & 0xFF0000) >> 16)

#include "Universal_System/roomsystem.h"
#include "Universal_System/instance_system.h"
#include "Universal_System/graphics_object.h"
#include "Universal_System/depth_draw.h"
#include "Platforms/platforms_mandatory.h"
#include "Graphics_Systems/graphics_mandatory.h"
#include <limits>

//Fuck whoever did this to the spec
#ifndef GL_BGR
  #define GL_BGR 0x80E0
#endif

using namespace enigma;
namespace enigma_user {
  extern int window_get_width();
  extern int window_get_height();
}

static inline void draw_back()
{
    using enigma_user::background_x;
    using enigma_user::background_y;
    using enigma_user::background_visible;
    using enigma_user::background_alpha;
    using enigma_user::background_xscale;
    using enigma_user::background_yscale;
    using enigma_user::background_htiled;
    using enigma_user::background_vtiled;
	using enigma_user::background_hspeed;
	using enigma_user::background_vspeed;
    using enigma_user::background_index;
    using enigma_user::background_coloring;
    using enigma_user::draw_background_tiled_ext;
    using enigma_user::draw_background_ext;
    // Draw the rooms backgrounds
    for (int back_current = 0; back_current < 8; back_current++) {
        if (background_visible[back_current] == 1) {
			//NOTE: This has been double checked with Game Maker 8.1 to work exactly the same, the background_x/y is modified just as object locals are
			//and also just as one would assume the system to work.
			//TODO: This should probably be moved to room system.
			background_x[back_current] += background_hspeed[back_current];
			background_y[back_current] += background_vspeed[back_current];
            if (background_htiled[back_current] || background_vtiled[back_current]) {
                draw_background_tiled_ext(background_index[back_current], background_x[back_current], background_y[back_current], background_xscale[back_current], 
					background_xscale[back_current], background_coloring[back_current], background_alpha[back_current], background_htiled[back_current], background_vtiled[back_current]);
            } else {
                draw_background_ext(background_index[back_current], background_x[back_current], background_y[back_current], background_xscale[back_current], background_xscale[back_current], 0, background_coloring[back_current], background_alpha[back_current]);
			}
		}
    }
}

namespace enigma
{
	extern GLuint msaa_fbo;
    extern bool d3dMode;
	extern bool d3dZWriteEnable;
	extern int d3dCulling;
    extern std::map<int,roomstruct*> roomdata;
    particles_implementation* particles_impl;
    void set_particles_implementation(particles_implementation* part_impl)
    {
        particles_impl = part_impl;
    }

	unsigned gui_width;
	unsigned gui_height;
	unsigned int bound_framebuffer = 0; //Shows the bound framebuffer, so glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &fbo); don't need to be called (they are very slow)
}

namespace enigma_user
{

void screen_redraw()
{
	oglmgr->BeginScene();
	// Clean up any textures that ENIGMA may still think are binded but actually are not
	texture_reset();
	d3d_set_zwriteenable(true);
    if (!view_enabled)
    {
        glViewport(0, 0, window_get_region_width_scaled(), window_get_region_height_scaled());
        glLoadIdentity();
        glScalef(1, (bound_framebuffer==0?-1:1), 1);
        glOrtho(0, room_width, 0, room_height, 0, 1);
        glGetDoublev(GL_MODELVIEW_MATRIX,projection_matrix);
        glMultMatrixd(transformation_matrix);

        int clear_bits = 0;
        if (background_showcolor)
        {
            int clearcolor = ((int)background_color) & 0x00FFFFFF;
            glClearColor(__GETR(clearcolor) / 255.0, __GETG(clearcolor) / 255.0, __GETB(clearcolor) / 255.0, 1);
            clear_bits |= GL_COLOR_BUFFER_BIT;
        }

        // Clear the depth buffer if 3d mode is on at the beginning of the draw step.
        if (enigma::d3dMode)
            clear_bits |= GL_DEPTH_BUFFER_BIT;

        if (clear_bits)
            glClear(clear_bits);

        draw_back();

        // Apply and clear stored depth changes.
        for (map<int,pair<double,double> >::iterator it = id_to_currentnextdepth.begin(); it != id_to_currentnextdepth.end(); it++)
        {
            enigma::object_graphics* inst_depth = (enigma::object_graphics*)enigma::fetch_instance_by_id((*it).first);
            if (inst_depth != NULL) {
                drawing_depths[(*it).second.first].draw_events->unlink(inst_depth->depth.myiter);
                inst_iter* mynewiter = drawing_depths[(*it).second.second].draw_events->add_inst(inst_depth->depth.myiter->inst);
                if (instance_event_iterator == inst_depth->depth.myiter) {
                    instance_event_iterator = inst_depth->depth.myiter->prev;
                }
                inst_depth->depth.myiter = mynewiter;
            }
        }
        id_to_currentnextdepth.clear();

        if (enigma::particles_impl != NULL) {
            const double high = numeric_limits<double>::max();
            const double low = drawing_depths.rbegin() != drawing_depths.rend() ? drawing_depths.rbegin()->first : -numeric_limits<double>::max();
            (enigma::particles_impl->draw_particlesystems)(high, low);
        }
        bool stop_loop = false;

        for (enigma::diter dit = drawing_depths.rbegin(); dit != drawing_depths.rend(); dit++)
        {
            texture_reset();
            if (dit->second.tiles.size())
                glCallList(drawing_depths[dit->second.tiles[0].depth].tilelist);

            enigma::inst_iter* push_it = enigma::instance_event_iterator;
            //loop instances
            for (enigma::instance_event_iterator = dit->second.draw_events->next; enigma::instance_event_iterator != NULL; enigma::instance_event_iterator = enigma::instance_event_iterator->next) {
                enigma::instance_event_iterator->inst->myevent_draw();
                if (enigma::room_switching_id != -1) {
                    stop_loop = true;
                    break;
                }
            }
            enigma::instance_event_iterator = push_it;
            if (stop_loop) break;
            //particles
            if (enigma::particles_impl != NULL) {
                const double high = dit->first;
                dit++;
                const double low = dit != drawing_depths.rend() ? dit->first : -numeric_limits<double>::max();
                dit--;
                (enigma::particles_impl->draw_particlesystems)(high, low);
            }
        }
    }
    else
    {
        bool stop_loop = false;
		//TODO: Possibly implement view option from Stupido to control which view clears the background
		// Only clear the background on the first visible view by checking if it hasn't been cleared yet
		bool view_first = true;
        for (view_current = 0; view_current < 8; view_current++)
        {
            if (!view_visible[(int)view_current]) { continue; }
			int vc = (int)view_current;
			int vob = (int)view_object[vc];

			if (vob != -1)
			{
				object_basic *instanceexists = fetch_instance_by_int(vob);

				if (instanceexists)
				{
					object_planar* vobr = (object_planar*)instanceexists;

					double vobx = vobr->x, voby = vobr->y;

					//int bbl=*vobr.x+*vobr.bbox_left,bbr=*vobr.x+*vobr.bbox_right,bbt=*vobr.y+*vobr.bbox_top,bbb=*vobr.y+*vobr.bbox_bottom;
					//if (bbl<view_xview[vc]+view_hbor[vc]) view_xview[vc]=bbl-view_hbor[vc];

					double vbc_h, vbc_v;
					(view_hborder[vc] > view_wview[vc]/2) ? vbc_h = view_wview[vc]/2 : vbc_h = view_hborder[vc];
					(view_vborder[vc] > view_hview[vc]/2) ? vbc_v = view_hview[vc]/2 : vbc_v = view_vborder[vc];

					if (view_hspeed[vc] == -1)
					{
						if (vobx < view_xview[vc] + vbc_h)
							view_xview[vc] = vobx - vbc_h;
						else if (vobx > view_xview[vc] + view_wview[vc] - vbc_h)
							view_xview[vc] = vobx + vbc_h - view_wview[vc];
					}
					else
					{
						if (vobx < view_xview[vc] + vbc_h)
						{
							view_xview[vc] -= view_hspeed[vc];
							if (view_xview[vc] < vobx - vbc_h)
								view_xview[vc] = vobx - vbc_h;
						}
						else if (vobx > view_xview[vc] + view_wview[vc] - vbc_h)
						{
							view_xview[vc] += view_hspeed[vc];
							if (view_xview[vc] > vobx + vbc_h - view_wview[vc])
								view_xview[vc] = vobx + vbc_h - view_wview[vc];
						}
					}

					if (view_vspeed[vc] == -1)
					{
						if (voby < view_yview[vc] + vbc_v)
							view_yview[vc] = voby - vbc_v;
						else if (voby > view_yview[vc] + view_hview[vc] - vbc_v)
							view_yview[vc] = voby + vbc_v - view_hview[vc];
					}
					else
					{
						if (voby < view_yview[vc] + vbc_v)
						{
							view_yview[vc] -= view_vspeed[vc];
							if (view_yview[vc] < voby - vbc_v)
								view_yview[vc] = voby - vbc_v;
						}
						if (voby > view_yview[vc] + view_hview[vc] - vbc_v)
						{
							view_yview[vc] += view_vspeed[vc];
							if (view_yview[vc] > voby + vbc_v - view_hview[vc])
								view_yview[vc] = voby + vbc_v - view_hview[vc];
						}
					}

					if (view_xview[vc] < 0)
						view_xview[vc] = 0;
					else if (view_xview[vc] > room_width - view_wview[vc])
						view_xview[vc] = room_width - view_wview[vc];

					if (view_yview[vc] < 0)
						view_yview[vc] = 0;
					else if (view_yview[vc] > room_height - view_hview[vc])
						view_yview[vc] = room_height - view_hview[vc];
				}
			}

			glViewport(view_xport[vc], view_yport[vc], window_get_region_width_scaled() - view_xport[vc], window_get_region_height_scaled() - view_yport[vc]);
			glLoadIdentity();
			glScalef(1, (bound_framebuffer==0?-1:1), 1);

			glOrtho(int(view_xview[vc]), int(view_wview[vc] + view_xview[vc]), int(view_yview[vc]), int(view_hview[vc] + view_yview[vc]), 0, 1);
			glGetDoublev(GL_MODELVIEW_MATRIX,projection_matrix);
			glMultMatrixd(transformation_matrix);

			int clear_bits = 0;
			if (background_showcolor && view_first)
			{
				int clearcolor = ((int)background_color) & 0x00FFFFFF;
				glClearColor(__GETR(clearcolor) / 255.0, __GETG(clearcolor) / 255.0, __GETB(clearcolor) / 255.0, 1);
				clear_bits |= GL_COLOR_BUFFER_BIT;
			}

			// Clear the depth buffer if 3d mode is on at the beginning of the draw step.
			// Always clear the depth buffer for every view, this was Game Maker 8.1 behaviour
			if (enigma::d3dMode)
				clear_bits |= GL_DEPTH_BUFFER_BIT;

			if (clear_bits)
				glClear(clear_bits);

			if (view_first) {
				draw_back();
			}

			// Apply and clear stored depth changes.
			for (map<int,pair<double,double> >::iterator it = id_to_currentnextdepth.begin(); it != id_to_currentnextdepth.end(); it++)
			{
				enigma::object_graphics* inst_depth = (enigma::object_graphics*)enigma::fetch_instance_by_id((*it).first);
				if (inst_depth != NULL) {
					drawing_depths[(*it).second.first].draw_events->unlink(inst_depth->depth.myiter);
					inst_iter* mynewiter = drawing_depths[(*it).second.second].draw_events->add_inst(inst_depth->depth.myiter->inst);
					if (instance_event_iterator == inst_depth->depth.myiter) {
						instance_event_iterator = inst_depth->depth.myiter->prev;
					}
					inst_depth->depth.myiter = mynewiter;
				}
			}
			id_to_currentnextdepth.clear();

			if (enigma::particles_impl != NULL) {
				const double high = numeric_limits<double>::max();
				const double low = drawing_depths.rbegin() != drawing_depths.rend() ? drawing_depths.rbegin()->first : -numeric_limits<double>::max();
				(enigma::particles_impl->draw_particlesystems)(high, low);
			}
			for (enigma::diter dit = drawing_depths.rbegin(); dit != drawing_depths.rend(); dit++)
			{
				texture_reset();
				if (dit->second.tiles.size())
					glCallList(drawing_depths[dit->second.tiles[0].depth].tilelist);

				enigma::inst_iter* push_it = enigma::instance_event_iterator;
				//loop instances
				for (enigma::instance_event_iterator = dit->second.draw_events->next; enigma::instance_event_iterator != NULL; enigma::instance_event_iterator = enigma::instance_event_iterator->next) {
					enigma::instance_event_iterator->inst->myevent_draw();
					if (enigma::room_switching_id != -1) {
						stop_loop = true;
						break;
					}
				}
				enigma::instance_event_iterator = push_it;
				if (stop_loop) break;
				//particles
				if (enigma::particles_impl != NULL) {
					const double high = dit->first;
					dit++;
					const double low = dit != drawing_depths.rend() ? dit->first : -numeric_limits<double>::max();
					dit--;
					(enigma::particles_impl->draw_particlesystems)(high, low);
				}
			}
			view_first = false;
			if (stop_loop) break;
        }
        view_current = 0;
    }

	// Now process the sub event of draw called draw gui
	// It is for drawing GUI elements without view scaling and transformation
    if (enigma::gui_used)
    {
	    glViewport(0, 0, window_get_region_width_scaled(), window_get_region_height_scaled());
        glLoadIdentity();
        glScalef(1, (bound_framebuffer==0?-1:1), 1);
        glOrtho(0, enigma::gui_width, 0, enigma::gui_height, 0, 1);
        glGetDoublev(GL_MODELVIEW_MATRIX,projection_matrix);
        glMultMatrixd(transformation_matrix);

		// Clear the depth buffer if hidden surface removal is on at the beginning of the draw step.
        if (enigma::d3dMode)
			glClear(GL_DEPTH_BUFFER_BIT);

		int culling = d3d_get_culling();
		d3d_set_culling(rs_none);

        bool stop_loop = false;

        for (enigma::diter dit = drawing_depths.rbegin(); dit != drawing_depths.rend(); dit++)
        {

            enigma::inst_iter* push_it = enigma::instance_event_iterator;
            //loop instances
            for (enigma::instance_event_iterator = dit->second.draw_events->next; enigma::instance_event_iterator != NULL; enigma::instance_event_iterator = enigma::instance_event_iterator->next) {
				enigma::instance_event_iterator->inst->myevent_drawgui();
                if (enigma::room_switching_id != -1) {
                    stop_loop = true;
                    break;
                }
            }
            enigma::instance_event_iterator = push_it;
            if (stop_loop) break;
        }

		// reset the culling
		d3d_set_culling(culling);
    }
		
	oglmgr->EndScene();
		
	if (enigma::msaa_fbo != 0) {
		GLint fbo;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &fbo);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, enigma::msaa_fbo);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		//TODO: Change the code below to fix this to size properly to views
		glBlitFramebuffer(0, 0, window_get_region_width_scaled(), window_get_region_height_scaled(), 0, 0, window_get_region_width_scaled(), window_get_region_height_scaled(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
		// glReadPixels(0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
	}
	
    ///TODO: screen_refresh() shouldn't be in screen_redraw(). They are seperate functions for a reason.
    if (bound_framebuffer==0) { screen_refresh(); }
}

void screen_init()
{
	enigma::gui_width = window_get_region_width_scaled();
	enigma::gui_height = window_get_region_height_scaled();

	glClearColor(0,0,0,0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
    if (!view_enabled)
    {
        glMatrixMode(GL_PROJECTION);
          glClearColor(0,0,0,0);
          glLoadIdentity();
          gluPerspective(0, 1, 0, 1);
        glMatrixMode(GL_MODELVIEW);
          glViewport(0, 0, window_get_region_width_scaled(), window_get_region_height_scaled());
          glLoadIdentity();
          glScalef(1, -1, 1);
          glOrtho(0, room_width, 0, room_height, 0, 1);
          glGetDoublev(GL_MODELVIEW_MATRIX,projection_matrix);
    }
    else
    {
        for (view_current = 0; view_current < 7; view_current++)
        {
            if (view_visible[(int)view_current])
            {
                int vc = (int)view_current;
                glMatrixMode(GL_PROJECTION);
                  glClearColor(0,0,0,0);
                  glLoadIdentity();
                  gluPerspective(0, 1, 0, 1);
                glMatrixMode(GL_MODELVIEW);
                  glViewport(view_xport[vc], view_yport[vc], view_wport[vc], view_hport[vc]);
                  glLoadIdentity();
                  glScalef(1, -1, 1);
                  glOrtho(view_xview[vc], view_wview[vc] + view_xview[vc], view_yview[vc], view_hview[vc] + view_yview[vc], 0, 1);
                  glGetDoublev(GL_MODELVIEW_MATRIX,projection_matrix);
                  glMultMatrixd(transformation_matrix);
                break;
            }
        }
    }
	
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glAlphaFunc(GL_ALWAYS,0);
	texture_reset();
	draw_set_color(c_white);
}

int screen_save(string filename) //Assumes native integers are little endian
{
	unsigned int w=window_get_width(),h=window_get_height(),sz=w*h;
	
	string ext = enigma::image_get_format(filename);
	
	unsigned char *rgbdata = new unsigned char[sz*4];
	GLint prevFbo;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
 	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glReadPixels(0,0,w,h, GL_RGBA, GL_UNSIGNED_BYTE, rgbdata);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevFbo);

	int ret;
	if (ext == ".png") {
		unsigned char* data = image_reverse_scanlines(rgbdata, w, h, 4);
		ret = image_save(filename, data, w, h, w, h);
		delete[] data;
	} else {
		ret = image_save(filename, rgbdata, w, h, w, h);
	}
	
	delete[] rgbdata;
	return ret;
}

int screen_save_part(string filename,unsigned x,unsigned y,unsigned w,unsigned h) //Assumes native integers are little endian
{
	unsigned sz = w*h;
	
	string ext = enigma::image_get_format(filename);
	
	unsigned char *rgbdata = new unsigned char[sz*4];
	GLint prevFbo;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
 	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glReadPixels(x,window_get_region_height_scaled()-h-y,w,h, GL_RGBA, GL_UNSIGNED_BYTE, rgbdata);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevFbo);
	
	int ret;
	if (ext == ".png") {
		unsigned char* data = image_reverse_scanlines(rgbdata, w, h, 4);
		ret = image_save(filename, data, w, h, w, h);
		delete[] data;
	} else {
		ret = image_save(filename, rgbdata, w, h, w, h);
	}
	
	delete[] rgbdata;
	return ret;
}

void display_set_gui_size(unsigned width, unsigned height) {
	enigma::gui_width = width;
	enigma::gui_height = height;
}

}

