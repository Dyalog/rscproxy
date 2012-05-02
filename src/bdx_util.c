/*******************************************************************************
 *  BDX: Binary Data eXchange format library
 *  Copyright (C) 1999--2009 Thomas Baier
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  helper functions for BDX (e.g. memory management)
 *
 ******************************************************************************/

#ifndef  _BDX_UTIL_H_
#include "bdx_util.h"
#endif

#include "bdx.h"
#if defined(__WINDOWS__)
//#include "bdx_com.h"
#endif

#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#if defined(__WINDOWS__)
#include <windows.h>
#endif
#include <stdarg.h>
#include <stdio.h>

#ifndef _IN_RPROXY_
int bdx_trace_printf(char const* pFormat,...)
{
  static char __tracebuf[2048];

  va_list lArgs;
  va_start(lArgs, pFormat);
  vsnprintf(__tracebuf, 2048, pFormat, lArgs);
#if defined(__WINDOWS__)
  OutputDebugString(__tracebuf);
#else
#if 0
  /*
   * removed, because R packages must not write to stdout: 
   * Mail KH, 2012-05-02, 11:58
   */
  printf("%s",__tracebuf);
#endif
#endif
  return 0;
}

#endif

/* 05-05-15 | baier | now WINAPI */
/* 07-04-11 | baier | now SYSCALL */
/* 08-05-04 | baier | void parameters */
BDX_Data* SYSCALL bdx_alloc(void)
{
  BDX_Data* lData = NULL;

  lData = (BDX_Data*) malloc(sizeof(BDX_Data));
  memset(lData,0,sizeof(BDX_Data));
  lData->version = BDX_VERSION;

  return lData;
}

/* 05-05-13 | baier | release stream for BDX_HANDLE */
/* 05-05-15 | baier | now WINAPI */
/* 07-04-11 | baier | now SYSCALL */
void SYSCALL bdx_free (struct _BDX_Data* data)
{
  /* BDX_TRACE(printf("bdx_free #1: %d\n",_heapchk())); Sleep(2000); */
  if (data == NULL) {
    return;
  }

  /* even for scalars, the dimensions are correctly initialized (just a 1 */
  /* dimension structure with just one element) */

  /* we do not assert these conditions, so partly initialized BDX_Data can
   * be released using bdx_free() */

  /* is it a character array/vector/scalar? if yes, release the data */
  /* BDX_TRACE(printf("bdx_free #2: %d\n",_heapchk())); Sleep(2000); */
  if(data->data.raw_data
     && (((data->type & BDX_SMASK) == BDX_STRING)
	 || ((data->type & BDX_SMASK) == BDX_HANDLE)
	 || ((data->type & BDX_SMASK) == BDX_GENERIC))) {
    /* treat the data as a 1-dimensional array with |dim1|*|dim2|*... */
    /* elements */
    unsigned long total_size = 1;
    unsigned long i;

    for(i = 0;i < data->dim_count;i++) {
      total_size *= data->dimensions[i];
    }

    for(i = 0;i < total_size;i++) {
      switch(data->type & BDX_SMASK) {
      case BDX_STRING:
	if(data->data.raw_data[i].string_value != NULL) {
	  free (data->data.raw_data[i].string_value);
	}
	break;
      case BDX_HANDLE:
#if defined(__WINDOWS__)
	if(data->data.raw_data[i].ptr != NULL) {
	  LPSTREAM lStream = (LPSTREAM) data->data.raw_data[i].ptr;
	  lStream->lpVtbl->Release(lStream);
	}
#endif
	break;
      case BDX_GENERIC:
	switch(data->data.raw_data_with_type[i].type & BDX_SMASK) {
	case BDX_STRING:
	  if(data->data.raw_data_with_type[i].raw_data.string_value != NULL) {
	    free (data->data.raw_data_with_type[i].raw_data.string_value);
	  }
	  break;
	case BDX_HANDLE:
#if defined(__WINDOWS__)
	  if(data->data.raw_data_with_type[i].raw_data.ptr != NULL) {
	    LPSTREAM lStream = (LPSTREAM) data->data.raw_data_with_type[i].raw_data.ptr;
	    lStream->lpVtbl->Release(lStream);
	  }
#endif
	  break;
	}
      default:
	/* do nothing */
	break;
      }
    }
  }
  
  /* BDX_TRACE(printf("bdx_free #3: %d\n",_heapchk())); Sleep(2000); */
  /* free data pointers, dimension data and the data block itself */
  if(data->data.raw_data) free(data->data.raw_data);
  /* BDX_TRACE(printf("bdx_free #4: %d\n",_heapchk())); Sleep(2000); */
  if(data->dimensions) free(data->dimensions);
  /* BDX_TRACE(printf("bdx_free #5: %d\n",_heapchk())); Sleep(2000); */
  free(data);
  /* BDX_TRACE(printf("bdx_free #6: %d\n",_heapchk())); Sleep(2000); */
}


