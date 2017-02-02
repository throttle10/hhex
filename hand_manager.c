#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "hand_manager.h"

//#define DEBUG

#define LN (10*1024*1024)

static unsigned char *_lastchunk;
static unsigned char *_cp;
static unsigned char *_lastchunk_end;
static FILE *_f;
static int _all_actions_short;

static inline int _readchunk(void)
{
	return(fread(_lastchunk, 1, LN, _f));
}

static inline void _updatechunk(int len)
{
	if(_cp + len > _lastchunk_end && !feof(_f)) {
		lseek(fileno(_f), _cp - _lastchunk_end, SEEK_CUR);
		_lastchunk_end = _lastchunk + _readchunk();
		_cp = _lastchunk;
	}
}

static inline uint16_t _readmem_2(unsigned char *cur)
{
	return (*cur << 8) + *(cur + 1);
}

static inline uint32_t _readmem_3(unsigned char *cur)
{
	return (*cur << 16) + (*(cur + 1) << 8) + *(cur + 2);
}

static inline uint32_t _readmem_4(unsigned char *cur)
{
	return (*cur << 24) + (*(cur + 1) << 16) + (*(cur + 2) << 8) + *(cur + 3);
}

static inline uint64_t _readmem_8(unsigned char *cur)
{
	return ((uint64_t)*cur << 56) + ((uint64_t)*(cur + 1) << 48) + ((uint64_t)*(cur + 2) << 40) +
		((uint64_t)*(cur + 3) << 32) + (*(cur + 4) << 24) + (*(cur + 5) << 16) + (*(cur + 6) << 8) + *(cur + 7);
}

static unsigned char *read_actions(unsigned char *p, struct action_s *actions, int n)
{
	for(int i = 0; i < n; i++) {
		actions[i].jointbyte = *p;
		p += 1;
		if(IS_ACTION_WITHOUT_AMOUNT_ENCODED(actions[i].jointbyte))
			actions[i].amount = -1;
		else {
			if(_all_actions_short) {
				actions[i].amount = _readmem_2(p);
				p += 2;
			}
			else {
				actions[i].amount = _readmem_3(p);
				p += 3;
			}
		}
	}
	return p;
}

int hand_manager_load_hand(struct hand_s *h)
{
	unsigned char *p = _cp;
	// HEADER
	h->site = *p;
	p += 1;
#ifdef PERSIST_GAME_ID
	h->game_id = _readmem_8(p);
	p += 8;
#endif
	h->game = *p;
	p += 1;
	h->date = _readmem_4(p);
	p += 4;
	h->table_id = _readmem_4(p);
	p += 4;
	h->number_of_seats = *p;
	p += 1;
	h->blinds = *p;
	p += 1;
	h->button = *p;
	p += 1;

	//DATA
	h->players = malloc(sizeof(uint32_t) * h->number_of_seats);
	for(int i = 0; i < h->number_of_seats; i++) {
		h->players[i] = _readmem_3(p);
		p += 3;
	}

	//read stacks
	h->stacks = malloc(sizeof(uint32_t) * h->number_of_seats);
	int all_shorts = (*p == SIZE_STACKS2_ACTIONS2 || *p == SIZE_STACKS2_ACTIONS3);
	_all_actions_short = (*p == SIZE_STACKS2_ACTIONS2 || *p == SIZE_STACKS3_ACTIONS2);
	p += 1;
	if(all_shorts)
		for(int i = 0; i < h->number_of_seats; i++) {
			h->stacks[i] = _readmem_2(p);
			p += 2;
		}
	else
		for(int i = 0; i < h->number_of_seats; i++) {
			h->stacks[i] = _readmem_3(p);
			p += 3;
		}

	//read players cards
	h->cards_px = malloc(sizeof(uint8_t *) * MAX_CARDS_PX);
	for(int i = 0; i < MAX_CARDS_PX; i++) {
		int8_t mask = *p;
		p += 1;
		if(mask < 0)
			break;
		uint8_t len = mask / 10 + 2;
		h->cards_px[i] = malloc(len); //size
		h->cards_px[i][0] = mask % 10; //seat
		h->cards_px[i][1] = len;
		for(int j = 2; j < len; j++) {
			h->cards_px[i][j] = *p;
			p += 1;
		}
	}
	//read board cards
	for(int i = 0; i < 5; i++) {
		h->board[i] = *p;
		p += 1;
		if(h->board[i] == INVALID_CARD)
			break;
	}

	//read actions
	int n = *p;
	h->a_preflop = h->a_flop = h->a_turn = h->a_river = h->a_showdown = NULL; 
	p += 1;
	if(n > 0) {
		h->a_preflop = malloc(sizeof(struct action_s) * n);
		p = read_actions(p, h->a_preflop, n);
		h->a_preflop_len = n;
	}
	n = *p;
	p += 1;
	if(n > 0) {
		h->a_flop = malloc(sizeof(struct action_s) * n);
		p = read_actions(p, h->a_flop, n);
		h->a_flop_len = n;
	}
	n = *p;
	p += 1;
	if(n > 0) {
		h->a_turn = malloc(sizeof(struct action_s) * n);
		p = read_actions(p, h->a_turn, n);
		h->a_turn_len = n;
	}
	n = *p;
	p += 1;
	if(n > 0) {
		h->a_river = malloc(sizeof(struct action_s) * n);
		p = read_actions(p, h->a_river, n);
		h->a_river_len = n;
	}
	n = *p;
	p += 1;
	if(n > 0) {
		h->a_showdown = malloc(sizeof(struct action_s) * n);
		p = read_actions(p, h->a_showdown, n);
		h->a_showdown_len = n;
	}

	return 0;
}

