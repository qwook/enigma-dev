/********************************************************************************\
**                                                                              **
**  Copyright (C) 2008 Josh Ventura                                             **
**  Copyright (C) 2013 Robert B. Colton                                         **
**                                                                              **
**  This file is a part of the ENIGMA Development Environment.                  **
**                                                                              **
**                                                                              **
**  ENIGMA is free software: you can redistribute it and/or modify it under the **
**  terms of the GNU General Public License as published by the Free Software   **
**  Foundation, version 3 of the license or any later version.                  **
**                                                                              **
**  This application and its source code is distributed AS-IS, WITHOUT ANY      **
**  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS   **
**  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more       **
**  details.                                                                    **
**                                                                              **
**  You should have recieved a copy of the GNU General Public License along     **
**  with this code. If not, see <http://www.gnu.org/licenses/>                  **
**                                                                              **
**  ENIGMA is an environment designed to create games and other programs with a **
**  high-level, fully compilable language. Developers of ENIGMA or anything     **
**  associated with ENIGMA are in no way responsible for its users or           **
**  applications created by its users, or damages caused by the environment     **
**  or programs made in the environment.                                        **
**                                                                              **
\********************************************************************************/

#include "workdir.h"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>

using namespace std;


#include "parser/parser.h"

#include "backend/EnigmaStruct.h" //LateralGM interface structures
#include "compiler/compile_common.h"
#include "compiler/event_reader/event_parser.h"
#include "general/parse_basics_old.h"
#include "settings.h"

#include "languages/lang_CPP.h"

void print_to_sstream(std::string code, std::string synt, unsigned int &strc, varray<string> &strs, int, std::stringstream &sstream) {
    // needs a replacement so it can print to stream
    ofstream temp;
    temp.open((workdir + "Preprocessor_Environment_Editable/.temp").c_str(),ios_base::out);
    print_to_file(code, synt, strc, strs, 2, temp);
    temp.close();
    ifstream temp_;
    temp_.open((workdir + "Preprocessor_Environment_Editable/.temp").c_str());
    copy(istreambuf_iterator<char>(temp_), istreambuf_iterator<char>(), ostreambuf_iterator<char>(sstream));
    temp_.close();
    // end shitty code
}

inline bool iscomment(const string &n) {
    if (n.length() < 2 or n[0] != '/') return false;
    if (n[1] == '/') return true;
    if (n[1] != '*') return false;
    for (unsigned i = 2; i < n.length(); i++)
    {
        if (n[i] == '*' and n[i+1] == '/')
            return (i + 2 >= n.length());
    }
    return true;
}

typedef struct {
    vector<int> events;
    parsed_object *obj;
} parsed_parent;

// Recursive function to create object structures, parents first.
std::map<int, parsed_parent> parsed; // this vector will also order the structures by hiearchy
int writeObject(ofstream &wto, int i);

