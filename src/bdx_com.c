/*******************************************************************************
 *  BDX: Binary Data eXchange format library
 *  Copyright (C) 1999-2008 Thomas Baier, Erich Neuwirth
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 * 
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 * 
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301, USA.
 *
 *  Conversion functions from VARIANT to BDX and vice versa.
 *
 ******************************************************************************/

#include "SC_system.h"
#if defined(__WINDOWS__)

#define _FORCENAMELESSUNION
#ifndef  _BDX_COM_H_
#include "bdx_com.h"
#endif

#include "bdx.h"
#include "bdx_util.h"
/* #include "com_util.h" */
#include <windows.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <math.h>

/*
 * prototypes for internal helper functions
 */
static int BDXScalar2Variant(BDX_Data* pBDXData,VARIANT* pVariantData);
static int BDXArray2Variant(BDX_Data* pBDXData,VARIANT* pVariantData);
static int BDXBoolArray2Variant(BDX_Data* pBDXData,VARIANT* pVariantData);
static int BDXIntArray2Variant(BDX_Data* pBDXData,VARIANT* pVariantData);
static int BDXDoubleArray2Variant(BDX_Data* pBDXData,VARIANT* pVariantData);
static int BDXDTArray2Variant(BDX_Data* pBDXData,VARIANT* pVariantData);
#if 0
static int BDXCYArray2Variant(BDX_Data* pBDXData,VARIANT* pVariantData);
#endif
static int BDXSpecialArray2Variant(BDX_Data* pBDXData,VARIANT* pVariantData);
static int BDXStringArray2Variant(BDX_Data* pBDXData,VARIANT* pVariantData);
static int BDXGenericArray2Variant(BDX_Data* pBDXData,VARIANT* pVariantData);
static unsigned int GetArrayBounds (BDX_Data* pBDXData,SAFEARRAYBOUND** ppArrayBounds);
static char* MyW2A(wchar_t* w);
static int VariantScalar2BDX (VARIANT VariantData,BDX_Data** ppBDXData);
static int VariantArray2BDX (VARIANT VariantData,BDX_Data** ppBDXData);
static int VariantBoolArray2BDX(VARIANT VariantData,BDX_Data* ppBDXData,
				unsigned int pTotalElements);
static int VariantI2Array2BDX(VARIANT VariantData,BDX_Data* ppBDXData,
			      unsigned int pTotalElements);
static int VariantI4Array2BDX(VARIANT VariantData,BDX_Data* ppBDXData,
			      unsigned int pTotalElements);
static int VariantUI1Array2BDX(VARIANT VariantData,BDX_Data* ppBDXData,
			       unsigned int pTotalElements);
static int VariantR4Array2BDX(VARIANT VariantData,BDX_Data* ppBDXData,
			      unsigned int pTotalElements);
static int VariantR8Array2BDX(VARIANT VariantData,BDX_Data* ppBDXData,
			      unsigned int pTotalElements);
static int VariantDATEArray2BDX(VARIANT VariantData,BDX_Data* ppBDXData,
				unsigned int pTotalElements);
#if 0
static int VariantCYArray2BDX(VARIANT VariantData,BDX_Data* ppBDXData,
			      unsigned int pTotalElements);
#endif
static int VariantErrorArray2BDX(VARIANT VariantData,BDX_Data* ppBDXData,
			      unsigned int pTotalElements);
static int VariantStringArray2BDX(VARIANT VariantData,BDX_Data* ppBDXData,
				  unsigned int pTotalElements);
static int VariantVariantArray2BDX(VARIANT VariantData,BDX_Data* ppBDXData,
				   unsigned int pTotalElements);
static unsigned long getSpecialValueFromSCODE(SCODE pSCODE);
static SCODE getSCODEFromSpecialValue(unsigned long pSpecialVal);

static double __VariantDATEVal2BDXDTVal(double pVariantDATEVal);
static double __BDXDTVal2VariantDATEVal(double pBDXDTVal);

/*@FUNC**********************************************************************/
/** convert a Variant DATE value to a BDX DT value.
*
* @param pVariantDATEVal the Variant DATE value
* @return double the BDX DT value
* @exception 
* @see       
*//***************************************************************************
** 08-04-11 | baier | CREATED
*****************************************************************************/
double __VariantDATEVal2BDXDTVal
(
  double pVariantDATEVal
)
{
  SYSTEMTIME lSysTime;
  struct tm lTm;
  double lRc;

  /*
   * convert from VT_DATE to BDX_DT:
   *   1. convert from VT_DATE to SYSTEMTIME
   *   2. convert from SYSTEMTIME to struct tm
   *   3. convert from struct tm to time_t
   *   4. build BDX_DT from time_t and milliseconds-part of SYSTEMTIME
   */
  VariantTimeToSystemTime(pVariantDATEVal,&lSysTime);
  if(pVariantDATEVal < 1) {
    /* "normalize" the date to 1970-01-01 */
    time_t lNow;
    time(&lNow);
    lTm = *(localtime(&lNow));
  } else {
    lTm.tm_mday = lSysTime.wDay;
    lTm.tm_mon = lSysTime.wMonth - 1;
    lTm.tm_year = lSysTime.wYear - 1900;
  }
  lTm.tm_sec = lSysTime.wSecond;
  lTm.tm_min = lSysTime.wMinute;
  lTm.tm_hour = lSysTime.wHour;
  lRc = mktime(&lTm) + lSysTime.wMilliseconds / 1000.0;
  return lRc;
}


/*@FUNC**********************************************************************/
/** convert a BDX DT value to a Variant DATE value.
*
* @param pBDXDTVal the BDX DT value
* @return double the Variant DATE value
* @exception 
* @see       
*//***************************************************************************
** 08-04-11 | baier | CREATED
*****************************************************************************/
double __BDXDTVal2VariantDATEVal
(
  double pBDXDTVal
)
{
  SYSTEMTIME lSysTime;
  time_t lTimeT = (time_t) pBDXDTVal;
  struct tm* lTm;
  double lRc;

  /*
   * convert from something like a "time_t" to DATE:
   *    1. convert time_t-fraction of BDX_DT to SystemTime
   *    2. set milliseconds from rest of BDX_DT
   */
  lTm = localtime(&lTimeT);
  if(!lTm) {
    return 0.0;
  }
  lSysTime.wSecond = lTm->tm_sec;
  lSysTime.wMinute = lTm->tm_min;
  lSysTime.wHour = lTm->tm_hour;
  lSysTime.wDay = lTm->tm_mday;
  lSysTime.wMonth = lTm->tm_mon + 1;
  lSysTime.wYear = lTm->tm_year + 1900;
  lSysTime.wDayOfWeek = lTm->tm_wday;
  lSysTime.wMilliseconds = (WORD) (1000 * fmod(pBDXDTVal,1.0));
  SystemTimeToVariantTime(&lSysTime,&lRc);
  return lRc;
}

BSTR ANSI2BSTR(char const* str)
{
  OLECHAR* _str = com_getOLECHAR (str);
  BSTR bstr = SysAllocString (_str);
  free (_str);
  return bstr;
}

/** get the ANSI string from an BSTR */
/* 04-06-30 | baier | wrong parameters for WideCharToMultiByte */
/* 04-10-20 | baier | check for NULL first */
char* BSTR2ANSI(BSTR bstr)
{
  UINT count = 0;
  char* str;

  if(bstr == NULL) {
    return NULL;
  }
  count = SysStringLen (bstr);
  str = (char*) malloc (count+1);
  str[count] = 0x0;
  WideCharToMultiByte(CP_ACP,0,bstr,-1,str,count+1,NULL,NULL);
  return str;
}

OLECHAR* com_getOLECHAR(char const* str)
{
  int chars;
  OLECHAR* _str;

  chars = MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,str,-1,NULL,0);
  if (chars == 0) {
    return NULL;
  }
  _str = (OLECHAR*) calloc(chars,sizeof (OLECHAR));
  chars = MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,str,-1,_str,chars);
  return _str;
}


