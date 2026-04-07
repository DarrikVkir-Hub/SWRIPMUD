 /***************************************************************************
 *                   Star Wars: Rise in Power MUD Codebase                  *
 *--------------------------------------------------------------------------*
 * SWRiP Code Additions and changes from the SWReality and Smaug Code       *
 * copyright (c) 2001 by Mark Miller (Darrik Vequir)                        *
 *
 * This is a new automated arena for Smaug 1.4.
 * You can do anything you want with this code, I hope it will be
 * compatible with the DOS version.
 *
 * INSTALLATION:
 * Add to mud.h
 * in pc_data ...
 * char *     betted_on;
 * int 	      bet_amt;
 * down at the bottom of mud.h with all the rest of this stuff ...
 * #define GET_BETTED_ON(ch)    ((ch)->betted_on)
 * #define GET_BET_AMT(ch) ((ch)->bet_amt)
 *
 * change around the Makefile to include this file,
 * You also have to set the room flags in the limbo.are for the arena.
 * The arena bit is 67108864 (It's not included in the help roomflags)
 * This snippet is based on the ROM arena snippet by Kevin Hoogheem
 * It was ported to SMAUG1.4 by LrdElder
 * If you have any cool additions or questions just e-mail me at
 * tdison@swetland.net - LrdElder 10/24/98
 * Bugs fixed made for Star Wars: Rise in Power by Ulysses and Darrik Vequir
 */
 
#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mud.h"

#define PREP_START  42   /* vnum of first prep room */
#define PREP_END    43   /* vnum of last prep room */
#define ARENA_START number_range( 29, 41)    /* vnum of first real arena room*/
#define ARENA_END   41   /* vnum of last real arena room*/
#define HALL_FAME_FILE  SYSTEM_DIR "halloffame.lst"
#define ARENA_MAXBET 1000
struct hall_of_fame_element 
{
   char name[MAX_INPUT_LENGTH+1];
   time_t date;
   int award;
   struct  hall_of_fame_element *next;
};

struct struct_gladiator
{
	CHAR_DATA *ch;
	sh_int prev_hit;
	sh_int prev_move;
	sh_int prev_mana;
	sh_int place;
	struct struct_gladiator *next;
};


/*void sportschan(char *)*/
void start_arena(GameContext *game);
void show_jack_pot(GameContext *game);
void do_game(GameContext *game);
void find_game_winner(GameContext *game);
void do_end_game(GameContext *game);
void start_game(GameContext *game);
void silent_end(GameContext *game);
void write_fame_list(GameContext *game);
void write_one_fame_node(FILE * fp, struct hall_of_fame_element * node);
void load_hall_of_fame(GameContext *game);
void find_bet_winners(CHAR_DATA *winner);
void reset_bets(GameContext *game);
struct hall_of_fame_element *fame_list = NULL;
struct struct_gladiator *gladiators  = NULL;

int ppl_challenged = 0;
int in_start_arena = 0;
int start_time;
int game_length;
int lo_lim;
int hi_lim;
int time_to_start;
int time_left_in_game;
int arena_pot;
int bet_pot;
int barena = 0;
int num_gladiators = 0;


extern int parsebet (const int currentbet, char *s);
extern int advatoi (char *s);

void do_bet(CHAR_DATA *ch, char *argument)
 {
   char arg[MAX_INPUT_LENGTH];
   char buf[MAX_INPUT_LENGTH];
   char buf1[MAX_INPUT_LENGTH];
   int newbet;
       
   argument = one_argument(argument,arg);
   one_argument(argument,buf1);
           
   if (IS_NPC(ch))
   {
      send_to_char("Mobs cant bet on the arena.\n",ch);
      return;
   }

   if (BV_IS_SET(ch->in_room->room_flags, ROOM_ARENA))
   {
      send_to_char("Arena players can not make bets.",ch);
      return;
   }
                    
   if(arg[0]=='\0')
   {
      send_to_char("Usage: bet <player> <amt>\n",ch);
      return;
   }
   else if(!in_start_arena && !ppl_challenged)
   {
      send_to_char("Sorry the arena is closed, wait until it opens up to bet.\n", ch);
      return;
   }
   else if(num_gladiators > 0 && !in_start_arena)
   {
      send_to_char("Sorry Arena has already started, no more bets.\n", ch);
      return;
   }
   else if (!(ch->betted_on = get_char_world(ch, arg)))
      send_to_char("No such person exists in the galaxy.", ch);
   else if (ch->betted_on == ch)
      send_to_char("That doesn't make much sense, does it?\n", ch);
   else if(ch->in_room && !(BV_IS_SET(ch->betted_on->in_room->room_flags, ROOM_ARENA)))
      send_to_char("Sorry that person is not in the arena.\n", ch);
   else
      {
         if(GET_BET_AMT(ch) > 0)
         {
           send_to_char("Sorry you have already bet.\n", ch);
           return;
         }
         GET_BETTED_ON(ch) = ch->betted_on;
         newbet=parsebet(bet_pot,buf1); 
         if(newbet == 0)
         {
            send_to_char("Bet some gold why dont you!\n", ch);
            return;
         }
         if (newbet > ch->gold)
         {
            send_to_char("You don't have that much money!\n",ch);
            return;
         }
         if(newbet > ARENA_MAXBET)
         {
            send_to_char("Sorry the house will not accept that much.\n", ch);
            return;
         }
       
         ch->gold -= newbet;
         arena_pot += (newbet / 2);
         bet_pot += (newbet / 2);
         GET_BET_AMT(ch) = newbet;
         SPRINTF(buf, "You place %d credits on %s.\n", newbet, ch->betted_on->name);
         send_to_char(buf, ch);
         SPRINTF(buf,"%s has placed %d credits on %s.", ch->name,
         newbet, ch->betted_on->name);
         to_channel(buf,CHANNEL_ARENA,"&RArena&W",5);
  }
}

