#pragma once
#include "OrderBookEntry.h"
#include "CSVReader.h"
#include <string>
#include <vector>
#include <map>

class OrderBook
{
    public:
        /** Construct, reading a CSV data file and returning a vector of order book entries*/
        OrderBook(std::string filename);
        
        /** Return vector of all known products in the dataset*/
        std::vector<std::string> getKnownProducts();
        
        /** Return the earliest time in the orderbook by passing a vector of timestamps */
        std::string getEarliestTime(std::vector<std::string>& timestamps);        
        
        /** Return the latest time in the orderbook by passing a vector of timestamps  */
        std::string getLatestTime(std::vector<std::string>& timestamps);
        
        /** Return the next time after the sent time in the orderbook.
         * If there is no next timestamp, wraps around to the start*
         * Takes a timestamp as an input and a vector of timestamps as input*/
        std::string getNextTime(const std::string& timestamp, std::vector<std::string>& timestampsList);

        /** Insert order to order book */
        void insertOrder(OrderBookEntry& order);

        /** matching engine, takes a vector of orders a product and a timestamps as inputs *
         * returns a list of orderbook entries representing executed sales*
         * also records unfulfilled bot orders to a vector of active orders*/
        std::vector<OrderBookEntry> matchAsksToBids(std::vector<OrderBookEntry>& currentOrders, 
                                                    std::string product, 
                                                    std::string timestamp);

        /** map of timestamps to vectors of all orders */
        std::map<std::string,std::vector<OrderBookEntry> > ordersByTimestamp;

        /** vector of strings to store timestamps */
        std::vector<std::string> timestamps;

        /** transfer active user orders from one timestamp to the next */
        void transferActiveOrders(std::string prevTimestamp, std::string nextTimestamp);
        
    private:

        /** Function to extract orders data and place it into a map data structure. Also extracts all timestamps in the dataset*/
        void extractTimestampData();

        /** vector of order book entries to store all the unprocessed bot orders */
        std::vector<OrderBookEntry> activeUserOrders;

        /** store orders read from the CSV file */
        std::vector<OrderBookEntry> orders;

        /** Return vector of orders according to the filters applied*/
        static std::vector<OrderBookEntry> getOrders(std::vector<OrderBookEntry>& ordersList,
                                              OrderBookType type, 
                                              std::string product,
                                              std::string timestamp);
        
};