static char* MyW2A(wchar_t* w)
{
  char* rc = BSTR2ANSI(w);
  if(rc == NULL) {
    rc = strdup("");
  }
  return rc;
}



/* 06-02-15 | baier | trace on error */
int WINAPI BDX2Variant (BDX_Data* pBDXData,VARIANT* pVariantData)
{
  int lRet = 0;

  if(pBDXData->version != BDX_VERSION) {
    BDX_ERR(printf("BDX2Variant: got invalid BDX version. Expected %d, got %d\n",BDX_VERSION,
	  pBDXData->version));
    return -1;
  }

  switch (pBDXData->type & BDX_CMASK)
    {
      /* scalar? */
    case BDX_SCALAR:
      lRet = BDXScalar2Variant (pBDXData,pVariantData);
      break;
      /* treat vectors and arrays the same for the moment */
    case BDX_ARRAY:
      lRet = BDXArray2Variant (pBDXData,pVariantData);
      break;
    default:
      BDX_TRACE(printf("BDX2Variant: unknown type, bailing out\n"));
      lRet = -2;
    }

  return lRet;
}


/*
*/
int WINAPI Variant2BDX (VARIANT VariantData,BDX_Data** ppBDXData)
{
  /* scalar or array/vector? */
  if(V_ISARRAY (&VariantData)) /* (data.vt & VT_ARRAY) == VT_ARRAY) */
    {
      return VariantArray2BDX (VariantData,ppBDXData);
    }
  else {
    return VariantScalar2BDX (VariantData,ppBDXData);
  }
  return -2;
}


/* 05-05-20 | baier | BDX_SPECIAL, BDX_HANDLE */
/* 06-02-15 | baier | fixes for COM objects/EXTPTRSXP */
/* 08-04-11 | baier | added BDX_DT and BDX_CY */
static int BDXScalar2Variant (BDX_Data* pBDXData,VARIANT* pVariantData)
{
  int lRet = 0;

  switch (pBDXData->type & BDX_SMASK)
    {
    case BDX_BOOL:
      pVariantData->vt = VT_BOOL;
      pVariantData->boolVal = pBDXData->data.raw_data[0].bool_value ? VARIANT_TRUE : VARIANT_FALSE;
      break;
    case BDX_INT:
      pVariantData->vt = VT_I4;
      pVariantData->lVal = pBDXData->data.raw_data[0].int_value;
      break;
    case BDX_DOUBLE:
      pVariantData->vt = VT_R8;
      pVariantData->dblVal = pBDXData->data.raw_data[0].double_value;
      break;
    case BDX_DT:
      V_VT(pVariantData) = VT_DATE;
      V_DATE(pVariantData) =
	__BDXDTVal2VariantDATEVal(pBDXData->data.raw_data[0].dt_value);
      break;
#if 0
    case BDX_CY:
      V_VT(pVariantData) = VT_CY;
      V_CY(pVariantData).int64 = pBDXData->data.raw_data[0].cy_value;
      break;
#endif
    case BDX_STRING:
      {
	pVariantData->vt = VT_BSTR;
	pVariantData->bstrVal = 
	  ANSI2BSTR(pBDXData->data.raw_data[0].string_value);
      }
      break;
    case BDX_HANDLE:
      {
	LPSTREAM lStream = pBDXData->data.raw_data[0].ptr;
	void* lTmpPtr = (void*) V_DISPATCH(pVariantData);
	HRESULT lRc = CoUnmarshalInterface(lStream,&IID_IDispatch,
					   &lTmpPtr);
	if(FAILED(lRc)) {
	  BDX_ERR(printf("unmarshalling stream ptr %08x failed with hr=%08x\n",
			 lStream,lRc));
	  return -1;
	} else {
	  BDX_ERR(printf("successfully marshalled COM interface\n"));
	}

	/* create SEXP for COM object */
	V_VT(pVariantData) = VT_DISPATCH;
	break;
      }
    case BDX_SPECIAL:
      V_VT(pVariantData) = VT_ERROR;
      V_ERROR(pVariantData) = 
	getSCODEFromSpecialValue(pBDXData->data.raw_data[0].special_value);
      break;
    default:
      BDX_ERR(printf("BDXScalar2Variant: unsupported BDX type %08x found\n",
			pBDXData->type));
      lRet = -2;
    }

  return lRet;
}


/* 05-06-05 | baier | BDX_GENERIC */
/* 05-06-08 | baier | array of BDX_SPECIAL */
/* 06-02-15 | baier | fixes for COM objects/EXTPTRSXP */
/* 08-04-11 | baier | added BDX_DT and BDX_CY */
static int BDXArray2Variant (BDX_Data* pBDXData,VARIANT* pVariantData)
{
  int lRet = 0;

  switch (pBDXData->type & BDX_SMASK)
    {
    case BDX_BOOL:
      lRet = BDXBoolArray2Variant (pBDXData,pVariantData);
      break;
    case BDX_INT:
      lRet = BDXIntArray2Variant (pBDXData,pVariantData);
      break;
    case BDX_DOUBLE:
      lRet = BDXDoubleArray2Variant (pBDXData,pVariantData);
      break;
    case BDX_DT:
      lRet = BDXDTArray2Variant(pBDXData,pVariantData);
      break;
#if 0
    case BDX_CY:
      lRet = BDXCYArray2Variant(pBDXData,pVariantData);
      break;
#endif
    case BDX_SPECIAL:
      lRet = BDXSpecialArray2Variant (pBDXData,pVariantData);
      break;
    case BDX_STRING:
      lRet = BDXStringArray2Variant (pBDXData,pVariantData);
      break;
    case BDX_GENERIC:
      lRet = BDXGenericArray2Variant(pBDXData,pVariantData);
      break;
    default:
      BDX_ERR(printf("BDXArray2Variant: unsupported array type %x\n",
		     pBDXData->type));
      lRet = -2;
    }

  return lRet;
}

static int BDXBoolArray2Variant(BDX_Data* pBDXData,VARIANT* pVariantData)
{
  unsigned int lMaxIndex;
  SAFEARRAYBOUND* lArrayBounds;
  VARIANT_BOOL HUGEP* lBoolArray;
  unsigned int i;
	
  lMaxIndex = GetArrayBounds (pBDXData,&lArrayBounds);
  
  pVariantData->vt = VT_ARRAY | VT_BOOL;
  pVariantData->parray = SafeArrayCreate (VT_BOOL,pBDXData->dim_count,lArrayBounds);
  free (lArrayBounds); lArrayBounds = 0;

  if (pVariantData->parray == 0)
    {
      return -4;
    }

  /* access the data */

  if(SafeArrayAccessData (pVariantData->parray,(void*) &lBoolArray) != S_OK) {
    /* free safe array data */
    SafeArrayDestroy (pVariantData->parray);
    pVariantData->parray = 0;
    
    return -3;
  }

  /* copy the data */
  for(i = 0;i < lMaxIndex;i++) {
    lBoolArray[i] = pBDXData->data.raw_data[i].bool_value ? VARIANT_TRUE : VARIANT_FALSE;
  }

  SafeArrayUnlock (pVariantData->parray);

  return 0;
}

static int BDXIntArray2Variant(BDX_Data* pBDXData,VARIANT* pVariantData)
{
  unsigned int lMaxIndex;
  SAFEARRAYBOUND* lArrayBounds;
  long HUGEP* lIntArray;
  unsigned int i;
	
  lMaxIndex = GetArrayBounds (pBDXData,&lArrayBounds);

  pVariantData->vt = VT_ARRAY | VT_I4;
  pVariantData->parray = SafeArrayCreate (VT_I4,pBDXData->dim_count,lArrayBounds);
  free (lArrayBounds);

  if (pVariantData->parray == 0)
    {
      return -4;
    }

  /* access the data */

  if (SafeArrayAccessData (pVariantData->parray,(void*) &lIntArray) != S_OK)
    {
      /* free array data */
      SafeArrayDestroy (pVariantData->parray);
      pVariantData->parray = 0;

      return -3;
    }

  /* copy the data */
  for (i = 0;i < lMaxIndex;i++)
    {
      lIntArray[i] = pBDXData->data.raw_data[i].int_value;
    }

  /* @TODO: SafeArrayUnaccessData()*/
  SafeArrayUnlock (pVariantData->parray);

  return 0;
}

