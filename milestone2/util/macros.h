#ifndef __MACROS_H__
#define __MACROS_H__

#define container_of(PTR, STRUCT_TYPE, MEMBER_NAME) \
    (STRUCT_TYPE *)((uint8_t *)(PTR) - offsetof(STRUCT_TYPE, MEMBER_NAME))

#endif