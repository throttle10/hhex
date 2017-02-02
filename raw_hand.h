#ifndef RAW_HAND_H
#define RAW_HAND_H
#include <stdint.h>

struct action_s {
	int8_t jointbyte; //encodes player seat and action type in one byte
	int32_t amount; //3 bytes, amount in Big Blinds. -1 if action does not imply amount, e.g. check, call, fold.
};

#define ACTION_NONE             -1
#define ACTION_BIGBLIND          0
#define ACTION_SMALLBLIND        1
#define ACTION_BOTHBLINDS        2
#define ACTION_DEAD_SB           3
#define ACTION_FOLD              4
#define ACTION_CHECK             5
#define ACTION_CALL              6 //amounts are not saved but calculated (automatically on call). Call amounts are the
                                   //delta that has to be called (e.g. if player spent 3 and there is raise to 6, then
								   //call 3
#define ACTION_BET               7 //amounts are saved only for this
#define ACTION_RAISE             8 //amounts are saved only for this, semantics is RAISE *TO* the amount
#define ACTION_ALLIN_DEPRECATED  9 //ALLIN events are deprecated: these are not parsed due to bet/raise & all-in
                                   //ambiquity
#define ACTION_LEAVETABLE        10
#define ACTION_JOINTABLE         11
#define ACTION_SITTINGOUT        12
#define ACTION_SITTINGIN         13
#define ACTION_WON               14 //Pot awarded to player, also used for uncalled bets

int _awoa[16]; //action without award
int _amMask[300];
#define IS_ACTION_WITHOUT_AMOUNT_ENCODED(b) (_amMask[b + 128])
#define ACTION_PLAYER_SEAT(b) ((b + 128) / 16)
#define ACTION_TYPE(b) ((b + 128) % 16)

struct hand_s {
// HEADER 21 or 13 bytes
#define SITE_DEFAULT 0
#define SITE_PS      0
#define SITE_FTP     1
#define SITE_PARTY   2
#define SITE_IPN     3
#define SITE_BOSS    4
#define SITE_UB      5
#define SITE_HHDB    6
	uint8_t site;
#ifdef PERSIST_GAME_ID
#define HEADER_SIZE 21
	uint64_t game_id;
#else
#define HEADER_SIZE 13
#endif
#define GAME_UNKNOWN   0
#define GAME_HOLDEM_NL 1
#define GAME_HOLDEL_PL 2
#define GAME_HOLDEM_FL 3
#define GAME_OMAHA_NL  4
#define GAME_OMAHA_PL  5
#define GAME_OMAHA_FL  6
	uint8_t game;
	uint32_t date;
	uint32_t table_id;
	uint8_t number_of_seats;
#define BLINDS_UNKNOWN 0
	uint8_t blinds;
#define BUTTON_UNKNOWN 12
	uint8_t button;

// DATA
	uint32_t *players;
#define SIZE_STACKS3_ACTIONS3 0
#define SIZE_STACKS3_ACTIONS2 1
#define SIZE_STACKS2_ACTIONS3 2
#define SIZE_STACKS2_ACTIONS2 3
	uint32_t *stacks;
#define MAX_CARDS_PX 7
	uint8_t **cards_px;
#define INVALID_CARD 100
	uint8_t board[5];

	struct action_s *a_preflop;
	int a_preflop_len;
	struct action_s *a_flop;
	int a_flop_len;
	struct action_s *a_turn;
	int a_turn_len;
	struct action_s *a_river;
	int a_river_len;
	struct action_s *a_showdown;
	int a_showdown_len;
};
#endif