static int BDXDoubleArray2Variant(BDX_Data* pBDXData,VARIANT* pVariantData)
{
  unsigned int lMaxIndex;
  SAFEARRAYBOUND* lArrayBounds;
  double HUGEP* lDoubleArray;
  unsigned int i;
	
  lMaxIndex = GetArrayBounds (pBDXData,&lArrayBounds);

  pVariantData->vt = VT_ARRAY | VT_R8;
  pVariantData->parray = SafeArrayCreate (VT_R8,pBDXData->dim_count,lArrayBounds);
  free (lArrayBounds);

  if (pVariantData->parray == 0)
    {
      return -4;
    }

  /* access the data */

  if (SafeArrayAccessData (pVariantData->parray,(void*) &lDoubleArray) != S_OK)
    {
      /* free safe array data */
      SafeArrayDestroy (pVariantData->parray);
      pVariantData->parray = 0;

      return -3;
    }

  /* copy the data */
  for (i = 0;i < lMaxIndex;i++)
    {
      lDoubleArray[i] = pBDXData->data.raw_data[i].double_value;
    }

  SafeArrayUnlock (pVariantData->parray);

  return 0;
}

/* 08-04-11 | baier | NEW */
static int BDXDTArray2Variant(BDX_Data* pBDXData,VARIANT* pVariantData)
{
  unsigned int lMaxIndex;
  SAFEARRAYBOUND* lArrayBounds;
  DATE HUGEP* lDATEArray;
  unsigned int i;
	
  lMaxIndex = GetArrayBounds(pBDXData,&lArrayBounds);

  pVariantData->vt = VT_ARRAY | VT_DATE;
  pVariantData->parray = SafeArrayCreate(VT_DATE,
					 pBDXData->dim_count,lArrayBounds);
  free(lArrayBounds);

  if(pVariantData->parray == 0) {
    return -4;
  }

  /* access the data */

  if(SafeArrayAccessData(pVariantData->parray,(void*) &lDATEArray) != S_OK) {
    /* free array data */
    SafeArrayDestroy(pVariantData->parray);
    pVariantData->parray = 0;
    
    return -3;
  }

  /* copy the data */
  for(i = 0;i < lMaxIndex;i++) {
    lDATEArray[i] =
      __BDXDTVal2VariantDATEVal(pBDXData->data.raw_data[i].dt_value);
  }

  /* @TODO: SafeArrayUnaccessData()*/
  SafeArrayUnlock(pVariantData->parray);

  return 0;
}

#if 0
/* 08-04-11 | baier | NEW */
static int BDXCYArray2Variant(BDX_Data* pBDXData,VARIANT* pVariantData)
{
  unsigned int lMaxIndex;
  SAFEARRAYBOUND* lArrayBounds;
  CY HUGEP* lCYArray;
  unsigned int i;
	
  lMaxIndex = GetArrayBounds(pBDXData,&lArrayBounds);

  pVariantData->vt = VT_ARRAY | VT_CY;
  pVariantData->parray = SafeArrayCreate(VT_CY,
					 pBDXData->dim_count,lArrayBounds);
  free(lArrayBounds);

  if(pVariantData->parray == 0) {
    return -4;
  }

  /* access the data */

  if (SafeArrayAccessData(pVariantData->parray,(void*) &lCYArray) != S_OK) {
    /* free array data */
    SafeArrayDestroy(pVariantData->parray);
    pVariantData->parray = 0;
    
    return -3;
  }

  /* copy the data */
  for (i = 0;i < lMaxIndex;i++) {
    lCYArray[i].int64 = pBDXData->data.raw_data[i].cy_value;
  }

  /* @TODO: SafeArrayUnaccessData()*/
  SafeArrayUnlock (pVariantData->parray);

  return 0;
}
#endif

/* 06-05-16 | baier | fix: use ...raw_data[i].special_value instead of
 *                    ...raw_data[i].double_value */
static int BDXSpecialArray2Variant(BDX_Data* pBDXData,VARIANT* pVariantData)
{
  unsigned int lMaxIndex;
  SAFEARRAYBOUND* lArrayBounds;
  SCODE HUGEP* lErrorArray;
  unsigned int i;
	
  lMaxIndex = GetArrayBounds (pBDXData,&lArrayBounds);

  pVariantData->vt = VT_ARRAY | VT_ERROR;
  pVariantData->parray = SafeArrayCreate (VT_R8,pBDXData->dim_count,lArrayBounds);
  free (lArrayBounds);

  if (pVariantData->parray == 0)
    {
      return -4;
    }

  /* access the data */

  if (SafeArrayAccessData (pVariantData->parray,(void*) &lErrorArray) != S_OK)
    {
      /* free safe array data */
      SafeArrayDestroy (pVariantData->parray);
      pVariantData->parray = 0;

      return -3;
    }

  /* copy the data */
  for (i = 0;i < lMaxIndex;i++)
    {
      lErrorArray[i] =
	getSCODEFromSpecialValue(pBDXData->data.raw_data[i].special_value);
    }

  SafeArrayUnlock (pVariantData->parray);

  return 0;
}

static int BDXStringArray2Variant(BDX_Data* pBDXData,VARIANT* pVariantData)
{
  unsigned int lMaxIndex;
  SAFEARRAYBOUND* lArrayBounds;
  unsigned int i;
  BSTR HUGEP* lStringArray;
	
  lMaxIndex = GetArrayBounds (pBDXData,&lArrayBounds);

  pVariantData->vt = VT_ARRAY | VT_BSTR;
  pVariantData->parray = SafeArrayCreate (VT_BSTR,pBDXData->dim_count,lArrayBounds);
  free (lArrayBounds);

  if (pVariantData->parray == 0)
    {
      return -4;
    }

  /* access the data */

  if (SafeArrayAccessData (pVariantData->parray,(void*) &lStringArray) != S_OK)
    {
      /* free safe array data */
      SafeArrayDestroy (pVariantData->parray);
      pVariantData->parray = 0;

      return -3;
    }

  /* copy the data */
  for(i = 0;i < lMaxIndex;i++) {
    lStringArray[i] = ANSI2BSTR(pBDXData->data.raw_data[i].string_value);
  }

  SafeArrayUnlock (pVariantData->parray);

  return 0;
}


