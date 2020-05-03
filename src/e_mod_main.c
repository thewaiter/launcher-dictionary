#include "e.h"
#include "e_mod_main.h"
#include "evry_api.h"

#define WRAP 50
#define STARDICT   1

typedef struct _Plugin Plugin;
typedef struct _Module_Config Module_Config;

struct _Plugin
{
  Evry_Plugin base;
  struct
  {
    Ecore_Event_Handler *data;
    Ecore_Event_Handler *del;
  } handler;
  Ecore_Exe *exe;
  const char *command;
  const char *input;
  Eina_Bool is_first;
};

struct _Module_Config
{
  int version;

  const char *command;
  const char *custom;
  int engine;

  E_Config_Dialog *cfd;
  E_Module *module;
};

static char *engines[] =
  {
    "sdcv %s",
    "cust %s "
  };

static const Evry_API *evry = NULL;
static Evry_Module *evry_module = NULL;
static Module_Config *_conf;
static Evry_Plugin *_plug = NULL;
static char _config_path[] =  "launcher/everything-dict";
static char _config_domain[] = "module.everything-dict";
static char _module_icon[] = "accessories-dictionary";
static E_Config_DD *_conf_edd = NULL;
static const char TRIGGER[] = "d ";
static int instances = 0;

static Eina_Bool
_exe_restart(Plugin *p)
{
   char cmd[1024];
   const char *command_val;
   int len;

   if (p->command && (p->command[0] != '\0'))
       command_val = p->command;
   else if (_conf->command)
       command_val = _conf->command;
   else
       command_val = "";

  
   len = snprintf(cmd, sizeof(cmd),
        engines[_conf->engine - 1], command_val);
   
     printf("CMD je: %s\n",cmd);
   if (len >= (int)sizeof(cmd))
     return 0;

   if (p->exe)
     ecore_exe_quit(p->exe);
   p->exe = ecore_exe_pipe_run
     (cmd,
      ECORE_EXE_PIPE_READ | ECORE_EXE_PIPE_READ_LINE_BUFFERED |
      ECORE_EXE_PIPE_WRITE,
      NULL);
   p->is_first = 1;
   return !!p->exe;
}

static const char *
_space_skip(const char *line)
{
   for (; *line != '\0'; line++)
     if (!isspace(*line))
       break;
   return line;
}

static char *
_lower_case(const char *line)
{ 
   char *str = strdup(line);
   char *start;
   start = str; 
   for (; *str != '\0'; str++)
   *str = tolower(*str);

   return start;
}


static Evas_Object *
_icon_get(Evry_Item *it, Evas *e)
{
   Evas_Object *o = NULL;
             o = e_icon_add(e);
             e_util_icon_theme_set(o, "accessories-dictionary");
        return o;
}

static Evas_Object *
_no_icon_get(Evry_Item *it, Evas *e)
{
   Evas_Object *o = NULL;
             o = e_icon_add(e);
             e_util_icon_theme_set(o, "");
        return o;
}

static void
_item_add(Plugin *p, const char *word, int word_size, int prio)
{
   Evry_Item *it;
   char cmd[30];
   
   if(strstr(word, "Found") != NULL) return;
   
   snprintf(cmd, sizeof(cmd), "-->%s", p->input);
   if (strstr(word, cmd) != NULL)  return;
      
   if (strstr(word, "-->") != NULL) 
      it = EVRY_ITEM_NEW(Evry_Item, p, NULL, _icon_get, NULL);
   else 
      it = EVRY_ITEM_NEW(Evry_Item, p, NULL, _no_icon_get, NULL);
   //~ it->browseable = EINA_TRUE;
   if (!it) return;
   it->priority = prio;
   it->label = eina_stringshare_add_length(word, word_size);
   EVRY_PLUGIN_ITEM_APPEND(p, it);
}