void do_arena(CHAR_DATA *ch, char *argument)
{
 char buf[MAX_INPUT_LENGTH];
 struct struct_gladiator *gladiator_node;
 if (IS_NPC(ch))
 {
   send_to_char("Mobs cant play in the arena.\n",ch);
   return;
 }

 if(!in_start_arena)
 {
   send_to_char("The killing fields are closed right now.\n", ch);
   return;
 }
 
 if(ch->top_level < lo_lim)
 {
   SPRINTF(buf, "Sorry but you must be at least level %d to enter this arena.\n", lo_lim);
   send_to_char(buf, ch);
   return;	
 }
 
 if( ch->top_level > hi_lim)
 {
    send_to_char("This arena is for lower level characters.\n", ch);
    return;
 } 
 
 if(BV_IS_SET(ch->in_room->room_flags, ROOM_ARENA))
 { 
    send_to_char("You are in the arena already\n",ch);
    return;
 }	
 else
 {
    act(AT_RED, "$n has been whisked away to the killing fields.", ch, NULL, NULL, TO_ROOM);
    ch->retran = ch->in_room->vnum;
    char_from_room(ch);
    char_to_room(ch, get_room_index(PREP_START)); 
    act(AT_WHITE,"$n is dropped from the sky.", ch, NULL, NULL, TO_ROOM);
    send_to_char("You have been taken to the killing fields\n",ch);
    do_look(ch, "auto");
    SPRINTF(buf, "%s has joined the blood bath.", ch->name);
    to_channel(buf,CHANNEL_ARENA,"&RArena&W",5);
    send_to_char(buf, ch);
    CREATE(gladiator_node, struct struct_gladiator, 1);
    gladiator_node->ch = ch;
    gladiator_node->prev_hit = ch->hit;
    gladiator_node->prev_move = ch->move;
    gladiator_node->prev_mana = ch->mana;
    gladiator_node->place = 0;                  //0 means still in it
    gladiator_node->next = gladiators;
    gladiators = gladiator_node; 
    num_gladiators++;
    return;
  }
}