/* 05-06-05 | baier | convert a BDX_GENERIC array to a SAFEARRAY of VARIANT */
/* 08-04-11 | baier | added BDX_DT and BDX_CY */
static int BDXGenericArray2Variant(BDX_Data* pBDXData,VARIANT* pVariantData)
{
  unsigned int lMaxIndex;
  SAFEARRAYBOUND* lArrayBounds;
  VARIANT HUGEP* lVariantArray;
  unsigned int i;
	
  lMaxIndex = GetArrayBounds (pBDXData,&lArrayBounds);

  pVariantData->vt = VT_ARRAY | VT_VARIANT;
  pVariantData->parray = SafeArrayCreate (VT_VARIANT,
					  pBDXData->dim_count,lArrayBounds);
  free (lArrayBounds);

  if (pVariantData->parray == 0)
    {
      return -4;
    }

  /* access the data */

  if (SafeArrayAccessData (pVariantData->parray,(void*) &lVariantArray) != S_OK)
    {
      /* free safe array data */
      SafeArrayDestroy (pVariantData->parray);
      pVariantData->parray = 0;

      return -3;
    }

  /* copy the data */
  for (i = 0;i < lMaxIndex;i++) {
    switch(pBDXData->data.raw_data_with_type[i].type) {
    case BDX_BOOL:
      V_VT(&lVariantArray[i]) = VT_BOOL;
      V_BOOL(&lVariantArray[i]) = 
	pBDXData->data.raw_data_with_type[i].raw_data.bool_value
	? VARIANT_TRUE : VARIANT_FALSE;
      break;
    case BDX_INT:
      V_VT(&lVariantArray[i]) = VT_I4;
      V_I4(&lVariantArray[i]) =
	pBDXData->data.raw_data_with_type[i].raw_data.int_value;
      break;
    case BDX_DOUBLE:
      V_VT(&lVariantArray[i]) = VT_R8;
      V_R8(&lVariantArray[i]) =
	pBDXData->data.raw_data_with_type[i].raw_data.double_value;
      break;
    case BDX_DT:
      V_VT(&lVariantArray[i]) = VT_DATE;
      V_DATE(&lVariantArray[i]) =
	__BDXDTVal2VariantDATEVal(pBDXData->data.raw_data[i].dt_value);
      break;
#if 0
    case BDX_CY:
      V_VT(&lVariantArray[i]) = VT_CY;
      V_CY(&lVariantArray[i]).int64 =
	pBDXData->data.raw_data_with_type[i].raw_data.cy_value;
      break;
#endif
    case BDX_STRING:
      {
	V_VT(&lVariantArray[i]) = VT_BSTR;
	V_BSTR(&lVariantArray[i]) =
	  ANSI2BSTR(pBDXData->data.raw_data_with_type[i].raw_data.string_value);
      }
      break;
    case BDX_HANDLE:
      {
	LPSTREAM lStream = pBDXData->data.raw_data_with_type[i].raw_data.ptr;
	HRESULT lRc = CoUnmarshalInterface(lStream,&IID_IDispatch,
					   (void*) V_DISPATCH(&lVariantArray[i]));
	if(FAILED(lRc)) {
	  BDX_ERR(printf("unmarshalling stream ptr %p (index %d) failed with hr=%08x\n",
			    lStream,i,lRc));
	  return -1;
	}

	/* create SEXP for COM object */
	V_VT(&lVariantArray[i]) = VT_DISPATCH;
      }
    case BDX_SPECIAL:
      V_VT(&lVariantArray[i]) = VT_ERROR;
      V_ERROR(&lVariantArray[i]) =
	getSCODEFromSpecialValue(pBDXData->data.raw_data_with_type[i].raw_data.special_value);
      break;
    default:
      BDX_ERR(printf("unsupported BDX type %08x found at index %d\n",
			pBDXData->data.raw_data_with_type[i].type,i));
      return -2;
    }
  }
  
  SafeArrayUnlock (pVariantData->parray);

  return 0;
}


static unsigned int GetArrayBounds (BDX_Data* pBDXData,SAFEARRAYBOUND** ppArrayBounds)
{
  /* number of dimensions in lData->dim_count, allocate the array */
  SAFEARRAYBOUND* lArrayBounds = (SAFEARRAYBOUND*) malloc (pBDXData->dim_count * sizeof (SAFEARRAYBOUND));
  unsigned int i;
  unsigned int lMaxIndex = 1;

  for (i = 0;i < pBDXData->dim_count;i++)
    {
      lArrayBounds[i].cElements = pBDXData->dimensions[i];
      lMaxIndex *= pBDXData->dimensions[i];
      lArrayBounds[i].lLbound = 0;
    }

  *ppArrayBounds = lArrayBounds;
  return lMaxIndex;
}


/* 04-11-16 | baier | set BDX version */
/* 05-05-19 | baier | VT_DISPATCH, VT_ERROR */
/* 05-11-29 | baier | handle VT_EMPTY, too */
/* 08-04-11 | baier | added BDX_DT and BDX_CY */
/* 08-06-05 | baier | added support for VT_VARIANT */
int VariantScalar2BDX(VARIANT VariantData,BDX_Data** ppBDXData)
{
  BDX_Data* lData;

  /* 08-06-05 | baier | added support for VT_VARIANT */
  if((V_VT(&VariantData) & VT_TYPEMASK) == VT_VARIANT) {
    return Variant2BDX(*V_VARIANTREF(&VariantData),ppBDXData);
  }
    
  /* allocate base buffer */
  lData = (BDX_Data*) malloc (sizeof (BDX_Data));

  lData->version = BDX_VERSION;

  *ppBDXData = 0;

  /* scalar */
  lData->type = BDX_SCALAR;
  lData->dim_count = 1;
  lData->dimensions = (BDX_Dimension*) malloc (sizeof (BDX_Dimension));
  lData->dimensions[0] = 1;
  lData->data.raw_data = (BDX_RawData*) malloc (sizeof (BDX_RawData));

  switch(VariantData.vt & VT_TYPEMASK) {
    /* boolean data */
  case VT_BOOL:   /* boolean: 0x0000 is false, 0xffff is true (boolVal) */
    lData->type |= BDX_BOOL;
    
    if(!V_ISBYREF(&VariantData)) {
      if (VariantData.boolVal == VARIANT_FALSE) {
	lData->data.raw_data[0].bool_value = 0;
      } else if (VariantData.boolVal == VARIANT_TRUE) {
	lData->data.raw_data[0].bool_value = 1;
      } else {
	free (lData->data.raw_data);
	free (lData->dimensions);
	free (lData);
	return -4;
      }
    } else {
      if (*VariantData.pboolVal == VARIANT_FALSE) {
	lData->data.raw_data[0].bool_value = 0;
      } else if (*VariantData.pboolVal == VARIANT_TRUE) {
	lData->data.raw_data[0].bool_value = 1;
      } else {
	free (lData->data.raw_data);
	free (lData->dimensions);
	free (lData);
	return -4;
      }
    }
    break;

    /* integer data */
  case VT_I2:     /* 2-byte signed integer (iVal) */
    lData->type |= BDX_INT;
    if(!V_ISBYREF(&VariantData)) {
      lData->data.raw_data[0].int_value = VariantData.iVal;
    } else {
      lData->data.raw_data[0].int_value = *VariantData.piVal;
    }
    break;
    
  case VT_I4:     /* 4-byte signed integer (lVal) */
    lData->type |= BDX_INT;
    if(!V_ISBYREF(&VariantData)) {
      lData->data.raw_data[0].int_value = VariantData.lVal;
    } else {
      lData->data.raw_data[0].int_value = *VariantData.plVal;
    }
    break;
    
  case VT_UI1:    /* unsigned one-byte integer (bVal) */
    lData->type |= BDX_INT;
    if(!V_ISBYREF(&VariantData)) {
      lData->data.raw_data[0].int_value = VariantData.bVal;
    } else {
      lData->data.raw_data[0].int_value = *VariantData.pbVal;
    }
    break;
    
    /* real and double data */
  case VT_R4:     /* 4-byte IEEE floating point (fltVal) */
    lData->type |= BDX_DOUBLE;
    if(!V_ISBYREF(&VariantData)) {
      lData->data.raw_data[0].double_value = VariantData.fltVal;
    } else {
      lData->data.raw_data[0].double_value = *VariantData.pfltVal;
    }
    break;
    
  case VT_R8:     /* 8-byte IEEE floating point (dblVal) */
    lData->type |= BDX_DOUBLE;
    if(!V_ISBYREF(&VariantData)) {
      lData->data.raw_data[0].double_value = VariantData.dblVal;
    } else {
      lData->data.raw_data[0].double_value = *VariantData.pdblVal;
    }
    break;
    
  case VT_DATE:     /* VARIANT DATE */
    lData->type |= BDX_DT;
    if(!V_ISBYREF(&VariantData)) {
      lData->data.raw_data[0].dt_value =
	__VariantDATEVal2BDXDTVal(V_DATE(&VariantData));
    } else {
      lData->data.raw_data[0].dt_value =
	__VariantDATEVal2BDXDTVal(*V_DATEREF(&VariantData));
    }
    break;
    
#if 0
  case VT_CY:     /* VARIANT Currency */
    lData->type |= BDX_CY;
    if(!V_ISBYREF(&VariantData)) {
      lData->data.raw_data[0].cy_value = V_CY(&VariantData).int64;
    } else {
      lData->data.raw_data[0].cy_value = V_CYREF(&VariantData)->int64;
    }
    break;
#endif
    /* string data */
  case VT_BSTR:   /* basic string (bstrVal) */
    {
      lData->type |= BDX_STRING;
      if(!V_ISBYREF(&VariantData)) {
	lData->data.raw_data[0].string_value = MyW2A(VariantData.bstrVal);
      } else {
	lData->data.raw_data[0].string_value = MyW2A(*VariantData.pbstrVal);
      }
    }
    break;
    
    
  case VT_EMPTY: /* empty cells in Excel for example */
    lData->type |= BDX_SPECIAL;
    lData->data.raw_data[0].special_value = BDX_SV_NULL;
    break;
    
  case VT_ERROR:  /* special values (e.g. NA, NaN) */
    {
      SCODE lCode;
      lData->type |= BDX_SPECIAL;
      
      if(!V_ISBYREF(&VariantData)) {
	lCode = V_ERROR(&VariantData);
      } else {
	lCode = *V_ERRORREF(&VariantData);
      }
      lData->data.raw_data[0].special_value =
	getSpecialValueFromSCODE(lCode);
    }
    break;
    
  case VT_DISPATCH: /* COM objects (IDispatch) */
    {
      /* COM object is marshalled into stream
       *
       * 1. stream object holds reference to IDispatch
       * 2. reference count is increased in the meanwhile
       * 3. must use CoGetInterfaceAndReleaseStream() to unmarshal
       * 4. Release() must be called on object afterwards
       */
      LPSTREAM lStream = NULL;
      HRESULT lRc;
      
      lRc = CoMarshalInterThreadInterfaceInStream(&IID_IUnknown,
						  (LPUNKNOWN) V_DISPATCH(&VariantData),
						  &lStream);
      
      if(FAILED(lRc)) {
	BDX_TRACE(printf("VariantScalar2BDX: error %08x marshalling interface into stream\n",
			 lRc));
	return -5;
      }
      lData->data.raw_data[0].ptr = lStream;
      lData->type |= BDX_HANDLE;
      
    }
    break;
    
    /* unknown? */
  default:
    free(lData->data.raw_data);
    free(lData->dimensions);
    free(lData);
    return -4;
  }
  
  *ppBDXData = lData;
  return 0;
}