static void
_suggestions_add(Plugin *p, const char *line)
{
	const char *s, *right_margin;
	int length;
	
	length = strlen(line);
	s = line;
	
	
	//word wrap adapted from	https://c-for-dummies.com/blog/?p=682
	
	 while(*s) 
    {
        if(length <= WRAP)
        {
           _item_add(p, s, WRAP, 1);     /* display string */
            return;      /* and leave */
        }
        right_margin = s + WRAP;
        while(!isspace(*right_margin))
        {
            right_margin--;
            if( right_margin == s)
            {
                right_margin += WRAP;
                while(!isspace(*right_margin))
                {
                    if( *right_margin == '\0')
                        break;
                    right_margin++;
                }
            }
        }
        _item_add(p, s, right_margin - s, 1); ;
        length -= right_margin-s+1;      /* +1 for the space */
        s = right_margin + 1;
    }    
}

static Eina_Bool
_cb_data(void *data, int type __UNUSED__, void *event)
{
   GET_PLUGIN(p, data);
   Ecore_Exe_Event_Data *e = event;
   Ecore_Exe_Event_Data_Line *l;
   const char *word;
 
   if (e->exe != p->exe)
     return ECORE_CALLBACK_PASS_ON;

   EVRY_PLUGIN_ITEMS_FREE(p);

   word = p->input; 
   for (l = e->lines; l && l->line; l++)
     {
	   if (p->is_first)
		 {
			ERR("DICT: %s", l->line);
			p->is_first = 0;
			continue;
		 }
     _suggestions_add(p, l->line);
     }
     
   if (p->base.items)
    EVRY_PLUGIN_UPDATE(p, EVRY_UPDATE_ADD);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_del(void *data, int type __UNUSED__, void *event)
{
   Plugin *p = data;
   Ecore_Exe_Event_Del *e = event;

   if (e->exe != p->exe)
     return ECORE_CALLBACK_PASS_ON;

   p->exe = NULL;
   return ECORE_CALLBACK_PASS_ON;
}

static Evry_Plugin *
_begin(Evry_Plugin *plugin, const Evry_Item *it __UNUSED__)
{
   Plugin *p;

   if (instances > 1)
     return NULL;

   EVRY_PLUGIN_INSTANCE(p, plugin);

   instances++;

   return EVRY_PLUGIN(p);
}

static int
_action(Evry_Action *act)
{
   char *tmp = evry->util_url_unescape(act->it1.item->label, 0);
   printf("This is an action %s\n", tmp);
 }    

static int
_fetch(Evry_Plugin *plugin, const char *input)
{
   GET_PLUGIN(p, plugin);
   const char *s;
   unsigned int len;
   unsigned int inp_len;

   if (!input) return 0;

   if (!p->handler.data)
     {
   if (!p->handler.data)
     p->handler.data = ecore_event_handler_add
       (ECORE_EXE_EVENT_DATA, _cb_data, p);
   if (!p->handler.del)
     p->handler.del = ecore_event_handler_add
       (ECORE_EXE_EVENT_DEL, _cb_del, p);

   if (!_exe_restart(p))
     return 0;
     }

   input = _space_skip(input);
   input = _lower_case(input);
   for (s = input; *s != '\0'; s++)
     ;
   for (s--; s > input; s--)
     if (!isspace(*s))
       break;

   len = s - input + 1;
   if (len < 1)
     return 0;
   
   inp_len = strlen(input)-1;
   printf("Word is: %s %d\n", input, *(input + inp_len));
   if ((*(input + inp_len) >= 48) && (*(input + inp_len) <= 57)) {
        input = input + inp_len;        
	}
  
   input = eina_stringshare_add_length(input, len);
   IF_RELEASE(p->input);
   if (p->input == input)
     return 0;

   p->input = input;
   if (!p->exe) return 0;
   
   ecore_exe_send(p->exe, (char *)p->input, len);
   ecore_exe_send(p->exe, "\n", 1); //this means the enter key press send to the prog.

   return EVRY_PLUGIN_HAS_ITEMS(p);
}

static void
_finish(Evry_Plugin *plugin)
{
   GET_PLUGIN(p, plugin);

   EVRY_PLUGIN_ITEMS_FREE(p);

   instances--;

   if (p->handler.data)
     ecore_event_handler_del(p->handler.data);

   if (p->handler.del)
     ecore_event_handler_del(p->handler.del);

   if (p->exe)
     {
   ecore_exe_quit(p->exe);
   ecore_exe_free(p->exe);
     }
   IF_RELEASE(p->command);
   IF_RELEASE(p->input);

   E_FREE(p);
}

static int
_plugins_init(const Evry_API *_api)
{
   evry = _api;
   Evry_Action *act;

   if (!evry->api_version_check(EVRY_API_VERSION))
     return EINA_FALSE;

   _plug = EVRY_PLUGIN_BASE(N_("Dictionary"), _module_icon, EVRY_TYPE_TEXT,
             _begin, _finish, _fetch);
   _plug->config_path = _config_path;
   _plug->history     = EINA_FALSE;
   _plug->async_fetch = EINA_TRUE;

   if (evry->plugin_register(_plug, EVRY_PLUGIN_SUBJECT, 100))
     {
   Plugin_Config *pc = _plug->config;
   pc->view_mode = VIEW_MODE_LIST;
   pc->aggregate = EINA_FALSE;
   /* pc->top_level = EINA_FALSE; */
   pc->trigger = eina_stringshare_add(TRIGGER);
   pc->trigger_only = EINA_TRUE;
   pc->min_query = 1;
     }
   
   #define ACTION_NEW(_name, _type, _icon, _action, _check, _method)	\
   act = EVRY_ACTION_NEW(_name, _type, 0, _icon, _action, _check);	\
   act->remember_context = EINA_TRUE;					\
   EVRY_ITEM_DATA_INT_SET(act, _method);				\
   EVRY_ITEM(act)->icon_get = &_icon_get;				\
   evry->action_register(act, 0);				       	\
   //~ actions = eina_list_append(actions, act);				\
   
    ACTION_NEW(N_("Select"), EVRY_TYPE_TEXT, "edit-find",
	      _action, NULL, NULL);

   return EINA_TRUE;
}

static void
_plugins_shutdown(void)
{
   EVRY_PLUGIN_FREE(_plug);
}

/***************************************************************************/

struct _E_Config_Dialog_Data
{
  int  engine;
  char *custom;
  char *command;
};

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static void _fill_data(E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);

static E_Config_Dialog *
_conf_dialog(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd = NULL;
   E_Config_Dialog_View *v = NULL;

   if (e_config_dialog_find(_config_path, _config_path))
     return NULL;

   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return NULL;

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;

   cfd = e_config_dialog_new(con, _("Dictionary"),
              _config_path, _config_path, _module_icon, 0, v, NULL);

   _conf->cfd = cfd;
   return cfd;
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o = NULL, *of = NULL, *ow = NULL;
   E_Radio_Group *rg;
   o = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, _("General"), 0);
   e_widget_framelist_content_align_set(of, 0.0, 0.0);

   rg = e_widget_radio_group_new(&cfdata->engine);

   ow = e_widget_label_add(evas, _("Dictionary engines"));
   e_widget_framelist_object_append(of, ow);

   ow = e_widget_radio_add(evas, _("Stardict (default)"), 1, rg);
   e_widget_framelist_object_append(of, ow);

   ow = e_widget_radio_add(evas, _("Custom"), 0, rg);
   e_widget_disabled_set(ow, 0);
   e_widget_framelist_object_append(of, ow);

   ow = e_widget_label_add(evas, _("Custom engine"));
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_entry_add(evas, &cfdata->custom, NULL, NULL, NULL);
   e_widget_disabled_set(ow, 0);
   e_widget_framelist_object_append(of, ow);

   ow = e_widget_label_add(evas, _("Engine command"));
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_entry_add(evas, &cfdata->command, NULL, NULL, NULL);
   e_widget_framelist_object_append(of, ow);

   e_widget_list_object_append(o, of, 1, 1, 0.5);
   return o;
}

