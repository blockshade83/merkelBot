#include "Wallet.h"
#include "OrderBook.h"
#include "OrderBookEntry.h"
#include "Assets.h"
#include <vector>
#include <fstream>

class MerkelBot
{
    public:
        MerkelBot();
		/** Call this to start the application */
		void init();

    private:
        // variable to store and process bot assets
        Assets botAssets;

        // read order book from file
        OrderBook fullOrderBook{"20200317.csv"};

        // vector with all products on the exchange 
        std::vector<std::string> botProducts = fullOrderBook.getKnownProducts();

        std::vector<std::string> timestamps = fullOrderBook.getTimestamps();

        // variable to store orders valid for current period only
        // std::vector<OrderBookEntry> liveOrders;

        // functions to extract bids and asks by product
        std::vector<OrderBookEntry> getLiveBidsForProduct(std::string const product);
        std::vector<OrderBookEntry> getLiveAsksForProduct(std::string const product);

        // map variables to store current market statistics
        std::map<std::string, double> maxBidPrices;
        std::map<std::string, double> minAskPrices;
        std::map<std::string, double> avgCurrentPrices;

        // a vector to store historical prices for all timestamps up to current time 
        std::vector<std::map<std::string, double> > avgHistoricalPrices;

        /** function to extract historical prices from a specific product*/
        std::vector<double> getHistoricalPricesByProduct(std::string const product);

        /** store current price prediction for each product*/
        std::map<std::string, double> pricePrediction;

        /** function to estimate next likely value of a numerical array using linear regression*/
        static double linRegressionPrediction(std::vector<double>& priceHistory);

        /** update price prediction based on the historical and the most recent market prices*/
        void updatePricePrediction();

        /** get current market prices */
        void getMarketPrices();

        /** function to execute all actions, including bot decisions for current time*/
        void processTimeframe();

        /** move to the next timeframe */
        void gotoNextTimeframe();

        /** analyze current market and place bot bids */ 
        void placeBotBids();

        /** analyze current market and place bot asks */ 
        void placeBotAsks();

        /** analyze current market and cancel carryover orders where needed */ 
        void cancelBotOrders(std::string& timestamp);

        /** add bot order to the order log */
        void logBotOrder(OrderBookEntry& order, std::ofstream& logFile);

        /** add sale to the sale log*/
        void logBotSale(OrderBookEntry& sale);

        /** update the assets
         * assumes the order was made by the owner of the wallet */
        void processSale(OrderBookEntry& sale, Wallet& standard, Wallet& reserved);

        /** log total value of assets in USD equivalent */
        void logTotalAssetsUSD(std::ofstream& logFile);

        // class variables to store current time, earliest time and latest time for the order book
        std::string currentTime;
        std::string earliestTimestamp;
        std::string latestTimestamp;

        // file stream variables for the logs
        std::ofstream botActiveOrdersLog;
        std::ofstream botCancelledOrdersLog;
        std::ofstream botAssetsLog;
        std::ofstream botSalesLog;
};