struct cspair { string c, s; };
int lang_CPP::compile_writeObjectData(EnigmaStruct* es, parsed_object* global)
{
    parsed.clear();

    ofstream wto;
    wto.open((workdir +"Preprocessor_Environment_Editable/IDE_EDIT_objectdeclarations.h").c_str(),ios_base::out);
    
    // HEADER
    wto << license;
    wto << "#include \"Universal_System/collisions_object.h\"\n";
    wto << "#include \"Universal_System/object.h\"\n";
    wto << "#include <map>\n";

    //- SCRIPTS
    for (int i = 0; i < es->scriptCount; i++) {
        wto << "#define " << es->scripts[i].name << "(arguments...) _SCR_" << es->scripts[i].name << "(arguments)\n";

        parsed_script* scr = scr_lookup[es->scripts[i].name];
        const char* comma = "";
        wto << "variant _SCR_" << es->scripts[i].name << "(";
        scr->globargs=16;//Fixes too many arguments error (scripts can be called dynamically with any number of arguments)
        for (int argn = 0; argn < scr->globargs; argn++) {
            wto << comma << "variant argument" << argn << "=0";
            comma = ", ";
        }
        wto << ");\n";
    }

    wto <<
    "namespace enigma\n"
    "{\n"
    "    extern objectstruct** objectdata;\n"
    "    struct object_locals: event_parent"
    ;

    //- PARSED EXTENSIONS
    for (unsigned i = 0; i < parsed_extensions.size(); i++) {
        if (!parsed_extensions[i].implements.empty()) {
            wto << ", virtual " << parsed_extensions[i].implements;
        } else {
            wto << "/*, " << parsed_extensions[i].name << "*/";
        }
    }
    wto << " {\n";

    wto <<
    "        #include \"Preprocessor_Environment_Editable/IDE_EDIT_inherited_locals.h\"\n"
    "        std::map<string, var> *vmap;\n"
    "        object_locals() {vmap = NULL;}\n"
    "        object_locals(unsigned _x, int _y): event_parent(_x,_y) {vmap = NULL;}\n"
    "    };\n"
    ;
    
    // OBJECTS
    std::cout << "== Parsed Objects ==" << std::endl;
    for (po_i i = parsed_objects.begin(); i != parsed_objects.end(); i++) {
        parsed_object *o = i->second;
        int err = writeObject(wto, o->id);
        if (err > 0) {
            wto.close();
            return err;
        }
    }

    // FOOTER
    wto << "    objectstruct objs[] = {\n";
    int objcount = parsed.size(), obmx = 0;
    for (std::map<int, parsed_parent> ::iterator i = parsed.begin(); i != parsed.end(); i++) {
        parsed_object *o = i->second.obj;

        wto << "        {"
            << o->sprite_index << ","
            << o->solid << ","
            << o->visible << ","
            << o->depth << ","
            << o->persistent << ","
            << o->mask_index << ","
            << o->parent << ","
            << o->id <<
        "},\n";

        if (o->id >= obmx) obmx = o->id;
    }
    wto << "    };\n";
    wto << "    int objectcount = " << objcount << ";\n";
    wto << "    int obj_idmax = " << obmx+1 << ";\n";

    wto << "}\n";

    wto.close();


    // todo: replace this

    /* NEXT FILE `******************************************\
    ** Object functions: events, constructors, other codes.
    ********************************************************/

        cout << "DBGMSG 1" << endl;
    wto.open((workdir +"Preprocessor_Environment_Editable/IDE_EDIT_objectfunctionality.h").c_str(),ios_base::out);
        wto << license;

        cout << "DBGMSG 2" << endl;
        // Export globalized scripts
        for (int i = 0; i < es->scriptCount; i++)
        {
            parsed_script* scr = scr_lookup[es->scripts[i].name];
            const char* comma = "";
            wto << "variant _SCR_" << es->scripts[i].name << "(";
            for (int argn = 0; argn < scr->globargs; argn++) //it->second gives max argument count used
            {
                wto << comma << "variant argument" << argn;
                comma = ", ";
            }
            wto << ")\n{\n  ";
            parsed_event& upev = scr->pev_global?*scr->pev_global:scr->pev;
            print_to_file(upev.code,upev.synt,upev.strc,upev.strs,2,wto);
            wto << "\n  return 0;\n}\n\n";
        }

        cout << "DBGMSG 3" << endl;
        // Export everything else
        for (po_i i = parsed_objects.begin(); i != parsed_objects.end(); i++)
        {
        cout << "DBGMSG 4" << endl;
            for (unsigned ii = 0; ii < i->second->events.size; ii++)
            if  (i->second->events[ii].code != "")
            {
            cout << "DBGMSG 4-1" << endl;
                const int mid = i->second->events[ii].mainId, id = i->second->events[ii].id;
                string evname = event_get_function_name(mid,id);
            bool defined_inherited = false;
            if (setting::inherit_objects) {
            po_i parent = parsed_objects.find(i->second->parent);
            if (parent != parsed_objects.end()) {
                parsed_parent parent_defined = parsed.find(i->second->parent)->second;
                if (find(parent_defined.events.begin(), parent_defined.events.end(), i->second->events[ii].mainId) != parent_defined.events.end()) {
                    wto << "#define event_inherited OBJ_" + parent->second->name + "::myevent_" + evname + "\n";
                        defined_inherited = true;
                }
            }
        }
        
        // cout << "DBGMSG 4-2" << endl;
        //         wto << "variant enigma::OBJ_" << i->second->name << "::myevent_" << evname << "()\n{\n  ";
        //             if (!event_execution_uses_default(i->second->events[ii].mainId,i->second->events[ii].id))
        //                 wto << "enigma::temp_event_scope ENIGMA_PUSH_ITERATOR_AND_VALIDATE(this);\n  ";
        // cout << "DBGMSG 4-3" << endl;
        //             if (event_has_sub_check(mid, id))
        //                 wto << event_get_sub_check_condition(mid, id) << endl;
        //             if (event_has_const_code(mid, id))
        //                 wto << event_get_const_code(mid, id) << endl;
        //             if (event_has_prefix_code(mid, id))
        //                 wto << event_get_prefix_code(mid, id) << endl;
        // cout << "DBGMSG 4-4" << endl;
        //             print_to_file(i->second->events[ii].code,i->second->events[ii].synt,i->second->events[ii].strc,i->second->events[ii].strs,2,wto);
        //             if (event_has_suffix_code(mid, id))
        //                 wto << event_get_suffix_code(mid, id) << endl;
        // cout << "DBGMSG 4-5" << endl;
        //         wto << "\n  return 0;\n}\n";
        
        if (defined_inherited) {
            wto << "#undef event_inherited\n";
        }
        
            }
        cout << "DBGMSG 5" << endl;

    
            parsed_object* t = i->second;
            for (parsed_object::funcit it = t->funcs.begin(); it != t->funcs.end(); it++) //For each function called by this object
            {
                map<string,parsed_script*>::iterator subscr = scr_lookup.find(it->first); //Check if it's a script
                if (subscr != scr_lookup.end() // If we've got ourselves a script
                and subscr->second->pev_global) // And it has distinct code for use at the global scope (meaning it's more efficient locally)
                {
                    const char* comma = "";
                    wto << "variant enigma::OBJ_" << i->second->name << "::_SCR_" << it->first << "(";
                    for (int argn = 0; argn < it->second; argn++) //it->second gives max argument count used
                    {
                        wto << comma << "variant argument" << argn;
                        comma = ", ";
                    }
                    wto << ")\n{\n  ";
                    print_to_file(subscr->second->pev.code,subscr->second->pev.synt,subscr->second->pev.strc,subscr->second->pev.strs,2,wto);
                    wto << "\n  return 0;\n}\n\n";
                }
            }
        cout << "DBGMSG 6" << endl;
        }
        cout << "DBGMSG 7" << endl;
    
        wto << "namespace enigma\n{\n"
        "  callable_script callable_scripts[] = {\n";
        int scr_count = 0;
        for (int i = 0; i < es->scriptCount; i++)
        {
            while (es->scripts[i].id > scr_count)
            {
                    wto << "    { NULL, -1 },\n";
                    scr_count++;
            }
            scr_count++;
            wto << "    { (variant(*)())_SCR_" << es->scripts[i].name << ", " << scr_lookup[es->scripts[i].name]->globargs << " },\n";
        }
        wto << "  };\n  \n";

        cout << "DBGMSG 8" << endl;
        wto << "  void constructor(object_basic* instance_b)\n  {\n"
        "    //This is the universal create event code\n    object_locals* instance = (object_locals*)instance_b;\n    \n"
        "    instance->xstart = instance->x;\n    instance->ystart = instance->y;\n    instance->xprevious = instance->x;\n    instance->yprevious = instance->y;\n\n"
        "    instance->gravity=0;\n    instance->gravity_direction=270;\n    instance->friction=0;\n    \n"
        "    \n"
        "    instance->image_alpha = 1.0;\n    instance->image_angle = 0;\n    instance->image_blend = 0xFFFFFF;\n    instance->image_index = 0;\n"
        "    instance->image_speed  = 1;\n    instance->image_xscale = 1;\n    instance->image_yscale = 1;\n    \n"
        "instancecount++;\n    instance_count++;\n  }\n}\n";
    wto.close();

    parsed.clear();

    return 0;
}