/* 04-11-16 | baier | set BDX version */
/* 05-06-08 | baier | array of error values */
/* 08-04-11 | baier | added BDX_DT and BDX_CY */
static int VariantArray2BDX(VARIANT VariantData,BDX_Data** ppBDXData)
{
  SAFEARRAY* lArray;
  int lRc = 0;

  /* allocate base buffer */
  BDX_Data* lData = (BDX_Data*) malloc (sizeof (BDX_Data));
  unsigned int lTotalSize = 1;

  lData->version = BDX_VERSION;

  *ppBDXData = 0;

  if(!V_ISBYREF(&VariantData)) {
    lArray = VariantData.parray;
  } else {
    lArray = *VariantData.pparray;
  }

  /* vector */
  lData->type = BDX_ARRAY;
  lData->dim_count = SafeArrayGetDim (lArray);
  lData->dimensions = (BDX_Dimension*) malloc (sizeof (BDX_Dimension) * lData->dim_count);

  /* get the dimensions */
  {
    unsigned int i;

    for (i = 0;i < lData->dim_count;i++)
      {
	long lUpperBound;
	long lLowerBound;

	if (FAILED (SafeArrayGetLBound (lArray,i+1,&lLowerBound)))
	  {
	    free (lData->dimensions);
	    free (lData);
	    return -3;
	  }
	if (FAILED (SafeArrayGetUBound (lArray,i+1,&lUpperBound)))
	  {
	    free (lData->dimensions);
	    free (lData);
	    return -3;
	  }
	lData->dimensions[i] = lUpperBound - lLowerBound + 1;
	lTotalSize *= lData->dimensions[i];
      }
  }

  switch (VariantData.vt & VT_TYPEMASK)
    {
      /* boolean data */
    case VT_BOOL:   /* boolean: 0x0000 is false, 0xffff is true (boolVal) */
      lData->data.raw_data = (BDX_RawData*) malloc (sizeof (BDX_RawData) * lTotalSize);
      lRc = VariantBoolArray2BDX (VariantData,lData,lTotalSize);
      break;

      /* integer data */
    case VT_I2:     /* 2-byte signed integer (iVal) */
      lData->data.raw_data = (BDX_RawData*) malloc (sizeof (BDX_RawData) * lTotalSize);
      lRc = VariantI2Array2BDX (VariantData,lData,lTotalSize);
      break;

    case VT_I4:     /* 4-byte signed integer (lVal) */
      lData->data.raw_data = (BDX_RawData*) malloc (sizeof (BDX_RawData) * lTotalSize);
      lRc = VariantI4Array2BDX (VariantData,lData,lTotalSize);
      break;

    case VT_UI1:    /* unsigned one-byte integer (bVal) */
      lData->data.raw_data = (BDX_RawData*) malloc (sizeof (BDX_RawData) * lTotalSize);
      lRc = VariantUI1Array2BDX (VariantData,lData,lTotalSize);
      break;

      /* real and double data */
    case VT_R4:     /* 4-byte IEEE floating point (fltVal) */
      lData->data.raw_data = (BDX_RawData*) malloc (sizeof (BDX_RawData) * lTotalSize);
      lRc = VariantR4Array2BDX (VariantData,lData,lTotalSize);
      break;

    case VT_R8:     /* 8-byte IEEE floating point (dblVal) */
      lData->data.raw_data = (BDX_RawData*) malloc (sizeof (BDX_RawData) * lTotalSize);
      lRc = VariantR8Array2BDX (VariantData,lData,lTotalSize);
      break;

    case VT_DATE:     /* VARIANT DATE */
      lData->data.raw_data = (BDX_RawData*) malloc(sizeof(BDX_RawData) * lTotalSize);
      lRc = VariantDATEArray2BDX(VariantData,lData,lTotalSize);
      break;

#if 0
    case VT_CY:     /* VARIANT Currency */
      lData->data.raw_data = (BDX_RawData*) malloc (sizeof(BDX_RawData) * lTotalSize);
      lRc = VariantCYArray2BDX(VariantData,lData,lTotalSize);
      break;
#endif
      /* string data */
    case VT_BSTR:   /* basic string (bstrVal) */
      lData->data.raw_data = (BDX_RawData*) malloc (sizeof (BDX_RawData) * lTotalSize);
      lRc = VariantStringArray2BDX (VariantData,lData,lTotalSize);
      break;

      /* variant array */
    case VT_VARIANT:
      lData->data.raw_data_with_type = (BDX_RawDataWithType*) malloc (sizeof (BDX_RawDataWithType) * lTotalSize);
      lRc = VariantVariantArray2BDX (VariantData,lData,lTotalSize);
      break;

      /* array of error values */
    case VT_ERROR:
      lData->data.raw_data = (BDX_RawData*) malloc (sizeof (BDX_RawData) * lTotalSize);
      lRc = VariantErrorArray2BDX (VariantData,lData,lTotalSize);
      break;

      /* unknown? */
    default:
      lRc = -2;
    }

  if (lRc < 0) {
    /* even if it doesn't matter */
    switch(lData->type & ~BDX_ARRAY) {
    case BDX_GENERIC:
      free(lData->data.raw_data_with_type);
    case BDX_BOOL:
    case BDX_INT:
    case BDX_DOUBLE:
    case BDX_DT:
#if 0
    case BDX_CY:
#endif
    case BDX_STRING:
    case BDX_SPECIAL:
      free(lData->data.raw_data);
      break;
    default:
      /* don't know */
      break;
    }
    free (lData->dimensions);
    free (lData);
  } else {
    *ppBDXData = lData;
  }

  return lRc;
}