static int bdx_trace_element(BDX_Type pType,BDX_RawData pData,int pIndex)
{
  switch(pType & BDX_SMASK) {
  case BDX_BOOL:
    BDX_TRACE(printf("  elem %02d: BDX_BOOL,    value is %s\n",pIndex,
		pData.bool_value ? "TRUE" : "FALSE"));
    return 0;
  case BDX_INT:
    BDX_TRACE(printf("  elem %02d: BDX_INT,     value is %d\n",pIndex,
		pData.int_value));
    return 0;
  case BDX_DOUBLE:
    BDX_TRACE(printf("  elem %02d: BDX_DOUBLE,  value is %g\n",pIndex,
		pData.double_value));
    return 0;
  case BDX_STRING:
    BDX_TRACE(printf("  elem %02d: BDX_STRING,  value is %s\n",pIndex,
		pData.string_value));
    return 0;
  case BDX_SPECIAL:
    BDX_TRACE(printf("  elem %02d: BDX_SPECIAL, value is %s\n",pIndex,
		(pData.special_value == BDX_SV_NULL) ? "BDX_SV_NULL" :
		(pData.special_value == BDX_SV_NA) ? "BDX_SV_NA" :
		(pData.special_value == BDX_SV_DIV0) ? "BDX_SV_DIV0" :
		(pData.special_value == BDX_SV_NAN) ? "BDX_SV_NAN" :
		(pData.special_value == BDX_SV_INF) ? "BDX_SV_INF" :
		(pData.special_value == BDX_SV_NINF) ? "BDX_SV_NINF" :
		"unknown"));
    return 0;
  case BDX_HANDLE:
    BDX_TRACE(printf("  elem %02d: BDX_HANDLE,  value is %p\n",pIndex,
		pData.ptr));
    return 0;
  case BDX_POINTER:
    BDX_TRACE(printf("  elem %02d: BDX_POINTER, value is %p\n",pIndex,
		pData.ptr));
    return 0;
  default:
    BDX_TRACE(printf("  elem %02d: unknown type %10x\n",pIndex,pType));
    return 0;
  }
}

/* 05-05-15 | baier | now WINAPI */
/* 07-04-11 | baier | now SYSCALL */
void SYSCALL bdx_trace(struct _BDX_Data* pData)
{
  unsigned long lTotalElements = 1;
  unsigned long i;

  if(pData->version != BDX_VERSION) {
    BDX_TRACE(printf("bdx_trace: unsupported BDX version %d, expected %d\n",
		pData->version,BDX_VERSION));
    return;
  }
  switch(pData->type & BDX_CMASK) {
  case BDX_SCALAR:
    BDX_TRACE(printf("bdx_trace: scalar found\n"));
    bdx_trace_element(pData->type,pData->data.raw_data[0],0);
    return;
  case BDX_ARRAY:
    /* count total number of elements */
    for(i = 0;i < pData->dim_count;i++) {
      lTotalElements *= pData->dimensions[i];
    }
    /* most arrays will have at most 4 dims */
    switch(pData->dim_count) {
    case 1:
      BDX_TRACE(printf("bdx_trace: one-dimensional array with %d elements found\n",
		  pData->dimensions[0]));
      break;
    case 2:
      BDX_TRACE(printf("bdx_trace: two-dimensional array with (%d/%d) elements found\n",
		  pData->dimensions[0],pData->dimensions[1]));
      break;
    case 3:
      BDX_TRACE(printf("bdx_trace: three-dimensional array with (%d/%d/%d) elements found\n",
		  pData->dimensions[0],pData->dimensions[1],
		  pData->dimensions[2]));
      break;
    case 4:
      BDX_TRACE(printf("bdx_trace: four-dimensional array with (%d/%d/%d/%d) elements found\n",
		  pData->dimensions[0],pData->dimensions[1],
		  pData->dimensions[2],pData->dimensions[3]));
      break;
    default:
      BDX_TRACE(printf("bdx_trace: %d-dimensional array with total %d elements found\n",
		  pData->dim_count,lTotalElements));
    }
    
    /* generic array or typed array? */
    if((pData->type & BDX_SMASK) == BDX_GENERIC) {
      BDX_TRACE(printf("  generic array\n"));
    }
    for(i = 0;i < lTotalElements;i++) {
      if((pData->type & BDX_SMASK) == BDX_GENERIC) {
	bdx_trace_element(pData->data.raw_data_with_type[i].type,
			  pData->data.raw_data_with_type[i].raw_data,
			  i);
      } else {
	bdx_trace_element(pData->type,
			  pData->data.raw_data[i],
			  i);
      }
    }
    break;
  default:
    BDX_TRACE(printf("bdx_trace: unknown type (BDX_CMASK) %10x\n",pData->type));
    return;
  }
}
