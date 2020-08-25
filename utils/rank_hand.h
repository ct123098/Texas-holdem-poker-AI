#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#define __STDC_LIMIT_MACROS

#include <stdint.h>
#include <bitset>
#include <sstream>
#include <vector>

#include "evalHandTables"

#define MAX_SUITS 4
#define MAX_RANKS 13

#define rankOfCard(card) ((card)/MAX_SUITS)
#define suitOfCard(card) ((card)%MAX_SUITS)
#define makeCard(rank, suit) ((rank)*MAX_SUITS+(suit))

constexpr int kMaxSuits = 4;

// This is an equivalent wrapper to acpc evalHandTables.Cardset.
// It stores the cards for each color over 16 * 4 bits. The use of a Union
// allows to access only a specific color (16 bits) using bySuit[color].
// A uint8_t card is defined by the integer <rank> * MAX_SUITS + <suit>
class CardSet {
public:
    union CardSetUnion {
        CardSetUnion() : cards(0) {}

        uint16_t bySuit[kMaxSuits];
        uint64_t cards;
    } cs;

public:
    CardSet() : cs() {}

    explicit CardSet(std::string cardString);

    explicit CardSet(std::vector<int> cards);

    // Returns a set containing num_ranks cards per suit for num_suits.
    CardSet(uint16_t num_suits, uint16_t num_ranks);

    std::string ToString() const;

    // Returns the cards present in this set in ascending order.
    std::vector<uint8_t> ToCardArray() const;

    // Add a card, as MAX_RANKS * <suite> + <rank> to the CardSet.
    void AddCard(uint8_t card);

    // Toogle (does not remove) the bit associated to `card`.
    void RemoveCard(uint8_t card);

    bool ContainsCards(uint8_t card) const;

    int NumCards() const;

    // Returns the ranking value of this set of cards as evaluated by ACPC.
    int RankCards() const;

    // TODO(author2): Remove?
    std::vector<CardSet> SampleCards(int nbCards);
};