/* 04-05-12 | baier | handle VT_BYREF */
static int VariantBoolArray2BDX(VARIANT VariantData,
				BDX_Data* pBDXData,
				unsigned int pTotalElements)
{
  /* access the data */
  VARIANT_BOOL HUGEP* lBoolArray;
  SAFEARRAY* lArray;
  unsigned int i;

  if(!V_ISBYREF(&VariantData)) {
    lArray = VariantData.parray;
  } else {
    lArray = *VariantData.pparray;
  }
  if (SafeArrayAccessData (lArray,(void*) &lBoolArray) != S_OK) {
    return -3;
  }

  pBDXData->type |= BDX_BOOL;

  /* copy the data */

  for (i = 0;i < pTotalElements;i++) {
    if (lBoolArray[i] == VARIANT_FALSE) {
      pBDXData->data.raw_data[i].bool_value = 0;
    } else if (lBoolArray[i] == VARIANT_TRUE)	{
      pBDXData->data.raw_data[0].bool_value = 1;
    } else {
      return -2;
    }
  }
  SafeArrayUnlock (lArray);

  return 0;
}

/* 04-05-12 | baier | handle VT_BYREF */
static int VariantI2Array2BDX(VARIANT VariantData,
			      BDX_Data* pBDXData,
			      unsigned int pTotalElements)
{
  /* access the data */
  short HUGEP* lIntArray;
  SAFEARRAY* lArray;
  unsigned int i;


  if(!V_ISBYREF(&VariantData)) {
    lArray = VariantData.parray;
  } else {
    lArray = *VariantData.pparray;
  }

  if (SafeArrayAccessData (lArray,(void*) &lIntArray) != S_OK)
    {
      return -3;
    }

  pBDXData->type |= BDX_INT;

  /* copy the data */
  for (i = 0;i < pTotalElements;i++) {
    pBDXData->data.raw_data[i].int_value = lIntArray[i];
  }

  SafeArrayUnlock (lArray);

  return 0;
}

/* 04-05-12 | baier | handle VT_BYREF */
static int VariantI4Array2BDX(VARIANT VariantData,
			      BDX_Data* pBDXData,
			      unsigned int pTotalElements)
{
  /* access the data */
  long HUGEP* lIntArray;
  SAFEARRAY* lArray;
  unsigned int i;


  if(!V_ISBYREF(&VariantData)) {
    lArray = VariantData.parray;
  } else {
    lArray = *VariantData.pparray;
  }

  if (SafeArrayAccessData (lArray,(void*) &lIntArray) != S_OK)
    {
      return -3;
    }

  pBDXData->type |= BDX_INT;

  /* copy the data */
  for (i = 0;i < pTotalElements;i++)
    {
      pBDXData->data.raw_data[i].int_value = lIntArray[i];
    }

  SafeArrayUnlock (lArray);

  return 0;
}

/* 04-05-12 | baier | handle VT_BYREF */
static int VariantUI1Array2BDX(VARIANT VariantData,
			       BDX_Data* pBDXData,
			       unsigned int pTotalElements)
{
  /* access the data */
  unsigned char HUGEP* lIntArray;
  SAFEARRAY* lArray;
  unsigned int i;


  if(!V_ISBYREF(&VariantData)) {
    lArray = VariantData.parray;
  } else {
    lArray = *VariantData.pparray;
  }

  if (SafeArrayAccessData (lArray,(void*) &lIntArray) != S_OK)
    {
      return -3;
    }

  pBDXData->type |= BDX_INT;

  /* copy the data */
  for (i = 0;i < pTotalElements;i++)
    {
      pBDXData->data.raw_data[i].int_value = lIntArray[i];
    }

  SafeArrayUnlock (lArray);

  return 0;
}

/* 04-05-12 | baier | handle VT_BYREF */
static int VariantR4Array2BDX(VARIANT VariantData,
			      BDX_Data* pBDXData,
			      unsigned int pTotalElements)
{
  /* access the data */
  float HUGEP* lRealArray;
  SAFEARRAY* lArray;
  unsigned int i;


  if(!V_ISBYREF(&VariantData)) {
    lArray = VariantData.parray;
  } else {
    lArray = *VariantData.pparray;
  }

  if (SafeArrayAccessData (lArray,(void*) &lRealArray) != S_OK)
    {
      return -3;
    }

  pBDXData->type |= BDX_DOUBLE;

  /* copy the data */
  for (i = 0;i < pTotalElements;i++)
    {
      pBDXData->data.raw_data[i].double_value = lRealArray[i];
    }

  SafeArrayUnlock (lArray);

  return 0;
}

/* 04-05-12 | baier | handle VT_BYREF */
static int VariantR8Array2BDX(VARIANT VariantData,
			      BDX_Data* pBDXData,
			      unsigned int pTotalElements)
{
  /* access the data */
  double HUGEP* lRealArray;
  SAFEARRAY* lArray;
  unsigned int i;


  if(!V_ISBYREF(&VariantData)) {
    lArray = VariantData.parray;
  } else {
    lArray = *VariantData.pparray;
  }

  if (SafeArrayAccessData (lArray,(void*) &lRealArray) != S_OK)
    {
      return -3;
    }

  pBDXData->type |= BDX_DOUBLE;

  /* copy the data */
  for (i = 0;i < pTotalElements;i++)
    {
      pBDXData->data.raw_data[i].double_value = lRealArray[i];
    }

  SafeArrayUnlock (lArray);

  return 0;
}

/* 08-04-11 | baier | NEW */
static int VariantDATEArray2BDX(VARIANT VariantData,
				BDX_Data* pBDXData,
				unsigned int pTotalElements)
{
  /* access the data */
  DATE HUGEP* lDATEArray;
  SAFEARRAY* lArray;
  unsigned int i;


  if(!V_ISBYREF(&VariantData)) {
    lArray = V_ARRAY(&VariantData);
  } else {
    lArray = *V_ARRAYREF(&VariantData);
  }

  if (SafeArrayAccessData(lArray,(void*) &lDATEArray) != S_OK)
    {
      return -3;
    }

  pBDXData->type |= BDX_DT;

  /* copy the data */
  for (i = 0;i < pTotalElements;i++) {
    pBDXData->data.raw_data[i].dt_value =
      __VariantDATEVal2BDXDTVal(lDATEArray[i]);
  }

  SafeArrayUnlock (lArray);

  return 0;
}


#if 0
/* 08-04-11 | baier | NEW */
static int VariantCYArray2BDX(VARIANT VariantData,
			      BDX_Data* pBDXData,
			      unsigned int pTotalElements)
{
  /* access the data */
  CY HUGEP* lCYArray;
  SAFEARRAY* lArray;
  unsigned int i;


  if(!V_ISBYREF(&VariantData)) {
    lArray = V_ARRAY(&VariantData);
  } else {
    lArray = *V_ARRAYREF(&VariantData);
  }

  if (SafeArrayAccessData(lArray,(void*) &lCYArray) != S_OK)
    {
      return -3;
    }

  pBDXData->type |= BDX_DT;

  /* copy the data */
  for (i = 0;i < pTotalElements;i++) {
    pBDXData->data.raw_data[i].cy_value = lCYArray[i].int64;
  }

  SafeArrayUnlock (lArray);

  return 0;
}
#endif


/* 05-06-08 | baier | new */
static int VariantErrorArray2BDX(VARIANT VariantData,
				 BDX_Data* pBDXData,
				 unsigned int pTotalElements)
{
  /* access the data */
  SCODE HUGEP* lErrorArray;
  SAFEARRAY* lArray;
  unsigned int i;


  if(!V_ISBYREF(&VariantData)) {
    lArray = VariantData.parray;
  } else {
    lArray = *VariantData.pparray;
  }

  if (SafeArrayAccessData (lArray,(void*) &lErrorArray) != S_OK)
    {
      return -3;
    }

  pBDXData->type |= BDX_SPECIAL;

  /* copy the data */
  for (i = 0;i < pTotalElements;i++)
    {
      pBDXData->data.raw_data[i].special_value =
	getSpecialValueFromSCODE(lErrorArray[i]);
    }

  SafeArrayUnlock (lArray);

  return 0;
}

