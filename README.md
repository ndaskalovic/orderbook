# Order Book Simulation

This repository contains a c++ order book implementation supporting a variety of order types in a client-server architecture with an Aeron communication layer. A live simulation using this order book can be found at [this](https://nickdaskalovic.com/orderbook) link.

## Design

### The Book

The [orderbook](orderbook/src/orderBook.h) maintains two `std::map`s for the bid and ask queues ordered by price descending and ascending respectively. This allows for quick and easy access to the best bids and asks for matching. Each entry in these maps is an ordered `std::list` of orders which acts as a queue with the left most value being of highest priority. This structure creates an overall price-time priority system in the order book.

A [server](orderbook/server.cpp) application uses Aeron to wait for incoming orders and adds them to the book. To allow for multi-threaded access, the order book also maintains an `std::mutex` along with locks in sensitive public methods.

### Aeron

The [Aeron](https://github.com/real-logic/aeron) library provides UDP unicast and multicast message transport best suited to performance sensitive applications. Both of these modes are used in this project to allow clients to submit orders to the central server while also receiving real-time price data from it. A common library of utilities for interfacing with the Aeron library can be found in [aeronUtils.cpp](orderbook/aeronUtils.h). At a high level, there are two Aeron publications/subscriptions used in this project - one by clients to send orders to the server and one by the server to send live price data to all clients. Applications running a publication and subscription at the same time do so via additional threads.

### Database

Basic logging of order throughput, current asset price and most recent orders is done through a [small SQLite interface](orderbook/sqliteConnection.h) as well as a thread-safe buffer of the most recent orders.

### Simulation

The custom [simulation client](orderbook/simulationClient.cpp) provides a maximum throughtput stream of orders to the server and can be used to simulate real-time trading on the book. The prices of limit orders are modelled using a gamma distribution with parameters $\alpha = 3$ and $\beta = 2$ to mimic the spread of orders around a current asset price often found in real exchanges. By default, an equal amount of buy and sell orders are sent to the book to prevent biased price action when no external influence is applied. 

## Web App

A [FastAPI app](fapi/fapi.py) sits between the backend (order book + db) and frontend [website](fapi/index.html). It mostly serves only GET requests to the frontend but has one POST endpoint to allow users to apply buy or sell pressure to the book and watch the consequenting price action. The pressure is applied by shifting the volume of buy or sell orders to favour one or the other by 7% (e.g 57% Buy orders and 43% sell orders) for a few seconds. The current link to the demo can be found here [here](https://nickdaskalovic.com/orderbook).

The live demo is currently deployed on a VPS with only 1 Intel Xeon core (2 vCPU) at 2.2GHz and is able to achieve an average throughput of ~80k orders/second.

## Building

To build the order book simply `cd` into the `orderbook/` directory and run:

```bash
make build
```

Aeron and GoogleTest must be installed for the build to succeed.

An automation script to deploy all components of the app can be found in [run.sh](run.sh).

## Notes

- Market orders places when the opposite side of the order book is empty are ignored to prevent spikes in price movement in the simulation. This could be replaced by creating articifial orders to fill these market orders to act as a liquidity provider agent e.g provide artificial liquidity at 101 if a market order is placed with a top bid of 100 and no asks on the book

## Future work

- Support for more order types
- Faster DSA for price levels and matching