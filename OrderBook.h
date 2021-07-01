#pragma once
#include "OrderBookEntry.h"
#include "CSVReader.h"
#include <string>
#include <vector>
#include <map>

class OrderBook
{
    public:
        /** Construct, reading a CSV data file */
        OrderBook(std::string filename);
        /** Return vector of all known products in the dataset*/
        std::vector<std::string> getKnownProducts();
        /** Return vector of orders according to the filters applied*/
        static std::vector<OrderBookEntry> getOrders(std::vector<OrderBookEntry>& ordersList,
                                              OrderBookType type, 
                                              std::string product,
                                              std::string timestamp);
        
        /** function to extract orders for a specific timestamp*/
        std::vector<OrderBookEntry> getLiveOrders(std::string& timestamp);
        
        /** Return the earliest time in the orderbook */
        std::string getEarliestTime(std::vector<std::string>& timestamps);        
        
        /** Return the latest time in the orderbook */
        std::string getLatestTime(std::vector<std::string>& timestamps);
        
        /** Return the next time after the sent time in the orderbook.
         * If there is no next timestamp, wraps around to the start*/
        std::string getNextTime(const std::string& timestamp, std::vector<std::string>& timestampsList);

        void insertOrder(OrderBookEntry& order);

        std::vector<OrderBookEntry> matchAsksToBids(std::vector<OrderBookEntry>& currentOrders, 
                                                       std::string product, 
                                                       std::string timestamp);
        
        /** Return the price of the highest bid/ask in the sent set*/                                      
        static double getHighPrice(std::vector<OrderBookEntry>& orders);
        /** Return the price of the lowest bid/ask in the sent set*/
        static double getLowPrice(std::vector<OrderBookEntry>& orders);

        /** Return the money ratio for a product in the set at a specific time */
        /** Money ratio = Positive Money Flow / Negative Money Flow */
        /** https://docs.anychart.com/Stock_Charts/Technical_Indicators/Mathematical_Description */
        /** normally calculated with all executed transactions in a specific period */
        /** adapting to the context of the available data */
        double getMoneyRatio(const std::string& product,
                                    const std::string& timestamp);

        std::vector<OrderBookEntry> orders;

        // defining a vector of order book entries to store all the unprocessed bot orders
        std::vector<OrderBookEntry> activeUserOrders;   

        // function to transfer active user orders from one timestamp to the next
        void transferActiveOrders(std::string prevTimestamp, std::string nextTimestamp);

        // map of timestamps to vectors of all orders
        std::map<std::string,std::vector<OrderBookEntry> > ordersByTimestamp;

        // map of strings to store timestamps 
        std::map<std::string,bool> timestampsMap;
        
        /** Return vector of all timestamps in the dataset*/
        std::vector<std::string> getTimestamps();
    
    private:
        
};