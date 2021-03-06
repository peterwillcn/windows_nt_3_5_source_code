#include "sol.h"
#include <shellapi.h>  // To pick up ShellAbout()
VSZASSERT

#define rgbGreen RGB(0x00,0x80,0x00)
#define rgbWhite RGB(0xff,0xff,0xff)

PT ptNil = {0x7fff, 0x7fff};
CHAR szAppName[10];
CHAR szScore[10];
FARPROC lpfnTimerProc;

/* Instance info */
HWND hwndApp;
HANDLE hinstApp;
HANDLE hAccel;
BOOL fBW;
HBRUSH hbrTable;
LONG rgbTable;

BOOL fIconic = fFalse;

INT dyChar;
INT dxChar;

#define modeNil -1
INT modeFaceDown = modeNil;


GM *pgmCur = NULL;
/* card extent info */
DEL delCrd;
DEL delScreen;

RC rcClient;

INT igmCur;   /* the current game #, srand seeded with this */
#ifdef DEBUG
BOOL fScreenShots = fFalse;
#endif

/* window messages for external app drawing */
WORD wmCardDraw;


HDC hdcCur = NULL;
INT usehdcCur = 0;   /* hdcCur use count */
X xOrgCur = 0;
Y yOrgCur = 0;

CHAR szClass[] = "Solitaire";

CHAR szOOM[50];

// BUG: some of these should go in gm struct
//
BOOL fStatusBar   = fTrue;
BOOL fTimedGame   = fTrue;
BOOL fKeepScore   = fFalse;
SMD  smd          = smdStandard;  /* Score MoDe */
INT  ccrdDeal     = 3;
BOOL fOutlineDrag = fFalse;

BOOL fHalfCards = fFalse;


// Win 3.1 merge for resizing
//
INT xCardMargin;
#define MIN_MARGIN  (dxCrd / 8 + 3)

CHAR szNull[] = {'\000', '\000'};


#ifdef WINVER_2
HBITMAP hbmBackground = NULL;
#endif

/********************  Internal Functions ****************/
BOOL FSolInit( HANDLE, HANDLE, LPSTR, INT );
VOID GetIniFlags( BOOL * );
VOID APIENTRY cdtTerm( VOID );
VOID DoHelp( INT );


// International stuff
//
INT  iCurrency;
CHAR szCurrency[5];


/******************************************************************************
 * WINMAIN/ENTRY POINT
 *   This is the main entry-point for the application.  It uses the porting
 *   macro MMain() since it was ported from 16bit Windows.
 *
 *   The accelerator-table was added from demo-purposes.
 *
 *
 *****************************************************************************/
MMain( hinst, hinstPrev, lpszCmdLine, sw )

    MSG msg;


    // Initialize the application.
    //
    if (!FSolInit(hinst, hinstPrev, lpszCmdLine, sw))
            return(0);


    // Message-Polling loop.
    //
    msg.wParam = 1;
    while (GetMessage((LPMSG)&msg, NULL, 0, 0))
    {
        if( !TranslateAccelerator( hwndApp, hAccel, &msg ))
        {
            TranslateMessage((LPMSG)&msg);
            DispatchMessage((LPMSG)&msg);
        }
    }

    return (msg.wParam);

    // Eliminate unreferenced-variable warnings from
    // porting macro.
    //
    (void)_argv;
    (void)_argc;
}


/******************************************************************************
 *      FSolInit
 *
 *      Main program initialization.
 *
 *      Arguments:
 *              hinst - instance of this task
 *              hinstPrev - previous instance, or NULL if this is the
 *                      first instance
 *              lpszCmdLine - command line argument string
 *              sw - show window command
 *
 *      Returns:
 *              fFalse on failure.
 *
 *****************************************************************************/
