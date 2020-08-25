#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <getopt.h>
#include "utils/game.h"
#include "utils/rng.h"
#include "utils/net.h"
#include "utils/rank_hand.h"

/* TODO: implement your own poker strategy in this function!
 * 
 * The function is called when it is the agent's turn to play!
 * All the information about the game and the current state is preprocessed
 * and stored in game and state.
 * 
 * */

#include <iostream>
#include <fstream>
#include <cmath>
using namespace std;


rng_state_t _rng, *rng = &_rng;

CardSet operator + (const CardSet &a, const CardSet &b) {
	CardSet ret;
	ret.cs.cards = a.cs.cards | b.cs.cards;
	return ret;
}
CardSet operator - (const CardSet &a, const CardSet &b) {
	CardSet ret;
	ret.cs.cards = a.cs.cards & (~b.cs.cards);
	return ret;
}
CardSet from_array(int n, uint8_t arr[]) {
	CardSet ret;
	for (int i = 0; i < n; i++) {
		ret.AddCard(arr[i]);
	}
	return ret;
}
CardSet random_padding(int n, const CardSet &base = CardSet(), const CardSet &avoid = CardSet()) {
	CardSet ret = base;
	int r = n - ret.NumCards();
	while (r--) {
		int suit = genrand_int32(rng) % 4, rank = genrand_int32(rng) % 13;
		int card = makeCard(rank, suit);
		if (ret.ContainsCards(card) || avoid.ContainsCards(card)) {
			r++; continue;
		}
		ret.AddCard(card);
	}
	return ret;
}


double TABLE_WIN_EXP2[13][13] = {
	{0.01322, -0.27766, -0.26197, -0.24598, -0.24873, -0.22719, -0.19424, -0.15501, -0.10443, -0.05149, 0.00668, 0.06749, 0.14778, },
	{-0.35483, 0.07019, -0.2277, -0.20908, -0.20934, -0.19672, -0.17903, -0.13199, -0.08881, -0.03285, 0.02017, 0.07745, 0.16417, },
	{-0.33753, -0.30007, 0.14122, -0.17317, -0.173, -0.16388, -0.14364, -0.11785, -0.06744, -0.01977, 0.04148, 0.0971, 0.18373, },
	{-0.3128, -0.27035, -0.23122, 0.20811, -0.14233, -0.12599, -0.10611, -0.08815, -0.05645, 0.00568, 0.05888, 0.1109, 0.20187, },
	{-0.32087, -0.27846, -0.24214, -0.19924, 0.26415, -0.09566, -0.07175, -0.05241, -0.02633, 0.01727, 0.06907, 0.13537, 0.19604, },
	{-0.3104, -0.27064, -0.22952, -0.19159, -0.14943, 0.32768, -0.03849, -0.01459, 0.01368, 0.05195, 0.08591, 0.15262, 0.22293, },
	{-0.26938, -0.25385, -0.21547, -0.16998, -0.1324, -0.10307, 0.38349, 0.01761, 0.04036, 0.0727, 0.119, 0.1703, 0.23613, },
	{-0.21329, -0.2028, -0.1837, -0.15446, -0.10361, -0.07933, -0.03714, 0.43893, 0.08172, 0.11997, 0.15014, 0.20366, 0.25713, },
	{-0.16612, -0.14952, -0.13296, -0.11359, -0.07842, -0.04466, -0.01099, 0.03062, 0.50006, 0.15441, 0.18318, 0.23406, 0.29206, },
	{-0.1182, -0.098, -0.07571, -0.05439, -0.04595, -0.00213, 0.03515, 0.06959, 0.10708, 0.54777, 0.20866, 0.25353, 0.30767, },
	{-0.04777, -0.03788, -0.01628, -0.00259, 0.01792, 0.03718, 0.07005, 0.11191, 0.15088, 0.16095, 0.59807, 0.26789, 0.3282, },
	{0.01414, 0.0287, 0.04741, 0.06299, 0.08574, 0.1011, 0.11473, 0.1559, 0.20178, 0.20629, 0.22882, 0.64394, 0.34245, },
	{0.09743, 0.12328, 0.13433, 0.15181, 0.15687, 0.17382, 0.20177, 0.21932, 0.25725, 0.27378, 0.29087, 0.30338, 0.70146, },
};