static void *
_create_data(E_Config_Dialog *cfd __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = NULL;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   E_FREE(cfdata->custom);
   E_FREE(cfdata->command);
   _conf->cfd = NULL;
   E_FREE(cfdata);
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
#define CP(_name) cfdata->_name = _conf->_name ? strdup(_conf->_name) : strdup("");
#define C(_name) cfdata->_name = _conf->_name;
   C(engine);
   CP(custom);
   CP(command);
#undef CP
#undef C
}

static int
_basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
#define CP(_name)             \
   if (_conf->_name)             \
     eina_stringshare_del(_conf->_name);     \
   _conf->_name = eina_stringshare_add(cfdata->_name);
#define C(_name) _conf->_name = cfdata->_name;
   C(engine);
   CP(custom);
   CP(command);
#undef CP
#undef C

   e_config_domain_save(_config_domain, _conf_edd, _conf);
   e_config_save_queue();
   return 1;
}

static void
_conf_new(void)
{
   _conf = E_NEW(Module_Config, 1);
   _conf->version = (MOD_CONFIG_FILE_EPOCH << 16);

#define IFMODCFG(v) if ((_conf->version & 0xffff) < v) {
#define IFMODCFGEND }

   /* setup defaults */
   IFMODCFG(0x008d);
   _conf->engine = 1;
   _conf->custom = NULL;
   _conf->command = eina_stringshare_add("-e");
   IFMODCFGEND;

   _conf->version = MOD_CONFIG_FILE_VERSION;

   e_config_save_queue();
}

