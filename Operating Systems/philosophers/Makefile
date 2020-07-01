#
# Projekt: philosophers
# Autor:   David Krym
# Datum:   25.4.2010
# 
# Pou¾ití:
#   - pøeklad:      make
#   - ladit:        make debug
#   - zabalit:      make pack
#   - vyèistit:     make clean
#   - vyèistit v¹e: make clean-all
#

NAME=philosophers

CC=gcc                                            # pøekladaè jazyka C
CFLAGS=-std=gnu99 -Wall -Wextra -Werror -pedantic # parametry pøekladaèe

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