double win_exp(const CardSet &holeCards, const CardSet &boardCards) {
	if (holeCards.NumCards() == 2 && boardCards.NumCards() == 0) {
		double tmp = 0;
		vector<uint8_t> v = holeCards.ToCardArray();
		int rank1 = rankOfCard(v[0]), suit1 = suitOfCard(v[0]), rank2 = rankOfCard(v[1]), suit2 = suitOfCard(v[1]);
		if (suit1 == suit2) tmp =  TABLE_WIN_EXP2[min(rank1, rank2)][max(rank1, rank2)];
		else tmp = TABLE_WIN_EXP2[max(rank1, rank2)][min(rank1, rank2)];
		return tmp;
		// cerr << tmp << ' ' <<  (double)ret / N_SAMPLE << endl;
		// assert(abs(tmp - (double)ret / N_SAMPLE) < 0.1);
	}
	const int N_SAMPLE = 10000;
	int ret = 0;
	for (int t = 0; t < N_SAMPLE; t++) {
		CardSet oppoCards = random_padding(2, CardSet(), holeCards + boardCards);
		CardSet allBoardCards = random_padding(5, boardCards, oppoCards + holeCards);
		int my_rank = (holeCards + allBoardCards).RankCards(), oppo_rank = (oppoCards + allBoardCards).RankCards();
		if (my_rank > oppo_rank) ret++;
		if (my_rank < oppo_rank) ret--;
	}
	// cerr << ret << endl;
	return (double)ret / N_SAMPLE;
}

enum Strategy {
		SINVALID = 0, SFOLD, SCALL, SRAISE, SALLIN
};

struct PlayerState {
	int round, hasInit;
	Strategy type;
	int raisev, upperv;
	PlayerState() { this->reset(); }
	void reset() {
		memset(this, 0, sizeof(PlayerState));
		round = -1;
	}
};

class Configuration 
{
public:

	// double KEYPOINT[4][3] = {{0.0, 0.05, 0.2},	{-0.1, 0.15, 0.3}, 	{-0.2, 0.2, 0.4}, 	{-0.4, 0.3, 0.4}};
	// double CALL_COEF[4][4] = {{200., 2500., 0.0, 0.0},	{540., 5000., 0.0, 0.0},	{1380., 6600., 0.0, 0.0},	{4000., 20000., 0.0, 0.0}};
	// double RAISE_COEF[4][4] = {{125., 2500., 0.0, 0.0},	{125., 5000., 0.0, 0.0}, 	{125., 10000., 0.0, 0.0},	{125., 20000., 0.0, 0.0}};
	double KEYPOINT[4][3] = {{-0.0113425, 0.0426328, 0.284467}, {-0.100236, 0.261431, 0.437723}, {-0.133017, 0.295322, 0.357588}, {-0.329114, 0.117276, 0.486182}};
	double CALL_COEF[4][4] = {{200, 2649.42, -136.598, -666.013}, {403.424, 3932.59, -167.904, 29.4301}, {1360.26, 6419.59, 264.882, 1770.67}, {4270.02, 19267.1, 948.997, -491.894}};
	double RAISE_COEF[4][4] = {{-689.149, 2251.45, 428.701, 666.427}, {-123.375, 5087.91, -1531.62, 1252.22}, {410.354, 9922.06, 1146.19, -1102.27}, {254.358, 19795.7, 368.699, -656.29}};

	Configuration() {}
	void load(const string path) {
		ifstream fin(path.c_str());
		for (int i = 0; i < 4; i++) for(int j = 0; j < 3; j++) fin >> KEYPOINT[i][j];
		for (int i = 0; i < 4; i++) for(int j = 0; j < 4; j++) fin >> CALL_COEF[i][j];
		for (int i = 0; i < 4; i++) for(int j = 0; j < 4; j++) fin >> RAISE_COEF[i][j];
		fin.close();
	}
	void save(const string path) {
		ofstream fout(path.c_str());
		for (int i = 0; i < 4; i++) for(int j = 0; j < 3; j++) fout << KEYPOINT[i][j] << ' ';		fout << endl;
		for (int i = 0; i < 4; i++) for(int j = 0; j < 4; j++) fout << CALL_COEF[i][j] << ' ';		fout << endl;
		for (int i = 0; i < 4; i++) for(int j = 0; j < 4; j++) fout << RAISE_COEF[i][j] << ' ';		fout << endl;
		fout.close();
	}
};