#ifdef DEBUG
static inline void _printf_hand_action(struct action_s *a)
{
	printf(" %d", ACTION_PLAYER_SEAT(a->jointbyte));
	switch(ACTION_TYPE(a->jointbyte)) {
		case ACTION_NONE:
			printf(" NONE");
			break;
		case ACTION_BIGBLIND:
			printf(" BIGBLIND");
			break;
		case ACTION_SMALLBLIND:
			printf(" SMALLBLIND");
			break;
		case ACTION_BOTHBLINDS:
			printf(" BOTHBLINDS");
			break;
		case ACTION_DEAD_SB:
			printf(" DEAD_SB");
			break;
		case ACTION_FOLD:
			printf(" FOLD");
			break;
		case ACTION_CHECK:
			printf(" CHECK");
			break;
		case ACTION_CALL:
			printf(" CALL");
			break;
		case ACTION_BET:
			printf(" BET");
			break;
		case ACTION_RAISE:
			printf(" RAISE");
			break;
		case ACTION_ALLIN_DEPRECATED:
			printf(" ALLIN_DEPRECATED");
			break;
		case ACTION_LEAVETABLE:
			printf(" LEAVETABLE");
			break;
		case ACTION_JOINTABLE:
			printf(" JOINTABLE");
			break;
		case ACTION_SITTINGOUT:
			printf(" SITTINGOUT");
			break;
		case ACTION_SITTINGIN:
			printf(" SITTINGIN");
			break;
		case ACTION_WON:
			printf(" WON");
			break;
	}
	if(a->amount != -1)
		printf(" %d", a->amount);
	printf(";");
}
static inline void _printf_hand(int len)
{
	struct hand_s hand;
	unsigned char *p = _cp;
	printf("Header hex:");
#ifdef PERSIST_GAME_ID
	for(int i = 0; i < 8; i++)
		printf(" %.2x", *p++);
#endif
	for(int i = 0; i < 12; i++)
		printf(" %.2x", *p++);
	printf("\n");

	hand_manager_load_hand(&hand);
	printf("Header:");
	printf(" site:%u", hand.site);
#ifdef PERSIST_GAME_ID
	printf(" game_id:%u", hand.game_id);
#endif
	printf(" game:%u", hand.game);
	printf(" date:%u", hand.date);
	printf(" table_id:%u", hand.table_id);
	printf(" number_of_seats:%u", hand.number_of_seats);
	printf(" blinds:%u", hand.blinds);
	printf(" button:%u", hand.button);
	printf("\n");

	len -= HEADER_SIZE - 1;
	printf("Data hex:");
	while(len--) {
		printf(" %.2x", *p++);
	}
	printf("\n");
	printf("Data:");
	for(int i = 0; i < hand.number_of_seats; i++) {
		printf(" player %d:%x", i, hand.players[i]);
	}
	for(int i = 0; i < hand.number_of_seats; i++) {
		printf(" stack %d:%x", i, hand.stacks[i]);
	}
	printf(" cards");
	for(int i = 0; i < MAX_CARDS_PX; i++) {
		if(hand.cards_px[i]) {
			printf(" %d:", hand.cards_px[i][0]);
			for(int j = 2; j < hand.cards_px[i][1]; j++) {
				printf(" %d", hand.cards_px[i][j]);
			}
		}
	}
	printf(" board:");
	for(int i = 0; i < 5; i++) {
		if(hand.board[i] == INVALID_CARD)
			break;
		printf(" %d", hand.board[i]);
	}
	printf("\n");

	if(!hand.a_preflop)
		goto out;
	printf("Actions preflop %d:", hand.a_preflop_len);
	for(int i = 0; i < hand.a_preflop_len; i++)
		_printf_hand_action(&hand.a_preflop[i]);
	printf("\n");

	if(!hand.a_flop)
		goto out;
	printf("Actions flop %d:", hand.a_flop_len);
	for(int i = 0; i < hand.a_flop_len; i++)
		_printf_hand_action(&hand.a_flop[i]);
	printf("\n");

	if(!hand.a_turn)
		goto out;
	printf("Actions turn %d:", hand.a_turn_len);
	for(int i = 0; i < hand.a_turn_len; i++)
		_printf_hand_action(&hand.a_turn[i]);
	printf("\n");

	if(!hand.a_river)
		goto out;
	printf("Actions river %d:", hand.a_river_len);
	for(int i = 0; i < hand.a_river_len; i++)
		_printf_hand_action(&hand.a_river[i]);
	printf("\n");

	if(!hand.a_showdown)
		goto out;
	printf("Actions showdown %d:", hand.a_showdown_len);
	for(int i = 0; i < hand.a_showdown_len; i++)
		_printf_hand_action(&hand.a_showdown[i]);
	printf("\n");
out:
	printf("\n");

	hand_manager_free_hand(&hand);
}
#endif

