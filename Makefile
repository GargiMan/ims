# Makefile IMS Project
# Marek Gergel (xgerge01)

TOPIC = T4
LOGIN = xgerge01
PROG_NAME = main
CC = g++
CCFLAGS := -O2 -std=c++11 -Wall -Wextra -pedantic
LDFLAGS := -lsimlib -lm

.PHONY: all run docs clean zip tar

all: $(PROG_NAME)

%: %.cpp
	$(CC) $(CCFLAGS) $< -o $@ $(LDFLAGS)

run: $(PROG_NAME) exp1 exp2

exp1:
	./$(PROG_NAME) 1 0.0 90.0 6.75 16.5 80.0 10.0 >exp1_1.out
	./$(PROG_NAME) 1 0.0 70.0 6.75 16.5 80.0 10.0 >exp1_2.out
	./$(PROG_NAME) 1 0.0 50.0 6.75 16.5 80.0 10.0 >exp1_3.out
	./$(PROG_NAME) 1 0.0 30.0 6.75 16.5 80.0 10.0 >exp1_4.out
	./$(PROG_NAME) 1 0.0 10.0 6.75 16.5 80.0 10.0 >exp1_5.out

exp2:
	./$(PROG_NAME) 2 0.0 20.0 7.00 16.0 80.0 80.0 >exp2_1.out
	./$(PROG_NAME) 4 0.0 20.0 7.00 16.0 80.0 80.0 >exp2_2.out
	./$(PROG_NAME) 6 0.0 20.0 7.00 16.0 80.0 80.0 >exp2_3.out
	./$(PROG_NAME) 8 0.0 20.0 7.00 16.0 80.0 80.0 >exp2_4.out
	./$(PROG_NAME) 10 0.0 20.0 7.00 16.0 80.0 80.0 >exp2_5.out

clean:
	$(RM) $(PROG_NAME)

docs: docs/sim_studia.md
	pandoc docs/sim_studia.md -o sim_studia.pdf -V geometry:margin=1in

zip: clean docs
	zip -r $(TOPIC)_$(LOGIN).zip $(PROG_NAME).* Makefile sim_studia.pdf

tar: clean docs
	tar -cvzf $(TOPIC)_$(LOGIN).tar.gz $(PROG_NAME).* Makefile sim_studia.pdf
