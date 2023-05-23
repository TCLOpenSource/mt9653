#ifndef _SHARE_STRUCT_
#define _SHARE_STRUCT_
struct key_map_table {
        u32 scancode;
        u32 keycode;
};

struct key_map {
        struct key_map_table   *scan;
        unsigned int               size; /* Max number of entries */
        const char                         *name;
        u32                                        headcode;
};

struct key_map_list {
        struct list_head list;
        struct key_map map;
};

int MIRC_Map_Register(struct key_map_list *map);
void MIRC_Map_UnRegister(struct key_map_list *map);

#endif