/* Updates _cp and returns len of new hand.
 * hand - output parameter, next hand pointer
 */
int hand_manager_next_hand(unsigned char **hand)
{
	int len = 0;
	// if hand is splitted between chunks, seek behind, at this hand start, and read full chunk
	_updatechunk(2);
	//calc next hand length
	if(_cp + 1 <= _lastchunk_end) {
		len = ((int)*_cp * 100) + *(_cp + 1);
		_cp += 2;
	}
	_updatechunk(len);

#ifdef DEBUG
	_printf_hand(len);
#endif

	*hand = _cp;
	_cp += len;
	return len;
}

int hand_manager_has_more_hands(void)
{
	return(!(feof(_f) && _cp == _lastchunk_end));
}

int hand_manager_init(const char *file_name)
{
	_f = fopen(file_name, "r");
	if(!_f) {
		fprintf(stderr, "%s[%d] can't open file\n", __func__, __LINE__);
		return 1;
	}
	_lastchunk = malloc(LN);
	if(!_lastchunk) {
		fprintf(stderr, "%s[%d] can't allocate memory for lastchunk, size:%d\n", __func__, __LINE__, LN);
		return 1;
	}
	_lastchunk_end = _lastchunk + _readchunk();
	_cp = _lastchunk;

	//awoa
	memset(_awoa, 1, sizeof(_awoa));
	_awoa[ACTION_BET] = 0;
	_awoa[ACTION_RAISE] = 0;
	_awoa[ACTION_WON] = 0;
#ifdef SAVE_CALL_AMOUNTS
	_awoa[ACTION_CALL] = 0;
#endif

	//amMask
	memset(_amMask, 0, sizeof(_amMask));
	for(int b1 = 0; b1 < 16; b1++) {
		for(int b2 = 0; b2 < 16; b2++) {
			int b = b1*16 + b2 - 128;
			if(_awoa[b2])
				_amMask[b + 128] = 1;
		}
	}

	return 0;
}

void hand_manager_free(void)
{
	free(_lastchunk);
	fclose(_f);
}

void hand_manager_free_hand(struct hand_s *h)
{
	if(h->players)
		free(h->players);
	if(h->stacks)
		free(h->stacks);
	if(h->cards_px) {
		for(int i = 0; i < MAX_CARDS_PX; i++) {
			if(h->cards_px[i])
				free(h->cards_px[i]);
		}
		free(h->cards_px);
	}
	if(h->a_preflop)
		free(h->a_preflop);
	if(h->a_flop)
		free(h->a_flop);
	if(h->a_turn)
		free(h->a_turn);
	if(h->a_river) {
		free(h->a_river);
	}
	if(h->a_showdown)
		free(h->a_showdown);
}
