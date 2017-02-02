#include <stdio.h>
#include "hand_manager.h"
#include "raw_hand.h"

int main(void)
{
	hand_manager_init("sample/pokerai.org.sample0_1_6_w8.hhex");

	int count = 0;

	while(hand_manager_has_more_hands()) {
		unsigned char *hand;

		hand_manager_next_hand(&hand);
		count++;
	}

	printf("Hand count:%d\n", count);

	hand_manager_free();

	return 0;
}