void do_marena(CHAR_DATA *ch, char *argument)
{
 char buf[MAX_INPUT_LENGTH];
 struct struct_gladiator *gladiator_node;
 CHAR_DATA *mob;
 char arg[MAX_INPUT_LENGTH];

 one_argument( argument, arg );
 if ( arg[0] == '\0' ) {
   send_to_char( "marena whom?\n", ch );
   return;
 }
 if ( arg[0] != '\'' && arg[0] != '"' && strlen(argument) > strlen(arg) )
   SPRINTF( arg, "%s", argument );

 if ( ( mob = get_char_world( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\n", ch );
        return;
 }

 if (!IS_NPC(mob)) {
   send_to_char("You can not marena a player.\n",ch);
   return;
 }

 if(!in_start_arena)
 {
   send_to_char("The killing fields are closed right now.\n", ch);
   return;
 }

 if(BV_IS_SET(ch->in_room->room_flags, ROOM_ARENA))
 {
    send_to_char("You are in the arena already\n",ch);
    return;
 }
 else
 {
    act(AT_RED, "$n has been whisked away to the killing fields.", mob, NULL, NULL, TO_ROOM);
    SPRINTF(buf,"%s has been whisked away to the killing fields.\n",mob->name);
    send_to_char(buf,ch);
  
    mob->retran = mob->in_room->vnum;
    char_from_room(mob);
    char_to_room(mob, get_room_index(PREP_START));
    act(AT_WHITE,"$n is dropped from the sky.", mob, NULL, NULL, TO_ROOM);
    send_to_char("You have been taken to the killing fields\n",mob);
    do_look(mob, "auto");
    SPRINTF(buf, "%s has joined the blood bath.", mob->name);
    to_channel(buf,CHANNEL_ARENA,"&RArena&W",5);
    send_to_char(buf, mob);
    CREATE(gladiator_node, struct struct_gladiator, 1);
    gladiator_node->ch = mob;
    gladiator_node->prev_hit = mob->hit;
    gladiator_node->prev_move = mob->move;
    gladiator_node->prev_mana = mob->mana;
    gladiator_node->place = 0;                  //0 means still in it
    gladiator_node->next = gladiators;
    gladiators = gladiator_node;
    num_gladiators++;
    return;
  }
}

void do_chaos(CHAR_DATA *ch, char *argument)
{
  char lolimit[MAX_INPUT_LENGTH];
  char hilimit[MAX_INPUT_LENGTH], start_delay[MAX_INPUT_LENGTH];
  char length[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  char purse[MAX_INPUT_LENGTH];          
  /*Usage: chaos lo hi start_delay cost/lev length*/
        
  argument = one_argument(argument, lolimit);
  lo_lim = atoi(lolimit);
  argument = one_argument(argument, hilimit);
  hi_lim = atoi(hilimit);
  argument = one_argument(argument, start_delay);
  start_time = atoi(start_delay);
  argument = one_argument(argument, length);
  game_length = atoi(length);
  one_argument(argument, purse);
  arena_pot = atoi(purse);
                                 
  SPRINTF(buf,"LowLim %d HiLim %d Delay %d Length %d\n", lo_lim,
         hi_lim, start_time, game_length);
  send_to_char(buf,ch);
      
  if(hi_lim >= LEVEL_IMPLEMENTOR +1)
  {
     send_to_char("Please choose a hi_lim under the Imps level\n", ch);
     return;
  }
  
  if(!*lolimit || !*hilimit || !*start_delay || !*length)
  {
    send_to_char("Usage: chaos lo hi start_delay length [purse]", ch);
    return;
  }

  if (lo_lim >= hi_lim)
  {
    send_to_char("Ya that just might be smart.\n", ch);
    return;
  }
                              
  if ((lo_lim < 0 || hi_lim < 0 || game_length < 0))
  {
    send_to_char("I like positive numbers thank you.\n", ch);
    return;
  }
                                            
  if ( start_time <= 0)
  {
    send_to_char("Lets at least give them a chance to enter!\n", ch);
    return;
  }
  
  if (num_gladiators > 0 || in_start_arena) {
    send_to_char("The arena is already in use!\n",ch);
    return;
  }
  num_gladiators = 0;
  in_start_arena = 1;
  time_to_start = start_time;
  time_left_in_game =0;
  bet_pot = 0;
  barena = 1;
  start_arena(ch->game);
                                   
}
// Modernized s printf - AI/DV 3-13-26
void start_arena(GameContext *game)
{
    char buf1[256] = "";
    char buf2[256] = "";

    if (!ppl_challenged)
    {
        if (time_to_start == 0)
        {
            in_start_arena = 0;
            show_jack_pot(game);
            time_left_in_game = game_length;
            start_game(game);
        }
        else
        {
            // Arena announcement header
            to_channel(buf1, CHANNEL_ARENA, "&RArena&W", lo_lim);

            // Killing Fields message
            SPRINTF(buf1, 
                     "&WThe Killing Fields are open to levels &R%d &Wthru &R%d\n",
                     lo_lim, hi_lim);
            buf1[sizeof(buf1) - 1] = '\0';

            SPRINTF(buf2, "The killing fields are open.\n");
            buf2[sizeof(buf2) - 1] = '\0';

            // Append time-to-start
            size_t len1 = strlen(buf1);
            size_t len2 = strlen(buf2);

            if (time_to_start > 1)
            {
                snprintf(buf1 + len1, sizeof(buf1) - len1, "%d &Whours to start\n", time_to_start);
                snprintf(buf2 + len2, sizeof(buf2) - len2, "&R%d &Whour to start\n", time_to_start);
            }
            else
            {
                snprintf(buf1 + len1, sizeof(buf1) - len1, "1 &Whour to start\n");
                snprintf(buf2 + len2, sizeof(buf2) - len2, "&R1 &Whour to start\n");
            }

            // Final instructions / betting
            len1 = strlen(buf1);
            len2 = strlen(buf2);
            snprintf(buf1 + len1, sizeof(buf1) - len1, "Type &Rarena &Wto enter.\n");
            snprintf(buf2 + len2, sizeof(buf2) - len2, "Place your bets!!!\n");

            to_channel(buf2, CHANNEL_ARENA, "&RArena&W", 5);
            // echo_to_all(AT_WHITE, buf1, ECHOTAR_ALL);  // optional broadcast

            time_to_start--;
        }
    }
    else if (!num_gladiators)
    {
        if (time_to_start == 0)
        {
            ppl_challenged = 0;
            show_jack_pot(game);
            num_gladiators = 2;
            time_left_in_game = 5;
            start_game(game);
        }
        else
        {
            snprintf(buf1, sizeof(buf1),
                     "The duel will start in %d %s. Place your bets!",
                     time_to_start,
                     time_to_start > 1 ? "hours" : "hour");
            to_channel(buf1, CHANNEL_ARENA, "&RArena&W", 5);
            time_to_start--;
        }
    }
}

void start_game(GameContext *game)
{
  struct struct_gladiator *g;    
  for (g = gladiators; g; g=g->next) {
    if (g->ch == NULL)
      continue;
    if (1==1)  {  //yea, I know this is true all the time.  Bite me.     
      if (g->ch->in_room && BV_IS_SET(g->ch->in_room->room_flags, ROOM_ARENA)) {
        send_to_char("\nThe floor falls out from below, dropping you in the arena.\n", g->ch);
        char_from_room(g->ch);
        char_to_room(g->ch, get_room_index( ARENA_START));
        do_look(g->ch,"auto");
      }
    }
  }
  do_game(game);
}

void do_game(GameContext *game)
{
  struct struct_gladiator *g;
  int count;
  char buf[MAX_INPUT_LENGTH];
  
  if(!in_start_arena && num_gladiators == 1)
  {
    ppl_challenged = 0;
    find_game_winner(game);
    num_gladiators = 0;
  }
  else if(!in_start_arena && time_left_in_game == 0)
  {
    do_end_game(game);
  }
  else if(!in_start_arena  && num_gladiators == 0)
  {
    num_gladiators = 0;
    ppl_challenged = 0;
    silent_end(game);
  }
  else if(!in_start_arena && time_left_in_game % 5)
  {
     SPRINTF(buf, "With %d hours left in the game there are %d players left.",
             time_left_in_game, num_gladiators);
     to_channel(buf,CHANNEL_ARENA,"&RArena&W",5);
  }
  else if(!in_start_arena && time_left_in_game == 1)
  {
    SPRINTF(buf, "With 1 hour left in the game there are %d players left.",
                  num_gladiators);
    to_channel(buf,CHANNEL_ARENA,"&RArena&W",5);
  }
  else if(!in_start_arena && time_left_in_game <= 4)
  {
    SPRINTF(buf, "With %d hours left in the game there are %d players left.",
            time_left_in_game, num_gladiators);
    to_channel(buf,CHANNEL_ARENA,"&RArena&W",5);
  }
  time_left_in_game--;
  count = 0;
  for (g=gladiators; g; g=g->next) 
    if (g->ch && g->place == 0)
      count++;
  num_gladiators = count;
}

void find_game_winner(GameContext *game)
{
  char buf[MAX_INPUT_LENGTH];
//char buf2[MAX_INPUT_LENGTH];
  CHAR_DATA *i;
  struct struct_gladiator *g;
  struct hall_of_fame_element *fame_node;
          
  for(g=gladiators;g;g=g->next) {
    if (g->ch == NULL)
      continue;
    i = g->ch;
    if (i->in_room && BV_IS_SET(i->in_room->room_flags,ROOM_ARENA) && !IS_IMMORTAL(i) &&
	i->hit > 0 && g->place == 0) {
      remove_from_arena(i);
      if(time_left_in_game == 1) {
               SPRINTF(buf, "After 1 hour of battle %s is declared the winner",i->name);
               to_channel(buf,CHANNEL_ARENA,"&RArena&W",5);
      } else {
               SPRINTF(buf, "After %d hours of battle %s is declared the winner",
                     game_length - time_left_in_game, i->name);
               to_channel(buf,CHANNEL_ARENA,"&RArena&W",5);
      }

      SPRINTF(buf, "You have been awarded %d credits for winning the arena\n",
             (arena_pot/2));
      i->gold += (arena_pot/2);
      send_to_char(buf, i);
      bug("%s awarded %d credits for winning arena", i->name,
              (arena_pot/2));
      CREATE(fame_node, struct hall_of_fame_element, 1);
      strncpy(fame_node->name, i->name, MAX_INPUT_LENGTH);
      fame_node->name[MAX_INPUT_LENGTH] = '\0';
      fame_node->date = time(0);
      fame_node->award = (arena_pot/2);
      fame_node->next = fame_list;
      fame_list = fame_node;
      write_fame_list(game);
      find_bet_winners(i);
      reset_bets(game);
      ppl_challenged = 0;

    } 
  }
  g = gladiators;
  while(gladiators) {
     g=gladiators;
     gladiators=gladiators->next;
     free(g);
  }
  gladiators = NULL;
}

void show_jack_pot(GameContext *game)
{
  char buf1[MAX_INPUT_LENGTH];
  char buf2[MAX_INPUT_LENGTH];
  char buf3[MAX_INPUT_LENGTH];    
  SPRINTF(buf1, "\nLets get ready to RUMBLE!!!!!!!!\n");
  SPRINTF(buf2, "%.40sThe jack pot for this arena is %.60d credits\n",
  buf1, arena_pot);
  SPRINTF(buf3, "%.120s%.60d credits have been bet on this arena.\n",buf2, bet_pot);
  to_channel(buf3,CHANNEL_ARENA,"&RArena&W",5);
                    
}

void silent_end(GameContext *game)
{
  char buf[MAX_INPUT_LENGTH];
  struct struct_gladiator *g;
  ppl_challenged = 0;
  in_start_arena = 0;
  start_time = 0;
  game_length = 0;
  time_to_start = 0;
  time_left_in_game = 0;
  arena_pot = 0;
  bet_pot = 0;
  SPRINTF(buf, "It looks like no one was brave enough to enter the Arena.");
  to_channel(buf,CHANNEL_ARENA,"&RArena&W",5);
  reset_bets(game);
  g = gladiators;
  while(gladiators) {
     g=gladiators;
     gladiators=gladiators->next;
     free(g);
  }
  gladiators = NULL;
}
       
void do_end_game(GameContext *game)
{
  char buf[MAX_INPUT_LENGTH];
  struct struct_gladiator *g;
  CHAR_DATA *i;
      
  for(g = gladiators; g ; g = g->next) {
       i = g->ch;

       if (i == NULL)
         continue;

       if (i->in_room && BV_IS_SET(i->in_room->room_flags, ROOM_ARENA))
       {
          remove_from_arena(i);
       }
     }
     SPRINTF(buf, "After %d hours of battle the Match is a draw",game_length);
     to_channel(buf,CHANNEL_ARENA,"&RArena&W",5);
     time_left_in_game = 0;
     ppl_challenged = 0;
     reset_bets(game);
     g = gladiators;
     while(gladiators) {
       g=gladiators;
       gladiators=gladiators->next;
       free(g);
     }
     gladiators = NULL;

}                                                                                                                                                                  
                                                                                                        
/*void sportschan(char *argument)
{
  char buf1[MAX_INPUT_LENGTH];
  DESCRIPTOR_DATA *i;
        
  SPRINTF(buf1, "&R[Arena] &W%s\n", argument);
            
  for (i = first_descriptor; i; i = i->next)
  {
    if (!i->connected && i->character)
    {
       send_to_char(buf1, i->character);
    }
  }
}*/
                                                               
void do_awho(CHAR_DATA *ch, char *argument)
{
  struct struct_gladiator *g;
  char buf[MAX_INPUT_LENGTH];
  char buf2[MAX_INPUT_LENGTH];
  int num=num_gladiators;
          
  if(num==0 && ppl_challenged == 0)
  {
     send_to_char("There is noone in the arena right now.\n", ch);
     return;
  }
  
  send_to_char("&W  Players in the &BRise in Power&W Arena\n", ch);
  send_to_char("-&B-&W-&B-&W-&B-&W-&B-&W-&B-&W-&B-&W-&B-&W-&B-&W-&B-&W-&B-&W-&B-&W-&B-&W-", ch);  
  send_to_char("&B-&W-&B-&W-&B-&W-&B-&W-&B-&W-&B-&W-&B-&W-&B-&W-&B-&W-&B-&W-&B-&W-&B-&W-\n", ch);  

  SPRINTF(buf,"Game Length = &R%-3d   &WTime To Start &R%-3d\n", game_length, time_to_start);
  send_to_char(buf, ch);
  SPRINTF(buf,"&WLevel Limits &R%d &Wto &R%d\n", lo_lim, hi_lim);
  send_to_char(buf, ch);
  SPRINTF(buf,"         &WJackpot = &R%d\n",arena_pot);
  send_to_char(buf, ch);
  send_to_char("&W-&B-&W-&B-&W-&B-&W-&B-&W-&B-&W-&B-&W-&B-&W-&B-&W-&B-&W-&B-&W-&B-&W-&B-&W-&B", ch);
  send_to_char("-&W-&B-&W-&B-&W-&B-&W-&B-&W-&B-&W-&B-&W-&B-&W-&B-&W-&B-&W-&B-&W-&B-&W-&B-&W-&B\n", ch);
  send_to_char(buf, ch);

 
  for(g = gladiators; g; g=g->next)
    if (g->ch && g->place == 0) {
      SPRINTF(buf2, "&W%s\n", g->ch->name);
      send_to_char(buf2,ch);
    }
  return;
}

void do_ahall(CHAR_DATA *ch, char *argument)
{
  char site[MAX_INPUT_LENGTH], *timestr;
//char format[MAX_INPUT_LENGTH], format2[MAX_INPUT_LENGTH];
  struct hall_of_fame_element *fame_node;
      
  char buf[MAX_INPUT_LENGTH];
  char buf2[MAX_INPUT_LENGTH];
          
  if (!fame_list)
  {
     send_to_char("No-one is in the Hall of Fame.\n", ch);
     return;
  }
                                  
     SPRINTF(buf2, "&B|---------------------------------------|\n");
     STRAPP(buf2, "| &WPast Winners of The Rise in Power Arena&B  |\n");
     STRAPP(buf2, "|---------------------------------------|\n\n"); 

     send_to_char(buf2, ch);
     SPRINTF(buf, "%-25.25s  %-10.10s  %-16.16s\n",
      "&RName",
      "&RDate",
      "&RAward Amt");
     send_to_char(buf, ch);
     SPRINTF(buf, "%-25.25s  %-10.10s  %-16.16s\n",
        "&B---------------------------------",
        "&B---------------------------------",
        "&B---------------------------------");
 
     send_to_char(buf, ch);
//   SPRINTF(format2, "&W%-25.25s  &R%-10.10s  &Y%-16d\n");
     for (fame_node = fame_list; fame_node; fame_node = fame_node->next)
     {
        if (fame_node->date)
        {
           timestr = asctime(localtime(&(fame_node->date)));
           *(timestr + 10) = 0;
           SPRINTF(site, "%s", timestr);
        }
     else
        SPRINTF(site, "Unknown");
     SPRINTF(buf, "&W%-25.25s  &R%-10.10s  &Y%-16d\n", fame_node->name, site, fame_node->award);
     send_to_char(buf, ch);
     }
     return;
 }

void load_hall_of_fame(GameContext *game)
{
  FILE *fl;
  int date, award;
  char name[MAX_INPUT_LENGTH + 1];
  struct hall_of_fame_element *next_node;
        
  fame_list = 0;
          
  if (!(fl = fopen(HALL_FAME_FILE, "r")))
  {
    perror("Unable to open hall of fame file");
    return;
  }
  while (fscanf(fl, "%s %d %d", name, &date, &award) == 3)
  {
    CREATE(next_node, struct hall_of_fame_element, 1);
    strncpy(next_node->name, name, MAX_INPUT_LENGTH);
    next_node->date = date;
    next_node->award = award;
    next_node->next = fame_list;
    fame_list = next_node;
  }
  
  FCLOSE(fl);
  return;
}
                                                        
void write_fame_list(GameContext *game)
{
  FILE *fl;
  
  if (!(fl = fopen(HALL_FAME_FILE, "w")))
  {
     bug("Error writing _hall_of_fame_list", 0);
     return;
  }
  write_one_fame_node(fl, fame_list);/* recursively write from end to start */
  FCLOSE(fl);
                    
   return;
}

void write_one_fame_node(FILE * fp, struct hall_of_fame_element * node)
{
  if (node)
  {
    write_one_fame_node(fp, node->next);
    fprintf(fp, "%s %ld %d\n",node->name,(long) node->date, node->award);
  }
}

void find_bet_winners(CHAR_DATA *winner)
{
  DESCRIPTOR_DATA *d;
  CHAR_DATA *wch;
    
  char buf1[MAX_INPUT_LENGTH];
      
  for (d = first_descriptor; d; d = d->next)
    if (!d->connected)
    {
       wch = d->original ? d->original : d->character;

       if (wch == NULL)
         continue;
       
       if ((!IS_NPC(wch)) && (GET_BET_AMT(wch) > 0) && (GET_BETTED_ON(wch) == winner))
       {
          SPRINTF(buf1, "You have won %d credits on your bet.\n",(GET_BET_AMT(wch))*2);
          send_to_char(buf1, wch);
          wch->gold += GET_BET_AMT(wch)*2;
          GET_BETTED_ON(wch) = NULL;
          GET_BET_AMT(wch) = 0;
       }
    }
}

void do_challenge(CHAR_DATA *ch, char *argument)
{
 CHAR_DATA *victim;
 char buf[MAX_INPUT_LENGTH];
   
 if ( ch->challenged != NULL) {
   send_to_char("&WSomeone has challenged YOU already.\n",ch);
   return;
 }
 if ( ( victim = get_char_world( ch, argument ) ) == NULL)
 {
    send_to_char("&WThat character is not of these realms!\n",ch);  
    return;
 }
 
 if (IS_IMMORTAL(ch) || IS_IMMORTAL(victim))
 { 
    send_to_char("Sorry, Immortals are not allowed to participate in the arena.\n",ch);
    return;
 }
      
 if (IS_NPC(victim))
 {
    send_to_char("&WYou cannot challenge mobiles!\n",ch);
    return;
 }
 
 if (victim->name == ch->name)
 {
   send_to_char("&WYou cannot challenge yourself!",ch);
   return;
 }
 
 if (victim->top_level<5)
 {
   send_to_char("&WThat character is too young.\n",ch);
   return;
 }
 
 if ((!(ch->top_level-15<victim->top_level))||(!(ch->top_level+15>victim->top_level)))
 {
   send_to_char("&WThat character is out of your level range.\n",ch);
   return;
 }
 
 if (get_timer(victim,TIMER_PKILLED)>0)
 {
   send_to_char("&WThat player has died within the last 5 minutes and cannot be challenged!\n",ch);
   return;
 }
 
 if ( victim->top_level < 5 )
 {
   send_to_char("You are too young to die.\n",ch);
   return;
 }
 
 if (get_timer(ch,TIMER_PKILLED)>0)
 {
   send_to_char("&WYou have died within the last 5 minutes and cannot challenge anyone.\n",ch);
   return;
 }        
 
 if (num_gladiators>0 || in_start_arena)
 {
    send_to_char("&WSomeone is already in the arena!\n",ch);
    return;
 }
 SPRINTF(buf,"&R%s &Whas challenged you to a duel!\n",ch->name);
 send_to_char(buf,victim);
 send_to_char("&WPlease either accept or decline the challenge.\n\n",victim);
 SPRINTF(buf,"%s has challenged %s to a duel!!\n",ch->name,victim->name);
 to_channel(buf,CHANNEL_ARENA,"&RArena&W",5);
 victim->challenged = ch;
}

void do_mchallenge(CHAR_DATA *ch, char *argument)
{
 CHAR_DATA *victim;
 CHAR_DATA *mob;
 char buf[MAX_INPUT_LENGTH];
 char arg[MAX_INPUT_LENGTH];

 argument = one_argument( argument, arg );
 if ( arg[0] == '\0' ) {
   send_to_char( "mchallenge <mob> <player>\n", ch );
   return;
 }

 if ( ( mob = get_char_world( ch, arg ) ) == NULL ) {
        send_to_char( "They are not here.\n", ch );
        return;
 }

 if ( ( victim = get_char_world( ch, argument ) ) == NULL)
 {
    send_to_char("&WThat character is not of these realms!\n",ch);
    return;
 }

 if (!IS_NPC(mob))
 {
    send_to_char("&WYou must mchallenge with mobiles!\n",ch);
    return;
 }

 if (IS_NPC(victim))
 {
    send_to_char("&WYou cannot challenge mobiles!\n",ch);
    return;
 }

 if (victim->name == ch->name)
 {
   send_to_char("&WYou cannot challenge yourself!",ch);
   return;
 }

 if (victim->top_level<5)
 {
   send_to_char("&WThat character is too young.\n",ch);
   return;
 }

 if (get_timer(victim,TIMER_PKILLED)>0)
 {
   send_to_char("&WThat player has died within the last 5 minutes and cannot be challenged!\n",ch);
   return;
 }

 if (num_gladiators>0) {
    send_to_char("&WSomeone is already in the arena!\n",ch);
    return;
 }
 SPRINTF(buf,"&R%s &Whas challenged you to a duel!\n",mob->name);
 send_to_char(buf,victim);
 send_to_char("&WPlease either accept or decline the challenge.\n\n",victim);
 SPRINTF(buf,"%s has challenged %s to a duel!!\n",mob->name,victim->name);
 to_channel(buf,CHANNEL_ARENA,"&RArena&W",5);
 victim->challenged = mob;
}


void do_accept(CHAR_DATA *ch, char *argument)
{
  char buf[MAX_INPUT_LENGTH];
  struct struct_gladiator *gladiator_node;        
  if (num_gladiators>0)
  {
   send_to_char("Please wait until the current arena is closed before you accept.\n",ch);
   return;
  }
  
  if (!(ch->challenged))
  {
    send_to_char("You have not been challenged!\n",ch);
    return;
  }
  else
  {            
    CHAR_DATA *dch;
    dch = ch->challenged;

    if (!dch || !(dch->in_room) || !(dch->name) || (ch->name[0] == '\0'))
      return;
      
    if ( dch->in_room == ch->in_room )
    {
      send_to_char("You must be in a different room as your challenger.\n",ch);
    }
      
    if ( dch->in_room == ch->in_room )
    {
      send_to_char("You must be in a different room as your challenger.\n",ch);
      return;
    }
    
    SPRINTF(buf,"%s has accepted %ss challenge!\n",ch->name,dch->name);
    to_channel(buf,CHANNEL_ARENA,"&RArena&W",5);
    ch->challenged = NULL;
    ch->retran = ch->in_room->vnum;
    char_from_room(ch);
    char_to_room(ch, get_room_index(PREP_END));
    do_look(ch,"auto");
    CREATE(gladiator_node, struct struct_gladiator, 1);
    gladiator_node->ch = ch;
    gladiator_node->prev_hit = ch->hit;
    gladiator_node->prev_move = ch->move;
    gladiator_node->prev_mana = ch->mana;
    gladiator_node->place = 0;                  //0 means still in it
    gladiator_node->next = gladiators;
    gladiators = gladiator_node;
    dch->retran = dch->in_room->vnum;
    char_from_room(dch);
    char_to_room(dch, get_room_index(PREP_START));
    do_look(dch,"auto");
    CREATE(gladiator_node, struct struct_gladiator, 1);
    gladiator_node->ch = dch;
    gladiator_node->prev_hit = dch->hit;
    gladiator_node->prev_move = dch->move;
    gladiator_node->prev_mana = dch->mana;
    gladiator_node->place = 0;                  //0 means still in it
    gladiator_node->next = gladiators;
    gladiators = gladiator_node; 
    ppl_challenged = 1;
    time_to_start = 3;
    time_left_in_game =0;
    arena_pot =0;
    bet_pot = 0;
    num_gladiators=0;
    start_arena(ch->game);
    return;
   }
}

void do_decline(CHAR_DATA *ch, char *argument)
{
 char buf[MAX_INPUT_LENGTH];
 
 if (ch->challenged)
 {
   SPRINTF(buf,"%s has DECLINED %ss challenge! WHAT A WUSS!!!\n",ch->name,ch->challenged->name);
   to_channel(buf,CHANNEL_ARENA,"&RArena&W",5);
   ch->challenged=NULL;
   return;
 }
 else 
 {
   send_to_char("You have not been challenged!\n",ch);
   return;
 }
}                                                                                                                                                                                                 

/*
 * Reset bets for those that did not win.
 * Added by Ulysses, rewritten by Darrik Vequir.
 */
void reset_bets(GameContext *game)
{
  CHAR_DATA *ch;

  for (ch = first_char; ch; ch = ch->next )
    {
      if (ch == NULL)
        continue;

      if (!IS_NPC(ch))
      {
        GET_BETTED_ON(ch) = NULL;
        GET_BET_AMT(ch) = 0;
      }
    }
}

/* Remove a player from the arena */
void remove_from_arena(CHAR_DATA *ch) 
{
  struct struct_gladiator *g;
  char buf[MAX_INPUT_LENGTH];
  for(g = gladiators;g;g=g->next) {
    if (g->ch == ch) {
      g->place = num_gladiators;
      ch->hit = g->prev_hit;
      ch->mana = g->prev_mana;
      ch->move = g->prev_move;
      ch->challenged = NULL;
      stop_fighting( ch, TRUE );
      char_from_room(ch);
      char_to_room(ch,get_room_index(ch->retran));
      gmcp_evt_char_vitals(ch);
      do_look(ch,"auto");
      act(AT_YELLOW,"$n falls from the sky.", ch, NULL, NULL, TO_ROOM);
      switch (g->place) {
        case 1: SPRINTF(buf,"%s is out of the fight in 1st place.",ch->name); break;
        case 2: SPRINTF(buf,"%s is out of the fight in 2nd place.",ch->name); break;
        case 3: SPRINTF(buf,"%s is out of the fight in 3rd place.",ch->name); break;
        default:SPRINTF(buf,"%s is out of the fight in %dth place.",ch->name,g->place);
      }
      to_channel(buf,CHANNEL_ARENA,"&RArena&W",5);
      num_gladiators--;
    }
  }
}