static void
_conf_free(void)
{
   if (_conf)
     {
   if (_conf->custom) eina_stringshare_del(_conf->custom);
   if (_conf->command)   eina_stringshare_del(_conf->command);

   E_FREE(_conf);
     }
}

static void
_conf_init(E_Module *m)
{
   char title[4096];

   e_configure_registry_category_add
     ("launcher", 80, _("Launcher"), NULL, "modules-launcher");

   snprintf(title, sizeof(title), "%s: %s", _("Launcher Plugin"), _("Launcher Dictionary"));
   e_configure_registry_item_add(_config_path, 110, title,
             NULL, _module_icon, _conf_dialog);

   _conf_edd = E_CONFIG_DD_NEW("Module_Config", Module_Config);

#undef T
#undef D
#define T Module_Config
#define D _conf_edd
   E_CONFIG_VAL(D, T, version, INT);
   E_CONFIG_VAL(D, T, engine, INT);
   E_CONFIG_VAL(D, T, custom, STR);
   E_CONFIG_VAL(D, T, command, STR);
#undef T
#undef D

   _conf = e_config_domain_load(_config_domain, _conf_edd);

   if (_conf && !e_util_module_config_check(_("Everything Dictionary"),
                   _conf->version,
                   MOD_CONFIG_FILE_VERSION))
     _conf_free();

   if (!_conf) _conf_new();

   _conf->module = m;
}

static void
_conf_shutdown(void)
{
   e_configure_registry_item_del(_config_path);
   e_configure_registry_category_del("launcher");

   _conf_free();

   E_CONFIG_DD_FREE(_conf_edd);
}

/***************************************************************************/

EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
   "everything-dict"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   _conf_init(m);

   EVRY_MODULE_NEW(evry_module, evry, _plugins_init, _plugins_shutdown);

   e_module_delayed_set(m, 1);

   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   EVRY_MODULE_FREE(evry_module);

   _conf_shutdown();

   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   e_config_domain_save(_config_domain, _conf_edd, _conf);
   return 1;
}

/***************************************************************************/
