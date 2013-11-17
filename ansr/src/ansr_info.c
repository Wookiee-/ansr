/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
*	ROM 2.4 is copyright 1993-1995 Russ Taylor			   *
*	ROM has been brought to you by the ROM consortium		   *
*	    Russ Taylor (rtaylor@pacinfo.com)				   *
*	    Gabrielle Taylor (gtaylor@pacinfo.com)			   *
*	    Brian Moore (rom@rom.efn.org)				   *
*	By using this code, you have agreed to follow the terms of the	   *
*	ROM license, in the file Rom24/doc/rom.license			   *
***************************************************************************/

/*************************************************************************** 
*       ROT 1.4 is copyright 1996-1997 by Russ Walsh                       * 
*       By using this code, you have agreed to follow the terms of the     * 
*       ROT license, in the file doc/rot.license                           * 
***************************************************************************/

#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include "merc.h"
#include "magic.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "interp.h"

/*
 * bitcount is used in roomcount procedure, everything what it does is that it
 * returns amount of bits set in the 8bit variable
 */
int bitcount(char c)
{
    int count = 0;

    if (c & A) count++;
    if (c & B) count++;
    if (c & C) count++;
    if (c & D) count++;
    if (c & E) count++;
    if (c & F) count++;
    if (c & G) count++;
    if (c & H) count++;
    
    return count;
}

/*
 * roomcount is called from do_score in act_info.c, it returns number of
 * explored rooms
 */
int roomcount(CHAR_DATA * ch)
{
    int index = 0;
    int count = 0;

    if (IS_NPC(ch)) return top_room;

    for ( index = 0; index < 8192; index++)
	count += bitcount(ch->pcdata->explored[index]);

    return count;
}

/*
 * getbit returns 1 if the specific bit is set, 0 otherwise, it is used by
 * fwrite_rle procedure.
 */
int getbit(char *explored, int index)
{
    int bit = 0;

    switch (index % 8)
    {
	case 0:	 bit = A; break;
	case 1:	 bit = B; break;
        case 2:	 bit = C; break;
        case 3:	 bit = D; break;
        case 4:	 bit = E; break;
        case 5:	 bit = F; break;
        case 6:	 bit = G; break;
        case 7:	 bit = H; break;
        default: bit = 0; break;
    }

    if (IS_SET(explored[index/8], bit)) return 1;
    else return 0;
    
}

/*
 * setbit sets the specific bit pointed on by the index [vnum] of the room,
 * there is no need for remove bit at this time. setbit is used in char_to_room
 * and also in fread_rle to restore the array from pfile.
 */
void set_bit(char *explored, int index)
{
    int bit = 0;

    switch (index % 8)
     {
	case 0:	 bit = A; break;
	case 1:	 bit = B; break;
        case 2:	 bit = C; break;
        case 3:	 bit = D; break;
	case 4:	 bit = E; break;
        case 5:	 bit = F; break;
	case 6:	 bit = G; break;
        case 7:	 bit = H; break;
	default: bit = 0; break;
    }

    SET_BIT(explored[index/8], bit);
    return;
}

/* fwrite_rle saves the status of the array in already open file it doesn't 
 * check for NPCs or anything, however it uses simple RLE-like bit compression
 * to save some space on the drive. this procedure is called from fwrite_char
 */
void fwrite_rle(char *explored, FILE *fp)
{
    int index;
    int bit = getbit(explored,0);
    int count = 0;

    fprintf(fp,"RoomRLE  ");

    for (index = 0; index < 32768; index++)
    {
	if (getbit(explored,index) == bit) count++;
	else
	{
	    fprintf(fp," %d %d",count,bit);
	    count = 1;
	    bit = getbit(explored,index);
	}
    }
    fprintf(fp," %d %d -1\n",count,bit);
    return;
}

/* fread_rle reads the data back in the bit array, it automatically
 * decompresses them to their original state, the bit compression was hammered
 * out on #rom channel with Peter Vidler :)
 */
void fread_rle(char *explored, FILE *fp)
{
    int index;
    int bit   = 0;
    int count = 0;
    int pos   = 0;

    index = 0;

    for (;;)
    {
	count = fread_number(fp);
	if (count < 0) break;

	bit = fread_number(fp);

	do 
	{
	    if (bit == 1) set_bit(explored,index);
	    index++;
	} while (index < pos+count);
	pos = index;
    }
    return;
}

void do_explored( CHAR_DATA *ch, char *argument )
{
    char arg[MIL];
    int offset;
    int bit;
    float count = 0;

    if (IS_NPC(ch)) return;

    one_argument(argument, arg);

    if ( arg[0] == '\0' )
    {
        for ( offset = 0; offset < 8192; offset++ )
            for( bit = 0; bit < 8; bit++ )
                if (ch->pcdata->explored[offset] & 1 << bit)
                    count++;

        printf_to_char(ch, "{wYou have explored {c%.0f {wof {c%d {wrooms "
                "{C({c%.2f%% {wof the mud{C)\n\r{x",
                count, top_room, count  / top_room );

        return;
    }

    if (!str_cmp(arg, "area"))
    {
        ROOM_INDEX_DATA *pRoom;
        int start, end;
        int area_count;
        int i;

        if (!ch->in_room ) return;

        start = ch->in_room->area->min_vnum;
        end   = ch->in_room->area->max_vnum;

        area_count = 0;

        for ( i = start; i <= end; i++ )
        {
            if ( ( pRoom = get_room_index(i) ) == NULL )
                continue;

            area_count++;

            offset = i / 8;
            bit    = i % 8;

            if (ch->pcdata->explored[offset] & 1 << bit )
                count++;
        }

        printf_to_char(ch, "{wYou have explored {c%.0f {wof {c%d {wrooms in "
                "this area. {C({c%.2f%%{C)\n\r{x", count, area_count,
                            (area_count != 0 ) ? count / area_count : 0 );

        return;
    }
    else
    {
        send_to_char("{wUsage:\n\r"
         "{wexplored      - {cShow total amount of the explored\n\r"
         "{wexplored area - {cShow total amount of the current area explored.\n\r",
         ch);

        return;
    }

    return;
}