BOOL FSolInit(HANDLE hinst, HANDLE hinstPrev, LPSTR lpszCmdLine, INT sw)
{
        WNDCLASS   cls;
        HDC        hdc;
        TEXTMETRIC tm;
        HANDLE     hcrsArrow;
        BOOL       fStartIconic;
        CHAR FAR   *lpch;
        BOOL       fOutline;
        CHAR       szT[20];
        RECT       rect;
        extern HWND hwndApp;
        extern HANDLE hinstApp;
        extern HBRUSH hbrTable;
        extern BOOL fBW;
        extern DX dxMScreen;
        extern DY dyMScreen;
        extern INT modeFaceDown;
        extern INT igmCur;
        extern WORD wmCardDraw;
//      INT cdecl rand();
        WORD APIENTRY TimerProc(HWND, WORD, INT, DWORD);
        LONG APIENTRY SolWndProc(HWND, WORD, WPARAM, LONG);
        LONG APIENTRY BackPushProc();

        hinstApp = hinst;

        /* create stock objects */

        CchString(szOOM, idsOOM);
        if(!cdtInit((INT FAR *)&dxCrd, (INT FAR *)&dyCrd))
                goto OOMError;
        hcrsArrow = LoadCursor(NULL, IDC_ARROW);
        hdc = GetDC(NULL);
        if(hdc == NULL)
                {
OOMError:
                OOM();
                return fFalse;
                }

        GetTextMetrics(hdc, (LPTEXTMETRIC)&tm);
        dyChar = tm.tmHeight;
        dxChar = tm.tmMaxCharWidth;
        if (GetDeviceCaps(hdc, NUMCOLORS) == 2)
                fBW = fTrue;

        /* BUG:  if HORZRES not big enough, have to call cdtDrawExt & shrink dxCrd */
        /* BUG:  Need to check VERTRES and divide dxCrd by 2 (esp w/ lores ega) */
        dxScreen = GetDeviceCaps(hdc, HORZRES);
        dyScreen = GetDeviceCaps(hdc, VERTRES);
        if(fHalfCards = dyScreen < 300)
                dyCrd /= 2;
        ReleaseDC(NULL, hdc);
        rgbTable = fBW ? rgbWhite : rgbGreen;
        hbrTable = CreateSolidBrush(rgbTable);

        srand((WORD) time(NULL));

        /* load strings */
        CchString(szAppName, idsAppName);
        CchString(szScore, idsScore);

        CchString(szT, idsCardDraw);
        wmCardDraw = RegisterWindowMessage(szT);

        /* scan cmd line to see if should come up iconic */
        /* this may be unnecessary with win3.0 (function may be provided to */
        /* do it automatically */

        fStartIconic = fFalse;
        for(lpch = lpszCmdLine; *lpch != '\000'; lpch++)
                {
                if(*lpch == '/' && *(lpch+1) == 'I')
                        {
                        fStartIconic = fTrue;
                        break;
                        }
                }


        /* register window classes */

        if (hinstPrev == NULL)
                {
                cls.style = CS_BYTEALIGNWINDOW | CS_DBLCLKS,
                cls.lpfnWndProc = (WNDPROC)SolWndProc;
                cls.cbClsExtra = cls.cbWndExtra = 0;
                cls.hInstance = hinstApp;
                cls.hIcon = LoadIcon(hinstApp, "Sol");
                cls.hCursor = hcrsArrow;
#ifdef OLD /* WINVER_2 */
                cls.hbrBackground = NULL;
#else
                cls.hbrBackground = hbrTable;
#endif
                cls.lpszMenuName = MAKEINTRESOURCE(idmSol);
                cls.lpszClassName = (LPSTR)szClass;
                if (!RegisterClass((LPWNDCLASS)&cls))
                        goto OOMError;
                }

// BabakJ: Merge from 3.1 --------------------------------------
	/* Determine the proper starting size for the window */

	/* Card margin is just a little bigger than 1/8 of a card */
	xCardMargin = MIN_MARGIN;
	
	/* We need 7 card widths and 8 margins */
	rect.right = dxCrd * 7 + 8 * xCardMargin;

	/* Compute the window size we need for a client area this big */
	rect.bottom = dyCrd * 4;
	rect.left = rect.top = 0;
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, TRUE);
	rect.right -= rect.left;
	rect.bottom -= rect.top;

	/* Make sure it's not too big */
	if (rect.bottom > dyScreen)
	    rect.bottom = dyScreen;

        /* create our windows */
        if (!
        (hwndApp = CreateWindow((LPSTR)szClass, (LPSTR)szAppName,
                        fStartIconic ? WS_OVERLAPPEDWINDOW | WS_MINIMIZE | WS_CLIPCHILDREN:
				    WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                        CW_USEDEFAULT, 0,
			rect.right, rect.bottom,
                        (HWND)NULL, (HMENU)NULL, hinstApp, (LPSTR)NULL)))
            goto OOMError;