/* 04-05-12 | baier | handle VT_BYREF */
static int VariantStringArray2BDX(VARIANT VariantData,
				  BDX_Data* pBDXData,
				  unsigned int pTotalElements)
{
  /* access the data */
  BSTR HUGEP* lStringArray;
  SAFEARRAY* lArray;
  unsigned int i;
  unsigned int limit = pTotalElements;


  if(!V_ISBYREF(&VariantData)) {
    lArray = VariantData.parray;
  } else {
    lArray = *VariantData.pparray;
  }

  if (SafeArrayAccessData (lArray,(void*) &lStringArray) != S_OK)
    {
      return -3;
    }

  pBDXData->type |= BDX_STRING;

  /* copy the data */
  for (i = 0;i < pTotalElements;i++)
    {
#if 1
      pBDXData->data.raw_data[i].string_value = MyW2A(lStringArray[i]);
#else
#endif
      if(i >= limit)
      {
	i=i;
      }
    }

  SafeArrayUnlock (lArray);

  return 0;
}

/* 05-11-29 | baier | handle VT_EMPTY, too */
/* 06-07-03 | baier | bug fix: homogenous array never was transferred, fixed */
/* 08-04-11 | baier | added BDX_DT and BDX_CY */
/* 08-06-05 | baier | fix errors with VT_DATE */
static int VariantVariantArray2BDX(VARIANT VariantData,
				   BDX_Data* pBDXData,
				   unsigned int pTotalElements)
{
  SAFEARRAY* lArray;
  VARIANT HUGEP* lVariantArray;
  VARTYPE lCoerceTo = VT_UNKNOWN;
  unsigned int i;


  if(!V_ISBYREF(&VariantData)) {
    lArray = VariantData.parray;
  } else {
    lArray = *VariantData.pparray;
  }

  /* access the data */

  if (SafeArrayAccessData (lArray,(void*) &lVariantArray) != S_OK)
    {
      return -3;
    }

  /*
   * Transfer of VARIANT-arrays: The algorithm is defined as:
   *
   *   1. if all elements are of the same type, the (compound) type
   *      is transferred as a homogenous array
   *
   *   2. if both integer and floating points variables are found,
   *      and no other types used, data is converted to floating point
   *      and transferred as a floating point array
   *
   *   3. VT_DATE and VT_CY can be converted to VT_R8xs
   *
   *   4. arrays in arrays are NOT supported
   *
   *   5. VT_ERROR (e.g. NAN, N/A) will cause a transfer of maybe
   *      homogenous data but with "special" types, too
   *
   *   6. everything else (which can be transferred) is transferred
   *      as is.
   */

  /*
   * first check, if all values are of similar type
   */

  for(i = 0;i < pTotalElements;i++) {
    /* just check the common data type to coerce to */
    if(V_ISARRAY(&lVariantArray[i])) {
      /* we cannot handle arrays in arrays */
      BDX_ERR(printf("VariantVariantArray2BDX: array found at index %d; not supported\n",i));
      SafeArrayUnlock (lArray);
      return -4;
    }
    switch(V_VT(&lVariantArray[i])) {
    case VT_BOOL:
      if((lCoerceTo == VT_UNKNOWN) || (lCoerceTo == VT_BOOL)) {
	lCoerceTo = VT_BOOL;
      } else {
	lCoerceTo = VT_ERROR; /* this really means: transfer as is */
      }
      break;
    case VT_I2:     /* 2-byte signed integer (iVal) */
    case VT_UI1:    /* unsigned one-byte integer (bVal) */
    case VT_I4:
      if((lCoerceTo == VT_UNKNOWN) || (lCoerceTo == VT_I4)) {
	lCoerceTo = VT_I4;
      } else if(lCoerceTo != VT_R8) {
	lCoerceTo = VT_ERROR; /* this really means: transfer as is */
      }
      /* VT_R8 stays VT_R8 */
      break;
    case VT_R4:
    case VT_R8:
      if((lCoerceTo == VT_UNKNOWN) || (lCoerceTo == VT_I4)
	 || (lCoerceTo == VT_R8)) {
	lCoerceTo = VT_R8;
      } else {
	lCoerceTo = VT_ERROR; /* this really means: transfer as is */
      }
      break;
    case VT_DATE:
      if((lCoerceTo == VT_UNKNOWN) || (lCoerceTo == VT_DATE)) {
	lCoerceTo = VT_DATE;
      } else if((lCoerceTo == VT_I4) || (lCoerceTo == VT_R8)
		/*|| (lCoerceTo == VT_CY)*/) {
	lCoerceTo = VT_R8;
      } else {
	lCoerceTo = VT_ERROR; /* this really means: transfer as is */
      }
      break;
#if 0
    case VT_CY:
      if((lCoerceTo == VT_UNKNOWN) || (lCoerceTo == VT_CY)) {
	lCoerceTo = VT_CY;
      } else if((lCoerceTo == VT_I4) || (lCoerceTo == VT_R8)
		|| (lCoerceTo == VT_DATE)) {
	lCoerceTo = VT_R8;
      } else {
	lCoerceTo = VT_ERROR; /* this really means: transfer as is */
      }
      break;
#endif
    case VT_BSTR:
      if((lCoerceTo == VT_UNKNOWN) || (lCoerceTo == VT_BSTR)) {
        lCoerceTo = VT_BSTR;
      } else {
	lCoerceTo = VT_ERROR; /* this really means: transfer as is */
      }
    case VT_EMPTY: /* empty cells in Excel for example */
    case VT_ERROR:
      /* excel specifies cell errors with the following typedef. These are ORed with 0x800a0000
          typedef enum {
            xlErrDiv0 = 2007,
            xlErrNA = 2042,
            xlErrName = 2029,
            xlErrNull = 2000,
            xlErrNum = 2036,
            xlErrRef = 2023,
            xlErrValue = 2015
          } XlCVError;
       */
      lCoerceTo = VT_ERROR; /* this really means: transfer as is */
      break;
    default:
      BDX_ERR(printf("VariantVariantArray2BDX: type %d found at index %d; not supported\n",
	          V_VT(&lVariantArray[i]),i));
      SafeArrayUnlock (lArray);
      return -4;
    }
  }
  switch(lCoerceTo) {
  case VT_BOOL:
    pBDXData->type |= BDX_BOOL;
    break;
  case VT_I4:
    pBDXData->type |= BDX_INT;
    break;
  case VT_R8:
    pBDXData->type |= BDX_DOUBLE;
    break;
  case VT_DATE:
    pBDXData->type |= BDX_DT;
    break;
#if 0
  case VT_CY:
    pBDXData->type |= BDX_CY;
    break;
#endif
  case VT_BSTR:
    pBDXData->type |= BDX_STRING;
    break;
  case VT_ERROR:
    /* data will be transferred as is */
    pBDXData->type |= BDX_GENERIC; /* generic array (no specific base type) */
    break;
  default:
    BDX_ERR(printf("VariantVariantArray2BDX: internal error (#1, val %d)\n",lCoerceTo));
    SafeArrayUnlock (lArray);
    return -4;
  }

  /* copy the data */
  if(lCoerceTo != VT_ERROR) {
    for (i = 0;i < pTotalElements;i++) {
      VARIANT lTmpVariant;
      VariantInit(&lTmpVariant);
      VariantChangeTypeEx(&lTmpVariant,&lVariantArray[i],MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_NEUTRAL),0),VARIANT_ALPHABOOL,lCoerceTo);
      switch(lCoerceTo) {
      case VT_BOOL:
	if (lTmpVariant.boolVal == VARIANT_FALSE) {
	  pBDXData->data.raw_data[i].bool_value = 0;
	} else {
	  pBDXData->data.raw_data[i].bool_value = 1;
	}
	break;
      case VT_I4:
	pBDXData->data.raw_data[i].int_value = V_I4(&lTmpVariant);
	break;
      case VT_R8:
	pBDXData->data.raw_data[i].double_value = V_R8(&lTmpVariant);
	break;
      case VT_DATE:
	pBDXData->data.raw_data[i].dt_value =
	  __VariantDATEVal2BDXDTVal(V_DATE(&lTmpVariant));
	break;
#if 0
      case VT_CY:
	pBDXData->data.raw_data[i].cy_value = V_CY(&lTmpVariant).int64;
	break;
#endif
      case VT_BSTR:
	pBDXData->data.raw_data[i].string_value = MyW2A(V_BSTR(&lTmpVariant));
      default:
	/* never reached */
        BDX_ERR(printf("VariantVariantArray2BDX: internal error (#1, type %d at %i)\n",
	            lCoerceTo,i));
	break;
      }
      VariantClear(&lTmpVariant);
    }
  } else {
    for (i = 0;i < pTotalElements;i++) {
      switch(V_VT(&lVariantArray[i])) {
      case VT_BOOL:
	pBDXData->data.raw_data_with_type[i].type = BDX_BOOL;
	if(!V_ISBYREF(&lVariantArray[i])) {
  	  if(lVariantArray[i].boolVal == VARIANT_FALSE) {
	    pBDXData->data.raw_data_with_type[i].raw_data.bool_value = 0;
	  } else {
	    pBDXData->data.raw_data_with_type[i].raw_data.bool_value = 1;
	  }
	} else {
  	  if(*lVariantArray[i].pboolVal == VARIANT_FALSE) {
	    pBDXData->data.raw_data_with_type[i].raw_data.bool_value = 0;
	  } else {
	    pBDXData->data.raw_data_with_type[i].raw_data.bool_value = 1;
	  }
	}
	break;
      case VT_I2:     /* 2-byte signed integer (iVal) */
	pBDXData->data.raw_data_with_type[i].type = BDX_INT;
	if(!V_ISBYREF(&lVariantArray[i])) {
	  pBDXData->data.raw_data_with_type[i].raw_data.int_value = lVariantArray[i].iVal;
	} else {
	  pBDXData->data.raw_data_with_type[i].raw_data.int_value = *lVariantArray[i].piVal;
	}
	break;
      case VT_UI1:    /* unsigned one-byte integer (bVal) */
	pBDXData->data.raw_data_with_type[i].type = BDX_INT;
	if(!V_ISBYREF(&lVariantArray[i])) {
	  pBDXData->data.raw_data_with_type[i].raw_data.int_value = lVariantArray[i].bVal;
	} else {
	  pBDXData->data.raw_data_with_type[i].raw_data.int_value = *lVariantArray[i].pbVal;
	}
	break;
      case VT_I4:
	pBDXData->data.raw_data_with_type[i].type = BDX_INT;
	if(!V_ISBYREF(&lVariantArray[i])) {
	  pBDXData->data.raw_data_with_type[i].raw_data.int_value = lVariantArray[i].lVal;
	} else {
	  pBDXData->data.raw_data_with_type[i].raw_data.int_value = *lVariantArray[i].plVal;
	}
	break;
      case VT_R4:
	pBDXData->data.raw_data_with_type[i].type = BDX_DOUBLE;
	if(!V_ISBYREF(&lVariantArray[i])) {
	  pBDXData->data.raw_data_with_type[i].raw_data.double_value = lVariantArray[i].fltVal;
	} else {
	  pBDXData->data.raw_data_with_type[i].raw_data.double_value = *lVariantArray[i].pfltVal;
	}
	break;
      case VT_R8:
	pBDXData->data.raw_data_with_type[i].type = BDX_DOUBLE;
	if(!V_ISBYREF(&lVariantArray[i])) {
	  pBDXData->data.raw_data_with_type[i].raw_data.double_value = lVariantArray[i].dblVal;
	} else {
	  pBDXData->data.raw_data_with_type[i].raw_data.double_value = *lVariantArray[i].pdblVal;
	}
	break;
      case VT_DATE:     /* VARIANT DATE */
	pBDXData->data.raw_data_with_type[i].type = BDX_DT;
	if(!V_ISBYREF(&lVariantArray[i])) {
	  pBDXData->data.raw_data_with_type[i].raw_data.dt_value =
	    __VariantDATEVal2BDXDTVal(V_DATE(&lVariantArray[i]));
	} else {
	  pBDXData->data.raw_data_with_type[i].raw_data.dt_value =
	    __VariantDATEVal2BDXDTVal((*V_DATEREF(&lVariantArray[i])));
	}
	break;
#if 0
      case VT_CY:     /* VARIANT Currency */
	pBDXData->data.raw_data_with_type[i].type = BDX_CY;
	if(!V_ISBYREF(&lVariantArray[i])) {
	  pBDXData->data.raw_data_with_type[i].raw_data.cy_value =
	    V_CY(&lVariantArray[i]).int64;
	} else {
	  pBDXData->data.raw_data_with_type[i].raw_data.cy_value =
	    V_CYREF(&lVariantArray[i])->int64;
	}
	break;
#endif
      case VT_BSTR:
	pBDXData->data.raw_data_with_type[i].type = BDX_STRING;
	if(!V_ISBYREF(&lVariantArray[i])) {
	  pBDXData->data.raw_data_with_type[i].raw_data.string_value = MyW2A(lVariantArray[i].bstrVal);
	} else {
	  pBDXData->data.raw_data_with_type[i].raw_data.string_value = MyW2A(*lVariantArray[i].pbstrVal);
	}
	break;
      case VT_EMPTY: /* empty cells in Excel for example */
	pBDXData->data.raw_data_with_type[i].type = BDX_SPECIAL;
	pBDXData->data.raw_data_with_type[i].raw_data.special_value =
	  BDX_SV_NULL;
	break;
      case VT_ERROR:
	pBDXData->data.raw_data_with_type[i].type = BDX_SPECIAL;
	{
	  SCODE sc;

	  if(!V_ISBYREF(&lVariantArray[i])) {
	    sc = lVariantArray[i].scode;
	  } else {
	    sc = *lVariantArray[i].pscode;
	  }
	  pBDXData->data.raw_data_with_type[i].raw_data.special_value =
	    getSpecialValueFromSCODE(sc);
	}
	break;
      default:
        BDX_ERR(printf("VariantVariantArray2BDX: internal error (#1, type %d at %i)\n",
	            V_VT(&lVariantArray[i]),i));
	break;
      }
    }
  }
  SafeArrayUnlock (lArray);

  return 0;
}


