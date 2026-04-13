# SWRIPMUD
SWRIP MUD Codebases

SWRIP is the codebase used for the SWRIP MUD based on SWR (Star Wars Reality)

Github contains the latest changes as I work through FUSS bugfixes and memory leaks.

And continue upgrading the codebase in general:
    New custom 'OLC' system - in progress, but rooms, objects, mobs, and shops are up.
    Telnet protocols have been upgraded, handles MCCP, GMCP, NAWS, TTYPE, SGA, and MSSP.
    While doing that, the descriptor->game and game->descriptor flow has been improved substantially, although not fully cleaned up yet.  Also made things mostly UTF-8 safe.

Moved a lot of the bitmasks to use vector based bit flags (BitSet)

Started the process of encapsulating globals - only got sysdata up, but the GameContext class is passed everywhere (mostly with chars and descs)


Changed most (non-do_fun) functions to use std::strings instead of char*.
   I treated the char *argument in the do_funs as const, despite them not being there yet.  So that's an intended upgrade.
   I'm still mostly stuck casting into printfs as I haven't changed those to std::format.  Most act() calls can get away without it as i've overloaded it a lot, but occasionally you need to add c_strs to get it to compile since I'd need dozens of overloads to cover all of them, plus the ambiguous ones.

   Went through a full round of testing practically everything (as much as I can solo) while doing the above, so it's relatively clean at the moment.  I even found one in do_detrap from SWR!  Stay tuned, more bugs will be introduced soon I'm sure.

