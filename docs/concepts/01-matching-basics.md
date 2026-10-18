# Matching basics: price-time priority, crossing, partial fills, market vs limit

Concept notes to read before writing the matching engine.

## 1. The book

An order book is two sorted lists of resting orders:

- **Bids** (buyers), best first, which means **highest price first**.
- **Asks** (sellers), best first, which means **lowest price first**.

The **spread** is the gap between the best bid and the best ask. When nothing can trade, best bid < best ask.

Prices are integer **ticks**. In these notes 1 tick = $0.01, so `101` means $1.01. Decimal conversion only happens at the edges (input parsing and display).

## 2. Price-time priority

When an incoming order can trade against several resting orders, the engine picks which one fills first using two rules:

1. **Price first.** The best price fills first. A seller asking 101 fills before a seller asking 102.
2. **Then time.** At the same price, the order that arrived first fills first (FIFO).

So each price level is a **queue**. You can't jump the queue by being bigger. You can only jump it by offering a better price.

## 3. Worked example

Orders arrive in this sequence:

| t | ID | Side | Type   | Qty | Price |
|---|----|------|--------|-----|-------|
| 1 | A  | Sell | Limit  | 5   | 101   |
| 2 | B  | Sell | Limit  | 3   | 101   |
| 3 | C  | Sell | Limit  | 4   | 102   |
| 4 | D  | Buy  | Limit  | 6   | 99    |
| 5 | E  | Buy  | Limit  | 10  | 101   |
| 6 | F  | Sell | Market | 7   | —     |

### After t=1–4: nothing crosses, so every order rests

```
ASKS   102: [C 4]
       101: [A 5, B 3]      <- best ask (A is ahead of B: arrived first)
------------- spread = 101 - 99 = 2 ticks
BIDS    99: [D 6]           <- best bid
```

### t=5: E buys 10 @ 101. A limit order crosses the spread

E is willing to pay up to 101, and the best ask is 101. Because 101 >= 101, E **crosses** and trades immediately instead of resting. The engine walks the ask side from best to worst:

1. Level 101, front of queue is A: trade **5 @ 101**. A is fully filled and removed. E has 5 left.
2. Level 101, next is B: trade **3 @ 101**. B is fully filled and removed. E has 2 left.
3. Next level is 102, but 102 > E's limit of 101, so matching **stops**.

E is **partially filled** (8 of 10). Its remaining 2 **rest on the book** at 101 as a new bid, at the back of the 101 bid queue.

```
ASKS   102: [C 4]           <- best ask
------------- spread = 1 tick
BIDS   101: [E 2]           <- best bid (E's remainder)
        99: [D 6]
```

Two key points here:

- **The trade price is the resting order's price**, not the incoming one. If E had bid 105, it would still have paid 101 for those 8 shares, then gone on to take C at 102. The order that was resting set the price.
- A limit order sets a **worst acceptable price**. It's not a target price.

### t=6: F market sell 7. A market order

A market order has **no price**. It means "fill me now at whatever the book offers." It walks the bid side:

1. Level 101, E: trade **2 @ 101**. E is fully filled. F has 5 left.
2. Level 99, D: trade **5 @ 99**. D is **partially filled** and keeps 1. F has 0 left, so it's done.

```
ASKS   102: [C 4]
-------------
BIDS    99: [D 1]           <- D keeps its place in the queue
```

D was partially filled but **keeps its time priority**. A partial fill shrinks the order's quantity. It doesn't send the order to the back of the queue.

If F had been for 20 and the book ran dry, a market order **never rests**, because it has no price to rest at. The unfilled remainder is cancelled.

## 4. Summary

|                   | Limit order                          | Market order                         |
|-------------------|--------------------------------------|--------------------------------------|
| Has a price?      | Yes (worst acceptable)               | No                                   |
| Can cross?        | Yes, if it reaches the opposite best | Always tries to                      |
| Leftover quantity | Rests on the book                    | Cancelled                            |
| Risk              | May not fill                         | May fill at bad prices ("walking the book") |

**Trades from the example:** A×E 5@101, B×E 3@101, E×F 2@101, D×F 5@99.

These trades make a good first unit test once the engine exists.

## 5. Check your understanding

1. Starting from the **final** book above, a new **Buy Limit 5 @ 102** arrives. Which trades happen, at what prices, and what does the book look like afterwards?
2. Suppose at t=5 E had been a **Buy Limit 10 @ 100** instead of @ 101. What happens to E, and what's the spread afterwards? Why?
3. Back at the start: suppose that between t=2 and t=3, A sent a request to **increase** its quantity from 5 to 50. Should A keep its place in front of B? What would go wrong in a real exchange if it did?

## 6. Answers

### Q1: Buy Limit 5 @ 102 against the final book

- **Trade:** C × new buy, **4 @ 102**. C had 4 and sold 4, so **C is fully filled and removed**.
- The *incoming* buy wanted 5 and got 4, so **the buy has 1 left**. It's a limit order, so that 1 **rests as a bid @ 102**.

```
ASKS   (empty)
-------------
BIDS   102: [new 1]     <- best bid
        99: [D 1]
```

The ask side is empty, so there's no spread. The engine must handle "no best ask" without crashing.

**Rule:** after a match, *both* sides lose quantity. The resting order is removed if it reaches 0. The incoming order rests only if it's a limit order with quantity left.

### Q2: E as Buy Limit 10 @ 100

At t=5, best ask is 101 and 100 < 101, so E **does not cross**. All 10 **rest on the book**:

```
ASKS   102: [C 4]
       101: [A 5, B 3]   <- best ask
------------- spread = 101 - 100 = 1 tick
BIDS   100: [E 10]       <- best bid
        99: [D 6]
```

**Spread = best ask - best bid = 1 tick.**

At t=6, F (market sell 7) takes the **highest** bid first. E @ 100 beats D @ 99 on **price**. Time priority only breaks ties at the *same* price. F trades **7 @ 100 with E**, E keeps 3, and D is untouched.

```
ASKS   102: [C 4]
       101: [A 5, B 3]
-------------
BIDS   100: [E 3]
        99: [D 6]
```

**Rule:** "best" depends on the side. Best bid = highest. Best ask = lowest.

### Q3: Should A keep its place after increasing 5 -> 50?

**No.** If increasing quantity kept priority, anyone could game the queue:

1. Post a tiny order early, such as 1 share @ 101, just to take first place in the queue.
2. Others (B) arrive and queue behind it.
3. Increase to 1000 shares and stay first. B now waits behind quantity it never agreed to queue behind.

Time priority is meant to reward **committing quantity early**, so new quantity has to go to the back of the queue.

**Decreasing quantity keeps priority.** It can only *help* everyone behind (there's less quantity in front of them), and nobody gets overtaken.

**Engine implication:**
- Increasing quantity (or changing price) = **cancel + new order**, which goes to the back of the queue.
- Decreasing quantity = **modify in place**, which keeps the queue position.

### Re-check: Bids `100: [X 4]`, `98: [Y 10]`; market sell 6 arrives

- Sellers take the highest bid first: **X 4 @ 100** (X removed), then **Y 2 @ 98**.
- Y keeps **8 @ 98** on the book, still at its place in the queue.
- The market sell was fully filled. If it had had quantity left, that remainder would be **cancelled**, because market orders never rest.