static unsigned long getSpecialValueFromSCODE(SCODE pSCODE)
{
  /* excel specifies cell errors with the following typedef. These are ORed with 0x800a0000
     typedef enum {
     xlErrDiv0 = 2007,
     xlErrNA = 2042,
     xlErrName = 2029,
     xlErrNull = 2000,
     xlErrNum = 2036,
     xlErrRef = 2023,
     xlErrValue = 2015
     } XlCVError;
  */
  switch(pSCODE & ~0x800a0000) {
  case 2007: /* xlErrDiv0 */
    return BDX_SV_DIV0;
  case 2042: /* xlErrNA */
    return BDX_SV_NA;
    break;
  case 2000: /* xlErrNull */
    return BDX_SV_NULL;
    break;
  default:
    return BDX_SV_UNK;
  }
}
static SCODE getSCODEFromSpecialValue(unsigned long pSpecialVal)
{
  /* excel specifies cell errors with the following typedef. These are ORed with 0x800a0000
     typedef enum {
     xlErrDiv0 = 2007,
     xlErrNA = 2042,
     xlErrName = 2029,
     xlErrNull = 2000,
     xlErrNum = 2036,
     xlErrRef = 2023,
     xlErrValue = 2015
     } XlCVError;
  */
  switch(pSpecialVal) {
  case BDX_SV_NULL:
    return 0x800a0000 | 2000;
  case BDX_SV_NA:
    return 0x800a0000 | 2042;
  case BDX_SV_DIV0:
  case BDX_SV_INF:
  case BDX_SV_NAN:
  case BDX_SV_NINF:
    /* DIV/0 and +Inf transform to +Inf */
    /* NAN is just NaN */
    /* -Inf is -Inf */
    return 0x800a0000 | 2007;
  case BDX_SV_UNK:
  default:
    return 0x800a0000 | 2042;
  }
}
#else
static int dummy = 0;
#endif
