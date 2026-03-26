all: mydb run

mydb:
	gcc -Wall -Wextra -Werror -Iinclude src/*.c -o mydb

run: mydb
	./mydb

clean:
	rm -f mydb