// -----------------------------------------------------------------

        GetIniFlags(&fOutline);

        if((lpfnTimerProc = MakeProcInstance((FARPROC) TimerProc, hinst)) == NULL)
                return fFalse;

        if(SetTimer(hwndApp, 666, 250, (TIMERPROC)lpfnTimerProc) == 0)
                goto OOMError;

        FInitGm();
        FSetDrag(fOutline);

#ifdef BZZZ
        InitHighScore();
#endif

        ShowWindow(hwndApp, sw);
        UpdateWindow(hwndApp);

	// Babakj: accel for force win
        hAccel = LoadAccelerators( hinst, "HiddenAccel" );

        FRegisterStat(hinstPrev == NULL);
        if(fStatusBar)
                FCreateStat();

        Assert(pgmCur != NULL);
        if(sw != SW_SHOWMINNOACTIVE && sw != SW_MINIMIZE)
                PostMessage(hwndApp, WM_COMMAND, idsInitiate, 0L);
        return(fTrue);
}



VOID DoPaint(HWND hwnd)
        {
        PAINTSTRUCT paint;
        extern GM *pgmCur;

        BeginPaint(hwnd, (LPPAINTSTRUCT) &paint);
        if(pgmCur)
                SendGmMsg(pgmCur, msggPaint, (INT) &paint, 0);
        EndPaint(hwnd, (LPPAINTSTRUCT) &paint);
        }


/*      SolWndProc
 *
 *      Window procedure for main Sol window.
 *
 *      Arguments:
 *              hwnd - window handle receiving the message - should
 *                      be hwndSol
 *              wm - window message
 *              wParam, lParam - more info as required by wm
 *
 *      Returns:
 *              depends on the message
 */
