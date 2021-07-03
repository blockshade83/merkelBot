#pragma once
#include "OrderBookEntry.h"
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <iostream>
#include <chrono>

class OrderBook
{
    public:
        /** Construct, reading a CSV data file and returning a vector of order book entries*/
        OrderBook();     

        /** matching engine, takes a vector of orders a product and a timestamps as inputs *
         * returns a list of orderbook entries representing executed sales*
         * also records unfulfilled bot orders to a vector of active orders*/
        std::vector<OrderBookEntry> matchAsksToBids(std::vector<OrderBookEntry>& currentOrders, 
                                                    std::string product, 
                                                    std::string timestamp);

        /** map of timestamps to vectors of all orders */
        std::map<std::string,std::vector<OrderBookEntry> > ordersByTimestamp;

        /** vector of order book entries to store all the unprocessed bot orders */
        std::vector<OrderBookEntry> activeUserOrders;

        /** vector of strings to store timestamps */
        std::vector<std::string> timestamps;

        /** vector of strings to store products */
        std::vector<std::string> products;
        
    private:

        /** Return vector of orders according to the filters applied*/
        static std::vector<OrderBookEntry> getOrders(std::vector<OrderBookEntry>& ordersList,
                                              OrderBookType type, 
                                              std::string product,
                                              std::string timestamp);
        
};