class Agent
{
public:
	Configuration cf;
	PlayerState ps;
	Agent(Configuration _cf = Configuration()) {
		cf = _cf;
	}
	Action act(int id, Game *game, State *state);
	void reset(){
		ps.reset();
	}
}
_ai, *ai = &_ai;

Action Agent::act(int id, Game *game, State *state) {
	Action action = {(ActionType)0, 0};


	CardSet holeCards = from_array(2, state->holeCards[id]), boardCards = from_array(state->round == 0 ? 0 : 2 + state->round, state->boardCards);
	double win = win_exp(holeCards, boardCards);
	// fprintf(stderr, "INFO: %s|?|%s, wexp: %lf\n", holeCards.ToString().c_str(), boardCards.ToString().c_str(), win);

	int round = state->round;
	if (ps.round != round) {
		ps.round = round;
		double win0 = 1, win1 = win, win2 = win * win, win3 = win * win * win;
		double raisef = cf.RAISE_COEF[round][0] * win0 + cf.RAISE_COEF[round][1] * win1 + cf.RAISE_COEF[round][2] * win2 + cf.RAISE_COEF[round][3] * win3;
		double upperf = cf.CALL_COEF[round][0] * win0 + cf.CALL_COEF[round][1] * win1 + cf.CALL_COEF[round][2] * win2 + cf.CALL_COEF[round][3] * win3;
		// cerr << raisef << ' ' << upperf << endl;
		if (win < cf.KEYPOINT[round][0]) {
			ps.type = SFOLD; ps.raisev = 0, ps.upperv = 0;
		}
		else if (win < cf.KEYPOINT[round][1]) {
			ps.type = SCALL; ps.raisev = 0; ps.upperv = upperf;
		}
		else if (win < cf.KEYPOINT[round][2]) {
			ps.type = SRAISE; ps.raisev = raisef, ps.upperv = upperf;
		}
		else {
			ps.type = SALLIN; ps.raisev = raisef, ps.upperv = 20000;
		}
	}
	
	// cerr << ps.type << endl;
	if (ps.type == SFOLD) {
		action.type = a_fold; action.size = 0;
		if(!isValidAction( game, state, 0, &action )) {
			action.type = a_call; action.size = 0;
		}
	}
	else if (ps.type == SCALL) {
		// if the oppo raise too high, I choose to give up
		if (state->spent[id] < state->spent[id ^ 1] && state->spent[id ^ 1] > ps.upperv) {
			action.type = a_fold; action.size = 0;
			if(!isValidAction( game, state, 0, &action )) {
				action.type = a_call; action.size = 0;
			}
		}
		else // otherwise, i chose to call
		{
			action.type = a_call; action.size = 0;
		}
		
	}
	else if (ps.type == SRAISE) {
		// if the oppo raise too high, I choose to give up
		if (state->spent[id] < state->spent[id ^ 1] && state->spent[id ^ 1] > ps.upperv) {
			action.type = a_fold; action.size = 0;
			if(!isValidAction( game, state, 0, &action )) {
				action.type = a_call; action.size = 0;
			}
		}
		else {
			// otherwise, i chose to raise
			int32_t minv, maxv;
			if(!raiseIsValid( game, state, &minv, &maxv ) || minv > ps.raisev) {
				action.type = a_call; action.size = 0;
			}
			else {
				action.type = a_raise; action.size = min(maxv, max(minv, ps.raisev));
			}
		}
	}
	else if (ps.type == SALLIN) {
		int32_t minv, maxv;
		if(!raiseIsValid( game, state, &minv, &maxv )) {
			action.type = a_call; action.size = 0;
		}
		else {
			action.type = a_raise; action.size = min(maxv, max(minv, state->spent[id ^ 1] < ps.raisev ? ps.raisev : 2 * state->spent[id ^ 1]));
		}
	}
	
	assert( isValidAction( game, state, 0, &action ) );
	// fprintf(stderr, "INFO: my_action %d %d\n", action.type, action.size);
	return action;
}


void init() {
	// ai->cf.load("conf/best-49-43-0-139.999583-conf.txt");
	assert(ai->cf.KEYPOINT[0][0] != 0.0);
}

