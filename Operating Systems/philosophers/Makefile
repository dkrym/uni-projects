#
# Projekt: philosophers
# Autor:   David Krym
# Datum:   25.4.2010
# 
# Pou�it�:
#   - p�eklad:      make
#   - ladit:        make debug
#   - zabalit:      make pack
#   - vy�istit:     make clean
#   - vy�istit v�e: make clean-all
#

NAME=philosophers

CC=gcc                                            # p�eklada� jazyka C
CFLAGS=-std=gnu99 -Wall -Wextra -Werror -pedantic # parametry p�eklada�e

ALLFILES=$(NAME).c Makefile        # obsah projektu

$(NAME): $(NAME).c
	$(CC) $(CFLAGS) $(NAME).c -o $(NAME)

.PHONY: pack clean clean-exe clean-all


pack:
	tar cvzf xkrymd00.tar.gz $(ALLFILES)
	zip xkrymd00.zip $(ALLFILES)

clean:
	rm -f *~ *.bak
  
clean-exe:
	rm -f $(NAME)

clean-all: clean-exe clean
