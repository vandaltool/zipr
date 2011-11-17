/*      Program sudoku
!-----------------------------------------------------------------
! Program solves Sudoku puzzles
! 1. reads data from the file "sudoku.dat"
! 2. check the data for consistency
! 3. look for solutions using 3 methods
!-----------------------------------------------------------------
! Author:  Alex Godunov
! Date:    09 April 2007
! Version: 1.0 (C++)
!----------------------------------------------------------------*/

#include <iostream>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <cstdlib>

using namespace std;

/* function prototypes */
void input  (int[10][10]);
void output (int[10][10]);
void stop();
int  errors (int[10][10]);
int  empty_c(int[10][10]);
int  cellb  (int[10][10], int, int, int[10]);
int  method1(int[10][10]);
int  method2(int[10][10]);
int  method3(int[10][10]);

int main()
{
    int a[10][10];
    int err, empty;

    cout << "  Sudoku solver from AG " << endl;
    cout << "   version: April 2007" << endl;

/* read data and check for errors */
    input(a);
    err = errors(a);
    if(err > 0) return 1;

/* apply method 1 */
    empty = method1(a);
    cout << endl << "   Method 1 gives  " << endl;
    output(a);
    if(empty == 0) stop();

/* if method 1 has not succeeded call method 2 */
    if(empty != 0)
{
    empty = method2(a);
    cout << endl << "   Method 2 gives  " << endl;
    output(a);
    if(empty == 0) stop();
}
/* if method 2 has not succeeded call method 3 */
    if(empty != 0)
{
    empty = method3(a);
    cout << endl << "   Method 3 gives  " << endl;
    output(a);
    if(empty == 0) stop();
}
    if(empty != 0)
    {cout << endl << "  Sorry, can't solve, let AG know about that" <<endl;}

    system("pause");
}

//============================================
void input(int a[10][10])
{
    int i, j, empty;

    /* input from file */
    ifstream file;
    file.open ("sudoku.dat", ios::in);

    cout << endl << "   initial matrix " << endl;

    for (i=1; i<=9; i = i+1)
    {
        cout << "  ";
        for (j=1; j<=9; j = j + 1)
        {
           file >> a[i][j];
           cout << setw(2)<< a[i][j];
        }
          cout << endl;
    }
    empty = empty_c(a);
    cout << "   empty cells = " << empty << endl;
}

//============================================
void output(int a[10][10])
{
    int i, j, empty;

    for (i=1; i<=9; i = i+1)
    {
        cout << "  ";
        for (j=1; j<=9; j = j + 1)
        {
           cout << setw(2)<< a[i][j];
        }
          cout << endl;
    }
    empty = empty_c(a);
    cout << "   empty cells = " << empty << endl;
}

void stop()
{
    cout << endl <<"   Sudoku is done ! " << endl << endl;;
}

//============================================
int empty_c(int a[10][10])
/* calculate number of empty cells */
{
    int empty = 0;
    int i, j;
    for (i=1; i<=9; i = i+1)
    {
        for (j=1; j<=9; j = j + 1)
        {
           if(a[i][j] ==0) empty = empty + 1;
        }
    }
    return empty;
}

//==========================================================
int cellb(int a[10][10], int i,int j, int b[10])
/*==========================================================
     check for allowed numbers for element a(i,j)
     returns these numbers in array b
----------------------------------------------------------*/
{
      int k, n;
      int ib, jb, bi, bj;
// initialize b(k)
      for (k=1; k<=9; k = k+1) {b[k] = 1;}

// check the raw
      for (jb=1; jb<=9; jb = jb+1)
      { if(jb == j) continue;
        if(a[i][jb] != 0) b[a[i][jb]] = 0;}

// check the column
      for (ib=1; ib<=9; ib = ib+1)
      { if(ib == i) continue;
        if(a[ib][j] != 0) b[a[ib][j]] = 0; }

// check the small block
      bi = (i-1)/3 + 1;
      bj = (j-1)/3 + 1;
      if(bi == 2) bi = 4;
      if(bi == 3) bi = 7;
      if(bj == 2) bj = 4;
      if(bj == 3) bj = 7;

      for (ib = bi; ib <= bi+2; ib = ib+1)
       {
         for (jb = bj; jb <= bj+2; jb = jb+1)
           {
            if(ib==i && jb==j) continue;
            if(a[ib][jb] != 0) b[a[ib][jb]] = 0;
           }
       }

// check how many elements in b(k) are non-zero
      n = 0;
      for (k=1; k<=9; k = k+1) {n = n + b[k];}
      return n;
}

//==========================================================
int  method1(int a[10][10])
//==========================================================
/*   returns a new sudoku matrix
     method: look for cells with "one only number" and fill
             that number into updated sudoku matrix
----------------------------------------------------------*/
{
      int b[10];
      int i, j, n, k;
      int empty, empty0, progress;

      progress = 81;
      empty0 = 81;

      while (progress != 0)
{
      for (i=1; i<=9; i = i+1)
{
      for (j=1; j<=9; j = j+1)          
{
//----------------------------------------------------------
        if(a[i][j] != 0) continue;
        n = cellb(a,i,j,b);
// if only one element in b(k) is non-zero then update a(i,j)
        if(n == 1) 
        {
          for (k=1; k<=9; k = k+1)
          { 
          if(b[k] != 0) a[i][j] = k;
          }    
        }
}
}
//!----------------------------------------------------------
//!* find how many non-zero elements still in a(i,j)
      empty = empty_c(a);
      progress = empty0 - empty;
      empty0 = empty;
}
      return empty;
}

