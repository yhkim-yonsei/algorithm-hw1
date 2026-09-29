CC = gcc
CFLAGS = -std=c17 -O2 -Wall -Wextra

.PHONY: run test csv clean

sort.out: main.c
	$(CC) $(CFLAGS) -o sort.out main.c

run: sort.out
	./sort.out

test: sort.out
	./sort.out --csv > /dev/null
	@echo "모든 입력에서 정렬 확인 완료"

csv: sort.out
	./sort.out --csv > results.csv
	@echo "results.csv 저장 완료"

clean:
	rm -f sort.out results.csv
