int main(int argc, char **argv)
{
  volatile char c = 0;
  volatile short s = 0;
  volatile char uc = 0;
  volatile int i = 0;
  volatile unsigned short us = 0;
  volatile int ui = 0;

  // widening

  i = c; /* movzx   eax, [ebp+var_1]
            movsx   eax, al
            mov     [ebp+var_C], eax
         */

  i = uc; /* movzx   eax, [ebp+var_5]
             movsx   eax, al
             mov     [ebp+var_C], eax
          */

  ui = c; /*
                movzx   eax, [ebp+var_1]
                movsx   eax, al
                mov     [ebp+var_14], eax
          */

  ui = uc; /*   movzx   eax, [ebp+var_5]
                movsx   eax, al
                mov     [ebp+var_14], eax
           */

  s = c;   /*   movzx   eax, [ebp+var_1]
                cbw                         ; convert byte to word (sign-extend)
                mov     [ebp+var_4], ax
           */

  s = uc;  /*   movzx   eax, [ebp+var_5]
                cbw
                mov     [ebp+var_4], ax
           */

  us = c;  /*   movzx   eax, [ebp+var_1]
                cbw
                mov     [ebp+var_E], ax
           */

  us = uc; /*   movzx   eax, [ebp+var_5]
                cbw
                mov     [ebp+var_E], ax
           */

  //
  // truncating
  //
  c = i;   /*   mov     eax, [ebp+var_C]
                mov     [ebp+var_1], al
           */

  uc = i;  /*   mov     eax, [ebp+var_C]
                mov     [ebp+var_5], al
           */

  s = i;   /*   mov     eax, [ebp+var_C]
                mov     [ebp+var_4], ax
           */

  s = ui;  /*   mov     eax, [ebp+var_14]
                mov     [ebp+var_4], ax
           */

}