int writeObject(ofstream &wto, int i) {
    // Already parsed! Don't do anything.
    if (parsed.find(i) != parsed.end()) {
        return 0;
    }

    // Not a real object.
    po_i it = parsed_objects.find(i);
    if (it == parsed_objects.end()) {
        return 1;
    }

    // If we have a parent, write the parent first.
    parsed_object *o = it->second;
    parsed_parent *parent = NULL;
    if (o->parent >= 0) {
        int err = writeObject(wto, o->parent);
        if (err > 0) return err;

        std::map<int, parsed_parent>::iterator it = parsed.find(o->parent);
        // Parent doesn't exist... That shouldn't happen. Error out.
        if (it == parsed.end()) return 2;
        parent = &it->second;
    }

    // Start writing the structure!
    wto << "struct OBJ_" << o->name << ": ";

    if (setting::inherit_objects && o->parent >= 0) {
        wto << "OBJ_" << parent->obj->name << "\n";
    } else {
        wto << "object_locals\n";        
    }

    wto << "{\n";

    // CONSTRUCTOR
    wto << "    OBJ_" << o->name << "(int enigma_genericconstructor_newinst_x = 0, int enigma_genericconstructor_newinst_y = 0, const int id = (enigma::maxid++), const int enigma_genericobjid = 1)";

    if (setting::inherit_objects && o->parent >= 0) {
        wto << ": OBJ_" << parent->obj->name << "(enigma_genericconstructor_newinst_x,enigma_genericconstructor_newinst_y,id,enigma_genericobjid)";
    } else {
        wto << ": object_locals(id,enigma_genericobjid) ";
    }

    wto <<
    "    {\n"
    "        sprite_index = enigma::objectdata[" << o->id << "]->sprite;\n"
    "        mask_index = enigma::objectdata[" << o->id << "]->mask;\n"
    "        visible = enigma::objectdata[" << o->id << "]->visible;\n"
    "        solid = enigma::objectdata[" << o->id << "]->solid;\n"
    "        persistent = enigma::objectdata[" << o->id << "]->persistent;\n"
    "        activate();\n"
    "        x = enigma_genericconstructor_newinst_x, y = enigma_genericconstructor_newinst_y;\n"
    "        enigma::constructor(this);\n"
    "    }\n"
    ;

    // UNLINK
    wto <<
    "    void unlink() {\n"
    "        instance_iter_queue_for_destroy(ENOBJ_ITER_me); // Queue for delete while we're still valid\n"
    "        deactivate();\n"
    "    }\n";

    // MYEVENT / DEACTIVATE / ACTIVATE / TRACKER / DESTRUCTOR
    std::stringstream event_def;
    std::stringstream myevents;
    std::stringstream tracker;
    std::stringstream activate;
    std::stringstream deactivate;
    std::stringstream destructor;

    std::map<int, std::string> groups;

    if (!setting::inherit_objects || parent == NULL) {
        tracker <<    "    enigma::pinstance_list_iterator ENOBJ_ITER_me;\n";
        tracker <<    "    enigma::inst_iter *ENOBJ_ITER_myobj" << o->id << ";\n";
        activate <<   "        depth.init(enigma::objectdata[" << o->id << "]->depth, this);\n";
        activate <<   "        ENOBJ_ITER_me = enigma::link_instance(this);\n";
        deactivate << "        enigma::unlink_main(ENOBJ_ITER_me);\n";
        destructor << "        delete vmap;\n";
        destructor << "        enigma::winstance_list_iterator_delete(ENOBJ_ITER_me);\n";

        // Since we are not inheriting, we create our "parent"s variables here.
        for (po_i parent = parsed_objects.find(i); parent != parsed_objects.end(); parent = parsed_objects.find(parent->second->parent)) {
            activate <<   "        ENOBJ_ITER_myobj" << parent->second->id << " = enigma::link_obj_instance(this, " << parent->second->id << ");\n";
            deactivate << "        unlink_object_id_iter(ENOBJ_ITER_myobj" << parent->second->id << ", " << parent->second->id << ");\n";
            destructor << "        delete ENOBJ_ITER_myobj" << parent->second->id << ";\n";
        }
    } else {
        if (parent != NULL)
            myevents << "        OBJ_" << parent->obj->name << "::myevents_perf(type, numb);\n";

        tracker <<    "    enigma::inst_iter *ENOBJ_ITER_myobj" << o->id << ";\n";
        activate <<   "        depth.remove();\n";
        activate <<   "        depth.init(enigma::objectdata[" << o->id << "]->depth, this);\n";
        activate <<   "        ENOBJ_ITER_myobj" << o->id << " = enigma::link_obj_instance(this, " << o->id << ");\n";
        deactivate << "        OBJ_" << parent->obj->name << "::deactivate();\n";
        deactivate << "        unlink_object_id_iter(ENOBJ_ITER_myobj" << o->id << ", " << o->id << ");\n";
        destructor << "        delete ENOBJ_ITER_myobj" << o->id << ";\n";
    }

    vector<int> ev(0);

    varray<parsed_event> &events = o->events;
    for (unsigned int e_i = 0; e_i < o->events.size; e_i++) {
        parsed_event &event = events[e_i];

        // Populate our events list with this event if its not already in there.
        if (find(ev.begin(), ev.end(), event.mainId) == ev.end())
            ev.push_back(event.mainId);

        if (!event.code.empty()) {
            const string name = event_get_function_name(event.mainId, event.id);
            myevents << "        if (type == " << event.mainId << " && numb == " << event.id << ")\n";
            myevents << "            return myevent_" << name << "();\n";

            event_def << "    variant myevent_" << name << "() {\n";
            if (!event_execution_uses_default(event.mainId, event.id))
            event_def << "enigma::temp_event_scope ENIGMA_PUSH_ITERATOR_AND_VALIDATE(this);\n  ";
            if (event_has_sub_check(event.mainId, event.id))
                event_def << event_get_sub_check_condition(event.mainId, event.id) << endl;
            if (event_has_const_code(event.mainId, event.id))
                event_def << event_get_const_code(event.mainId, event.id) << endl;
            if (event_has_prefix_code(event.mainId, event.id))
                event_def << event_get_prefix_code(event.mainId, event.id) << endl;
            print_to_sstream(event.code, event.synt, event.strc, event.strs, 2, event_def);
            if (event_has_suffix_code(event.mainId, event.id))
                event_def << event_get_suffix_code(event.mainId, event.id) << endl;
            event_def << "        return 0;\n";
            event_def << "    }\n";

            if (event_is_instance(event.mainId, event.id)) {
                if (groups.find(event.mainId) == groups.end())
                    groups[event.mainId] = "";

                groups[event.mainId] += "        ";

                if (event_has_super_check(event.mainId, event.id)) 
                    groups[event.mainId] += "if (" + event_get_super_check_condition(event.mainId, event.id) + ") ";
                
                groups[event.mainId] += "myevent_" + name + "();\n";
            }

        }

        if (setting::inherit_objects && parent != NULL && std::find(parent->events.begin(), parent->events.end(), event.mainId) != parent->events.end()) {
            // Populate our events list with our parent's event list.
            if (find(ev.begin(), ev.end(), event.mainId) == ev.end())
                ev.push_back(event.mainId);
            continue;
        }

        if (!event_is_instance(event.mainId, event.id)) {
            const string name = event_get_function_name(event.mainId,event.id);

            // Tracker
            if (event_has_iterator_declare_code(event.mainId,event.id)) {
                if (!iscomment(event_get_iterator_declare_code(event.mainId,event.id)))
                    tracker << "    " << event_get_iterator_declare_code(event.mainId,event.id) << "\n";
            } else
                    tracker << "    enigma::inst_iter *ENOBJ_ITER_myevent_" << name << ";\n";


            // Activate
            if (event_has_iterator_initialize_code(event.mainId,event.id)) {
                if (!iscomment(event_get_iterator_initialize_code(event.mainId,event.id)))
                    activate << "        " << event_get_iterator_initialize_code(event.mainId,event.id) << "\n";
            } else
                    activate << "        ENOBJ_ITER_myevent_" << name << " = enigma::event_" << name << "->add_inst(this);\n";

            // Deactivate
            if (event_has_iterator_unlink_code(event.mainId,event.id)) {
                if (!iscomment(event_get_iterator_unlink_code(event.mainId,event.id)))
                    deactivate << "        " << event_get_iterator_unlink_code(event.mainId,event.id) << "\n";
            } else
                    deactivate << "        enigma::event_" << name << "->unlink(ENOBJ_ITER_myevent_" << name << ");\n";

            // Deconstructor
            if (event_has_iterator_delete_code(event.mainId,event.id)) {
                if (!iscomment(event_get_iterator_delete_code(event.mainId,event.id)))
                    destructor << "        " << event_get_iterator_delete_code(event.mainId,event.id) << "\n";
            } else
                    destructor << "        delete ENOBJ_ITER_myevent_" << event_get_function_name(event.mainId,event.id) << ";\n";
        }
    }

    for (std::map<int, std::string>::iterator it = groups.begin(); it != groups.end(); it++) {
        const std::string rootname = event_stacked_get_root_name(it->first);
        tracker <<      "    enigma::inst_iter *ENOBJ_ITER_myevent_" << rootname << ";\n";
        activate <<     "        ENOBJ_ITER_myevent_" << rootname << " = enigma::event_" << rootname << "->add_inst(this);\n";
        deactivate <<   "        enigma::event_" << rootname << " ->unlink(ENOBJ_ITER_myevent_" << rootname << ");\n";
        destructor <<   "        delete ENOBJ_ITER_myevent_" << rootname << ";\n";
    }

    // Write the bodies into the file.
    wto << tracker.str();
    wto << event_def.str();

    wto <<
    "    variant myevents_perf(int type, int numb) {\n"
    << myevents.str() <<
    "    }\n";

    for (std::map<int, std::string>::iterator it = groups.begin(); it != groups.end(); it++) {
        const std::string rootname = event_stacked_get_root_name(it->first);
        wto << "    void myevent_" << rootname << "() {\n";
        wto << it->second;
        wto << "    }\n";
    }

    wto <<
    "    void activate() {\n"
    << activate.str() <<
    "    }\n";

    wto <<
    "    void deactivate() {\n"
    << deactivate.str() <<
    "    }\n";

    wto <<
    "    ~OBJ_" << o->name << "() {\n"
    << destructor.str() <<
    "    }\n";

    // OBJECT SCRIPTS
    // for (parsed_object::funcit it = o->funcs.begin(); it != o->funcs.end(); it++) {
    //     std::map<string,parsed_script *>::iterator subscr = scr_lookup.find(it->first);
    //     parsed_script *script = subscr->second;
    //     if (subscr != scr_lookup.end() && script->pev_global) {
    //         wto << "    variant _SCR_" << it->first << "(";
    //         for (int argn = 0; argn < it->second; argn++) {
    //             if (argn == 0) {
    //                 wto << "variant argument" << argn;
    //             } else {
    //                 wto << ", variant argument" << argn;
    //             }
    //         }
    //         wto << ") {\n";
    //         print_to_file(subscr->second->pev.code,subscr->second->pev.synt,subscr->second->pev.strc,subscr->second->pev.strs,2,wto);
    //         wto << "        return 0;\n";
    //         wto << "    }\n";
    //     }
    // }

    wto <<
    "};\n";

    parsed_parent p = {ev, o};
    parsed[i] = p;

    return 0;
}

