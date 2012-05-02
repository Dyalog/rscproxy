/*******************************************************************************
 *  RProxy: Connector implementation between application and R language
 *  Copyright (C) 1999--2009 Thomas Baier
 *  Copyright 2006-8 R Development Core Team
 *
 *  R_Proxy_init based on rtest.c,  Copyright (C) 1998--2000
 *                                  R Development Core Team
 *
 *  Copyright (C) 2000--2009 Thomas Baier
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
 ******************************************************************************/

#include "SC_system.h"
#if defined(__WINDOWS__)
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(__WINDOWS__)
#define R_INTERFACE_PTRS 1
#endif
#if 1
#include <Rembedded.h>
#else
#include <config.h>
#endif

#include <Rinternals.h>
#if !defined(__WINDOWS__)
#include <Rinterface.h>
#endif
#include <Rversion.h>
#include <Rembedded.h>
#include <R_ext/RStartup.h>
#include <R_ext/GraphicsEngine.h>
#include <R_ext/GraphicsDevice.h>
#if defined(__WINDOWS__)
#include <graphapp.h>
#endif

#include "bdx_SEXP.h"
#include "bdx_util.h"
#include "SC_proxy.h"
#include "rproxy.h"
#include "rproxy_impl.h"

# include <R_ext/Parse.h>

#define TRCBUFSIZE 2048

struct _R_Proxy_init_parameters g_R_Proxy_init_parameters = { 0 };

/* calls into the R DLL */
extern char *getRHOME(void);

int R_Proxy_Graphics_Driver (pDevDesc pDD,
			     char* pDisplay,
			     double pWidth,
			     double pHeight,
			     double pPointSize);

extern SC_CharacterDevice* __output_device;

