/*******************************************
 **
 ** Copyright (C) 2011 The MITRE Corporation. ALL RIGHTS RESERVED
 **
 ** Author: Mitchell Wills
 ** Date: 8/4/2011
 **
 ** Base Test Program -- solitare.c
 **
 ** <base test program description, note that this description
 ** should not talk about things like source taint and any other
 ** variant categories as those details will change>
 **
 ** Variant Test Case Program
 **
 ** The program does this:
 ** 1) Reads in up to 25 bytes from a key file, and converts those characters into a 32-bit unsigned integer
 ** 2) Using that 32-bit unsigned integer as a seed, creates a shuffled deck of 54 cards (with jokers)
 ** 3) Read in a message to be encoded
 ** 4) Strip the message of all non-alphabet-letters
 ** 5) Read a new keystream value from the deck by
 ** 	a) Moving the first joker one spot down the deck
 ** 	b) Moving the second joker two spots down the deck
 ** 	c) Cutting the deck into three parts, and reversing the order of them relative to each other, delimited by the jokers
 ** 	d) Cutting the deck by as many cards as the value of the bottom card
 ** 	e) The next keystream value is the nth card from the top, where n is the card at the top of the deck
 ** 	f) The deck stays as it ends up
 ** 6) Encrypt the first character by adding the keystream value to it, mod 26
 ** 7) Perform steps 5-6 until done
 **
 ** Decryption works similarly.
 **
 ** See http://en.wikipedia.org/wiki/Solitaire_(cipher)
 **
 **
 ** STONESOUP Weakness Class: Buffer overflows/underflows/out of bounds/accesses/memory safety errors
 ** CWE ID: CWE-170
 ** Variant Spreadsheet Rev #: 2
 ** Variant Spreadsheet ID: 905
 **
 ** Variant Features:
 **   Source Taint: STDIN
 **   Data Type:    UNSIGNED_BYTE
 **   Control Flow: VARIABLE_ARGUMENT_LIST
 **   Data Flow:    ADDRESS_AS_A_CONSTANT
 **
 ** Initial Testing: (x means yes, - means no)
 **   Tested in MS Windows XP 32bit        x
 **   Tested in MS Windows 7  64bit        -
 **   Tested in Ubuntu10_10 Linux 32bit    -
 **   Tested in Ubuntu10_10 Linux 64bit    -
 **
 ** Workflow:
 **   Created:8/4/2011
 **   1st Vett: <programmer> on <date>
 **   2nd Vett: <peer> on <date>
 **   3rd Vett: <teamleader> on <date>
 **   4th Vett: Fortify Issues Fixed on <date>
 **   5th Vett: Tested in Harness on <date>
 **
 **
 ** I/0 Pairs:
 **   Good: 1st Set:
 **         2nd Set:
 **         3rd Set:
 **         4th Set:
 **         5th Set:
 **    Bad: 1st Set:
 *********************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#define KEY_LENGTH 25
#define DECK_SIZE 54
#define ALPHA_SIZE 26
#define FIRST_JOKER 53
#define SECOND_JOKER 54
#define MAX_MESSAGE_LENGTH 20

typedef char Card;

void print_deck (Card *deck) {
	int i;
	for (i = 0; i < DECK_SIZE; i++) {
		printf("%d ", deck[i]);
	}
	printf("\n");
}

//In-place Fischer-Yates shuffle
void shuffle(Card *deck, unsigned int seed) {
	int cards_left = DECK_SIZE;
	int next_index;
	int cur_index;
	Card temp;
	srand(seed);
	while (cards_left != 0) {
		next_index = DECK_SIZE - rand() % cards_left - 1;
		cur_index = DECK_SIZE - cards_left;
		temp = deck[cur_index];
		deck[cur_index] = deck[next_index];
		deck[next_index] = temp;
		cards_left--;
	}
}

//turn the key into a 32-bit unsigned int via questionably required
unsigned int seedify (char *key) {
	int i;
	unsigned int seed = 0;
	for (i = 0; i < KEY_LENGTH; i++) {
		seed += (unsigned int)(key[i]) << (i % 32);
	}
	return seed;
}

//randomly initializes the deck
void init_deck(Card *deck, char *key) {
	int i;
	for (i = 0; i < DECK_SIZE; i++) {
		deck[i] = (Card)(i+1);
	}
	shuffle(deck, seedify(key));
}

int indexof (Card *deck, Card c) {
	int i;
	for (i = 0; i < DECK_SIZE; i++) {
		if (deck[i] == c) {
			return i;
		}
	}
	return -1;
}

void joker_shift(Card *deck) {
	int fji = indexof(deck, FIRST_JOKER);
	if (fji < DECK_SIZE - 1 && fji >= 0) {
		deck[fji] = deck[fji+1];
		deck[fji+1] = FIRST_JOKER;
		fji++;
	}
	int sji = indexof(deck, SECOND_JOKER);
	if (sji < DECK_SIZE - 1 && sji >= 0) {
		deck[sji] = deck[sji+1];
		deck[sji+1] = SECOND_JOKER;
		sji++;
	}
	if (sji < DECK_SIZE - 1 && sji >= 0) {
		deck[sji] = deck[sji+1];
		deck[sji+1] = SECOND_JOKER;
		sji++;
	}
}

void triple_cut(Card *deck) {
	Card helper_deck[DECK_SIZE];
	int fji = indexof(deck, FIRST_JOKER);
	int sji = indexof(deck, SECOND_JOKER);
	if (fji > DECK_SIZE || fji < 0 || sji > DECK_SIZE || sji < 0) {
		printf("Something bad happened... jokers are not there right.\n");
		return;
	}
	int top_joker_index;
	int bottom_joker_index;
	if (fji < sji) {
		top_joker_index = fji;
		bottom_joker_index = sji;
	} else {
		top_joker_index = sji;
		bottom_joker_index = fji;
	}
	int write_index = 0;
	int read_index = bottom_joker_index + 1;
	while (read_index < DECK_SIZE) {
		helper_deck[write_index] = deck[read_index];
		read_index++;
		write_index++;
	}
	read_index = top_joker_index;
	while (read_index <= bottom_joker_index) {
		helper_deck[write_index] = deck[read_index];
		read_index++;
		write_index++;
	}
	read_index = 0;
	while (read_index < top_joker_index) {
		helper_deck[write_index] = deck[read_index];
		read_index++;
		write_index++;
	}
	memcpy(deck, helper_deck, DECK_SIZE*sizeof(Card));
}

void bottom_value_cut(Card *deck) {
	Card helper_deck[DECK_SIZE];
	int bottom_value = (int)deck[DECK_SIZE - 1];
	if (bottom_value >= DECK_SIZE) {
		bottom_value = DECK_SIZE - 1;
	}
	int read_index = bottom_value;
	int write_index = 0;
	while (read_index < DECK_SIZE) {
		helper_deck[write_index] = deck[read_index];
		read_index++;
		write_index++;
	}
	read_index = 0;
	while (read_index < bottom_value) {
		helper_deck[write_index] = deck[read_index];
		read_index++;
		write_index++;
	}
	read_index = DECK_SIZE - 1;
	while (read_index < DECK_SIZE) {
		helper_deck[write_index] = deck[read_index];
		read_index++;
		write_index++;
	}
	memcpy(deck, helper_deck, DECK_SIZE*sizeof(Card));
}

//Does all the algorithmy stuff described in the header
int next_keystream_value (Card *deck) {
	joker_shift(deck);
	triple_cut(deck);
	bottom_value_cut(deck);
	int return_index = deck[0];
	if (return_index >= DECK_SIZE) {
		return_index = DECK_SIZE - 1;
	}
	return deck[return_index];
}

char *encrypt(Card *deck, char *message, char *encrypted) {
	int i = 0; //message index
	int j = 0; //encrypted index
	char c;
	while (message[i] != '\0' && i<MAX_MESSAGE_LENGTH) {
		c = message[i];
		if (isalpha(c)) {
			c = toupper(c) - 65;
			int keystream = next_keystream_value(deck);
			encrypted[j] = (((int)c + keystream) % ALPHA_SIZE) + 65;
			//printf("%d\n", keystream);
			j++;
		}
		i++;
	}
	encrypted[j] = '\0';
	return encrypted;
}

char *decrypt(Card *deck, char *message, char *decrypted) {
	int i = 0; //message index
	int j = 0; //decrypted index
	char c;
	while (message[i] != '\0' && i<MAX_MESSAGE_LENGTH) {
		c = message[i];
		if (isalpha(c)) {
			c = toupper(c) - 65;
			int keystream = next_keystream_value(deck);
			decrypted[j] = (((int)c + (ALPHA_SIZE - (keystream % ALPHA_SIZE))) % ALPHA_SIZE) + 65;
			//printf("%d\n", keystream);
			j++;
		}
		i++;
	}
	decrypted[j] = '\0';
	return decrypted;
}

void processStuff(int mode, ...){//STONESOUP:feature-VARIABLE_ARGUMENT_LIST
	va_list varargs;
	va_start(varargs, mode);

	Card* deck = va_arg(varargs, Card*);
	const char* in_message = va_arg(varargs, char*);//STONESOUP:feature-ADDRESS_AS_A_CONSTANT
	char* out_message = va_arg(varargs, char*);

	if (!mode) {
		printf("Unencrypted: %s\n", in_message);//STONESOUP:TRIGGER_POINT
		printf("Encrypted: %s\n", encrypt(deck, in_message, out_message));
	} else {
		printf("Encrypted: %s\n", in_message);
		printf("Decrypted: %s\n", decrypt(deck, in_message, out_message));
	}
	/*for(i=0; 1==1; ++i){
	printf("%i - %c\n", in_message[i], in_message[i]);
	fflush(stdout);
	if(in_message[i]=='\0')
		break;
	}
	for(i=0; 1==1; ++i){
		printf("%i - %c\n", out_message[i], out_message[i]);
		fflush(stdout);
		if(in_message[i]=='\0')
			break;
	}*/
	va_end(varargs);
}