#ifdef COMMENTED_OUT
    //NEXT FILE ----------------------------------------
    //Object declarations: object classes/names and locals.
    ofstream wto;
    wto.open((workdir +"Preprocessor_Environment_Editable/IDE_EDIT_objectdeclarations.h").c_str(),ios_base::out);
    // ostream &wto = cout;
        wto << license;
        wto << "#include \"Universal_System/collisions_object.h\"\n";
        wto << "#include \"Universal_System/object.h\"\n\n";
        wto << "#include <map>";

        // Write the script names
        wto << "// Script identifiers\n";
        for (int i = 0; i < es->scriptCount; i++) {
            wto << "#define " << es->scripts[i].name << "(arguments...) _SCR_" << es->scripts[i].name << "(arguments)\n";
        }
        wto << "\n\n";

        for (int i = 0; i < es->scriptCount; i++)
        {
            parsed_script* scr = scr_lookup[es->scripts[i].name];
            const char* comma = "";
            wto << "variant _SCR_" << es->scripts[i].name << "(";
            scr->globargs=16;//Fixes too many arguments error (scripts can be called dynamically with any number of arguments)
            for (int argn = 0; argn < scr->globargs; argn++) {
                wto << comma << "variant argument" << argn << "=0";
                comma = ", ";
            }
            wto << ");\n";
        }
        wto << "\n";
        wto << "namespace enigma\n{\n";
        wto << "  extern objectstruct** objectdata;\n";
        wto << "  struct object_locals: event_parent";
            for (unsigned i = 0; i < parsed_extensions.size(); i++)
                if (parsed_extensions[i].implements != "")
                    wto << ", virtual " << parsed_extensions[i].implements;
                else
                    wto << " /*" << parsed_extensions[i].name << "*/";
        wto << "\n  {\n";
        wto << "    #include \"Preprocessor_Environment_Editable/IDE_EDIT_inherited_locals.h\"\n\n";
        wto << "    std::map<string, var> *vmap;\n";
        wto << "    object_locals() {vmap = NULL;}\n";
        wto << "    object_locals(unsigned _x, int _y): event_parent(_x,_y) {vmap = NULL;}\n  };\n";
        po_i i = parsed_objects.begin();

        vector<int> parsed(0);
        // Hold an iterator for our parent for later usage
        po_i parent = parsed_objects.find(i->second->parent);
        // Hold a map of all the
        map<int, vector<unsigned> > parent_definitions;
        vector<unsigned> parent_defined;
        std::cout << "HEN: " << parsed.size() << " " << parsed_objects.size() << std::endl;
        while (parsed.size() < parsed_objects.size())
        {
            if (i == parsed_objects.end()) { i = parsed_objects.begin(); }

            // if we have already been written or we have a parent that has not been written, continue
            // if (find(parsed.begin(), parsed.end(), i->first) != parsed.end()
        //       || (parsed_objects.find(i->second->parent)->second && find(parsed.begin(), parsed.end(), i->second->parent) == parsed.end())) { 
            //  i++; continue; 
            // }
            parent = parsed_objects.find(i->second->parent);

        
        wto << "  \n  struct OBJ_" << i->second->name;
        if (setting::inherit_objects && parsed_objects.find(i->second->parent) != parsed_objects.end()) {
            wto << ": OBJ_" << parsed_objects.find(i->second->parent)->second->name;
        } else {
            wto << ": object_locals";
        }

        wto << "\n  {\n // Local variables\n    ";
        for (unsigned ii = 0; ii < i->second->events.size; ii++)
        {
            string addls = event_get_locals(i->second->events[ii].mainId,i->second->events[ii].id);
            if (addls != "")
            {
                pt pos;
                string type, name, pres, sufs;
                for (pos = 0; pos < addls.length(); pos++)
                {
                    if (is_useless(addls[pos])) continue;
                    if (addls[pos] == ';') { i->second->locals[name] = dectrip(type, pres, sufs); type = pres = sufs = ""; continue; }
                    if (addls[pos] == ',') { i->second->locals[name] = dectrip(type, pres, sufs); pres = sufs = ""; continue; }
                    if (is_letter(addls[pos]) or addls[pos] == '$') {
                        const pt spos = pos;
                        while (is_letterdd(addls[++pos]));
                        string tn = addls.substr(spos,pos-spos);
                        (find_typename(tn) ? type : name) = tn;
                        pos--; continue;
                    }
                    if (addls[pos] == '*') { pres += '*'; continue; }
                    if (addls[pos] == '[') {
                        int cnt = 1;
                        const pt spos = pos;
                        while (cnt and ++pos < addls.length())
                            if (addls[pos] == '[' or addls[pos] == '(') cnt++;
                            else if (addls[pos] == ')' or addls[pos] == ']') cnt--;
                        sufs += addls.substr(spos,pos-spos+1);
                        continue;
                    }
                    if (addls[pos] == '=') {
                        int cnt = 0;

                        pt spos = ++pos;
                        while (is_useless(addls[spos])) spos++;
                        pos = spos - 1;

                        while (++pos < addls.length() and (cnt or (addls[pos] != ',' and addls[pos] != ';')))
                            if (addls[pos] == '[' or addls[pos] == '(') cnt++;
                            else if (addls[pos] == ')' or addls[pos] == ']') cnt--;
                        bool redundant = false;
                        for (size_t j = 0; j < i->second->initializers.size(); j++)
                            if (i->second->initializers[j].first == name) { redundant = true; break; }
                        if (!redundant)
                            i->second->initializers.push_back(initpair(name,addls.substr(spos,pos-spos)));
                        pos--; continue;
                    }
                }
            }
        }

        for (deciter ii =  i->second->locals.begin(); ii != i->second->locals.end(); ii++) {

            bool writeit = true; //Whether this "local" should be declared such
            if (setting::inherit_objects) {
                bool foundmatch = false;
                // Look to see if the parent declares this local otherwise we don't need to or it will get overridden and screwed up, eg. nulled
                for (po_i her = parsed_objects.find(i->second->parent); her != parsed_objects.end(); her = parsed_objects.find(her->second->parent)) {
                    for (deciter it =  her->second->locals.begin(); it != her->second->locals.end(); it++)
                    {
                        if (it->first == ii->first && it->second.prefix == ii->second.prefix && it->second.type == ii->second.type && it->second.suffix == ii->second.suffix) {
                            foundmatch = true; break;
                        }
                    }
                    if (foundmatch) { break; }
                }
                if (foundmatch) { continue; }
            }

            // If it's not explicitely defined, we must question whether it should be given a unique presence in this scope
            if (!ii->second.defined()) {
                parsed_object::globit ve = global->globals.find(ii->first); // So, we look for a global by this name
                if (ve != global->globals.end()) {  // If a global by this name is indeed found,
                    if (ve->second.defined()) // And this global is explicitely defined, not just accessed with a dot,
                        writeit = false; // We assume that its definition will cover us, and we do not redeclare it as a local.
                    cout << "enigma: scopedebug: variable `" << ii->first << "' from object `" << i->second->name << "' will be used from the " << (writeit ? "object" : "global") << " scope." << endl;
                }
            }
            if (writeit)
                wto << tdefault(ii->second.type) << " " << ii->second.prefix << ii->first << ii->second.suffix << ";\n    ";

        }

        // Next, we write the list of all the scripts this object will hoard a copy of for itself.
        wto << "\n    //Scripts called by this object\n    ";
        parsed_object* t = i->second;
        for (parsed_object::funcit it = t->funcs.begin(); it != t->funcs.end(); it++) { //For each function called by this object
            map<string,parsed_script*>::iterator subscr = scr_lookup.find(it->first); //Check if it's a script
            if (subscr != scr_lookup.end() && subscr->second->pev_global) { // If we've got ourselves a script And it has distinct code for use at the global scope (meaning it's more efficient locally)
                const char* comma = "";
                wto << "\n    variant _SCR_" << it->first << "(";
                for (int argn = 0; argn < it->second; argn++) { //it->second gives max argument count used
                    wto << comma << "variant argument" << argn << " = 0";
                    comma = ", ";
                }
                wto << ");";
            }
        } wto << "\n    ";

        // Keep a container of all the events the parent defined so they don't need searched more than one time.
        // vector<unsigned> parent_defined;
        // if (setting::inherit_objects) {
        //  for (unsigned ii = 0; ii < i->second->events.size; ii++) {
        //    for (po_i her = parsed_objects.find(i->second->parent); her != parsed_objects.end(); her = parsed_objects.find(her->second->parent)) {
        //      for (unsigned pi = 0; pi < her->second->events.size; pi++) {
        //          if (her->second->events[pi].mainId == i->second->events[ii].mainId && 
        //          her->second->events[pi].id == i->second->events[ii].id) {
        //              parent_defined.push_back(ii);
        //          }
        //      }
        //    }
        //  }
        //  parent_definitions[i->second->id] = parent_defined;
        // }

        // Now we output all the events this object uses
        // Defaulted events were already added into this array.
        map<int, cspair> nemap; // Keep track of events that need added to honor et_stacked
        for (unsigned ii = 0; ii < i->second->events.size; ii++) {
            if  (i->second->events[ii].code != "")
            {
                //Look up the event name
                string evname = event_get_function_name(i->second->events[ii].mainId,i->second->events[ii].id);
                if (event_is_instance(i->second->events[ii].mainId,i->second->events[ii].id))
                    nemap[i->second->events[ii].mainId].c += (event_has_super_check(i->second->events[ii].mainId,i->second->events[ii].id) ?
                        "        if (" + event_get_super_check_condition(i->second->events[ii].mainId,i->second->events[ii].id) + ") myevent_" : "        myevent_") + evname + "();\n",
                    nemap[i->second->events[ii].mainId].s = event_stacked_get_root_name(i->second->events[ii].mainId);
                wto << "    variant myevent_" << evname << "();\n    ";
            }
        }

        /* Event Perform Code */
        wto << "\n      //Event Perform Code\n      variant myevents_perf(int type, int numb)\n      {\n";
        
        if (setting::inherit_objects && parent != parsed_objects.end()) {
            wto << "        OBJ_" << parent->second->name << "::myevents_perf(type,numb);\n";
        }

        for (unsigned ii = 0; ii < i->second->events.size; ii++) {
            if (setting::inherit_objects && find(parent_defined.begin(), parent_defined.end(), ii) != parent_defined.end()) { continue; }
            if  (i->second->events[ii].code != "") {
                //Look up the event name
                string evname = event_get_function_name(i->second->events[ii].mainId,i->second->events[ii].id);
                wto << "        if (type == " << i->second->events[ii].mainId << " && numb == " << i->second->events[ii].id << ")\n";
                wto << "          return myevent_" << evname << "();\n";
            }
        }

        wto << "\n        return 0;\n      }\n";

        wto << "\n    //Locals to instances of this object\n    ";

        if (nemap.size())
        {
            wto << "\n    \n    // Grouped event bases\n    ";
            for (map<int,cspair>::iterator it = nemap.begin(); it != nemap.end(); it++) {
            wto << "  void myevent_" << it->second.s << "()\n      {\n";
                // If the parent also wrote this grouped event for instance some input events in the parent and some in the child, then we need to call the super method
                if (setting::inherit_objects) {
                    for (po_i her = parsed_objects.find(i->second->parent); her != parsed_objects.end(); her = parsed_objects.find(her->second->parent)) {
                        bool written = false;
                        for (unsigned ii = 0; ii < her->second->events.size; ii++) {
                            if  (her->second->events[ii].mainId == it->first && her->second->events[ii].code != "") {
                                wto << "        OBJ_" << her->second->name << "::myevent_" << it->second.s << "();\n"; 
                                written = true; break;
                            }
                        }
                        if (written) { break; }
                    }
                }
                wto << it->second.c << "      }\n    ";
            }
        }
        
        
        /**** Now we write the callable unlinker. Its job is to disconnect the instance for destroy.
        * @ * This is an important component that tracks multiple pieces of the instance. These pieces
        *//// are created for efficiency. See the instance system documentation for full details.

        // Here we write the pieces it tracks
        wto << "\n    // Self-tracking\n";

        // This tracks components of the instance system.
        bool has_parent = (parsed_objects.find(i->second->parent) != parsed_objects.end());
        if (!setting::inherit_objects || !has_parent) {
            wto << "      enigma::pinstance_list_iterator ENOBJ_ITER_me;\n";
            for (po_i her = i; her != parsed_objects.end(); her = parsed_objects.find(her->second->parent)) { // For this object and each parent thereof
                wto << "      enigma::inst_iter *ENOBJ_ITER_myobj" << her->second->id << ";\n"; // Keep track of a pointer to `this` inside this list.
            }
        } else {
            wto << "      enigma::inst_iter *ENOBJ_ITER_myobj" << i->second->id << ";\n"; // Keep track of a pointer to `this` inside this list.
        }
        // This tracks components of the event system.
        for (unsigned ii = 0; ii < i->second->events.size; ii++) { // Export a tracker for all events
            if (setting::inherit_objects && find(parent_defined.begin(), parent_defined.end(), ii) != parent_defined.end()) { continue; }
            if (!event_is_instance(i->second->events[ii].mainId,i->second->events[ii].id)) { //...well, all events which aren't stacked
                if (event_has_iterator_declare_code(i->second->events[ii].mainId,i->second->events[ii].id)) {
                    if (!iscomment(event_get_iterator_declare_code(i->second->events[ii].mainId,i->second->events[ii].id)))
                        wto << "      " << event_get_iterator_declare_code(i->second->events[ii].mainId,i->second->events[ii].id) << ";\n";
                } else {
                    wto << "      enigma::inst_iter *ENOBJ_ITER_myevent_" << event_get_function_name(i->second->events[ii].mainId,i->second->events[ii].id) << ";\n";
                }
            }
        }
                    for (map<int,cspair>::iterator it = nemap.begin(); it != nemap.end(); it++) // The stacked ones should have their root exported
                        wto << "      enigma::inst_iter *ENOBJ_ITER_myevent_" << it->second.s << ";\n";

                //This is the actual call to remove the current instance from all linked records before destroying it.
                wto << "\n    void unlink()\n    {\n";
        wto << "      instance_iter_queue_for_destroy(ENOBJ_ITER_me); // Queue for delete while we're still valid\n";
                    wto << "      deactivate();\n    }\n\n    void deactivate()\n    {\n";
            if (!setting::inherit_objects || !has_parent) { 
            wto << "      enigma::unlink_main(ENOBJ_ITER_me); // Remove this instance from the non-redundant, tree-structured list.\n";
            for (po_i her = i; her != parsed_objects.end(); her = parsed_objects.find(her->second->parent))
                wto << "      unlink_object_id_iter(ENOBJ_ITER_myobj" << her->second->id << ", " << her->second->id << ");\n";
            } else {
            wto << "      OBJ_" << parsed_objects.find(i->second->parent)->second->name << "::deactivate();\n";
            wto << "      unlink_object_id_iter(ENOBJ_ITER_myobj" << i->second->id << ", " << i->second->id << ");\n";
            }
                    for (unsigned ii = 0; ii < i->second->events.size; ii++) {
            if (setting::inherit_objects && find(parent_defined.begin(), parent_defined.end(), ii) != parent_defined.end()) { continue; }
                        if (!event_is_instance(i->second->events[ii].mainId,i->second->events[ii].id)) {
                            const string evname = event_get_function_name(i->second->events[ii].mainId,i->second->events[ii].id);
                            if (event_has_iterator_unlink_code(i->second->events[ii].mainId,i->second->events[ii].id)) {
                                if (!iscomment(event_get_iterator_unlink_code(i->second->events[ii].mainId,i->second->events[ii].id)))
                                    wto << "      " << event_get_iterator_unlink_code(i->second->events[ii].mainId,i->second->events[ii].id) << ";\n";
                            } else
                                wto << "      enigma::event_" << evname << "->unlink(ENOBJ_ITER_myevent_" << evname << ");\n";
                        }
            }
                    for (map<int,cspair>::iterator it = nemap.begin(); it != nemap.end(); it++) // The stacked ones should have their root exported
                        wto << "      enigma::event_" << it->second.s << "->unlink(ENOBJ_ITER_myevent_" << it->second.s << ");\n";
                    wto << "    }\n    ";


                /**** Next are the constructors. One is automated, the other directed.
                * @ *   Automatic constructor:  The constructor generates the ID from a global maximum and links by that alias.
                *////   Directed constructor:   Meant for use by the room system, the constructor uses a specified ID alias assumed to have been checked for conflict.
                wto <<   "\n    OBJ_" <<  i->second->name << "(int enigma_genericconstructor_newinst_x = 0, int enigma_genericconstructor_newinst_y = 0, const int id = (enigma::maxid++)" 
            << ", const int enigma_genericobjid = " << i->second->id << ")";
                 
         if (setting::inherit_objects && parsed_objects.find(i->second->parent) != parsed_objects.end()) { 
            wto << ": OBJ_" << parsed_objects.find(i->second->parent)->second->name << "(enigma_genericconstructor_newinst_x,enigma_genericconstructor_newinst_y,id,enigma_genericobjid)";
         } else {
            wto << ": object_locals(id,enigma_genericobjid) ";
         }
                    for (size_t ii = 0; ii < i->second->initializers.size(); ii++)
                        wto << ", " << i->second->initializers[ii].first << "(" << i->second->initializers[ii].second << ")";
                    wto << "\n    {\n";
                        // Sprite index
                            if (used_funcs::object_set_sprite) //We want to initialize
                                wto << "      sprite_index = enigma::object_table[" << i->second->id << "].->sprite;\n"
                                        << "      make_index = enigma::object_table[" << i->second->id << "]->mask;\n";
                            else
                                wto << "      sprite_index = enigma::objectdata[" << i->second->id << "]->sprite;\n"
                                        << "      mask_index = enigma::objectdata[" << i->second->id << "]->mask;\n";
                            wto << "      visible = enigma::objectdata[" << i->second->id << "]->visible;\n      solid = enigma::objectdata[" << i->second->id << "]->solid;\n";
                            wto << "      persistent = enigma::objectdata[" << i->second->id << "]->persistent;\n";
                            
            wto << "      activate();\n";
                
                
                        // Coordinates
                            wto << "      x = enigma_genericconstructor_newinst_x, y = enigma_genericconstructor_newinst_y;\n";

                    wto << "      enigma::constructor(this);\n";
                    wto << "    }\n\n";

        
                    wto << "    void activate()\n    {\n";
            if (setting::inherit_objects && has_parent) {
                // Have to remove the one the parent added so we can add our own
                wto << "      depth.remove();\n";
            }
            // Depth iterator used for draw events in graphics system screen_redraw
            wto << "      depth.init(enigma::objectdata[" << i->second->id << "]->depth, this);\n";
                        // Instance system interface
            if (!setting::inherit_objects || !has_parent) {
                            wto << "      ENOBJ_ITER_me = enigma::link_instance(this);\n";
                for (po_i her = i; her != parsed_objects.end(); her = parsed_objects.find(her->second->parent))
                                wto << "      ENOBJ_ITER_myobj" << her->second->id << " = enigma::link_obj_instance(this, " << her->second->id << ");\n";
            } else {
                wto << "      ENOBJ_ITER_myobj" << i->second->id << " = enigma::link_obj_instance(this, " << i->second->id << ");\n";
            }
                        // Event system interface
                            for (unsigned ii = 0; ii < i->second->events.size; ii++) {
                if (find(parent_defined.begin(), parent_defined.end(), ii) != parent_defined.end()) { continue; }
                                if (!event_is_instance(i->second->events[ii].mainId,i->second->events[ii].id)) {
                                    const string evname = event_get_function_name(i->second->events[ii].mainId,i->second->events[ii].id);
                                    if (event_has_iterator_initialize_code(i->second->events[ii].mainId,i->second->events[ii].id)) {
                                        if (!iscomment(event_get_iterator_initialize_code(i->second->events[ii].mainId,i->second->events[ii].id)))
                                            wto << "      " << event_get_iterator_initialize_code(i->second->events[ii].mainId,i->second->events[ii].id) << ";\n";
                                    } else {
                                        wto << "      ENOBJ_ITER_myevent_" << evname << " = enigma::event_" << evname << "->add_inst(this);\n";
                    }
                                }
                }
                            for (map<int,cspair>::iterator it = nemap.begin(); it != nemap.end(); it++)
                                wto << "      ENOBJ_ITER_myevent_" << it->second.s << " = enigma::event_" << it->second.s << "->add_inst(this);\n";
                
                    wto << "    }\n";


                    // Destructor
                    wto <<   "    \n    ~OBJ_" <<  i->second->name << "()\n    {\n";
                        
            if (!setting::inherit_objects || !has_parent) {
                wto << "      delete vmap;\n";
                wto << "      enigma::winstance_list_iterator_delete(ENOBJ_ITER_me);\n";
                for (po_i her = i; her != parsed_objects.end(); her = parsed_objects.find(her->second->parent))
                    wto << "      delete ENOBJ_ITER_myobj" << her->second->id << ";\n";
            } else {
                wto << "      delete ENOBJ_ITER_myobj" << i->second->id << ";\n";
            }
                        for (unsigned ii = 0; ii < i->second->events.size; ii++) {
                if (setting::inherit_objects && find(parent_defined.begin(), parent_defined.end(), ii) != parent_defined.end()) { continue; }
                            if (!event_is_instance(i->second->events[ii].mainId,i->second->events[ii].id)) {
                                if (event_has_iterator_delete_code(i->second->events[ii].mainId,i->second->events[ii].id)) {
                                    if (!iscomment(event_get_iterator_delete_code(i->second->events[ii].mainId,i->second->events[ii].id)))
                                        wto << "      " << event_get_iterator_delete_code(i->second->events[ii].mainId,i->second->events[ii].id) << ";\n";
                                } else
                                    wto << "      delete ENOBJ_ITER_myevent_" << event_get_function_name(i->second->events[ii].mainId,i->second->events[ii].id) << ";\n";
                            }
            }
                        for (map<int,cspair>::iterator it = nemap.begin(); it != nemap.end(); it++) // The stacked ones should have their root exported
                            wto << "      delete ENOBJ_ITER_myevent_" << it->second.s << ";\n";
                    wto << "    }\n";

                wto << "  };\n";
        parsed.push_back(i->first);
        i++;
            }
            wto << "\n  objectstruct objs[] = {\n  ";
            int objcunt = 0, obmx = 0;
            for (po_i i = parsed_objects.begin(); i != parsed_objects.end(); i++, objcunt++)
            {
                wto << "{" << i->second->sprite_index << "," << i->second->solid << "," << i->second->visible << "," << i->second->depth << "," << i->second->persistent << "," << i->second->mask_index << "," << i->second->parent << "," << i->second->id << "}, ";
                if (i->second->id >= obmx) obmx = i->second->id;
            }
            wto << "  };\n";
            wto << "  int objectcount = " << objcunt << ";\n";
            wto << "  int obj_idmax = " << obmx+1 << ";\n";
        wto << "}\n";
    //wto.close();



    /* NEXT FILE `******************************************\
    ** Object functions: events, constructors, other codes.
    ********************************************************/

        cout << "DBGMSG 1" << endl;
    wto.open((workdir +"Preprocessor_Environment_Editable/IDE_EDIT_objectfunctionality.h").c_str(),ios_base::out);
        wto << license;

        cout << "DBGMSG 2" << endl;
        // Export globalized scripts
        for (int i = 0; i < es->scriptCount; i++)
        {
            parsed_script* scr = scr_lookup[es->scripts[i].name];
            const char* comma = "";
            wto << "variant _SCR_" << es->scripts[i].name << "(";
            for (int argn = 0; argn < scr->globargs; argn++) //it->second gives max argument count used
            {
                wto << comma << "variant argument" << argn;
                comma = ", ";
            }
            wto << ")\n{\n  ";
            parsed_event& upev = scr->pev_global?*scr->pev_global:scr->pev;
            print_to_file(upev.code,upev.synt,upev.strc,upev.strs,2,wto);
            wto << "\n  return 0;\n}\n\n";
        }

        cout << "DBGMSG 3" << endl;
        // Export everything else
        for (po_i i = parsed_objects.begin(); i != parsed_objects.end(); i++)
        {
        cout << "DBGMSG 4" << endl;
            for (unsigned ii = 0; ii < i->second->events.size; ii++)
            if  (i->second->events[ii].code != "")
            {
            cout << "DBGMSG 4-1" << endl;
                const int mid = i->second->events[ii].mainId, id = i->second->events[ii].id;
                string evname = event_get_function_name(mid,id);
            bool defined_inherited = false;
            if (setting::inherit_objects) {
            parent = parsed_objects.find(i->second->parent);
            if (parent != parsed_objects.end()) {
                parent_defined = parent_definitions.find(i->second->parent)->second;
                if (find(parent_defined.begin(), parent_defined.end(), ii) != parent_defined.end()) {
                    wto << "#define event_inherited OBJ_" + parent->second->name + "::myevent_" + evname + "\n";
                        defined_inherited = true;
                }
            }
        }
        
        cout << "DBGMSG 4-2" << endl;
                wto << "variant enigma::OBJ_" << i->second->name << "::myevent_" << evname << "()\n{\n  ";
                    if (!event_execution_uses_default(i->second->events[ii].mainId,i->second->events[ii].id))
                        wto << "enigma::temp_event_scope ENIGMA_PUSH_ITERATOR_AND_VALIDATE(this);\n  ";
        cout << "DBGMSG 4-3" << endl;
                    if (event_has_sub_check(mid, id))
                        wto << event_get_sub_check_condition(mid, id) << endl;
                    if (event_has_const_code(mid, id))
                        wto << event_get_const_code(mid, id) << endl;
                    if (event_has_prefix_code(mid, id))
                        wto << event_get_prefix_code(mid, id) << endl;
        cout << "DBGMSG 4-4" << endl;
                    print_to_file(i->second->events[ii].code,i->second->events[ii].synt,i->second->events[ii].strc,i->second->events[ii].strs,2,wto);
                    if (event_has_suffix_code(mid, id))
                        wto << event_get_suffix_code(mid, id) << endl;
        cout << "DBGMSG 4-5" << endl;
                wto << "\n  return 0;\n}\n";
        
        if (defined_inherited) {
            wto << "#undef event_inherited\n";
        }
        
            }
        cout << "DBGMSG 5" << endl;

    
            parsed_object* t = i->second;
            for (parsed_object::funcit it = t->funcs.begin(); it != t->funcs.end(); it++) //For each function called by this object
            {
                map<string,parsed_script*>::iterator subscr = scr_lookup.find(it->first); //Check if it's a script
                if (subscr != scr_lookup.end() // If we've got ourselves a script
                and subscr->second->pev_global) // And it has distinct code for use at the global scope (meaning it's more efficient locally)
                {
                    const char* comma = "";
                    wto << "variant enigma::OBJ_" << i->second->name << "::_SCR_" << it->first << "(";
                    for (int argn = 0; argn < it->second; argn++) //it->second gives max argument count used
                    {
                        wto << comma << "variant argument" << argn;
                        comma = ", ";
                    }
                    wto << ")\n{\n  ";
                    print_to_file(subscr->second->pev.code,subscr->second->pev.synt,subscr->second->pev.strc,subscr->second->pev.strs,2,wto);
                    wto << "\n  return 0;\n}\n\n";
                }
            }
        cout << "DBGMSG 6" << endl;
        }
        cout << "DBGMSG 7" << endl;
    
    parent_defined.clear();
    parent_definitions.clear();

        wto << "namespace enigma\n{\n"
        "  callable_script callable_scripts[] = {\n";
        int scr_count = 0;
        for (int i = 0; i < es->scriptCount; i++)
        {
            while (es->scripts[i].id > scr_count)
            {
                    wto << "    { NULL, -1 },\n";
                    scr_count++;
            }
            scr_count++;
            wto << "    { (variant(*)())_SCR_" << es->scripts[i].name << ", " << scr_lookup[es->scripts[i].name]->globargs << " },\n";
        }
        wto << "  };\n  \n";

        cout << "DBGMSG 8" << endl;
        wto << "  void constructor(object_basic* instance_b)\n  {\n"
        "    //This is the universal create event code\n    object_locals* instance = (object_locals*)instance_b;\n    \n"
        "    instance->xstart = instance->x;\n    instance->ystart = instance->y;\n    instance->xprevious = instance->x;\n    instance->yprevious = instance->y;\n\n"
        "    instance->gravity=0;\n    instance->gravity_direction=270;\n    instance->friction=0;\n    \n"
        "    \n"
        "    instance->image_alpha = 1.0;\n    instance->image_angle = 0;\n    instance->image_blend = 0xFFFFFF;\n    instance->image_index = 0;\n"
        "    instance->image_speed  = 1;\n    instance->image_xscale = 1;\n    instance->image_yscale = 1;\n    \n"
        "instancecount++;\n    instance_count++;\n  }\n}\n";
    //wto.close();

    return 0;
}
#endif
