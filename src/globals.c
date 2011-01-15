/*globals.c*/


#include <exec/types.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/window.h>
#include <proto/button.h>
#include <proto/string.h>
#include <proto/layout.h>
#include <proto/label.h>
#include <proto/listbrowser.h>
#include <proto/asl.h>
#include <proto/elf.h>
#include <proto/utility.h>


#include <classes/window.h>
#include <gadgets/button.h>
#include <gadgets/listbrowser.h>
#include <images/label.h>

#include <reaction/reaction_macros.h>

#include <libraries/asl.h>
#include <interfaces/asl.h>

#include <libraries/elf.h>

#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "suspend.h"
#include "stabs.h"
#include "gui.h"
#include "variables.h"
#include "globals.h"
#include "freemem.h"

enum
{
	GLOBALS_BUTTON = 1,
	GLOBALS_LISTBROWSER
};

Object *GlobalsWinObj, *GlobalsButtonObj, *GlobalsListBrowserObj;
struct Window *globalswin = NULL;
struct ColumnInfo *globalscolumninfo = NULL;
BOOL globals_window_is_open = FALSE;

struct List globalslist;

struct List glob_freelist;
//BOOL has_globals_list = FALSE;

void globals_update_window()
{
	if (!globals_window_is_open)
		return;

	struct Node *node;

	//if (has_globals_list)
	
	IIntuition->SetAttrs( GlobalsListBrowserObj, LISTBROWSER_Labels, NULL, TAG_DONE );
	
	if(( !IsListEmpty( &globalslist )))
		globals_freelist();

	IExec->NewList (&globalslist);

	struct List *l = &(global_symbols);
	struct stab_symbol *s = (struct stab_symbol *)IExec->GetHead (l);

	if (s)
	while (1)
	{
	    TEXT buf[1024];
		char *str = print_variable_value(s);
		//add_freelist (&glob_freelist, 256, str);
		//char *namestr = IExec->AllocMem (1024, MEMF_ANY|MEMF_CLEAR);
		//add_freelist (&glob_freelist, 1024, namestr);

		if (s->type && s->type->type == T_POINTER)
			IUtility->SNPrintf(buf, sizeof(buf), "*%s", s->name);
		else
			IUtility->SNPrintf(buf, sizeof(buf), "%s", s->name);

		if (node = IListBrowser->AllocListBrowserNode(2,
            									LBNA_Column, 0,
            									LBNCA_CopyText, TRUE,
                								LBNCA_Text, buf, //namestr,
												LBNA_Column, 1,
												LBNCA_CopyText, TRUE,
												LBNCA_Text, str,
            									TAG_DONE))
        							{
							            IExec->AddTail(&globalslist, node);
									}
		if ((struct Node *)s == IExec->GetTail(l))
			break;
		s = (struct stab_symbol *)IExec->GetSucc((struct Node *)s);
	}

	//has_globals_list = TRUE;

	//if (globals_window_is_open) /* checked above */
		IIntuition->RefreshGadgets ((struct Gadget *)GlobalsListBrowserObj, globalswin, NULL);
		
	IIntuition->SetGadgetAttrs( (struct Gadget *)GlobalsListBrowserObj, globalswin, NULL, LISTBROWSER_Labels, &globalslist, LISTBROWSER_AutoFit, TRUE, TAG_DONE );
}

void globals_freelist()
{
	//if (has_globals_list)
	if(( !IsListEmpty( &globalslist )))
		freelist (&glob_freelist);
}