LONG APIENTRY SolWndProc(HWND hwnd, WORD wm, WPARAM wParam, LONG lParam)
        {
        HMENU hmenu;
        PT pt;
        INT msgg;
        extern GM *pgmCur;
        extern HWND hwndStat;
        VOID NewGame();
        VOID DdeInitiate();
        VOID DdeRequest();
        VOID DdePoke();
        VOID SetTitle();
        VOID Fonts();
        VOID StatString();

        // babakj: Debug code:
        //CHAR szDebugOut[80];
        //sprintf( szDebugOut, "Msg= %hX,  wParam= %X, lParam=%X \n", wm, wParam, lParam );
        //OutputDebugString( szDebugOut );



        switch (wm)
                {
        default:
                if(wm == wmCardDraw)
                        {
                        switch(wParam)
                                {
                        case drwInit:
                                return MAKELONG(dxCrd, dyCrd);

                        case drwDrawCard:
#define lpcddr ((CDDR FAR *)lParam)
                                return cdtDraw(lpcddr->hdc, lpcddr->x, lpcddr->y, lpcddr->cd, lpcddr->mode, lpcddr->rgbBgnd);
#undef lpcddr

                        case drwClose:
                                PostMessage(hwndApp, WM_SYSCOMMAND, SC_CLOSE, 0L);
                                return fTrue;
                                }
                        }
                break;

// Babakj: Removed for the addition of the accelerators
//	case WM_CREATE:
//              hwndApp = hwnd;
//              break;

        case WM_DESTROY:
                KillTimer(hwndApp, 666);
                FreeProcInstance(lpfnTimerProc);
                SendGmMsg(pgmCur, msggEnd, 0, 0);
                FSetDrag(fTrue);                /* Free up screen bitmaps if we made em */
                cdtTerm();
                DeleteObject(hbrTable);
                PostQuitMessage(0);
                break;

        case WM_ACTIVATE:
                if( GET_WM_ACTIVATE_STATE(wParam, lParam) &&
		   !GET_WM_ACTIVATE_FMINIMIZED(wParam, lParam) )
                        DoPaint(hwnd);
                break;

        case WM_KILLFOCUS:
            if(pgmCur->fButtonDown)
                SendGmMsg(pgmCur, msggMouseUp, 0, fTrue);
            /* Fall through. */
        case WM_SETFOCUS:
                ShowCursor(wm == WM_SETFOCUS);
                break;


        case WM_SIZE:
	{
	    int nNewMargin;
	    int nMinMargin;

	    fIconic = IsIconic(hwnd);
	    GetClientRect(hwnd, (LPRECT) &rcClient);

	    /* Compute the new margin size if any and if necessary, redraw */
	    nNewMargin = ((short)lParam - 7 * (short)dxCrd) / 8;
	    nMinMargin = MIN_MARGIN;
	    if (nNewMargin < nMinMargin && xCardMargin != nMinMargin)
		nNewMargin = nMinMargin;
	    if (nNewMargin >= nMinMargin)
	    {
		xCardMargin = nNewMargin;
		PositionCols();
		InvalidateRect(hwnd, NULL, TRUE);
	    }

	    /* Code always falls through here */
       }


        case WM_MOVE:
                StatMove();
                break;


        case WM_MENUSELECT:
	        // Don't send in garbage if not a menu item
	        if( GET_WM_MENUSELECT_FLAGS( wParam, lParam ) & MF_POPUP     ||
		    GET_WM_MENUSELECT_FLAGS( wParam, lParam ) & MF_SYSMENU   ||
		    GET_WM_MENUSELECT_FLAGS( wParam, lParam ) & MF_SEPARATOR ) {

		    StatString(idsNil);
		}
		else {
		    StatString( GET_WM_MENUSELECT_CMD( wParam, lParam ));
		}
                break;

        case WM_KEYDOWN:
                Assert(pgmCur);
                SendGmMsg(pgmCur, msggKeyHit, wParam, 0);
                break;

        case WM_LBUTTONDOWN:
/*              ProfStart(); */
                SetCapture(hwnd);
                if(pgmCur->fButtonDown)
                        break;
                msgg = msggMouseDown;
                goto DoMouse;

        case WM_LBUTTONDBLCLK:
                msgg = msggMouseDblClk;
                if(pgmCur->fButtonDown)
                        break;
                goto DoMouse;

        case WM_LBUTTONUP:
/*              ProfStop(); */
                ReleaseCapture();
                msgg = msggMouseUp;
                if(!pgmCur->fButtonDown)
                        break;
                goto DoMouse;

        case WM_MOUSEMOVE:
                msgg = msggMouseMove;
                if(!pgmCur->fButtonDown)
                        break;
DoMouse:
                Assert(pgmCur != NULL);
// BabakJ: replaced with macro from port layer
//              pt = *(PT *)&lParam;
                LONG2POINT( lParam, pt );
                Assert(pgmCur);
                SendGmMsg(pgmCur, msgg, (INT) &pt, 0);
                break;


        case WM_COMMAND:
                switch( GET_WM_COMMAND_ID( wParam, lParam ))
                        {
                /* Game menu */
                case idsInitiate:
                        NewGame(fTrue, fFalse);
                        break;
                case idsUndo:
                        Assert(pgmCur);
                        SendGmMsg(pgmCur, msggUndo, 0, 0);
                        break;
                case idsBacks:
                        DoBacks();
                        break;
                case idsOptions:
                        DoOptions();
                        break;
                case idsExit:
                        PostMessage(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0L);
                        break;
                /* Help Menu */
                case (WORD)idsHelpIndex:
                case (WORD)idsHelpSearch:
                case (WORD)idsHelpUsing:
                        DoHelp( (INT)(SHORT)GET_WM_COMMAND_ID( wParam, lParam ));
                        break;
                case idsAbout:
                {
                    char szExtraInfo[100];
                    CchString( szExtraInfo, idsExtraInfo );
                    ShellAbout(hwnd, szAppName, szExtraInfo, LoadIcon( hinstApp, "Sol" ));
                    break;
                }
		case idsForceWin:
                        SendGmMsg(pgmCur, msggForceWin, 0, 0);
                        break;
#ifdef DEBUG
                case idsGameNo:
                        if(FSetGameNo())
                                NewGame(fFalse, fFalse);
                        break;

                case idsCardMacs:
                        PrintCardMacs(pgmCur);
                        break;
                case idsAssertFail:
                        Assert(fFalse);
                        break;
                case idsMarquee:
                        break;

                case idsScreenShots:
                        fScreenShots ^= 1;
                        CheckMenuItem(GetMenu(hwnd), idsScreenShots, fScreenShots ? MF_CHECKED|MF_BYCOMMAND : MF_UNCHECKED|MF_BYCOMMAND);
                        InvalidateRect(hwndStat, NULL, fTrue);
                        if(fScreenShots)
                                InvalidateRect(hwnd, NULL, fTrue);
                        break;
#endif
                default:
                        break;
                        }
                break;

        case WM_INITMENU:
                hmenu = GetMenu(hwnd);
                Assert(pgmCur);
                EnableMenuItem(hmenu, idsUndo,
                        pgmCur->udr.fAvail && !FSelOfGm(pgmCur) ? MF_ENABLED : MF_DISABLED|MF_GRAYED);
                EnableMenuItem(hmenu, idsInitiate, FSelOfGm(pgmCur) ? MF_DISABLED|MF_GRAYED : MF_ENABLED);
                EnableMenuItem(hmenu, idsBacks,    FSelOfGm(pgmCur) ? MF_DISABLED|MF_GRAYED : MF_ENABLED);
                EnableMenuItem(hmenu, idsAbout,    FSelOfGm(pgmCur) ? MF_DISABLED|MF_GRAYED : MF_ENABLED);
                break;

        case WM_PAINT:
                if(!fIconic)
                        {
                        DoPaint(hwnd);
                        return(0L);
                        }
                break;
                }

        return(DefWindowProc(hwnd, wm, wParam, lParam));
        }