//==========================================================
int method2(int a0[10][10])
/*!==========================================================
!     returns a new sudoku matrix
!     method: loop over possible numbers in a cell
!             then call method 1. If the method has not
!             succeed, then return old sudoku
!----------------------------------------------------------*/
{     int a[10][10], b[10];
      int i, j, n, k, i1, j1;
      int round, empty;

      for (i1=1;i1<=9;i1++) {
      for (j1=1;j1<=9;j1++) {
        a[i1][j1] = a0[i1][j1]; 
      }}

//! find possible numbers for empty a(i,j)
      for (i=1;i<=9;i++) {
      for (j=1;j<=9;j++) {
//!----------------------------------------------------------
        if(a[i][j] != 0) continue;
        n = cellb(a,i,j,b);
        for(k=1;k<=9;k++){
          if(b[k] != 0)
          {
            a[i][j] = k;
            empty = method1(a);
            if(empty == 0) break;
            for (i1=1;i1<=9;i1++) {
            for (j1=1;j1<=9;j1++) {
              a[i1][j1] = a0[i1][j1]; 
            }}
          }     
        }      
}}

//! return new solutions if it was found
      if(empty == 0)
      {
       for (i1=1;i1<=9;i1++) {
       for (j1=1;j1<=9;j1++) {
         a0[i1][j1] = a[i1][j1];}}      
      }       
      return empty;
}

//==========================================================
int method3(int a0[10][10])
/*==========================================================
!     returns a new sudoku matrix
!     method: loop over possible numbers in a cell
!             then call method 2 to loop over other
!             possible number in another cell.
!----------------------------------------------------------*/
{     int a[10][10], b[10];
      int i, j, n, k, i1, j1;
      int round, empty;

      for (i1=1;i1<=9;i1++) {
      for (j1=1;j1<=9;j1++) {
        a[i1][j1] = a0[i1][j1]; 
      }}

//! find possible numbers for empty a(i,j)
      for (i=1;i<=9;i++) {
      for (j=1;j<=9;j++) {
//!----------------------------------------------------------
        if(a[i][j] != 0) continue;
        n = cellb(a,i,j,b);
        for(k=1;k<=9;k++){
          if(b[k] != 0)
          {
            a[i][j] = k;
            empty = method2(a);
            if(empty == 0) break;
            for (i1=1;i1<=9;i1++) {
            for (j1=1;j1<=9;j1++) {
              a[i1][j1] = a0[i1][j1]; 
            }}
          }     
        }      
}}

       for (i1=1;i1<=9;i1++) {
       for (j1=1;j1<=9;j1++) {
         a0[i1][j1] = a[i1][j1];}}      

      return empty;
}

//!==========================================================
  int errors(int a[10][10])
/*!==========================================================
!     check for errors in a(i,j): original sudoku matrix
!     procedure - like in cellb
!----------------------------------------------------------*/
{     int b[10];
      int i, j, n, k, i1, j1;
      int x, ner;
      int ib, jb, bi, bj;

      ner = 0;

      for (i=1;i<=9;i++){
      for (j=1;j<=9;j++){
        x = a[i][j];
        if(x == 0) continue;
//!* check the raw
        for(jb=1;jb<=9;jb++){
          if(jb == j) continue;
          if(a[i][jb] == x)
          {
            cout <<"   wrong sudoku: " << endl;
            cout <<"   row " << i << " position " << j 
                 <<"   number " << a[i][j] << endl;
            cout <<"   is equal to element in position "<< jb << endl;               
            ner = ner + 1;
          }
        }

//!* check the column
        for(ib=1;ib<=9;ib++){
          if(ib == i) continue;
          if(a[ib][j] == x)
          { 
            cout <<"   wrong sudoku: " << endl;
            cout <<"   row " << i << " position " << j 
                 <<"   number " << a[i][j] << endl;
            cout <<"   is equal to element in row "<< ib << endl;
            ner = ner + 1;
          }
        }

//!* check the small block
        bi = (i-1)/3 + 1;
        bj = (j-1)/3 + 1;
        if(bi == 2) bi = 4;
        if(bi == 3) bi = 7;
        if(bj == 2) bj = 4;
        if(bj == 3) bj = 7;

        for (ib=bi; ib<=bi+2; ib++){
        for (jb=bj; jb<=bj+2; jb++){
          if(ib ==i && jb == j) continue;
          if(a[ib][jb] == x)
          {
            cout <<"   wrong sudoku: " << endl;
            cout <<"   row " << i << " position " << j 
                 <<"   number " << a[i][j] << endl;
            cout <<"   is equal to element element in same block " << endl;
            ner = ner + 1;
          }
        }
        }        
}
}
      if(ner == 0) cout << "   sudoku is fine" << endl;
      if(ner > 0) system("pause");
      return ner;
}