void globals_open_window()
{
	if (globals_window_is_open)
		return;

	IExec->NewList (&glob_freelist);
	IExec->NewList (&globalslist);

    globalscolumninfo = IListBrowser->AllocLBColumnInfo(2,
        LBCIA_Column, 0,
            LBCIA_Title, "Variable",
            //LBCIA_Weight, 35,
            LBCIA_Sortable, TRUE,
            LBCIA_AutoSort, TRUE,
            LBCIA_SortArrow, TRUE,
            LBCIA_DraggableSeparator, TRUE,
        LBCIA_Column, 1,
            LBCIA_Title, "Value",
        TAG_DONE);


    /* Create the window object. */
    if(( GlobalsWinObj = WindowObject,
            WA_ScreenTitle, "Debug 101",
            WA_Title, "Globals",
            WA_Width, 400,
            WA_Height, 400,
            WA_DepthGadget, TRUE,
            WA_SizeGadget, TRUE,
            WA_DragBar, TRUE,
            WA_CloseGadget, TRUE,
            WA_Activate, TRUE,
            WINDOW_Position, WPOS_CENTERSCREEN,
            /* there is no HintInfo array set up here, instead we define the 
            ** strings directly when each gadget is created (OM_NEW)
            */
            WINDOW_GadgetHelp, TRUE,
            WINDOW_ParentGroup, VLayoutObject,
                LAYOUT_SpaceOuter, TRUE,
                LAYOUT_DeferLayout, TRUE,
                
                LAYOUT_AddChild, GlobalsListBrowserObj = ListBrowserObject,
                    GA_ID,                      GLOBALS_LISTBROWSER,
                    GA_RelVerify,               TRUE,
                    GA_TabCycle,                TRUE,
                    LISTBROWSER_AutoFit,       TRUE,
                    LISTBROWSER_Labels,         &globalslist,
                    LISTBROWSER_ColumnInfo,     globalscolumninfo,
                    LISTBROWSER_ColumnTitles,   TRUE,
                    //LISTBROWSER_ShowSelected,   TRUE,
                    LISTBROWSER_Striping,       LBS_ROWS,
                    LISTBROWSER_SortColumn,     0,
                    LISTBROWSER_TitleClickable, TRUE,
                ListBrowserEnd,

                LAYOUT_AddChild, GlobalsButtonObj = ButtonObject,
                    GA_ID, GLOBALS_BUTTON,
                    GA_RelVerify, TRUE,
                    GA_Text, "Done",
                ButtonEnd,
                CHILD_WeightedHeight, 0,
            EndMember,
        EndWindow))
    {
        /*  Open the window. */
        if( globalswin = (struct Window *) RA_OpenWindow(GlobalsWinObj) )
        {
            globals_window_is_open = TRUE;
            globals_update_window();
        }
    }
}

uint32 globals_obtain_window_signal()
{
	uint32 signal = 0x0;
	if (globals_window_is_open)
		IIntuition->GetAttr( WINDOW_SigMask, GlobalsWinObj, &signal );

	return signal;
}


void globals_event_handler()
{
            ULONG wait, signal, result, done = FALSE;
            WORD Code;
            CONST_STRPTR hintinfo;
			uint32 selected;
            
            /* Obtain the window wait signal mask. */
            //IIntuition->GetAttr( WINDOW_SigMask, WinObj, &signal );
            

                while ((result = RA_HandleInput(GlobalsWinObj, &Code)) != WMHI_LASTMSG)
                {
                    switch (result & WMHI_CLASSMASK)
                    {
                        case WMHI_CLOSEWINDOW:
                            done = TRUE;
                            break;
                         case WMHI_GADGETUP:
                            switch(result & WMHI_GADGETMASK)
                            {
                                case GLOBALS_BUTTON:

									done = TRUE;
                                    break;
                            }
                            break;
                    }
					if (done)
					{
						globals_close_window();
						break;
					}
                }
}

void globals_close_window()
{
	if (globals_window_is_open)
	{
        /* Disposing of the window object will
         * also close the window if it is
         * already opened and it will dispose of
         * all objects attached to it.
         */
        IIntuition->DisposeObject( GlobalsWinObj );

		globals_freelist();
		IListBrowser->FreeLBColumnInfo(globalscolumninfo);

		globals_window_is_open = FALSE;
    }
    
    return;

}