HDC HdcSet(HDC hdc, X xOrg, Y yOrg)
        {
        HDC hdcT = hdcCur;
        hdcCur = hdc;
        xOrgCur = xOrg;
        yOrgCur = yOrg;
        return hdcT;
        }



BOOL FGetHdc()
        {
        HDC hdc;

        Assert(hwndApp);
        if(hdcCur != NULL)
                {
                usehdcCur++;
                return fTrue;
                }

        hdc = GetDC(hwndApp);
        if(hdc == NULL)
                return fFalse;
        HdcSet(hdc, 0, 0);
        usehdcCur = 1;
        return fTrue;
        }


VOID ReleaseHdc()
        {
        if(hdcCur == NULL)
                return;
        if(--usehdcCur == 0)
                {
                ReleaseDC(hwndApp, hdcCur);
                hdcCur = NULL;
                }
        }


WORD APIENTRY TimerProc(HWND hwnd, WORD wm, INT id, DWORD dwTime)
        {
        extern GM *pgmCur;

        if(pgmCur != NULL)
		SendGmMsg(pgmCur, msggTimer, 0, 0);
        return fTrue;
        }





VOID ChangeBack(INT mode)
        {
        extern HWND hwndApp;
        extern INT modeFaceDown;

        if(mode == modeFaceDown)
                return;
        modeFaceDown = mode;
        InvalidateRect(hwndApp, NULL, fTrue);
        }


VOID NewGame(BOOL fNewSeed, BOOL fZeroScore)
        {
        extern GM *pgmCur;

#ifdef DEBUG
        InitDebug();
#endif
        if(fNewSeed)
                srand( igmCur = ((WORD) time(NULL)) & 0x7fff);

#ifdef DEBUG
        SendGmMsg(pgmCur, msggChangeScore, 0, 0);
#endif
        SendGmMsg(pgmCur, msggDeal, fZeroScore, 0);
        }



BOOL APIENTRY About(HWND hdlg, WORD iMessage, WPARAM wParam, LONG lParam)
        {
        if (iMessage == WM_COMMAND)
                {
                EndDialog(hdlg,fTrue);
                return fTrue;
                }
        else if (iMessage == WM_INITDIALOG)
                return fTrue;
        else
                return fFalse;
        }


