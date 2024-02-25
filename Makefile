.PHONY: clean

out: calc case_all
# Your code here.

clean:
	rm -f out calc casegen case_* *.o