int main (int argc, char *argv[]) {
	char key[KEY_LENGTH]; //char array, not a string
	Card deck[DECK_SIZE];
	char out_message[MAX_MESSAGE_LENGTH];
	Card starting_deck[DECK_SIZE];
	char in_message[MAX_MESSAGE_LENGTH];
	FILE *keyfile;
	int mode = 0; //encrypt = 0, decrypt = 1

	keyfile = fopen("key", "r");
	if (!keyfile) {
		printf("Unable to open key file\n");
		exit(1);
	}
	if (!fgets(key, KEY_LENGTH, keyfile)) {
		printf("Reading from key file failed\n");
		fclose(keyfile);
		exit(1);
	}
	fclose(keyfile);

	init_deck(starting_deck, key);
	while (1) {
		memcpy(deck, starting_deck, sizeof(Card) * DECK_SIZE); //set the deck to the key starting position
		//print_deck(deck);
		if (!mode) {
			printf("Enter message to be encrypted (type '/help' for help):\n");
		} else {
			printf("Enter message to be decrypted (type '/help' for help):\n");
		}
		fflush(stdout);

		int i;
		unsigned char c = '\0';//STONESOUP:feature-UNSIGNED_BYTE
		for (i = 0; (c = getchar()) != '\0' && i<MAX_MESSAGE_LENGTH; i++) {//STONESOUP:INTERACTION_POINT //STONESOUP:feature-STDIN
			in_message[i] = c;
			if (in_message[i] == '\n') {
				in_message[i] = '\0';
				break;
			}
		}//STONESOUP:CROSSOVER_POINT
		fflush(stdout);
		if (!strcmp(in_message, "/quit")) {
			break;
		} else if (!strcmp(in_message, "/encrypt")) {
			mode = 0;
			printf("Encryption mode\n");
		} else if (!strcmp(in_message, "/decrypt")) {
			mode = 1;
			printf("Decryption mode\n");
		} else if (!strcmp(in_message, "/help")) {
			printf("/encrypt: switch to encryption mode\n"
					"/decrypt: switch to decryption mode\n"
					"/quit: quit program\n");
		} else {
			processStuff(mode, deck, in_message, out_message);
		}
	}
	return 0;
}
