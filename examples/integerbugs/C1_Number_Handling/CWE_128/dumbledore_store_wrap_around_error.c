/*
@GOOD_ARGS <good.dat
@BAD_ARGS <bad.dat
@ATTACK_SUCCEEDED_OUTPUT_CONTAINS Your credit card will be charged \$-

bjm this example is intended to  illustrate wrap around error 
this an EXPLOIT that doesn't result in a buffer over flow but is very serous.

the following is three examples from the listed site.  This code was written to illustrate example 1

http://projects.webappsec.org/w/page/13246946/Integer-Overflows
1) When calculating a purchase order total, an integer overflow could allow the total to shift from a positive value to a negative one. This would, in effect, give money to the customer in addition to their purchases, when the transaction is completed. 

2) Withdrawing 1 dollar from an account with a balance of 0 could cause an integer underflow and yield a new balance of 4,294,967,295. 

3) A very large positive number in a bank transfer could be cast as a signed integer by a back-end system. In such case, the interpreted value could become a negative number and reverse the flow of money - from a victim's account into the attacker's. 

*/
 
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef ASSERT
#include <assert.h>
#include <limits.h>
#endif

int main(int argc, char**argv)
{
  short int price1 = 10000;
  short int price2 = 6000;
  short int price3 = 10;
  short int price4 = 1000;
/*bjm
short signed: -32768 to 32767
short unsigned: 0 to 65535
long signed:-2147483648 to 2147483647 (Default unless you're using DOS)
long unsigned: 0 to 4294967295
*/
  short int selected_item, number_of_items, total_price = 0;
  char *item1 =  "Dumbledore's Wand"; 
  char *item2 =  "Harry's Wand"; 
  char *item3 =  "Snake Potion"; 
  char *item4 =  "Flying Broom"; 
  char *selected_item_string = " "; 

/* display store  */
for(;;){
  printf("         Magic store  \n\n\n ");
  printf("Item 1.   %s  $=%i\n ", item1, price1);
  printf("Item 2.   %s  $=%i\n ", item2, price2);
  printf("Item 3.   %s  $=%i\n ", item3, price3);
  printf("Item 4.   %s  $=%i\n\n\n ", item4, price4);
  printf("Enter the number of the item you wish to purchase = ");

//get input  selected_item
  selected_item = getchar(); 
  selected_item = selected_item - '0';
//validate input

 if ( (0< selected_item) && (selected_item <5) ){
    break;
  }
  else{
	  printf("\n Invalid entry actual item %i\n",selected_item);
          exit(1);  
  }

}


switch(selected_item){
	case 1:
          printf("\nEnter how many %ss do you want = ",item1);
	  break;
	case 2:
          printf("\nEnter how many %ss do you want = ",item2);
	  break;
	case 3:
          printf("\nEnter how many %ss do you want = ",item3);
	  break;
	case 4:
          printf("\nEnter how many %ss do you want = ",item4);
	  break;
	default:
          printf("\nError unknown Item \n");
	  break;
}
//get input  number_of_items
  fscanf(stdin,"%hi",&number_of_items); 
  printf("\n");

//woops I forgot to check the inventory or limit the number of items


/* multiply number of items * dollar value  */  
switch(selected_item){
	case 1:
          total_price = number_of_items * price1 ; 
          selected_item_string = item1;
	  break;
	case 2:
          total_price = number_of_items * price2 ; 
          selected_item_string = item2;
	  break;
	case 3:
          total_price = number_of_items * price3 ; 
          selected_item_string = item3;
	  break;
	case 4:
          total_price = number_of_items * price4 ; 
          selected_item_string = item4;
	  break;
	default:
          printf("Error calculating price \n");
	  exit(1);
	  break;
}

/* display conformation page */
  printf("\n\n         Shopping Summary\n\n");

/* This is what is charged to your card 
   if it is -$ it is an exploit 
   credit cards will gladly go negative
 */
printf("Your credit card will be charged $%i\n\n",total_price);

/* N items will be shiped to you 
 if total price is -$ n is probaly a large number of items you will sell on E-bay
 */
printf("You will be shiped %i %ss\n", number_of_items, selected_item_string);

/* confirm purchase */
//bjm I didn't bother with a confirmation

//printf("Do wish to continue with the purchase Y or N \n");
//get input
//validate Y or N

printf("\n\nThank you for shoping at the Magic store\n\n");
/*
   charge the credit card
   */

#ifdef ASSERT
assert( total_price >0 );
#endif
  exit(0);
}