/* trace to DebugView */
/* 12-05-02 | baier | removed printf() on !__WINDOWS__ */
int R_Proxy_printf(char const* pFormat,...)
{
  static char __tracebuf[TRCBUFSIZE];

  va_list lArgs;
  va_start(lArgs, pFormat);
  vsnprintf(__tracebuf,TRCBUFSIZE, pFormat, lArgs);
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

#if defined(__WINDOWS__)
static void R_Proxy_askok (char const* pMsg)
{
  askok(pMsg);
  return;
}

static int R_Proxy_askyesnocancel (const char* pMsg)
{
#if 0
  return YES;
#else
  return 1;
#endif
}
#endif

#if defined(__WINDOWS__)
static int 
R_Proxy_ReadConsole(const char *prompt,char *buf, int len, int addtohistory)
#else
static int 
R_Proxy_ReadConsole(const char *prompt,unsigned char *buf, int len, int addtohistory)
#endif
{
  return 0;
}

static void R_Proxy_WriteConsole(const char *buf, int len)
{
  if (__output_device) {
    __output_device->vtbl->write_string (__output_device,buf);
  }
}

#if !defined(__WINDOWS__)
static void R_Proxy_WriteConsoleEx(const char* buf, int len, int something)
{
  R_Proxy_WriteConsole(buf,len);
}
#endif

static void R_Proxy_CallBack()
{
    /* called during i/o, eval, graphics in ProcessEvents */
}

static void R_Proxy_Busy(int which)
{
    /* set a busy cursor ... in which = 1, unset if which = 0 */
}

/* 00-02-18 | baier | parse parameter string and fill parameter structure */
/* 06-06-18 | baier | parse parameter "dm" */
int R_Proxy_parse_parameters (char const* pParameterString,
			      struct _R_Proxy_init_parameters* pParameterStruct)
{
  /*
   * parameter string is of the form name1=value1;name2=value2;...
   *
   * currently recognized parameter names (case-sensitive):
   *
   *   (obsolete) NSIZE ... number of cons cells, (unsigned int) parameter
   *   (obsolete) VSIZE ... size of vector heap, (unsigned int) parameter
   *   dm ...... data mode (unsigned long, see below)
   */
  int lDone = 0;
  char const* lParameterStart = pParameterString;
  int lIndexOfSemicolon = 0;
  char* lTmpBuffer = NULL;
  char* lPosOfSemicolon = NULL;

  RPROXY_TRACE(printf("R_Proxy_parse_parameters(\"%s\")\n",pParameterString));

  while (!lDone) {
    /*
     * dm: data mode?
     * --------------
     *
     *   0 ... default data transfer mode
     *   1 ... read +Inf and -Inf in double representation
     */
    if(strncmp (lParameterStart,"dm=",3) == 0) {
      RPROXY_TRACE(printf("param dm found, parsing\n"));
      lParameterStart += 3;
      
      lPosOfSemicolon = strchr (lParameterStart,';');
      lIndexOfSemicolon = lPosOfSemicolon - lParameterStart;
      
      if (lPosOfSemicolon) {
	lTmpBuffer = malloc (lIndexOfSemicolon + 1); /* to catch NSIZE=; */
	strncpy (lTmpBuffer,lParameterStart,lIndexOfSemicolon);
	*(lTmpBuffer + lIndexOfSemicolon) = 0x0;
	bdx_set_datamode(atol(lTmpBuffer));
	if(pParameterStruct) {
	  pParameterStruct->dm = atol (lTmpBuffer);
	}
	free (lTmpBuffer);
	lParameterStart += lIndexOfSemicolon + 1;
      } else {
	bdx_set_datamode(atol(lParameterStart));
	if(pParameterStruct) {
	  pParameterStruct->dm = atol(lParameterStart);
	}
	lDone = 1;
      }
    } else if (strncmp (lParameterStart,"REUSER",6) == 0) {
      if(pParameterStruct) {
	pParameterStruct->reuseR = 1;
      }
      lParameterStart = lParameterStart + 6;
      if(*lParameterStart == ';') {
	lParameterStart++;
      }
      RPROXY_TRACE(printf("param REUSER, rest is \"%s\"\n",
			  lParameterStart));
    } else {
      lDone = 1;
    }
  }

#if 0
      /* NSIZE? */
      if (strncmp (lParameterStart,"NSIZE=",6) == 0)
	{
	  lParameterStart += 6;

	  lPosOfSemicolon = strchr (lParameterStart,';');
	  lIndexOfSemicolon = lPosOfSemicolon - lParameterStart;

	  if (lPosOfSemicolon)
	    {
	      lTmpBuffer = malloc (lIndexOfSemicolon + 1); /* to catch NSIZE=; */
	      strncpy (lTmpBuffer,lParameterStart,lIndexOfSemicolon);
	      *(lTmpBuffer + lIndexOfSemicolon) = 0x0;
	      pParameterStruct->nsize_valid = 1;
	      pParameterStruct->nsize = atoi(lTmpBuffer);
	      free (lTmpBuffer);
	      lParameterStart += lIndexOfSemicolon + 1;
	    }
	  else
	    {
	      pParameterStruct->nsize_valid = 1;
	      pParameterStruct->nsize = atoi(lParameterStart);
	      lDone = 1;
	    }
	}
      else if (strncmp (lParameterStart,"VSIZE=",6) == 0)
	{
	  lParameterStart += 6;

	  lPosOfSemicolon = strchr (lParameterStart,';');
	  lIndexOfSemicolon = lPosOfSemicolon - lParameterStart;

	  if (lPosOfSemicolon)
	    {
	      lTmpBuffer = malloc (lIndexOfSemicolon + 1); /* to catch VSIZE=; */
	      strncpy (lTmpBuffer,lParameterStart,lIndexOfSemicolon);
	      *(lTmpBuffer + lIndexOfSemicolon) = 0x0;
	      pParameterStruct->vsize_valid = 1;
	      pParameterStruct->vsize = atoi (lTmpBuffer);
	      free (lTmpBuffer);
	      lParameterStart += lIndexOfSemicolon + 1;
	    }
	  else
	    {
	      pParameterStruct->vsize_valid = 1;
	      pParameterStruct->vsize = atoi (lParameterStart);
	      lDone = 1;
	    }
	}
#endif

  return 0;
}


/* 00-02-18 | baier | R_Proxy_init() now takes parameter string, parse it */
/*
** 03-06-01 | baier | now we add %R_HOME%\bin to %PATH%
** 06-06-18 | baier | parameter parsing enabled in parent function
** 09-04-08 | baier | do not use Rf_InitEmbeddedR() on Windows, version check
**                    does not check minor subversion (2.8.0 and 2.8.1 are
**                    thought compatbile)
*/
int R_Proxy_init (char const* pParameterString)
{
  structRstart rp;
  Rstart Rp = &rp;
#if defined(__WINDOWS__)
  char lVersion[25];
  char lDLLVersion[25];
  static char RHome[MAX_PATH];

  int __strip_version(char* pVersion) {
    char* lTmp;
    lTmp = strchr(pVersion,'.');
    if(!lTmp) {
      /* something's wrong */
      RPROXY_ERR(printf("rscproxy> __strip_version: \"%s\" returns -1\n",
			pVersion));
      return -1;
    }
    lTmp = strchr(lTmp,'.');
    if(!lTmp) {
      /* something's wrong */
      RPROXY_ERR(printf("rscproxy> __strip_version: \"%s\" returns -2\n",
			pVersion));
      return -2;
    }
    *lTmp = 0x0;
    return 0;
  }
  snprintf(lVersion,sizeof(lVersion),"%s.%s", R_MAJOR, R_MINOR);
  strncpy(lDLLVersion,getDLLVersion(),sizeof(lDLLVersion));
  /* cut after x.y (e.g. 2.8.0 and 2.8.1 get 2.8 */
  if(__strip_version(lVersion) < 0) {
    return SC_PROXY_ERR_UNKNOWN;
  }
  if(__strip_version(lDLLVersion) < 0) {
    return SC_PROXY_ERR_UNKNOWN;
  }
  if(strncmp(lDLLVersion,lVersion,sizeof(lDLLVersion)) != 0) {
    RPROXY_ERR(printf("rscproxy> R.DLL version is %s.%s, expected %s (%s!=%s)\n",
		      R_MAJOR,R_MINOR,getDLLVersion(),lVersion,lDLLVersion));
    return SC_PROXY_ERR_INVALIDINTERPRETERVERSION;
  }
#endif

  R_DefParams(Rp);

#if defined(__WINDOWS__)
  /* first, try process-local environment space (CRT) */
  if (getenv("R_HOME")) {
      strcpy(RHome, getenv("R_HOME"));
  } else {
    /* get variable from process-local environment space (Windows API) */
      if (GetEnvironmentVariable ("R_HOME", RHome, sizeof (RHome)) == 0) {
	/* not found, fall back to getRHOME() */
	strcpy(RHome, getRHOME());
      }
    }

  /* now we add %R_HOME%\bin to %PATH% (for dynamically loaded modules there) */
  {
    char buf[2048];
    snprintf(buf, 2048, "PATH=%s\\bin;%s",RHome,getenv("PATH"));
    putenv(buf);
  }
#endif

#if defined(__WINDOWS__)
  Rp->rhome = RHome;
  Rp->home = getRUser();
  Rp->CharacterMode = LinkDLL;
  Rp->ReadConsole = R_Proxy_ReadConsole;
  Rp->WriteConsole = R_Proxy_WriteConsole;
  Rp->CallBack = R_Proxy_CallBack;
  Rp->ShowMessage = R_Proxy_askok;
  Rp->YesNoCancel = R_Proxy_askyesnocancel;
  Rp->Busy = R_Proxy_Busy;
#endif
  Rp->R_Quiet = 1;
  Rp->RestoreAction = SA_NORESTORE;
  Rp->SaveAction = SA_NOSAVE; /* had 2, with comment 'no save' which is 3 */

  R_SetParams(Rp);
  R_set_command_line_arguments(0, NULL);

#if defined(__WINDOWS__)
  GA_initapp(0, 0);
  readconsolecfg();
  setup_Rmainloop();
#else
  {
    /** 07-05-24 | TB | added --no-save as a temporary work-around */
    char* argv[] = { "rproxy", "--silent", "--no-save" };
    Rf_initEmbeddedR(3,argv);
    /*    R_Interactive = FALSE; */
  }
#endif
  R_ReplDLLinit();
#if !defined(__WINDOWS__)
#ifdef R_INTERFACE_PTRS
  ptr_R_ReadConsole = R_Proxy_ReadConsole;
  ptr_R_WriteConsole = R_Proxy_WriteConsole;
  ptr_R_WriteConsoleEx = R_Proxy_WriteConsoleEx;
  ptr_R_Busy = R_Proxy_Busy;
#else
#error missing interface pointers
#endif
#endif
  return SC_PROXY_OK;
}

int R_Proxy_evaluate (char const* pCmd, BDX_Data** pData)
{
    SEXP lSexp;
    int lRc = SC_PROXY_OK, evalError = 0;
    ParseStatus lStatus;
    SEXP lResult;

    lSexp = R_ParseVector(mkString(pCmd), 1, &lStatus, R_NilValue);
    /* This is an EXPRSXP: we assume just one expression */

    switch (lStatus) {
    case PARSE_OK:
	PROTECT(lSexp);
	lResult = R_tryEval(VECTOR_ELT(lSexp, 0), R_GlobalEnv, &evalError);
	UNPROTECT(1);
	if(evalError) lRc = SC_PROXY_ERR_EVALUATE_STOP;
	else lRc = SEXP2BDX(lResult, pData);
	break;
    case PARSE_INCOMPLETE:
	lRc = SC_PROXY_ERR_PARSE_INCOMPLETE;
	break;
    default:
	lRc = SC_PROXY_ERR_PARSE_INVALID;
	break;
    }
    return lRc;
}

int R_Proxy_evaluate_noreturn (char const* pCmd)
{
    SEXP lSexp;
    int lRc = SC_PROXY_OK, evalError = 0;
    ParseStatus lStatus;
    SEXP lResult;

    lSexp = R_ParseVector(mkString(pCmd), 1, &lStatus, R_NilValue);
    /* It would make sense to allow multiple expressions here */
  
    switch (lStatus) {
    case PARSE_OK:
	PROTECT(lSexp);
	lResult = R_tryEval(VECTOR_ELT(lSexp, 0), R_GlobalEnv, &evalError);
	UNPROTECT(1);
	if(evalError) lRc = SC_PROXY_ERR_EVALUATE_STOP;
	else lRc = SC_PROXY_OK;
	break;
    case PARSE_INCOMPLETE:
	lRc = SC_PROXY_ERR_PARSE_INCOMPLETE;
	break;
    default:
	lRc = SC_PROXY_ERR_PARSE_INVALID;
	break;
    }
    return lRc;
}

int R_Proxy_get_symbol (char const* pSymbol, BDX_Data** pData)
{
    SEXP lVar = findVar (install((char*) pSymbol), R_GlobalEnv);

    if (lVar == R_UnboundValue) {
	RPROXY_TRACE(printf(">> %s is an unbound value\n", pSymbol));
	return SC_PROXY_ERR_INVALIDSYMBOL;
    } else if(SEXP2BDX(lVar, pData) == 0)
	return SC_PROXY_OK;
    else
	return SC_PROXY_ERR_UNSUPPORTEDTYPE;
}

/* 04-02-19 | baier | don't PROTECT strings in a vector, new data structs */
/* 04-03-02 | baier | removed traces */
/* 04-10-15 | baier | no more BDX_VECTOR (only BDX_ARRAY) */
/* 05-05-16 | baier | use BDX2SEXP, clean-up */
int R_Proxy_set_symbol (char const* pSymbol, BDX_Data const* pData)
{
  SEXP lSymbol = 0;
  SEXP lData = 0;

  if(BDX2SEXP(pData,&lData) != 0) {
    return SC_PROXY_ERR_UNSUPPORTEDTYPE;
  }
  /*  RPROXY_TRACE(printf("ok BDX2SEXP\n")); */

  /* install a new symbol or get the existing symbol */
  lSymbol = install ((char*) pSymbol);

  /* and set the data to the symbol */
  setVar(lSymbol, lData, R_GlobalEnv);

  return SC_PROXY_OK;
}

int R_Proxy_term (void)
{
  /* end_Rmainloop(); note, this never returns */
  Rf_endEmbeddedR(0);

  return SC_PROXY_OK;
}