void reset(Game *game, MatchState *_state, rng_state_t *_rng) {
	// TODO: to implement
	static double total_award = 0;
	double award = valueOfState(game, &_state->state, _state->viewingPlayer);
	total_award += award;
	fprintf(stderr, "INFO: end, award: %lf, accumulated: %lf\n", award, total_award);
	ai->reset();
}

Action act(Game *game, MatchState *_state, rng_state_t *_rng) {
	uint8_t id = _state->viewingPlayer;
	State *state = &(_state->state);

	return ai->act(id, game, state);
}

/* Anything related with socket is handled below. */
/* If you are not interested with protocol details, you can safely skip these. */

int step(int len, char line[], Game *game, MatchState *state, rng_state_t *rng) {
	/* add a colon (guaranteed to fit because we read a new-line in fgets) */
	line[ len ] = ':';
	++len;

	Action action = act(game, state, rng);

	/* do the action! */
	assert( isValidAction( game, &(state->state), 0, &action ) );
	int r = printAction( game, &action, MAX_LINE_LEN - len - 2, &line[ len ] );
	if( r < 0 ) {
		fprintf( stderr, "ERROR: line too long after printing action\n" );
		exit( EXIT_FAILURE );
	}
	len += r;
	line[ len ] = '\r';
	++len;
	line[ len ] = '\n';
	++len;

	return len;
}


int main( int argc, char **argv )
{
	int sock, len;
	uint16_t port;
	Game *game;
	MatchState state;
	FILE *file, *toServer, *fromServer;
	struct timeval tv;
	char line[ MAX_LINE_LEN ];

	/* we make some assumptions about the actions - check them here */
	assert( NUM_ACTION_TYPES == 3 );

	if( argc < 3 ) {

		fprintf( stderr, "usage: player server port\n" );
		exit( EXIT_FAILURE );
	}

	/* Initialize the player's random number state using time */
	gettimeofday( &tv, NULL );
	init_genrand( rng, tv.tv_usec );

	/* get the game */
	file = fopen( "./holdem.nolimit.2p.reverse_blinds.game", "r" );
	if( file == NULL ) {

		fprintf( stderr, "ERROR: could not open game './holdem.nolimit.2p.reverse_blind.game'\n");
		exit( EXIT_FAILURE );
	}
	game = readGame( file );
	if( game == NULL ) {

		fprintf( stderr, "ERROR: could not read game './holdem.nolimit.2p.reverse_blind.game'\n");
		exit( EXIT_FAILURE );
	}
	fclose( file );

	/* connect to the dealer */
	if( sscanf( argv[ 2 ], "%"SCNu16, &port ) < 1 ) {

		fprintf( stderr, "ERROR: invalid port %s\n", argv[ 2 ] );
		exit( EXIT_FAILURE );
	}
	sock = connectTo( argv[ 1 ], port );
	if( sock < 0 ) {

		exit( EXIT_FAILURE );
	}
	toServer = fdopen( sock, "w" );
	fromServer = fdopen( sock, "r" );
	if( toServer == NULL || fromServer == NULL ) {

		fprintf( stderr, "ERROR: could not get socket streams\n" );
		exit( EXIT_FAILURE );
	}

	/* send version string to dealer */
	if( fprintf( toServer, "VERSION:%"PRIu32".%"PRIu32".%"PRIu32"\n",
				 VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION ) != 14 ) {

		fprintf( stderr, "ERROR: could not get send version to server\n" );
		exit( EXIT_FAILURE );
	}
	fflush( toServer );

	init();

	/* play the game! */
	while( fgets( line, MAX_LINE_LEN, fromServer ) ) {

		/* ignore comments */
		if( line[ 0 ] == '#' || line[ 0 ] == ';' ) {
			continue;
		}

		len = readMatchState( line, game, &state );
		if( len < 0 ) {

			fprintf( stderr, "ERROR: could not read state %s", line );
			exit( EXIT_FAILURE );
		}

		if( stateFinished( &state.state ) ) {
			/* ignore the game over message */
			reset(game, &state, rng);
			continue;
		}

		if( currentPlayer( game, &state.state ) != state.viewingPlayer ) {
			/* we're not acting */

			continue;
		}

		len = step(len, line, game, &state, rng);

		if( fwrite( line, 1, len, toServer ) != len ) {

			fprintf( stderr, "ERROR: could not get send response to server\n" );
			exit( EXIT_FAILURE );
		}
		fflush( toServer );
	}

	return EXIT_SUCCESS;
}
