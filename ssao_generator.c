#include <stdio.h>
#include <math.h>
#include <stdlib.h>

void dump(float n)
{
	printf("%c%5f", n < 0 ? '-' : '+', fabs(n));
}

int main()
{
	srand(time(NULL));
	for (int i = 0; i < 128; ++i)
	{
		float x = rand() / (float)RAND_MAX * 2 - 1;
		float y = rand() / (float)RAND_MAX * 2 - 1;
		float z = rand() / (float)RAND_MAX;
		float len = sqrt(x * x + y * y + z * z);
		x /= len;
		y /= len;
		z /= len;
		float rnd = rand() / (float)RAND_MAX;
		x *= rnd;
		y *= rnd;
		z *= rnd;
		float scale = i / 128.;
		scale = scale * scale;
		scale = .1 + scale * .9;
		x *= scale;
		y *= scale;
		z *= scale;
		printf("vec3(");
		dump(x);
		printf(", ");
		dump(y);
		printf(", ");
		dump(z);
		printf("),");
		if (i % 4 == 3)
			printf("\n");
		else
			printf(" ");
	}
	return 0;
}
