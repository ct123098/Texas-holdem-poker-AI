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
#include <algorithm>
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

	double KEYPOINT[4][3] = {{0.0, 0.05, 0.2},	{-0.1, 0.15, 0.3}, 	{-0.2, 0.2, 0.4}, 	{-0.4, 0.3, 0.4}};
	double CALL_COEF[4][4] = {{200., 2500., 0.0, 0.0},	{540., 5000., 0.0, 0.0},	{1380., 6600., 0.0, 0.0},	{4000., 20000., 0.0, 0.0}};
	double RAISE_COEF[4][4] = {{125., 2500., 0.0, 0.0},	{125., 5000., 0.0, 0.0}, 	{125., 10000., 0.0, 0.0},	{125., 20000., 0.0, 0.0}};

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

const int N_ROUND = 1000;
class Arena {
public:
	Game game;
	State state;
	Arena () {}
	Arena (Game *_game) : game(*_game) { 
	}
	double compare(Agent *A, Agent *B, int N = N_ROUND, int seed=123);
}
cmp;

double Arena::compare(Agent *A, Agent *B, int N, int seed) {
	rng_state_t tmp_rng;
	init_genrand(&tmp_rng, seed);
	int ret = 0;
	Agent *agent[2] = {A, B};
	for (int t = 0; t < N; t++) {
		dealCards( &game, &tmp_rng, &state );
		for (int turn = 0; turn <= 1; turn++) {
			initState( &game, t, &state );
			agent[0]->reset(); agent[1]->reset();
			// cerr << "ROUND " << t << ' ' << turn << endl;
			while (!stateFinished(&state)) {
				int currentP = currentPlayer( &game, &state );
				Action action = agent[currentP ^ turn]->act(currentP, &game, &state);
				doAction( &game, &action, &state );
			}
			int value = valueOfState(&game, &state, turn);
			ret += value;
		}
	}
	return (double)ret / N_ROUND / 2;
}

double compare_con(Agent *A, Agent *B, int N, int T) {
	vector<double> vec;
	cerr << "@@ ";
	for (int i = 0; i < T; i++) {
		double tmp = cmp.compare(A, B, N, 123 * N + 321 * T + i);
		cerr << tmp << " ";
		vec.push_back(tmp);
	}
	cerr << endl;
	sort(vec.begin(), vec.end());
	double sum = 0;
	for (int i = 1; i < T - 1; i++)
		sum += vec[i];
	return sum / (T - 2);
}

void iteration(Configuration *ret, int type)
{
	Configuration cur_c = *ret, new_c = cur_c;
	Agent best_ai(cur_c), cur_ai(cur_c), new_ai(cur_c);
	double cur_val = 0, new_val = 0, best_val = 0;
	const int N_ITER = 10;
	double T = 1.0, T_LAM = pow(0.1 / T, 1.0 / N_ITER);
	for (int t = 0; t < N_ITER; t++, T *= T_LAM) {

		new_c = cur_c;

		if (type < 12)
			*(*new_c.KEYPOINT + type) = *(*new_c.KEYPOINT + type) + T * 0.05 * (genrand_real2(rng) - 0.5);
		else if (type - 12 < 16) {
			int j = (type - 12) % 4;
			*(*new_c.CALL_COEF + type - 12) = *(*new_c.CALL_COEF + type - 12) + T * 200 * (j + 1) *(genrand_real2(rng) - 0.5);
		}
		else if (type - 28 < 16) {
			int j = (type - 28) % 4;
			*(*new_c.RAISE_COEF + type - 28) = *(*new_c.RAISE_COEF + type - 28) + T * 200 * (j + 1) * (genrand_real2(rng) - 0.5);
		}
		else
			throw "exceed";

		new_ai = Agent(new_c);

		new_val = compare_con(&new_ai, &best_ai, 200, 4);
		if(new_val - cur_val > 0 || genrand_real2(rng) < exp((new_val - cur_val) / T)) {
			cur_ai = new_ai;
			cur_val = new_val;
			cur_c = new_c;
			cerr << "INFO: accept with val: " << new_val << endl;
		}
		if (new_val > best_val) {
			best_val = new_val; *ret = new_c;
		}
	}
}
void train()
{
	Configuration cur, best = cur;
	Agent init_ai, cur_ai;
	double best_val = 0;
	const int N_ITER = 10;
	// cur.load("conf/4/best-0-29-0-51.477183-conf.txt");
	cur_ai = Agent(cur);
	double value = compare_con(&cur_ai, &init_ai, 1000, 8);
	cerr << "eval: " << value << endl;
	if (value > best_val) {
		best_val = value; best = cur;
	}
	const int TYPE[12] = {0, 1, 2, 13, 14, 15, 16, 28, 29, 30, 31};
	const int DELTA[12] = {3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4};
	for (int i = 0; i < 100; i++)
	{
		// for(int type1 = 0; type1 < 4; type1++)
		for(int type1 = 3; type1 >= 0; type1--)
			for (int type2 = 0; type2 < 12; type2++) {
				int type = TYPE[type2] + type1 * DELTA[type2];
				for (int j = 0; j < 1; j++)
				{
					cerr << "INFO: UPDATE: " << type << "ROUND " << j << endl;
					iteration(&cur, type);
					cur_ai = Agent(cur);
					double value = compare_con(&cur_ai, &init_ai, 1000, 8);
					// for (int i = 0; i < 4; i++) {
					// 	double t_value = cmp.compare(&cur_ai, &best_ai);
					// 	value = min(value, t_value);
					// }
					cerr << "eval: " << value << endl;
					if (value > best_val) {
						best_val = value; best = cur;
					}
					cur.save("conf/" + to_string(i) + "-" + to_string(type) + "-" + to_string(j) + "-conf.txt");
					best.save("conf/best-" + to_string(i) + "-" + to_string(type) + "-" + to_string(j) + "-" + to_string(best_val) + "-conf.txt");
				} 
			}
		cur = best;
	}
}

int main( int argc, char **argv )
{
	Game *game;
	MatchState state;

	/* Initialize the player's random number state using time */
	struct timeval tv;
	gettimeofday( &tv, NULL );
	init_genrand( rng, tv.tv_usec );

	/* get the game */
	FILE *file, *toServer, *fromServer;
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

	cmp = Arena(game);

	Agent agent1, agent2;

	agent1.cf.load("conf/4/best-0-29-0-51.477183-conf.txt");

	for (int i = 0; i < 6; i++)
		cerr << cmp.compare(&agent1, &agent2, 1000, 1) << endl;;
	train();


	return EXIT_SUCCESS;
}