// BabakJ: 12/11/92,  merged with 3.1 sources.
VOID DoHelp(INT idContext)
    {
    CHAR sz[100];
    BOOL fResult;
    CchString(sz, idsHelpFile);
    switch(idContext)
      {
        case idsHelpUsing:
          fResult = WinHelp(hwndApp, (LPSTR)NULL, HELP_HELPONHELP, (DWORD)0);
          break;
        case idsHelpIndex:
          fResult = WinHelp(hwndApp, sz, HELP_INDEX, (DWORD)0);
          break;
        case idsHelpSearch:
	  fResult = WinHelp(hwndApp, sz, HELP_PARTIALKEY, (DWORD)(LPSTR)"");
          break;
      }
    if(!fResult)
        ErrorIds(idsNoHelp);
    }


VOID GetIniFlags(BOOL *pfOutline)
{
        INI    ini;
        INT    mode;
        extern CHAR szAppName[];

        ini.w = 0;
        ini.grbit.fStatusBar = fStatusBar;
        ini.grbit.fTimedGame = fTimedGame;
        ini.grbit.fOutlineDrag = fOutlineDrag;
        ini.grbit.fDrawThree = ccrdDeal == 3;
        ini.grbit.fKeepScore = fKeepScore;
        ini.grbit.fSMD = 0;

        ini.w = GetIniInt(idsAppName, idsOpts, ini.w);

	// BabakJ: Assigning the bit field to the boolean puts 0xfffffffff in it!! Is it a bug in compiler!?
        fStatusBar = ini.grbit.fStatusBar ? 1 : 0;
        fTimedGame = ini.grbit.fTimedGame ? 1 : 0;
        *pfOutline = ini.grbit.fOutlineDrag ? 1 : 0;
        ccrdDeal = ini.grbit.fDrawThree ? 3 : 1;
        fKeepScore = ini.grbit.fKeepScore ? 1 : 0;
        switch(ini.grbit.fSMD)
                {
        default:
                smd = smdStandard;
                break;
        case 1:
                smd = smdVegas;
                break;
        case 2:
                smd = smdNone;
                break;
                }

        mode = GetIniInt(idsAppName, idsBack, rand() % cIDFACEDOWN) + IDFACEDOWNFIRST-1;
        ChangeBack(PegRange(mode, IDFACEDOWNFIRST, IDFACEDOWN12));

#ifdef WINVER_2
        {
             CHAR szBmp[128];

             FGetIniString(idsAppName, idsBitmap, szBmp, szNull, sizeof(szBmp));
             FInitBgnd(szBmp);
        }
#endif

        iCurrency = GetIniInt(idsIntl, idsiCurrency, 0);
        FGetIniString(idsIntl, idssCurrency, szCurrency, "$", sizeof(szCurrency));
        }


VOID WriteIniFlags(INT wif)
        {
        INI ini;
        extern CHAR szAppName[];

        if(wif & wifOpts)
                {
                ini.w = 0;
                ini.grbit.fStatusBar = fStatusBar;
                ini.grbit.fTimedGame = fTimedGame;
                ini.grbit.fOutlineDrag = fOutlineDrag;
                ini.grbit.fDrawThree = ccrdDeal == 3;
                ini.grbit.fKeepScore = fKeepScore;
                switch(smd)
                        {
                default:
                        Assert(fFalse);
                        break;
                case smdStandard:
                        ini.grbit.fSMD = 0;
                        break;
                case smdVegas:
                        ini.grbit.fSMD = 1;
                        break;
                case smdNone:
                        ini.grbit.fSMD = 2;
                        break;
                        }

                FWriteIniInt(idsAppName, idsOpts, ini.w);
                }
#ifdef WINVER_2
        {
            CHAR szIni[128];
            if(wif & wifBitmap)
            {
                if(FGetBgndFile(szIni))
                    FWriteIniString(idsAppName, idsBitmap, szIni);
            }
        }
#endif
        if(wif & wifBack)
                FWriteIniInt(idsAppName, idsBack, modeFaceDown-IDFACEDOWNFIRST+1);
        }
