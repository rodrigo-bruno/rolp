/* 
 * File:   nogc.h
 * Author: underscore
 *
 * Created on December 16, 2015, 11:33 PM
 */

#ifndef NOGC_H
#define	NOGC_H

#define DEBUG_OBJ_ALLOC 0
#define DEBUG_SLOW_PATH_ALLOC 0
#define DEBUG_LARGE_OBJ_ALLOC 0
#define DEBUG_TLAB_ALLOC 0
#define DEBUG_ANNO_ALLOC 0
#define DEBUG_ASM_ALLOC 0
#define DEBUG_NEW_GEN 1
#define DEBUG_COLLECT_GEN 1
#define DEBUG_ALLOC_REGION 0
#define DEBUG_MINOR_CC 0
#define DEBUG_PRINT_REGIONS 0
#define DEBUG_SEND_FREGIONS 0

/**
 * If enabled, heap dumps will write objects' identity hash as their object's
 * ID instead of their memory address. This is used by the OLR profiler.
 */
#define DUMP_IDENTITY 1

#endif	/* NOGC_H */

