#include <stdio.h>
#include <stdbool.h>

bool check(long n);

int main() {
	int n;
	scanf("%d", &n);

	if (check(n)) {
		printf("Y\n");
	} else {
		printf("N\n");
	}
	return 0;
}

bool check(long n) {
	long num = n;
	long reverse_num = 0;
	while (n) {
		reverse_num = reverse_num * 10 + n % 10;
		n /= 10;
	}
	return num == reverse_num;
}