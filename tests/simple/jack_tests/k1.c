/*
 * Kernel 1 Hydro
 *
 * Translated from Fortran by Sanjay Jinturkar 
 * 6/28/92
 */
#define LOOP 50000

#ifndef TIMES
#define TIMES 100
#endif

main()
{
   double x[LOOP], y[LOOP], zx[LOOP+23];
   int i;

   for (i = 0; i < LOOP; i++)
      x[i] = y[i] = zx[i] = 3.0;

   for (i = 0; i < TIMES; i++)
      loop (x, y, zx, LOOP);

   printf("contents\n");
   for(i = 0;i<LOOP;i++)
   {
       printf("x: %lf\ty: %lf\tzx: %lf\n",x[i],y[i],zx[i]);
   }
}

loop(x, y, zx, n) 
double x[], y[], zx[];
int n;
{
  int k;
  double q = 12.0;
  double r = 5.2;
  double t = 13.4;

  for (k = 0; k < n; k++)
     x[k] = q + y[k] * (r * zx[k + 10] + t * zx[k + 11]);
}

