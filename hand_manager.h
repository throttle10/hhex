#ifndef HAND_MANAGER_H
#define HAND_MANAGER_H
#include "raw_hand.h"
#define SAVE_CALL_AMOUNTS 1

int hand_manager_next_hand(unsigned char **hand);
int hand_manager_has_more_hands(void);
int hand_manager_init(const char *file_name);
void hand_manager_free(void);
void hand_manager_free_hand(struct hand_s *h);
#endif
