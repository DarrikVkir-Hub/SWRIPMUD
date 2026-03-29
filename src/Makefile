CC      = g++
PROF    = 
NOCRYPT =
#Uncomment the next line if you want MCCP support
#DBUGFLG = -DMCCP
C_FLAGS = -g3 -Wall -MMD -MP -Wno-write-strings -fsanitize=address,undefined $(PROF) $(NOCRYPT) $(DBUGFLG)
L_FLAGS = -fsanitize=address,undefined -fno-omit-frame-pointer $(PROF)

C_FILES = act_comm.c act_info.c act_move.c act_obj.c act_wiz.c boards.c \
	  build.c clans.c comm.c comments.c const.c db.c fight.c \
	  handler.c hashstr.c id.c interp.c magic.c makeobjs.c \
	  misc.c mud_comm.c mud_prog.c krearena.c player.c requests.c \
	  reset.c save.c shops.c skills.c special.c tables.c track.c update.c \
	  space.c space2.c bounty.c swskills.c alias.c grub.c mapper.c templateparse.c trivia.c gmcp.c

O_FILES = $(C_FILES:.c=.o)

H_FILES = mud.h bet.h

all:
#	co $(H_FILES)
	make swrip
#	rm -f $(H_FILES)
	mv swrip ../bin/swrip

swrip: $(O_FILES)
	rm -f swrip
	$(CC) $(L_FLAGS) -o swrip $(O_FILES)  -lz -lm -lssl -lcrypto
	chmod g+w swrip
	chmod g+w $(O_FILES)

.c.o:
	$(CC) -c $(C_FLAGS) $< -o $@

clean:
	rm -f $(O_FILES) $(O_FILES:.o=.d) swrip

-include $(O_FILES:.o=.d)