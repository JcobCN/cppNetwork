#include <stdio.h>

int main()
{
	char *a = "sun \"abc\" 123";
	
	char a1[64];
char a2[64];
char a3[64];

	sscanf(a, "%s %[\"]%s%[\"] %s", a1, a2, a3);
	printf("a1=%s, a2=%s, a3=%s, a=%s\n", a1, a2, a3, a);
}
